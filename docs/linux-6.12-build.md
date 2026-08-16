# Linux 6.12 for THIS UNVR (sysid ea16) — cross-build + netboot

Goal: first mainline-era Linux on this unit (ea16, NAND-boot, no eMMC) by
**netboot** — no flashing, vendor NAND untouched. Cross-compiled on the x86 host.

- Base port: `/mnt/2tb/unvr-port-refs/linux-alpine-v2` (community 6.12 Alpine V2, target ea1a).
- Kernel source: mainline stable **6.12.103** (`linux-6.12.y`), `/mnt/2tb/unvr-port-refs/linux-6.12`.
- Toolchain: the port's Docker image (Debian Bookworm, `gcc-aarch64-linux-gnu` 12) —
  chosen over the host gcc 16 to match the port authors' tested build.
- Scripts (this repo): `scripts/build-linux-612-ea16.py`, `scripts/build-initramfs-ea16.py`.
- Artifacts: `/mnt/2tb/unvr-port-refs/build-out/`.

## The one real porting task: ea1a → ea16 device tree

Same SoC (Alpine V2 / AL-324), different boot/storage layout. The port's DTS
targets **ea1a** (eMMC UNVR, boots eMMC-over-USB, NAND disabled). This unit is
**ea16** (no eMMC, boots from NAND). New DTS:
`dts/alpine-v2-ubnt-unvr-ea16.dts`, derived from the port's
`dts/alpine-v2-ubnt-unvr.dts` and grounded in this unit's vendor DTB
(`docs/hw-reference/20260816-104601/live.dts`) + `docs/nand-1.3.35.md`.

### Changes vs the port's ea1a DTS (with rationale)

| # | Change | Rationale |
|---|--------|-----------|
| 1 | `sysid=ea1a` → `sysid=ea16`, add `ubnthal.sysid=ea16` in `chosen/bootargs`; `model` tagged "(ea16)" | This unit's identity (`identity-partitions.md`). Board table in U-Boot maps ea16 → `bootnand`. |
| 2 | **NAND controller ENABLED** (`nand@fa100000` status `okay`) with the 5-partition map `al_boot / device_tree / linux_kernel / rootfs / chike` | ea16 boots + roots from NAND. The port disabled NAND because ea1a boots eMMC. Map verified on hardware (`nand-1.3.35.md`). |
| 3 | SPI-NOR partition map corrected to this unit's real layout: adds `cksum` tail (`0x1fff000+0x1000`), `config` size `0xdff000`, vendor labels (`Factory`, `EEPROM`, `recovery kernel`) | Matches `live.dts`. `Factory`/`EEPROM` hold board identity (ubnthal reads them). |
| 4 | `ramoops` `0x12000000` → `0x92000000` | Matches vendor reserved region; keeps the pstore area clear of the low kernel + embedded-initramfs load region used for netboot. |
| — | **Unchanged** from the port: RAM (4 GB / 4 banks), internal PCIe + MSI-X, `eth0-3` (port1 RGMII PHY@4, port2 10G SerDes grp3/lane0), i2c-pld (pca9546 mux, pca9575 @0x20/0x21/0x29, s35390a, adt7475), sgpo, `sbclk` 500 MHz, U-logo LEDs, gpio-keys, hdd-pwrctl, sata-leds, external PCIe | ea16 and ea1a share the same SoC and enclosure wiring (confirmed against `live.dts`). |

Notes:
- `iommu.passthrough=1` is kept in the DTB bootargs **and** must be repeated in the
  U-Boot `bootargs` you set for netboot — U-Boot's `setenv bootargs` overrides the DTB value.
- First netboot uses an **embedded initramfs**, so no `root=`. A later NAND-root
  boot would append `root=` (e.g. an MTD/ubi rootfs) via U-Boot bootargs.
- The out-of-tree `al_eth` / SATA (internal PCIe) are **not** needed to reach a
  serial shell; they matter for a real root-on-NAND / networked boot later.

## Build (reproduce)

```bash
# 1. initramfs (aarch64 Alpine via docker+qemu-aarch64 binfmt) — run once
python3 scripts/build-initramfs-ea16.py

# 2. kernel Image (initramfs embedded) + ea16 DTB + out-of-tree modules + uImage
python3 scripts/build-linux-612-ea16.py
# logs -> tmp/logs/linux-build.log
```

What `build-linux-612-ea16.py` does:
1. Copies `unvr_defconfig` + the ea16/ea1a/udmpro DTS into the 6.12 tree; registers
   `alpine-v2-ubnt-unvr-ea16.dtb` in `arch/arm64/boot/dts/amazon/Makefile`.
2. Builds the port's Docker toolchain image.
3. In-container: `make unvr_defconfig`; `scripts/config` tweaks
   (`--disable WERROR`, `--disable TRIM_UNUSED_KSYMS`,
   `INITRAMFS_SOURCE=/initramfs-root`, `INITRAMFS_ROOT_UID/GID=0`, gzip);
   `make olddefconfig`; `make -j Image dtbs modules`; then each out-of-tree module
   (`al_eth al_dma al_ssm al_sgpo`) via `make -C /src KDIR=/src M=…`.
