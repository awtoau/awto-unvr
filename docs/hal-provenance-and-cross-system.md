# Annapurna HAL provenance + cross-system survey

Consolidated research behind the U-Boot eth/DDR/serdes port. Answers three questions asked of the
source corpus: (1) what the HAL-provenance report established, (2) how other systems handle the
i2c/RTC ch0 wedge, (3) what authoritative sources exist that we had not mined.

Source catalog: [reference-sources.md](reference-sources.md) → `/mnt/2tb/unvr-port-refs/MANIFEST.md`
(20 sources, ~9.9 GB, cloned 2026-08-16). Issues: #83 (eth), #85 (ccr2004), #86 (RTC), #87 (scratchpad).

## 1. HAL provenance (what "delroth" actually is)

- **delroth/alpine_hal @ eb6b9f1 is an EXTRACTION of Ubiquiti UDM GPL sources**, not a unique or
  reconstructed SDK. Import commit titled "Import from Ubiquiti UDM GPL sources".
- **Byte-identical to `fabianishere/udm-kernel/drivers/soc/alpine/HAL/`**: `drivers/` subtree git
  tree hashes match across eth/udma/serdes/pcie/iofic/ssm/sys_fabric. `al_hal_eth_main.c` =
  273,184 bytes, blob SHA `b87f550f2841fb42225bea4f3f2afdaa45bacf04`, same in both.
- **OS-independence is Annapurna's original design**, not a delroth property — README: "Operating
  System independent low-level drivers"; ships `include/plat_api/sample/al_hal_plat_services.h`.
- **Same HAL is public in many trees**: NETGEAR R9000 GPL (2013-era), ASUSTOR (ships a
  `standalone_files_prepare.sh` that assembles it from the kernel tree), FreeBSD/F-Stack
  (`contrib/alpine-hal/`), RouterOS GPL, CCR2004 UEFI. Not a "missing" or "uniquely recovered" HAL.
- **HAL version 2.9** (README); `version.txt` says 2.6 — Alpine V2 support is explicit regardless
  (ddr_alpine_v2, platform/alpine_v2, V1/V2 MAC, release notes 2.7→2.8 "V2 support").
- **Proprietary extension exists but is NOT shipped**: `proprietary/file_list_ex.mk` gated by
  `AL_HAL_EX`. The core is public; never enable `AL_HAL_EX`.
- **Vendor U-Boot al_eth is NOT self-contained**: 2015.07, pre-DM (`eth_device`/`eth_register`),
  expects external `HAL_TOP`. Good as a call-order/glue reference, not a drop-in.

