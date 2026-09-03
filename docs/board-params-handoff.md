# Board params — the bootloader→Linux handoff

How `al_eth` learns what hardware it is driving, why it is fragile, and what any
bootloader of ours must write. Load-bearing for #216 (SSD boot), #39 (EDK2),
#199/#207 (SerDes override), #200 (wrong board description).

---

## 1. The channel is two hardware registers, not device tree

The bootloader writes two 32-bit words into the **MAC's own scratch registers**;
Linux reads them back at probe:

| | register | written by | read by |
|---|---|---|---|
| board params **1** | `mac_1g.scratch` | `al_eth_board_params_set()` | `al_eth_board_params_get()` |
| board params **2** | `mac_10g.scratch` | same | same |

Per port. `al_hal_eth_main.c`, write ~`:5030`, read ~`:5114`.

**This is not device tree.** Nothing in the DT reaches `al_eth` — it is a PCI
driver bound by ID, and it matches its DT node only by `port-id` for phylink/SFP.
Every board fact below travels through those two registers.

---

## 2. Register 1 is also the validity marker — the sharp edge

`al_eth_board_params_get()` opens with:

```c
uint32_t reg = al_reg_read32(&mac_regs_base->mac_1g.scratch);

/* check if the register was initialized, 0 is not a valid value */
if (reg == 0)
    return -ENOENT;
```

and `al_eth_main.c` treats that as fatal:

```c
rc = al_eth_board_params_get(adapter->mac_base, &params);
if (rc) {
    dev_err(&adapter->pdev->dev, "board info not available\n");
    return -1;
}
```

**Probe fails. No netdev. No ethernet.** The setter guards the same invariant
from the other side, `al_hal_eth_main.c:5049`:

```c
al_assert(reg != 0);
al_reg_write32(&mac_regs_base->mac_1g.scratch, reg);
```

### Consequences

- **A port whose fields are all zero writes 0 and bricks its own probe.** Any
  writer must guarantee at least one non-zero field. In practice `media_type`
  and `serdes_grp` do it, but that is a property of our values, not of the code —
  do not rely on it silently.
- **Register 2 has no validity check at all.** Its trustworthiness is inferred
  entirely from register 1 being non-zero. So "reg 1 written, reg 2 not" reads as
  perfectly valid zeros — and zero in reg 2 means
  `dont_override_serdes = false`, `force_1000_base_x = false`, `dac = false`,
  `retimer_exist = false`. **Write both, always.**
- **"Deliberately false" and "never written" are indistinguishable** in register
  2. This is why #207 concludes `freeze-serdes-params` proves nothing about
  whether a value is correct.
- These are **MAC registers, not fuses**. Undefined on cold boot until someone
  writes them; they survive a warm reset, so a stale value from a previous boot
  can be read as current.

---

## 3. Field layout

### Register 1 — `mac_1g.scratch`

| bits | field |
|---|---|
| 3:0 | `media_type` |
| 4 | `phy_exist` |
| 9:5 | `phy_mdio_addr` |
| 10 | `sfp_plus_module_exist` |
| 11 | `autoneg_enable` |
| 12 | `kr_lt_enable` |
| 13 | `kr_fec_enable` |
| 15:14 | `mdio_freq` |
| 19:16 | `i2c_adapter_id` |
| 21:20 | `phy_if` |
| 22 | `an_mode` (in-band / out-of-band) |
| 23 | `serdes_grp` bit 2 |
| 26:25 | `serdes_grp` bits 1:0 |
| 28:27 | `serdes_lane` |
| — | `ref_clk_freq` |

`serdes_grp` is **split** across bits 23 and 26:25 — a 3-bit value in two pieces.
Our 10G port is group 3, so both halves matter.

### Register 2 — `mac_10g.scratch`

| bits | field |
|---|---|
| 0 | **`dont_override_serdes`** |
| 1 | `force_1000_base_x` |
| 2 | `an_disable` |
| 4:3 | `speed` |
| 5 | `half_duplex` |
| 6 | `fc_disable` |
| 7 | `retimer_exist` |
| 11:8 | `retimer_bus_id` |
| 18:12 | `retimer_i2c_addr` |
| 19 | `retimer_channel` |
| 23:20 | `dac_length` |
| 24 | `dac` |
| 26:25 | `retimer_type` |
| 28:27 | `retimer_channel_2` |
| 31:29 | `retimer_tx_channel` |

---

## 4. Who writes them today

| bootloader | writes board params? | consequence |
|---|---|---|
| stock U-Boot | **yes**, from its `/soc/board-cfg/ethernet/portN` DT | Linux probes fine — this is why everything works today |
| awto-uboot | **no** | masked only because stock runs first and chainloads us |
| EDK2 | **no** (no ethernet code at all yet) | would fail the moment it booted Linux |

The HAL setter is already present in the U-Boot tree
(`uboot-port/drivers/net/al_eth/hal/include/al_hal_eth.h:2348`); the only call in
`uboot-port` is a **get**.

**So the current arrangement depends on stock U-Boot for correct ethernet, and
that dependency is invisible** — nothing logs "board params came from stock".
#216 removes stock from the boot path, which is exactly when this bites.

---

## 5. The ground truth to reproduce

Linux prints what it decoded. On the working box:

```
al_eth_10g 0000:00:02.0: Board info: phy exist No.  phy addr 0. mdio freq 2500 Khz. SFP connected Yes. media 5
al_eth_1g  0000:00:01.0: Board info: phy exist Yes. phy addr 4. mdio freq 1000 Khz. SFP connected No.  media 1
```

**Any writer of ours must reproduce those two lines exactly.** That is the pass
condition — cheap, and it exercises the whole path rather than the code that
built the word.

`media 5` = auto-detect-auto-speed (10G port); `media 1` = the 1G RGMII port.

---

## 6. Source of the values

Stock reads them from its own `board-cfg` DT. We have that DT carved out:
`tmp/uboot-dtbs/dtb00-*.dts`, node `/soc/board-cfg/ethernet/port1` and `port2`
(see `docs/mtd.md`). Our own Linux DTS carries the same facts on
`eth1`/`eth2` — `serdes-grp`, `serdes-lane`, `phy-addr`, `dac-length`,
`auto-neg`, `link-training`, `fec`, `force-1000base-x`.

**Prefer our DTS over hardcoded constants**, so the board description has one
home.

### Known-wrong values we must reproduce anyway, for now

The stock board-cfg describes the SFP+ port as **direct-attach copper**:
`dac = "enabled"`, `dac-length = <3>`, `force-1000base-x = "enabled"` — with a
10GBASE-SR **fibre optic** fitted (#200).

Reproduce stock's values as-is initially. Matching behaviour and correcting the
description are two changes; doing them together makes a regression
un-attributable.

---

## 7. Rules for any bootloader we write

1. **Write both registers, for both ports.** Register 2's zeros are silently
   "valid".
2. **Guarantee register 1 ends non-zero.** It is the only validity marker.
3. **Reproduce the two `Board info` lines** before claiming success.
4. **Do not treat a read as proof of a write** — warm-reset survival means a
   stale value reads as current.
5. `dont_override_serdes` stays **false** unless deliberately changed; the
   `serdes freeze` command is the knob (#197/#199).
6. Source values from DT, not literals.

## 8. Worth fixing eventually

A private hardware register is a poor interface: invisible in DT, unverifiable,
bootloader-specific, undefined cold. Mainline's mechanism for exactly this is
device tree, and U-Boot's `ft_board_setup()` fixups exist to do it. Recorded in
#207 as the direction; not a prerequisite for #216.
