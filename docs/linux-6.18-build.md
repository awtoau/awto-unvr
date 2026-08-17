# Linux 6.18.44 LTS for UNVR ea16 — forward-port of the 6.12 build

Goal: same netboot bring-up as [linux-6.12-build.md](linux-6.12-build.md) but on
**6.18.44 LTS**. Cross-built on the host, netboot only (no flashing, stock NAND
untouched). This doc records ONLY the 6.12 -> 6.18 deltas; the mechanism
(embedded initramfs, gpio-hog drive power, uImage/bootm) is unchanged — see the
6.12 doc for it.

- Source: `/mnt/2tb/unvr-port-refs/linux-v6.18.44` (Makefile VERSION 6.18.44).
- Script: `scripts/build-linux-618-ea16.py` (native host build, no docker).
- Artifacts: `/mnt/2tb/unvr-port-refs/build-out-618/` (kept separate from 6.12).

## Toolchain — gcc-16 native (no docker)

- **Host `aarch64-linux-gnu-gcc` 16.1.1** (Fedora `gcc-aarch64-linux-gnu`), `make -j32`.
- 6.18 builds **clean on gcc-16**; no docker toolchain needed. Contrast 6.12,
  which does NOT build on gcc-16 and needs the gcc-12 Debian image.
- `CONFIG_WERROR` disabled defensively; `TRIM_UNUSED_KSYMS` disabled so the
  out-of-tree `al_*` modules resolve kernel symbols. Same two toggles as 6.12.
- KVER = `6.18.44-dirty` (tree has the copied-in driver files; no git tag).

## Forward-port deltas (the valuable output)

The 6.12 Annapurna patch set forward-ports to 6.18 with **three small changes**,
all mechanical API renames. No datapath/logic changes. This is the Stage-13
signal: the Annapurna-specific surface is tiny and stable across 6.12 -> 6.18.

### 1. `pcie-al-internal.c` — internal PCIe host driver (CRITICAL)

Two 6.18 API adaptations, applied by the build script to the copied-in file:

- **`pci_host_common_probe` / `pci_host_common_remove` moved out of the public
  header.** In 6.12 they were in `<linux/pci-ecam.h>`; in 6.18 they live in the
  private `drivers/pci/controller/pci-host-common.h`. Fix: add
  `#include "pci-host-common.h"` (relative — the driver sits in that dir).
- **`platform_driver.remove_new` removed (6.15).** `.remove` is now the
  void-returning callback. `pci_host_common_remove()` already returns void in
  6.18. Fix: `.remove_new = pci_host_common_remove` -> `.remove = ...`.

Everything else is unchanged and compiles as-is: the `pci_ecam_ops` /
`pci_ecam_map_bus`, the `BUS_NOTIFY_BIND_DRIVER` bus notifier, and the SMCC snoop
writes (0x110/0x130/0x150/0x170, `SNOOP_OVR|SNOOP_EN`) + `APP_CONTROL 0x220 =
0x03ff`. `bus_register_notifier()` now takes `const struct bus_type *` — passing
`&pci_bus_type` still compiles.

Verified: `drivers/pci/controller/pcie-al-internal.o` compiles;
`CONFIG_PCIE_AL_INTERNAL=y` in `.config`.

### 2. `pcie-al.c` — external DWC PCIe DBI fix (xHCI/USB)

- 6.18's in-tree `drivers/pci/controller/dwc/pcie-al.c` is **byte-identical to the
  6.12 base the port patched, plus one new line `pci->pp.native_ecam = true;`**.
- So instead of OVERWRITING it with the port's 6.12 copy (which would drop
  `native_ecam` — a regression), the build script **inserts the 13-line
  `dbi_base = controller_base + 0x10000` pre-set in place**, keeping the 6.18
  line. Net effect identical to the 6.12 `pcie-al-dbi-fix.c`, cleaner forward-port.
- Requires `CONFIG_PCIE_AL=y` (already in `unvr_defconfig`; script re-asserts it).

Verified: `drivers/pci/controller/dwc/pcie-al.o` compiles.

### 3. `al_sgpo.c` — SGPO LED gpio driver

- **`gpio_chip.set` (and `set_multiple`) return `int` now (6.15+)**, not void.
  gcc-14+ makes the mismatched function-pointer assignment a hard error
  (`-Werror=incompatible-pointer-types`), so this is fatal, not a warning.
- Fix (build script, applied to the copy): `al_sgpo_set` returns `int` — early
  `return -EINVAL` on out-of-range group, `return 0` on success.

### DTS — unchanged

`alpine-v2-ubnt-unvr-ea16.dts` compiles against 6.18's `alpine-v2.dtsi` with no
edits. The dtsi's only 6.12->6.18 changes are cosmetic and don't touch the board
file: GIC gained `#address-cells = <0>`, and `io-fabric@fc000000` was renamed
`io-bus@fc000000` (the ea16 DTS references neither — it uses the `uart0`/`uart2`/
`sbclk` labels, which still exist, and its own `soc { pci@fbc00000 … }`).
Benign dtc warnings only, on the *disabled* `pcie-external1/2/3` nodes (node name
not "pci"/"pcie") — same as 4.19/6.12, no functional effect.

### al_eth / al_dma / al_ssm — built UNCHANGED

- All three build against 6.18 with **no source edits** — the 6.12 `kcompat.h`
  and HAL are still API-compatible for these. This was the expected risk area
  (netdev/phylink/dmaengine drift); it did not materialise for a straight build.
- Only warnings (non-fatal): `-Wmissing-prototypes` (HAL statics), a few
  `-Wimplicit-fallthrough`, two `-Wframe-larger-than` (serdes 3216 B, ssm_crypto
  2336 B), and one `al_eth_main.c:846 'phydev' used uninitialized`. All
  pre-existing in the Annapurna code; none blocks the build. **Runtime bind on
  6.18 is unverified** (see risks).

