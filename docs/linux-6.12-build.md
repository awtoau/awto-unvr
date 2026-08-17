# Linux 6.12 for THIS UNVR (sysid ea16) — cross-build + netboot

Goal: first mainline-era Linux on this unit (ea16, NAND-boot, no eMMC) by
**netboot** — no flashing, stock NAND untouched. Cross-compiled on the x86 host.

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
`dts/alpine-v2-ubnt-unvr.dts` and grounded in this unit's stock DTB
(`docs/hw-reference/20260816-104601/live.dts`) + `docs/nand-1.3.35.md`.

### Changes vs the port's ea1a DTS (with rationale)

| # | Change | Rationale |
|---|--------|-----------|
| 1 | `sysid=ea1a` → `sysid=ea16`, add `ubnthal.sysid=ea16` in `chosen/bootargs`; `model` tagged "(ea16)" | This unit's identity (`identity-partitions.md`). Board table in U-Boot maps ea16 → `bootnand`. |
| 2 | **NAND controller ENABLED** (`nand@fa100000` status `okay`) with the 5-partition map `al_boot / device_tree / linux_kernel / rootfs / chike` | ea16 boots + roots from NAND. The port disabled NAND because ea1a boots eMMC. Map verified on hardware (`nand-1.3.35.md`). |
| 3 | SPI-NOR partition map corrected to this unit's real layout: adds `cksum` tail (`0x1fff000+0x1000`), `config` size `0xdff000`, stock partition labels (`Factory`, `EEPROM`, `recovery kernel`) | Matches `live.dts`. `Factory`/`EEPROM` hold board identity (ubnthal reads them). |
| 4 | `ramoops` `0x12000000` → `0x92000000` | Matches stock reserved region; keeps the pstore area clear of the low kernel + embedded-initramfs load region used for netboot. |
| — | **Unchanged** from the port: RAM (4 GB / 4 banks), internal PCIe + MSI-X, `eth0-3` (port1 RGMII PHY@4, port2 10G SerDes grp3/lane0), i2c-pld (pca9546 mux, pca9575 @0x20/0x21/0x29, s35390a, adt7475), sgpo, `sbclk` 500 MHz, U-logo LEDs, gpio-keys, hdd-pwrctl, sata-leds, external PCIe | ea16 and ea1a share the same SoC and enclosure wiring (confirmed against `live.dts`). |

Notes:
- `iommu.passthrough=1` is kept in the DTB bootargs **and** must be repeated in the
  U-Boot `bootargs` you set for netboot — U-Boot's `setenv bootargs` overrides the DTB value.
- First netboot uses an **embedded initramfs**, so no `root=`. A later NAND-root
  boot would append `root=` (e.g. an MTD/ubi rootfs) via U-Boot bootargs.
- The `pci@fbc00000` node keeps the mainline dtsi's reg/ranges/bus-range/SATA
  interrupt-map and only overrides `compatible = "annapurna-labs,alpine-internal-pcie"`
  + adds `dma-coherent` — this is the compatible the integrated `pcie-al-internal`
  driver binds to (see below).

## PCIe patch integration (REQUIRED — without it the internal bus is empty)

On Alpine V2 every on-SoC block (both AHCI SATA controllers, al_eth, al_dma,
al_ssm, the xHCI/USB) is a PCI endpoint on the **internal** PCIe bus at
`0xfbc00000`. Mainline has **no driver** for `annapurna-labs,alpine-internal-pcie`,
and `CONFIG_PCIE_AL_INTERNAL=y` in `unvr_defconfig` is silently dropped unless the
driver/Kconfig symbol exists. Result of skipping this: `/sys/bus/pci/devices` empty
→ no SATA, no net. `build-linux-612-ea16.py` integrates the port's patches:

1. **`pcie-al-internal.c`** → copied to `drivers/pci/controller/pcie-al-internal.c`;
   a `config PCIE_AL_INTERNAL` block is inserted into that dir's `Kconfig`
   (`select PCI_HOST_COMMON`), and `obj-$(CONFIG_PCIE_AL_INTERNAL) += pcie-al-internal.o`
   into its `Makefile`. `--enable PCIE_AL_INTERNAL` is then asserted and the build
   aborts if `.config` doesn't carry it. The driver uses `pci_host_common_probe` +
   a `BUS_NOTIFY_BIND_DRIVER` notifier that, for each Annapurna device (vendor
   0x1c36) on bus 0, sets **SMCC snoop** (config offsets 0x110/0x130/0x150/0x170,
   bits SNOOP_OVR|SNOOP_EN) and **APP_CONTROL** (0x220 low 16 bits = 0x03ff) — the
   coherent-DMA setup the generic ECAM driver never does.
2. **`pcie-al-dbi-fix.c`** → overwrites `drivers/pci/controller/dwc/pcie-al.c`.
   Adds the 13-line pre-set of `pci->dbi_base = controller_base + 0x10000` so the
   external DWC PCIe (xHCI/USB) probes without a resource conflict.

All the enclosure drivers are already `=y` in `unvr_defconfig` (`SATA_AHCI`,
`SATA_AHCI_PLATFORM`, `I2C_DESIGNWARE_PLATFORM`, `I2C_MUX_PCA954x`,
`GPIO_PCA953X`, `GPIO_SYSFS`, `SENSORS_ADT7475`, `RTC_DRV_S35390A`) — so once the
internal bus enumerates, AHCI + the i2c/gpio expanders bind automatically.

## Out-of-tree modules + initramfs autoload

`al_eth/al_dma/al_ssm/al_sgpo` are built (`make -C /src KDIR=/src M=…`) and
installed into `build-out/initramfs-root/lib/modules/6.12.103/extra/` **before**
the kernel Image is linked, so they ride inside the embedded initramfs.
`depmod -b` is run so `modprobe` resolves them. `/init` loads them in order
(`al_dma al_ssm al_eth al_sgpo`, with an `insmod`-by-path fallback).

Initramfs fixes vs the first attempt:
- **Controlling tty**: busybox here has no `cttyhack`, so a real `/dev/console`
  (plus `/dev/null,tty,ttyS0,kmsg`) is baked in via a `gen_init_cpio` device-node
  list (`initramfs-devnodes`, second `CONFIG_INITRAMFS_SOURCE` entry). `/init`
  ends with `exec setsid -c /bin/sh …/dev/console` → no "can't access tty".
- **On-box tools**: full aarch64 Alpine userland (`ip`, `mount`, `ls -l`, `lsmod`,
  `dmesg`, …) plus a `/bin/hdd-power-on` helper.

### Drive power — gpio-hog (primary) + gpioset fallback

`ui,hdd-pwrctl` has no mainline/out-of-tree driver here, so the SATA bays stay
powered OFF until the PCA9575 @0x21 `pwren` lines (0-3, bays 1-4) go high.

**Primary fix — DTB gpio-hog (zero userspace):** the ea16 DTS hogs those lines
`output-high` on the `gpio@21` node:
```
i2c_gpio1: gpio@21 {           /* PCA9575 @0x21 */
    ...
    hdd-pwren-hog {
        gpio-hog;
        gpios = <0 0>, <1 0>, <2 0>, <3 0>;   /* bays 1-4, active-high */
        output-high;
        line-name = "hdd-pwren";
    };
};
```
gpiolib drives them high at chip-probe, so drives spin up before userspace runs.
(On the stock kernel these same lines — gpiochip base 480, offsets 0-3 — power
the fitted drives on ata5/ata7.) The @0x29 PCA9575 fails i2c (-121) on this board
exactly as on the stock kernel; nothing depends on it.

**Fallback / diagnostic:** the initramfs ships libgpiod + i2c-tools, and `/init`
runs `/bin/hdd-power-on`, which prints `gpiodetect`/`gpioinfo` and (if the hog is
absent) drives the `*-0021` chip lines 0-3 high on the `/dev/gpiochipN` char
device — no `CONFIG_GPIO_SYSFS` needed:
```
gpiodetect                              # find the gpiochip whose label ends 0021
gpioset -c gpiochipN 0=1 1=1 2=1 3=1    # drive bays 1-4 pwren high
```
`CONFIG_GPIO_SYSFS` is also enabled now (via `CONFIG_EXPERT`), so
`/sys/class/gpio/export` works too if you prefer the legacy interface.

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

