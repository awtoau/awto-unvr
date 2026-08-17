# Linux 7.1.8 for UNVR ea16 — forward-port of the 6.18 build

Goal: same netboot bring-up as [linux-6.18-build.md](linux-6.18-build.md) but on
**7.1.8**. Cross-built on the host, netboot only (no flashing, stock NAND
untouched). This doc records ONLY the 6.18 -> 7.1 deltas; the mechanism
(embedded initramfs, gpio-hog drive power, uImage/bootm) is unchanged — see the
6.12 / 6.18 docs.

- Source: `/mnt/2tb/unvr-port-refs/linux-v7.1.8` (Makefile VERSION 7.1.8).
- Script: `scripts/build-linux-71-ea16.py` (native host build, no docker).
- Artifacts: `/mnt/2tb/unvr-port-refs/build-out-71/` (kept separate from 6.18/6.12).
- KVER = `7.1.8-dirty` (tree has the copied-in driver files; no git tag).

## Toolchain — gcc-16 native (no docker)

- **Host `aarch64-linux-gnu-gcc` 16.1.1** (Fedora `gcc-aarch64-linux-gnu`), `make -j32`.
- 7.1.8 builds **clean on gcc-16**; same toolchain as 6.18. Build time ~120 s.
- `CONFIG_WERROR` + `TRIM_UNUSED_KSYMS` disabled (same two toggles as 6.18).

## Forward-port deltas 6.18 -> 7.1 (the valuable output)

**All THREE 6.18 API adaptations persist UNCHANGED at 7.1.8. Zero new deltas.**
The Annapurna-specific surface stayed stable across 6.18 -> 7.1; the same
build-script edits apply byte-for-byte.

### 1. `pcie-al-internal.c` — internal PCIe host driver (CRITICAL) — unchanged

- `pci_host_common_probe` / `pci_host_common_remove` still declared in the
  **private** `drivers/pci/controller/pci-host-common.h` (not `<linux/pci-ecam.h>`).
  Fix: add `#include "pci-host-common.h"` (same as 6.18).
- `platform_driver.remove_new` still gone (removed 6.15); `.remove` is the
  void-returning callback, `pci_host_common_remove()` still returns void.
  Fix: `.remove_new = pci_host_common_remove` -> `.remove = ...` (same as 6.18).
- Everything else compiles as-is: `pci_ecam_ops` / `pci_ecam_map_bus`,
  `BUS_NOTIFY_BIND_DRIVER` notifier, SMCC snoop (0x110/0x130/0x150/0x170,
  `SNOOP_OVR|SNOOP_EN`), `APP_CONTROL 0x220 = 0x03ff`. `bus_register_notifier(&pci_bus_type, …)` still compiles.

Verified: `pcie-al-internal.o` compiles; `al_pcie_internal_*` symbols in
`System.map`; `CONFIG_PCIE_AL_INTERNAL=y`.

### 2. `pcie-al.c` — external DWC PCIe DBI fix (xHCI/USB) — unchanged

- 7.1.8's in-tree `drivers/pci/controller/dwc/pcie-al.c` still carries
  `pci->pp.native_ecam = true;` and the same
  `dev_dbg(dev, "From DT: controller_base: %pR\n", …)` anchor.
- So the 13-line `dbi_base = controller_base + 0x10000` pre-set is **inserted in
  place** after that anchor (keeps `native_ecam`), same as 6.18. Net effect =
  the 6.12 `pcie-al-dbi-fix.c`.
- Requires `CONFIG_PCIE_AL=y` (in `unvr_defconfig`; script re-asserts it).

Verified: patched file has both `native_ecam` (1×) and `controller_res->start + 0x10000` (1×); `pcie-al.o` compiles.

### 3. `al_sgpo.c` — SGPO LED gpio driver — unchanged

- `gpio_chip.set` signature still `int (*set)(struct gpio_chip *gc, unsigned int
  offset, int value)` at 7.1.8 (the 6.15 void->int change; no further churn, no
  `.set_rv` rename affecting us). gcc-16 makes the void mismatch a hard error.
- Fix (build script, applied to the copy): `al_sgpo_set` returns `int` — early
  `return -EINVAL` on out-of-range group, `return 0` on success (same as 6.18).

### DTS — unchanged

`alpine-v2-ubnt-unvr-ea16.dts` compiles against 7.1.8's `alpine-v2.dtsi` with no
edits. dtsi already `io-bus@fc000000` (renamed at 6.18). Same benign dtc
warnings on the *disabled* `pcie-external1/2/3` nodes (node name not "pci"/"pcie").

### al_eth / al_dma / al_ssm — built UNCHANGED

- All three build against 7.1.8 with **no source edits** — the 6.12 `kcompat.h`
  and HAL are still API-compatible (netdev/phylink/dmaengine/crypto drift did not
  materialise, same as 6.18).
- Only warnings (non-fatal, all pre-existing in Annapurna code):
  `al_hal_serdes_25g.c` frame 3216 B; three `al_hal_ssm_*` frames 2336 B;
  `al_eth_main.c:846 'phydev' used uninitialized`. **Runtime bind on 7.1
  unverified** (see risks).

## Build (reproduce)

```bash
# initramfs is REUSED from the 6.12 build-out (build-initramfs-ea16.py output);
# the 7.1 script clones it (minus modules) and rebuilds the al_*.ko against 7.1.
python3 scripts/build-linux-71-ea16.py
# logs -> tmp/logs/linux-71-build.log
```