## Build (reproduce)

```bash
# initramfs is REUSED from the 6.12 build-out (build-initramfs-ea16.py output).
# Just run the 6.18 kernel build; it clones that initramfs (minus modules),
# installs the 6.18 al_* modules, and embeds it.
python3 scripts/build-linux-618-ea16.py
# logs -> tmp/logs/linux-618-build.log
```

What the script does (native host, mirrors the 6.12 flow):
1. Copies `unvr_defconfig` + the ea16/ea1a/udmpro DTS into the 6.18 tree;
   registers the ea16 DTB in `arch/arm64/boot/dts/amazon/Makefile`.
2. Integrates the two PCIe patches with the 6.18 adaptations above.
3. Clones the 6.12 `initramfs-root` (**`symlinks=True`** — the alpine rootfs is
   hundreds of busybox symlinks; dereferencing them balloons 11 MB -> 400 MB and
   bloats the Image), drops its old `lib/modules`, refreshes the banner.
4. `make unvr_defconfig`; `scripts/config` (disable WERROR/TRIM_UNUSED_KSYMS,
   enable PCIE_AL + PCIE_AL_INTERNAL + EXPERT + GPIO_SYSFS, set
   INITRAMFS_SOURCE); `make olddefconfig`; asserts both PCIE symbols `=y`.
5. `make modules` (builds vmlinux + Module.symvers), then the four out-of-tree
   `al_*` (per-module failure is non-fatal + reported), installs the `.ko` into
   the embedded initramfs, `depmod`.
6. `make Image dtbs` (links Image embedding the now-populated initramfs).
7. Pure-python uImage wrap (magic 0x27051956, ARM64/Linux/kernel/none,
   load/entry 0x08080000) — no `mkimage` on host, same as 6.12.

## Artifacts (`/mnt/2tb/unvr-port-refs/build-out-618/`)

| File | What |
|------|------|
| `uImage-unvr-ea16-6.18` | legacy U-Boot kernel image, initramfs embedded — the `bootm` target |
| `Image` | raw arm64 kernel (pre-uImage) |
| `alpine-v2-ubnt-unvr-ea16-6.18.dtb` | ea16 device tree |
| `initramfs-ea16-6.18.cpio.gz` | standalone initramfs (also embedded) |
| `initramfs-root/` | initramfs contents (reused alpine + 6.18 `al_*.ko`) |
| `modules/*/al_*.ko` | out-of-tree drivers |
| `unvr-ea16-6.18.config` | exact `.config` used |

Sizes (6.18.44, gcc-16): `uImage-unvr-ea16-6.18` 18.9 MB (modules + initramfs
embedded) · `Image` 19 MB · `alpine-v2-ubnt-unvr-ea16-6.18.dtb` 15 KB ·
`initramfs-ea16-6.18.cpio.gz` 4.1 MB · `al_eth.ko` 396 KB · `al_ssm.ko` 167 KB ·
`al_dma.ko` 84 KB · `al_sgpo.ko` 12 KB. Verified: uImage header
magic 0x27051956 / ARM64 / Linux / kernel / none, load=entry 0x08080000;
`al_pcie_internal_*` symbols linked into vmlinux (`System.map`);
`CONFIG_PCIE_AL=y` + `CONFIG_PCIE_AL_INTERNAL=y` + `CONFIG_GPIO_SYSFS=y`; DTB
carries `hdd-pwren-hog` (gpio@21 lines 0-3 output-high); embedded initramfs has
all four `al_*.ko` + `modules.dep`.

## Netboot — same U-Boot sequence, new filenames

Identical to the 6.12 flow ([linux-6.12-build.md](linux-6.12-build.md#netboot--u-boot-command-sequence-no-flashing));
only the served filenames change:

```
tftpboot 0x02000000 uImage-unvr-ea16-6.18
tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-6.18.dtb
setenv bootargs 'console=ttyS0,115200 sysid=ea16 ubnthal.sysid=ea16 reboot=warm rw iommu.passthrough=1 pci=pcie_bus_perf'
bootm 0x02000000 - 0x04078000
```

## Netboot VERIFIED on hardware — 2026-08-16

`6.18.44-dirty` (gcc-16.1.1) netbooted on the UNVR. Full platform parity with
6.12 — every flagged risk area cleared:

- **Internal PCIe**: 8 devices enumerated (non-empty). 2× al_eth (1c36:0001/0002),
  al_ssm (0022 class 100000), al_dma (0022 class 010400), 2× ahci (0031 class 010601).
- **SMCC snoop**: configured on all slots — `registered SMCC snoop notifier`;
  ahci slots 8/9 (SM0 only), al_dma/al_ssm/al_eth slots 4/5/1/2 (all 4 sub-masters).
  The AXI-coherency patch works unchanged on 6.18.
- **SATA**: both WD82PURZ up on ata5/ata7 @ 6.0 Gbps (gpio-hog powered the bays).
- **al_eth** (risk #1 — CLEARED): eth0 (1G RJ45, phy@4) + eth1 (10G SFP, media 5)
  both bound and up; runtime bind on 6.18 works.
- **al_ssm**: AES-XTS / AES-CBC async crypto engine initialized.
- **al_dma**: 4 channels (XOR/PQ max 31 sources) — RAID offload live.
- **xHCI** (risk #2 — `native_ecam` on USB path CLEARED): USB 3.0 SuperSpeed host
  up, a SuperSpeed device enumerated on bus 2.
- `of_irq_parse_pci: failed with rc=-22` is the same benign fallback as 6.12
  (drivers use their own MSI/IRQ setup); non-fatal.

Netboot command sequence: unchanged from the block above.