4. Host: writes the legacy **uImage** in pure Python (no `mkimage` on host):
   magic `0x27051956`, OS Linux, arch **ARM64 (22)**, type kernel, comp none,
   CRC32 over data + header, load/entry **0x08080000**.

`WERROR` is disabled defensively; `TRIM_UNUSED_KSYMS` is disabled so the
out-of-tree modules resolve kernel symbols.

## Artifacts (`/mnt/2tb/unvr-port-refs/build-out/`)

| File | What |
|------|------|
| `uImage-unvr-ea16` | legacy U-Boot kernel image (CRC32, unsigned), initramfs embedded — the thing you `bootm` |
| `Image` | raw arm64 kernel (same, pre-uImage-wrap) |
| `alpine-v2-ubnt-unvr-ea16.dtb` | this unit's device tree, for `$fdtaddr` |
| `initramfs-ea16.cpio.gz` | standalone initramfs (also embedded in the uImage) |
| `initramfs-root/` | initramfs contents (aarch64 Alpine + `/init` serial-shell) |
| `modules/*/al_*.ko` | out-of-tree drivers |
| `unvr-ea16.config` | exact kernel `.config` used |

Built sizes (kernel 6.12.103, gcc-12, ~122 s on 31 cores):
`uImage-unvr-ea16` 18 MB · `Image` 18 MB · `alpine-v2-ubnt-unvr-ea16.dtb` 15 KB ·
`initramfs-ea16.cpio.gz` 3.7 MB · `al_eth.ko` 401 KB · `al_ssm.ko` 179 KB ·
`al_dma.ko` 91 KB · `al_sgpo.ko` 12 KB. uImage header verified: arch ARM64,
OS Linux, type kernel, comp none, load/entry 0x08080000. Module vermagic
`6.12.103 … aarch64`.

## Netboot — U-Boot command sequence (NO flashing)

This U-Boot (vendor 2015.07 fork) boots a **legacy uImage via `bootm`, unsigned**
(`docs/bootloader.md`, `docs/nor-boot-chain.md`); `tftpboot` is compiled in.
Interrupt autoboot at the `bootdelay=2` prompt to reach the `ALPINE_UBNT_NAS_ALL>`
console. Nothing below writes flash — vendor NAND stays intact.

Addresses (chosen to avoid overlap; kernel load/entry baked into the uImage = `0x08080000`):

| Var | Addr | Use |
|-----|------|-----|
| uImage staging | `0x02000000` | TFTP target for the kernel uImage (~25 MB, ends well below the DTB) |
| DTB | `0x04078000` | TFTP target for the ea16 DTB (vendor `loadaddr_dt`) |
| kernel run | `0x08080000` | where `bootm` relocates + enters the kernel (in the uImage header) |

```
# --- on your TFTP server: serve uImage-unvr-ea16 and
#     alpine-v2-ubnt-unvr-ea16.dtb from the tftp root ---

# set networking (static example; or use 'dhcp')
setenv ipaddr 192.168.1.50
setenv serverip 192.168.1.10

# load kernel + dtb into RAM
tftpboot 0x02000000 uImage-unvr-ea16
tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16.dtb

# bootargs (repeat iommu.passthrough here — U-Boot overrides the DTB value)
setenv bootargs 'console=ttyS0,115200 sysid=ea16 ubnthal.sysid=ea16 reboot=warm rw iommu.passthrough=1 pci=pcie_bus_perf'

# boot: kernel, no ramdisk (initramfs is embedded), dtb
bootm 0x02000000 - 0x04078000
```

- `bootm <kernel> - <fdt>` — the `-` means "no separate ramdisk" (initramfs is
  inside the kernel image). This is the vendor's `bootunsign` form
  (`bootm $loadaddr_payload - $fdtaddr`), just with our TFTP'd addresses.
- Expect: legacy-image CRC verify → kernel decompress → serial banner from
  `/init` (`UNVR (sysid ea16) - first Linux on the device`) → shell on `ttyS0`.
- To leave: `reboot -f` (or power-cycle). Vendor firmware is untouched; a normal
  reboot returns to the stock 4.19 kernel from NAND.

### Optional: separate (non-embedded) ramdisk

If booting `Image` without an embedded initramfs, wrap `initramfs-ea16.cpio.gz`
as a ramdisk uImage and pass it as the 2nd `bootm` arg instead of `-`:
`bootm 0x02000000 0x05000000 0x04078000` (ramdisk at `0x05000000`). The default
build embeds the initramfs, so this is not needed.

## Status / caveats

- Not yet run on the device (serial console in use). This is the netboot recipe +
  artifacts; on-device verification is the next step.
- `al_eth` link bring-up on ea16 is unverified (PHY is AR8031 at MDIO addr 4 per
  `live.dts`; the driver probes addr 4 regardless of vendor). Serial shell does
  not depend on it.
- Building on the device itself is a separate long-term goal — not attempted here.
