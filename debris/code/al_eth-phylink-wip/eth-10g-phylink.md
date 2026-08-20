# 10G SFP+ (eth2): phylink + sfp.c architecture

Scope: eth2 only (PCI `1c36:0002`, `enp0s2`). eth1 (`1c36:0001`, RGMII + AR8031
at MDIO 4) is unchanged — it has `phy_exist=true` and runs phylib.

Decision and feasibility study: **#113**. Not yet hardware-tested.

## Who owns what

| Layer | Owner |
|---|---|
| Cage presence, SFF-8472 EEPROM/DDM, interface selection, hotplug | mainline `drivers/net/phy/sfp.c` |
| Link state machine, carrier, `ethtool` link settings, `ethtool -m` | mainline `phylink` |
| SerDes bring-up + PCS/MAC link read (`phylink_pcs`) | `modules/al_eth/al_eth_phylink.c` |
| MAC ops (`mac_config`, `mac_link_up/down`) | `modules/al_eth/al_eth_phylink.c` |
| SerDes lane TX/RX parameter tables + sysfs overrides | `modules/al_eth/al_init_eth_lm.c` (unchanged HAL) |
| UDMA / descriptors / MAC datapath | `modules/al_eth/al_hal_*` (untouched) |

phylink never touches the SerDes. `pcs_pre_config`/`pcs_config` are where the
vendor bring-up now lives — the same code, re-hosted, not rewritten.

## Call order

Probe (`al_eth_probe`):
- board params → `use_lm && sfp_detection_needed && max_speed==10G` ⇒ phylink port
- `mac_mode` forced to `AL_ETH_MAC_MODE_10GbE_Serial` (media
  `AUTO_DETECT_AUTO_SPEED` leaves it unset; the vendor LM used to fill it in)
- `al_eth_serdes_init()` → `al_eth_lm_static_init()` → `al_eth_phylink_setup()`
- `al_eth_phylink_setup()` matches the DT node by `port-id == PCI_SLOT(devfn)`,
  `device_set_node()`s it onto the PCI device, then `phylink_create()`

`ndo_open`: SerDes group → KR, `al_eth_up()` (datapath + `al_eth_mac_config`),
`phylink_start()`.
`ndo_stop`: `phylink_stop()`, `al_eth_down()`.

phylink major-config order (`phylink.c:1239`) is
`mac_prepare → pcs_pre_config → mac_config → pcs_post_config → pcs_enable →
pcs_config → pcs_an_restart → mac_finish`; we implement `pcs_pre_config`
(SerDes group mode + settle), `pcs_enable` (sanity), `pcs_config` (lane TX/RX
tuning), `pcs_get_state` (link/fault read), and the three MAC ops.

`pcs->poll = true`: no link-change interrupt on this MAC, so phylink polls at
1 s — same cadence the vendor delayed-work used.

## Device tree

`dts/alpine-v2-ubnt-unvr-ea16.dts`:
- `sfp0: sfp { compatible = "sff,sfp"; i2c-bus = <&sfp_i2c>; }` at root
- `sfp_i2c: i2c@1` — PCA9546 channel 1, where the module EEPROM answers (#98)
- eth2 gains `managed = "in-band-status"` and `sfp = <&sfp0>`

The phandle is what removes the #98 failure class permanently: the vendor path
used the board-params i2c **adapter number**, which shifts whenever another i2c
controller is enabled or disabled.

No SFP control GPIOs are declared — none are identified on this board (#112).
`sfp.c` degrades cleanly: no mod-def0 ⇒ cage always reads present
(`sfp.c:3163`), no IRQ ⇒ 1 s poll (`sfp.c:3196`), no tx-disable ⇒ laser always
on with a warning (`sfp.c:3232`). Do not guess pin numbers — a wrong mod-def0
hides the module entirely.

## Kernel config

`CONFIG_PHYLINK` + `CONFIG_SFP` are mandatory; without them
`al_eth_phylink.c` compiles to nothing and the port silently has no link
management. `PHYLINK` has no Kconfig prompt (select-only), so
`scripts/build-linux-71-fedora.py` enables `PCS_XPCS` (prompted,
`select PHYLINK`) plus `SFP`/`MDIO_I2C`/`I2C_MUX_PCA954x` **after**
`localmodconfig`, and FATALs if they are not set afterwards. localmodconfig
cannot know about them by construction — it only sees the driver set the
*currently running* kernel loaded.

## Open items (file as issues)

1. **Nothing here is hardware-tested.** Needs a reboot on the box, then:
   `dmesg | grep -E 'sfp|phylink|enp0s2'`, `ethtool -m enp0s2` (should now
   decode the module without any driver-side i2c), `ethtool enp0s2`,
   `ip link set enp0s2 up`.
2. **1G optics / SGMII unsupported.** Only `10GBASE-R` is advertised, so a 1G
   module is refused by phylink rather than silently mis-configuring the MAC.
   Re-adding it needs `mac_config` to switch `al_eth_mac_config()` + the SerDes
   group mode live, bracketed by `mac_prepare`/`mac_finish`
   (`al_eth_mac_stop`/`al_eth_mac_start`) — untested on this board.
3. **Passive DAC not distinguishable from optic.** `sfp.c` reports
   `10GBASE-R` for both, so `da_tx_params`/`da_rx_params` and the
   `al_eth_rx_equal_run()` dcgain sweep in `al_init_eth_lm.c` are unreachable.
   Fine for the fitted 10GBASE-LR optic; a DAC would run with optic tuning.
4. **`al_init_eth_lm.c` detect/retimer/step-machine is now dead but still
   built** (~1600 LOC: `al_eth_sfp_detect`/`qsfp_detect`/`module_detect`, the
   retimer block, `al_eth_lm_link_detection_step`,
   `al_eth_lm_link_establish_step`). Left in place deliberately — that file had
   in-flight uncommitted work when this landed, and a bad deletion there is
   worse than dead code. Live parts: `al_eth_lm_init()`, the static TX/RX
   parameter setters, `al_eth_lm_static_parameters_*` (sysfs).
5. **`link_poll_interval` sysfs knob** no longer affects this port; phylink owns
   the poll. Either remove it for this port or wire it to phylink.
6. **SFP control GPIOs (#112).** mod-def0 / los / tx-fault / tx-disable are
   still unidentified; the DT node deliberately declares none.

## Retired

- `debris/code/al_eth-group-lm/` — `al_eth_group_lm.c/.h`, `alpine_group_lm.c/.h`
- `al_eth_main.c`: i2c byte read/write shims, `al_eth_lm_config`,
  `al_eth_link_status_task`, `al_eth_lm_mode_change`,
  `al_eth_group_lm_pre_establish`, `al_eth_group_lm_update_port_status`,
  `al_eth_lm_mode_apply`

`al_init_eth_lm.c` is **kept whole and still built**, but nothing calls its
detect/retimer/step-machine half any more. Only `al_eth_lm_init()`, the static
TX/RX parameter setters and the `al_eth_lm_static_parameters_*` sysfs API are
live.