Built sizes (kernel 6.12.103, gcc-12): `uImage-unvr-ea16` 18.7 MB (modules +
initramfs embedded) · `alpine-v2-ubnt-unvr-ea16.dtb` 15 KB ·
`initramfs-ea16.cpio.gz` 4.1 MB · `al_eth.ko` 401 KB · `al_ssm.ko` 179 KB ·
`al_dma.ko` 91 KB · `al_sgpo.ko` 12 KB. Verified: uImage header ARM64/Linux/
kernel/none, load/entry 0x08080000; `CONFIG_PCIE_AL_INTERNAL=y` linked into
vmlinux; DTB carries the `hdd-pwren-hog` (gpio@21 lines 0-3 output-high);
`CONFIG_GPIO_SYSFS=y` (via `CONFIG_EXPERT`); embedded initramfs carries the 4
`.ko` + `modules.dep` + `gpioset`/`gpiodetect`/`i2cset` + real `/dev/console`.

## Netboot — U-Boot command sequence (NO flashing)

This U-Boot (stock 2015.07 fork) boots a **legacy uImage via `bootm`, unsigned**
(`docs/bootloader.md`, `docs/nor-boot-chain.md`); `tftpboot` is compiled in.
Interrupt autoboot at the `bootdelay=2` prompt to reach the `ALPINE_UBNT_NAS_ALL>`
console. Nothing below writes flash — stock NAND stays intact.

Addresses (chosen to avoid overlap; kernel load/entry baked into the uImage = `0x08080000`):

| Var | Addr | Use |
|-----|------|-----|
| uImage staging | `0x02000000` | TFTP target for the kernel uImage (~25 MB, ends well below the DTB) |
| DTB | `0x04078000` | TFTP target for the ea16 DTB (stock `loadaddr_dt`) |
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
  inside the kernel image). This is the stock `bootunsign` form
  (`bootm $loadaddr_payload - $fdtaddr`), just with our TFTP'd addresses.
- Expect: legacy-image CRC verify → kernel decompress → serial banner from
  `/init` (`UNVR (sysid ea16) - first Linux on the device`) → shell on `ttyS0`.
- To leave: `reboot -f` (or power-cycle). Stock firmware is untouched; a normal
  reboot returns to the stock 4.19 kernel from NAND.

### Optional: separate (non-embedded) ramdisk

If booting `Image` without an embedded initramfs, wrap `initramfs-ea16.cpio.gz`
as a ramdisk uImage and pass it as the 2nd `bootm` arg instead of `-`:
`bootm 0x02000000 0x05000000 0x04078000` (ramdisk at `0x05000000`). The default
build embeds the initramfs, so this is not needed.

## Status / caveats

- **First netboot (pre-patch build) confirmed on hardware**: 6.12.103 booted to a
  serial shell (4 cores, 4 GB). But `/sys/bus/pci/devices` was empty and
  `/lib/modules` had no modules — both fixed by this patched build (internal PCIe
  driver + modules in initramfs).
- This patched build is **not yet re-run on the device**. Expected now: internal
  PCIe enumerates → `/sys/bus/pci/devices` non-empty, AHCI + `ata_port`s appear,
  al_eth/al_dma/al_ssm bind; `/init` powers the bays and auto-loads the modules.
- **Drive power**: no mainline `hdd-pwrctl` driver — `/init` asserts PCA9575 @0x21
  pwren via GPIO sysfs (see above). If a drive still won't spin, drive the pwren
  lines by hand from the shell.
- `al_eth` link bring-up on ea16 is unverified (PHY AR8031 @ MDIO addr 4).
- GCC 16 (host) does **not** build 6.12 — use the gcc-12 Docker toolchain.
  gcc-12 fallback artifacts (pre-patch) are in `build-out/gcc12-fallback/`.
- Building on the device itself is a separate long-term goal — not attempted here.