Reassessed earlier assertions: "vendor U-Boot incomplete" CONFIRMED; "HAL maintained separately"
CONFIRMED; "HAL left out of public sources" FALSE (it's in the UDM-derived GPL + others);
"vendor glue + delroth HAL" YES; "DM_ETH adaptation needed" YES.

### Architecture the report endorses (what we built)
`modern U-Boot net → DM_ETH wrapper → al_eth glue → {eth,udma,serdes} HAL → U-Boot plat_api → AL324`.
Top boundary: pre-DM `eth_register` → `UCLASS_ETH`/`eth_ops`. Bottom boundary: plat_api on U-Boot
(MMIO, barriers, cache, delays, DMA xlate). **CCR2004 EDK2 shim is the concrete porting model.**

### Don't-port-blindly cautions (carried into review #88)
- DMA coherency on AL324 in U-Boot is not a given — flush/invalidate may be needed (review #3).
- SerDes may be pre-configured by an earlier boot stage — blind reset/retrain can break a good config.
- Descriptor addresses are physical/bus, not U-Boot pointers.
- Board params (PHY addr, media, refclk, MDIO freq, serdes group/lane, retimer) → §3, not blind copy.

## 2. i2c/RTC ch0 wedge — cross-system survey (issue #86)

Searched mainline (4.19/6.12/6.18/7.1.8), UBNT 4.1.37, delroth HAL, all Alpine/UDM/UNVR/QNAP board
DTs. **No system combines s35390a + pca9546 + DesignWare with a working bus recovery.**

- **s35390a driver:** UBNT 4.1.37 does **reset-at-probe** when POC|BLD set. **Mainline (incl. 7.1.8)
  does NOT** — probe only reads STATUS1; RESET only via set_time()/RTC_VL_CLR. So mainline leaves a
  never-set-time chip in the wedge-prone state. *(This is the regression we reintroduced and now fix
  in U-Boot — see [rtc-s35390a-fault.md](rtc-s35390a-fault.md); the auto reset-at-probe lives in
  `board_late_init`.)*
- **DesignWare recovery:** `i2c_recover_bus()` exists but only if the DT wires `scl-gpios`
  (bit-bang). UBNT 4.1.37 DW core + Annapurna `al_hal_i2c.c` have **no recovery at all**. Zero DW
  i2c nodes anywhere wire scl-gpios.
- **Board DTs:** no Alpine board wires hold-time / idle-disconnect / mux-reset / scl-gpios. **QNAP
  sidesteps it by topology** — rx8010 RTC on the main bus, not behind the mux.

### Recovery paths + why each is blocked
1. **GPIO bit-bang 63-clock deblock:** generic recovery clocks only 9 (s35390a needs 63 → custom
   recover_bus), AND there is no mainline Alpine pinctrl/gpio on the pld SCL/SDA net (the only GPIOs
   are the PCA9575 expanders — on the wedging bus, chicken/egg). Blocked without a hardware tap.
2. **PCA9546 `reset-gpios`:** deselects channels → frees the MAIN bus (containment, stops the SFP
   flood) but does NOT clock the RTC → not real recovery. Verify if the board routes the !RESET pin.
3. **RTC power-cycle** (datasheet 0→1.3V/10ms): the only recovery needing no SCL clocking;
   MS621 backup-cell dependent.

Cheap wins with no hardware unknowns: adopt reset-at-probe (done), keep mux deselect-after-transfer
(U-Boot uclass does this by default — the #88 RTC fix relies on it), investigate a !RESET route.

## 3. Authoritative sources we had under-mined

### ccr2004-uefi — a WORKING AL324 firmware port (issue #85)
`imbushuo/ccr2004-uefi` is a real EDK2/UEFI port for the **same SoC**. Refutes the old uefi.md P3
"no EDK2 al_eth driver exists."
- **`Library/AlpineHalLib/`** = delroth-vintage HAL 2.9 compiled against a non-Linux plat shim
  (EDK2 BaseLib/IoLib/TimerLib) — a concrete, buildable model for our plat_api shim, better than the
  `dummy_plat` template. Same subset we chose (eth/udma/iofic/unit_adapter/pbs; no ddr/serdes).
- **`Drivers/AlEthNextDxe/`** = full al_eth init/send/recv against HAL 2.9 in firmware — a stronger
  sequence reference than the 2015 pre-DM vendor glue, for both 1G and 10G.
- Sibling DXE refs: **`DwI2cDxe`** (our i2c-pld), **`Pca954xDxe`** (our PCA9546!), `AlNandDxe`,
  `SpiNorFlashDxe`, `PlatformGpioDxe`, `FanDxe`.
- **`SpiLoader/`** = MikroTik open S2-equivalent (ELF-over-SPI, no DDR init) — independent
  confirmation of our "keep the proprietary S2 for DDR" decision.

### al_eth board params live in the MAC SCRATCHPAD, not DT (issue #87)
`al_eth_board_params_get(mac_base,...)` decodes `mac_1g.scratch`/`mac_10g.scratch`
(al_hal_eth_main.c ~625); stock U-Boot board.c WRITES them via `al_eth_board_params_set`
(al_hal_eth_main.c:4905-5115). **The DTS serdes/phy props on eth2 are documentation only — al_eth
ignores them.**
- Chainloaded U-Boot works because stock populated the scratchpad before `go`.
- **Standalone (Stage-3) MUST replicate `al_eth_board_params_set`** or al_eth reads reset-state
  garbage. That struct→register encoding is the authoritative spec to port. ("Assume nothing", made
  concrete.)

Pinned AL324/UNVR board params (triple-sourced: UNVR dts + UDM-Pro dts + board.c RE):
- **10G SFP+:** serdes group 3, lane 0; 10gbase-r (media 5, optic); **no external PHY** (addr 0);
  **no retimer**; direct SerDes→SFP. MDIO 2500 kHz is a driver default constant, not board data.
- **1G RJ45:** rgmii; PHY addr 4; MDIO 1000 kHz; AR8031/8033 (at803x); ref clk 500 MHz; shared MDIO.

### Retire/close list (RE superseded by source)
- hardware.md 10G open Qs: "which PHY + MDIO addr" → NO PHY, addr 0, direct SerDes; "SFP MDIO
  2500 kHz" → driver default. Trim eth2 DTS serdes props to doc-only; down-rank dt-gaps C3/C4/C7.
- `al_bootstrap` ddr_pll_freq: a source reader exists (`al_hal_bootstrap.c` `al_bootstrap_parse`) →
  trim the "live-open param" note in uboot-ddr-port §7 / ddr-config-reverse §7.
- preboot-decompile.md HAL-function VA tables → trim to "confirmed-by-source, impl = delroth HAL".
- **KEEP (genuinely irreplaceable RE):** ddr-config-reverse.md + ddr-s2-parser-analysis.md — they
  reverse the proprietary S2 stage2_loader blob. No published source has AL324 DDR-from-reset
  (confirmed zero hits; even CCR2004 keeps a proprietary DDR loader).
