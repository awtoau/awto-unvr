# Reference sources (offline)

Full catalog: **`/mnt/2tb/unvr-port-refs/MANIFEST.md`** (20 sources, cloned 2026-08-16, ~9.9 GB).
This is the in-repo index of the ones that matter per task — read it before hunting for HAL/driver
source, so nothing gets "lost" again.

**Research writeup:** [hal-provenance-and-cross-system.md](hal-provenance-and-cross-system.md) — HAL
provenance (delroth = UDM GPL extraction, verified by hash), the i2c/RTC ch0-wedge cross-system
survey, and the ccr2004/board-param findings. Read that for the *conclusions*; this file is the
*source list*.

## Ethernet — U-Boot TCP port (RGMII 1G first, then 10G)
- **HAL (the engine):** `delroth-alpine_hal/` — github.com/delroth/alpine_hal @ eb6b9f1. The full
  Annapurna HAL: `include/`+`drivers/` for eth, udma, udma_fast, serdes, iofic, pcie, **ddr**,
  common, + a **`plat_api/`** abstraction and `samples/eth.c` (bring-up reference),
  `samples/dummy_plat.c` (platform-port template). Tri-licensed (Annapurna/GPL-2/BSD).
  - **Provenance (verified):** NOT a unique/reconstructed SDK — an extraction of **Ubiquiti UDM
    GPL sources** (import commit "Import from Ubiquiti UDM GPL sources"), **byte-identical** (git
    object hash) to `fabianishere/udm-kernel/drivers/soc/alpine/HAL`. HAL **version 2.9**.
    OS-independence is Annapurna's original design, present in every copy — not a delroth property.
  - **Vintage caveat:** the UNVR *kernel* HAL (`UNVR-1.3.35-GPL/.../drivers/net/ethernet/al/`) is an
    **older generation** (`al_hal_eth_main.c` 237 KB vs delroth's 273 KB). Same lineage, different
    vintage. Same HAL is public in NETGEAR R9000 / ASUSTOR / FreeBSD F-Stack / RouterOS / CCR2004
    UEFI too. Our U-Boot port builds against delroth 2.9 consistently (HAL + our own DM_ETH glue).
  - **`AL_HAL_EX` proprietary extension is gated but the file isn't shipped** — public port uses the
    public core only, never enable `AL_HAL_EX`.
- **Glue (what to call, U-Boot-native):** `UBNT-source-code/UNVR-1.3.35-GPL/u-boot/drivers/net/al_eth.c`
  (1544 lines) + `al_eth_pci.c` — **U-Boot 2015, pre-DM** (`eth_register`/`eth_device`); HAL is
  NOT in this tree. Reference for the DM_ETH glue.
- **RJ45 = RGMII** (`eth1@fc100000`, `phy-mode="rgmii"`) → **no serdes** for the 1G port.

## Ethernet — Linux 7.2 driver (#79, SEPARATE from U-Boot TCP)
- **Base:** `mornepousse-al_eth-standalone/` — fork ahead of delroth, MDIO-C22 + ethtool for
  kernel ≥6.3; **builds clean on our 7.1.8** (de-risked). Bundles its own HAL copy.
- `delroth-al_eth-standalone/` (5.5 base) — 10G left untested (our path).
- `urnvr-kernel-4.19.152/drivers/net/ethernet/al/` — al_eth ground truth for UNVR.
- `linux-alpine-v2/` — 6.12 LTS UNVR port, tested on real hardware (best kernel-port start).

## DDR BIST (#80)
- **DDR HAL:** `delroth-alpine_hal/ddr/src/` — `al_hal_ddr*.c` (v1/v2/v3 + init + pmu). Same HAL +
  plat_api shim as the eth port.
- Stock commands: `UBNT-source-code/.../u-boot/board/annapurna-labs/common/cmd_ddr.c`,
  `cmd_dram_margins.c`.

## Kernels / U-Boot mainline
- `linux-v7.1.8/` — dev target (our Fedora build). `linux-v6.18/` — newest-LTS port target.
- `linux-v7.3-fresh/` — mainline dev tip (row 4b, `docs/build.md`), commit-pinned not tag-pinned;
  drifts stale fast (checked 2026-08-28: already 2607 commits / ~3 days behind real HEAD).
- `u-boot-v2026.07/` — mainline U-Boot our port overlays.

## Fedora's real kernel patch — checked, confirmed irrelevant to us
Our `build-linux-71-fedora.py` only takes Fedora's `.config` as a config starting point; it never
applies Fedora's own kernel patch. Checked whether that's actually a gap (2026-08-28, fetched
`patch-7.3-redhat.patch` from `src.fedoraproject.org/rpms/kernel`, rawhide branch, 77 files /
+2676/-113 lines) — **it isn't**. Breakdown:
- ~85% of the diff is RHEL-only infrastructure that doesn't compile on Fedora at all: `rh_kabi.h`
  (541 lines, RHEL point-release kABI-stability macros), `rh_waived.c`/`rh_flags.c`/`rh_messages.c`
  (RHEL's "waived items" feature-gate subsystem). Fedora never sets `CONFIG_RHEL_DIFFERENCES`.
- Every hardware-touching hunk is either gated the same way (`pci-driver.c`, `scsi/hpsa.c`,
  `scsi/qla2xxx/qla_os.c`, `scsi/sd.c` - all `#ifdef CONFIG_RHEL_DIFFERENCES`) or hardcoded to
  unrelated silicon: `drivers/ata/libahci.c` + `drivers/pci/quirks.c` fix a **Broadcom Vulcan
  CN99XX** SATA/BAR5 bug (`PCI_VENDOR_ID_BROADCOM, 0x9027`), `drivers/usb/core/hub.c` quirks a
  controller literally string-matched on `"tegra-ehci.0"` (NVIDIA Tegra TrimSlice), `ipmi_dmi.c`/
  `ipmi_msghandler.c` match `DMI_PRODUCT_NAME == "ProLiant m400 Server"`. None of these match
  Annapurna Alpine V2 / ASMedia hardware - dead code for us either way.
- `arch/arm64/kernel/setup.c` adds EFI Secure Boot mode detection via a DT `secure-boot-mode`
  `/chosen` property we never set - no-ops on our boot path (U-Boot chainload, no UEFI).
Not applying this patch is the right call, not a gap. Re-check only if a future Fedora patch
version starts touching PCIe/AHCI/xHCI code outside the RHEL-gated sections.

## Note
`delroth-alpine_hal` is catalogued as a "register/descriptor reference" but is in fact the full
buildable HAL (plat_api + samples) — usable as the port's engine, not just a reg reference.