Script flow (native host, identical to 6.18):
1. Copies `unvr_defconfig` + ea16/ea1a/udmpro DTS into the 7.1.8 tree; registers
   the ea16 DTB in `arch/arm64/boot/dts/amazon/Makefile`.
2. Integrates the two PCIe patches with the (unchanged) 7.1 adaptations above.
3. Clones the 6.12 `initramfs-root` (**`symlinks=True`**), drops old `lib/modules`,
   refreshes banner to "Linux 7.1 first boot".
4. `make unvr_defconfig`; `scripts/config` (disable WERROR/TRIM_UNUSED_KSYMS,
   enable PCIE_AL + PCIE_AL_INTERNAL + EXPERT + GPIO_SYSFS, set INITRAMFS_SOURCE);
   `make olddefconfig`; asserts both PCIE symbols `=y`.
5. `make modules` (vmlinux + Module.symvers), then the four out-of-tree `al_*`
   (per-module failure non-fatal + reported), installs `.ko` into the embedded
   initramfs, `depmod`.
6. `make Image dtbs` (links Image embedding the now-populated initramfs).
7. Pure-python uImage wrap (magic 0x27051956, ARM64/Linux/kernel/none,
   load/entry 0x08080000) — no `mkimage` on host.

## Artifacts (`/mnt/2tb/unvr-port-refs/build-out-71/`)

| File | What |
|------|------|
| `uImage-unvr-ea16-7.1` | legacy U-Boot kernel image, initramfs embedded — the `bootm` target |
| `Image` | raw arm64 kernel (pre-uImage) |
| `alpine-v2-ubnt-unvr-ea16-7.1.dtb` | ea16 device tree |
| `initramfs-ea16-7.1.cpio.gz` | standalone initramfs (also embedded) |
| `initramfs-root/` | initramfs contents (reused alpine + 7.1 `al_*.ko`) |
| `modules/*/al_*.ko` | out-of-tree drivers |
| `unvr-ea16-7.1.config` | exact `.config` used |

Sizes (7.1.8, gcc-16): `uImage-unvr-ea16-7.1` 18.7 MB (18665544 B) · `Image` 18 MB ·
`alpine-v2-ubnt-unvr-ea16-7.1.dtb` 15 KB · `initramfs-ea16-7.1.cpio.gz` 4.1 MB ·
`al_eth.ko` 403 KB · `al_ssm.ko` 169 KB · `al_dma.ko` 84 KB · `al_sgpo.ko` 11 KB.
Verified: uImage header magic 0x27051956 / arch ARM64 (22) / OS Linux (5) / type
kernel (2) / comp none (0), load=entry 0x08080000, name `unvr-ea16-7.1`,
dcrc 0x63e6a180; `al_pcie_internal_*` in `System.map`; `CONFIG_PCIE_AL=y` +
`CONFIG_PCIE_AL_INTERNAL=y` + `CONFIG_GPIO_SYSFS=y`; DTB carries `hdd-pwren-hog`;
embedded initramfs has all four `al_*.ko` + `modules.dep`.

## Netboot — use scripts/netboot.py (atomic catch+tftp+bootm)

`./scripts/netboot.py --tag 7.1` — streams ESC to catch U-Boot, then fires the
whole sequence on one socket session. Needed because the stock U-Boot has a
**~50s watchdog at the prompt**: catching in one step and sending commands in
another leaves a gap the watchdog resets into, and the next autoboot lands in
the stock OS. tftp keeps U-Boot busy so the watchdog stays fed.

- Order matters: send `reboot` from the stock shell FIRST, wait for shutdown to
  start, THEN launch netboot.py — streaming ESC at the stock bash prompt
  triggers tab-completion (`--More--` pager), not U-Boot capture.
- Adds `panic=15` to bootargs → a panic auto-reboots to U-Boot instead of
  stranding (a host/console drop kills the PID-1 initramfs shell → "Attempted to
  kill init" panic; `panic=15` recovers it).
- Manual equivalent (served filenames only differ from 6.12/6.18):
  `tftpboot 0x02000000 uImage-unvr-ea16-7.1` · `tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-7.1.dtb` · `bootm 0x02000000 - 0x04078000`.

## Netboot VERIFIED on hardware — 2026-08-16

`7.1.8-dirty` (gcc-16.1.1) netbooted on the UNVR via `scripts/netboot.py`. Full
platform parity with 6.18 — every flagged risk area cleared:

- **Internal PCIe**: 8 devices (2× al_eth 1c36:0001/0002, al_ssm 0022/100000,
  al_dma 0022/010400, 2× ahci 0031/010601, + the external xHCI bridge).
- **SMCC snoop**: configured on all slots (ahci 8/9 SM0-only; al_dma 5, al_ssm 4,
  al_eth 1/2 all-4-submasters). AXI-coherency patch works unchanged on 7.1.
- **SATA**: ata5/ata7 = WDC WD82PURZ @ 6.0 Gbps; ata3 = **Samsung SSD 850 EVO
  1TB** identified cleanly (ATA-9, UDMA/133) — the drive that misclassified on
  6.12/6.18 enumerated correctly here.
- **al_eth** (risk #1 CLEARED): eth0 (1G RJ45) + eth1 (10G SFP) both bound.
- **al_ssm**: AES-XTS / AES-CBC async crypto up. **al_dma**: 4 channels (XOR/PQ).
- **xHCI** (risk #2 `native_ecam` CLEARED): USB 3.0 SuperSpeed device enumerated.
- Reached the initramfs serial shell. `of_irq_parse_pci: rc=-22` benign as before.
