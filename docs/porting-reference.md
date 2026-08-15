# UNVR → generic Linux/NAS: reference

Goal: repurpose the UNVR as a NAS behind its 10G SFP+.
Researched 2026-08-15. Facts from the stock firmware we downloaded and analysed,
plus the prior art at the end.

Repo activity, open issues, forks and the GPL-source situation are traced
separately in [prior-art.md](prior-art.md).

## Headline: the port already exists

**[bcyangkmluohmars/linux-alpine-v2](https://github.com/bcyangkmluohmars/linux-alpine-v2)**
— mainline **Linux 6.12 LTS** for Alpine V2, with a `alpine-v2-ubnt-unvr.dts` and a
`unvr_defconfig`, **tested on a real UNVR**. Every UNVR peripheral is marked working,
10GbE SFP+ included. Do not start the driver work from scratch — start here.

The GPL kernel source is also public: **[UrNVR/unvr-kernel](https://github.com/UrNVR/unvr-kernel)**,
"UNVR Kernel as distributed by Ubiquiti", Linux **4.19.152** — the exact version the
stock firmware runs. Contains `drivers/net/ethernet/al/` (`al_eth.c` + full Annapurna
HAL: serdes, UDMA, MAC v1–v4, KR link management, retimers). riptidewave93's "can't get
the GPL source" note is out of date.

## Hardware

| | |
|---|---|
| SoC | Annapurna Labs (Amazon) Alpine V2, quad Cortex-A57 @ 1.7 GHz |
| Arch | **aarch64** |
| RAM | 4 GB |
| Board ID | `ea1a` (UNVR). UDM Pro is `ea15`. |
| Boot media | 8 GB USB stick, **glued down** (older units). Newer use eMMC. |
| Net | 10GbE SFP+ + 1GbE RJ45 |
| Bays | 4 hot-swap |

Same SoC also in Ubiquiti UNAS / UDM-Pro, QNAP TS-x32x, MikroTik RB1100AHx4 — so
fixes cross-pollinate between those projects.

Ours is an older USB-boot unit — see [recovery.md](recovery.md).

## Measured performance — reality check

From linux-alpine-v2, tested on a real UNVR (kernel 6.12.77, Alpine Linux 3.21 rootfs):

- **RAID5, 3× 3 TB HDD: 57 MB/s write, 204 MB/s read**
- HW crypto AES-XTS priority 400 (beats ARM CE at 300)
- Fans: 3×, PWM controllable, 2600–8600 RPM

**57 MB/s write is far below the 250–600 MB/s often assumed for a 3–4 disk array.**
Planning figures derived from raw HDD throughput do not survive contact with this
box — the A57 cores and the storage path bottleneck long before the 10G link does.
Benchmark before committing to a TrueNAS layout. ZFS on 4 GB RAM will be tighter
still than the mdraid figure above.

## Serial console — bring-up starts here

- **UNVR:** 4 pins on the PCB behind the SFP+ cage, middle of board. Only 3 needed:
  GND, TXD, RXD. Ignore the 3V3 pin.
- **UNVR Pro:** 4 pins near the DC Power Backup port.
- **115200 8N1.**
- u-boot shell: press **Esc twice within ~2 s** of power-on.
- Glued USB stick: hair dryer + dental floss, or heat gun.

## QEMU: what it can and cannot do

Checked `hw/arm/` in `/mnt/2tb/git_mirror/qemu` — there is **no Alpine/Annapurna
machine model**, only generic `virt.c` and `sbsa-ref.c`. Therefore:

- Stock u-boot is Alpine-V2 register-level code. On `-M virt` it faults immediately.
- **"One u-boot image for UNVR + TrueNAS + Fedora under QEMU" does not work** — that
  u-boot only runs on the real SoC.
- A `virt` VM with a generic aarch64 kernel boots and gives a serial console, but
  touches no Alpine hardware, so it validates nothing about this port.
- `al_eth` cannot be tested under QEMU at all — no device model to emulate against.

**What QEMU is for here:** `qemu-user-static`, user-mode emulation to
debootstrap/build an **aarch64 rootfs on this x86 box**. That is how UNVR-NAS builds
its image. Not system emulation.

Real bring-up happens on the hardware UART. Related: someone documented booting UDM
firmware to user space under emulation — see sources.

## Firmware container anatomy — decoded and verified

`sources/UNVR-5.1.25.bin`, 786253430 bytes. See [../sources/README.md](../sources/README.md).

Format is Ubiquiti's own, published under GPL in `mkfwimage` — `src/fw.h` in
[openwrt/firmware-utils](https://github.com/openwrt/firmware-utils). Verified
byte-for-byte against our image, **all CRCs valid**.

```
header  268 B (0x10C): magic[4] "UBNT" + version[256] + crc32 u32 BE @0x104 + pad
                       header crc = crc32(0, image, 0x104)
record   56 B (0x38):  magic[4] + name[16] + pad[12] + 6x u32 BE:
                       memaddr, index, baseaddr, entryaddr, data_size, part_size
        payload data_size B at record+0x38
        trailer  8 B: crc32(0, record_start, 0x38+data_size) u32 BE + pad
        stride = 0x38 + data_size + 8
END.    12 B  terminator, crc32 over the whole preceding image
ENDS   264 B  terminator, 256-byte RSA signature, no crc
```

**Walk the length chain, never scan for magics.** `PART`/`FILE` occur inside
compressed payloads, and the tag differs between revisions — UNVR 4.1.9 used
`FILE` for rootfs, 5.1.25 uses `PART`. The tag is cosmetic; the chain is the
structure. Known magics: `UBNT`/`OPEN`/`GEOS` (header), `PART`, `FILE`, `EXEC`,
`GART`, `END.`, `ENDS`.

### Sections in UNVR 5.1.25

| # | Off | Magic | Name | data_size | part_size | base |
|---|---|---|---|---|---|---|
| 1 | 0x0000010C | FILE | `uboot` | 1395740 (1.33 MB) | 2097152 | 0x00000000 |
| 2 | 0x00154D68 | FILE | `kernel` | 13846342 (13.20 MB) | 14680064 | 0x00200000 |
| 3 | 0x00E894EE | PART | `rootfs` | 766357504 (730.86 MB) | 1010827264 | 0x01000000 |
| 4 | 0x2E96452E | FILE | `updater` | 4653056 (4.44 MB) | 5242880 | 0x3D400000 |
| — | 0x2EDD456E | ENDS | — | — | — | RSA-signed |

**The image is `ENDS`-terminated — RSA-signed.** A modified container cannot be
re-signed, so repacking a stock `.bin` is not a route. Replace the `uImage` on the
boot partition instead.

`uboot` is the Annapurna bootloader — strings `AL_PBS_REGFILE_BASE`,
`AL_ETH_TX_FLAGS_*`, build path `/project/users/barak/tasks/al-boot-release/`.

Rootfs squashfs at 0x00E89526: SQUASHFS 4:0, **zstd**, block 262144, 63607 inodes,
built 2026-07-10 18:10:55. Read in place: `unsquashfs -o 15242534 <bin>`.

### Kernel

`kernel` section payload is a **legacy U-Boot uImage** (64-byte header, magic
`0x27051956`), gzip, arm64:

| | |
|---|---|
| uImage name | `4.19.152-alpine-unvr @ 20260710-` |
| load / entry | **0x04080000** |
| Built | 2026-07-10T08:11:27Z |
| Decompressed | 20744704 bytes, arm64 `ARM\x64` magic at 0x38 confirmed |
| Toolchain | gcc (Debian 10.2.1-6) 10.2.1 |

Extract everything with `scripts/analyse-unvr-firmware.py --extract` →
`tmp/sections/` (payloads, `kernel-Image.gz`, `kernel-Image`, `kernel.config`).

The SPI-NOR `mtd5 "recovery kernel"` is a **separate** kernel, not this one.

## Stock kernel config — settles the big questions

`CONFIG_IKCONFIG=y`, so the config is embedded in the kernel image and we
extracted it offline (92970 bytes) without needing a running device.

| Setting | Value | Consequence |
|---|---|---|
| `CONFIG_MODULE_SIG` | **not set** | **Module signing is NOT enforced.** You can `insmod` your own modules. Kills the long-standing "Ubiquiti is signing modules" worry, at least on 5.1.25 |
| `CONFIG_KEXEC` | **not set** | kexec is not built in — it would need `kexec-mod`, which is unported to Alpine V2. **kexec is definitively out** |
| lockdown | **absent entirely** | No kernel lockdown |
| `CONFIG_IKCONFIG_PROC` | `y` | `/proc/config.gz` also available on a running box |
| `CONFIG_NET_AL_ETH` | `y` | al_eth **built in**, not a module — no `.ko` to lift |
| `CONFIG_AL_DMA`, `CONFIG_AL_HAL` | `y` | Built in |
| `CONFIG_AL_THERMAL_V2`, `_V3` | `y` | Built in |
| `CONFIG_ARCH_ALPINE` | `y` | |
| `CONFIG_AL_ETH_ALLOC_FRAG` | `y` | frag allocator (not PAGE, not SKB) |
| `CONFIG_AL_ETH_FORCE_SFP_1G` | not set | SFP+ not pinned to 1G |

Built into the vendor kernel (from `modules.builtin`, so no `.ko` to lift):
`al_eth_drv`, `al_dma_drv`, `soc/alpine/hal`, `soc/alpine/al_hal_export`.
Shipped as modules: `al_nand`, `marvell10g`, `phylink`, `ubnthal`, `ubnt_common`,
`ui-hdd-pwrctl`.

## Mainline gap

Mainline **has**: `arch/arm64/boot/dts/amazon/alpine-v2.dtsi` + `alpine-v2-evp.dts`,
`drivers/irqchip/irq-alpine-msi.c`, `drivers/pci/controller/dwc/pcie-al.c`,
`ARCH_ALPINE`.

Mainline **lacks**: `al_eth` (no `drivers/net/ethernet/al/` — `amazon/ena` is the
AWS virtual NIC, unrelated), `al_dma`, the Alpine SoC HAL. Amazon bought Annapurna in
2015, pivoted to Graviton, and never upstreamed these.

linux-alpine-v2 closes that gap out-of-tree:

| Component | What |
|---|---|
| `pcie-al-internal.c` (patch) | Internal PCIe host controller, AXI SMCC snoop + APP_CONTROL |
| `pcie-al-dbi-fix.c` (patch) | DWC PCIe DBI base offset fix for external PCIe (xHCI/USB) |
| `quirks.c` (snippet) | PCI fixup for AXI snoop on all AL PCI devices |
| `al_eth` (module) | 1GbE + 10GbE, shared MDIO. From delroth/al_eth-standalone, 5.5 → 6.12 |
| `al_dma` (module) | RAID5/6 HW parity (XOR/PQ) |
| `al_ssm` (module) | HW AES-XTS/CBC |
| `al_sgpo` (module) | Serial GPIO, HDD bay LEDs — reverse-engineered |
| `rtl8370mb` (module) | 8-port switch, UDM Pro only |

UNVR support status per that repo: CPU, internal+external PCIe, 1GbE, **10GbE SFP+
(LM mode)**, shared MDIO, AHCI SATA, RAID5/6 parity, AES crypto, bay LEDs, ADT7475
fans, pca953x GPIO, MSI-X, xHCI, S35390A RTC, SP805 watchdog, SPI flash — all marked
working.

### Which `al_eth` to port from — revisit before building

linux-alpine-v2's `al_eth` derives from `delroth/al_eth-standalone`, whose source
is a **QNAP drop targeting Linux 4.2.8** 📄 — *older* than either Ubiquiti tree.
Meanwhile the best published Alpine V2 `al_eth` is Ubiquiti's own **UDM 1.12.22**
revision in `fabianishere/udm-kernel` `flavour/stock-v4.19` (326,611 bytes) ✅,
which over the UNVR tree adds device-tree board parameters, GPIO-descriptor SFP
control (`mod-def0`/`los`/`tx-fault`/`tx-disable`), I2C SFP shims, refactored MDIO
setup, and LM-aware ethtool. Full comparison in [sources.md](sources.md).

So there are two candidate bases for a 6.12 driver:

1. **linux-alpine-v2's existing module** — already builds and runs on 6.12, but on
   an older upstream.
2. **Re-port from UDM 1.12.22** — better driver glue, but the 6.12 forward-port
   work would have to be redone.

Not yet resolved. The pragmatic path is (1) to get booting, then diff (2) in for
the SFP/DT handling if the 10G link misbehaves. Note the UNVR tree carries
`al_mod_eth_lm_retimer_ds125.c`, which the UDM trees lack — our SFP cage may need
it, so a straight swap to the UDM revision is not safe.

**AL-314 vs AL-324:** linux-alpine-v2's README says AL-314; our firmware string is
literally `al324` ✅. Both are Alpine V2 and the names are used interchangeably in
the wild — do not key anything on the digits.

Build: Docker (Debian Bookworm, `aarch64-linux-gnu`), `make ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu- unvr_defconfig`, then `Image dtbs modules`, then the
out-of-tree modules (`rtl8370mb` needs `KBUILD_EXTRA_SYMBOLS` from `al_eth`).

## Hardware gotchas worth knowing before you start

From linux-alpine-v2's reverse-engineering notes — not documented elsewhere:

1. **AXI Sub-Master Snoop (SMCC)** — internal PCI devices need regs 0x110/0x130/0x150/0x170
   with bits 0+1 (`SNOOP_OVR | SNOOP_ENABLE`) for cache-coherent DMA. Without it TX DMA
   reads stale cache.
2. **APP_CONTROL (0x220)** — lower 16 bits must be `0x03FF` or DMA does not work. Found
   by `vmlinux-to-elf` disassembly of stock firmware.
3. **DBI base** is `controller_base + 0x10000`, not `controller_base`.
4. **ECAM** is at `0xfb600000` (1 MB); the 64 KB region at `0xfd810000` is DBI, not ECAM.
5. `al,alpine-msix` node needs `interrupt-controller` + `#interrupt-cells` for `of_irq_init()`.
6. `linux,pci-probe-only` must be in `/chosen` and `= <1>` (u32, not boolean).
7. `iommu.passthrough=1` must be in **DTB bootargs**, not u-boot env — u-boot's
   `setenv bootargs` overrides DTB.
8. **Shared MDIO** — all MACs share one physical bus; first port with `phy_exist`
   registers it, others reuse.
9. **AT803X PHY needs `CONFIG_REGULATOR=y`** — otherwise `CONFIG_AT803X_PHY=y` is
   silently dropped by Kconfig (`depends on REGULATOR`).
10. **EEPROM identity** in the `eeprom` MTD partition (typically `/dev/mtd4ro`), read by
    `ubnthal.ko` at `/proc/ubnthal/board`. Layout: MAC @0x0000 (6), board ID @0x000C (2),
    HW rev @0x000E (2), device ID @0x0010 (4), magic `UBNT` @0x8000 (4).
11. **Platform detection without ubnthal** — DT `compatible`, or `boardid=` in `/proc/cmdline`.
12. `docker export` writes `/dev/console` as a regular file, which blocks `devtmpfs` and
    panics with "No working init found". Fix: `rm -rf $ROOTFS/dev && mkdir -p $ROOTFS/dev`.

## Install process (UNVR-NAS route)

Build: needs `docker-ce losetup wget sudo make qemu-user-static squashfs-tools`, Linux
host only. `BOARD=UNVR make` (or `BOARD=UNVRPRO`), ~1 hour, output in `./output`.
Requires the matching stock .bin in `unifi-firmware/` — no prebuilt images distributed,
to avoid redistributing Ubiquiti IP. Device firmware version **must match** the build's
reference, or things break (touchscreen cited).

1. Put the built image on an ext4 HDD/SSD, install as the only drive.
2. Attach UART, power on, Esc-Esc into u-boot.
3. ```
   setenv rootfs PARTLABEL=rootfs
   setenv bootargsextra boot=local rw
   saveenv
   ```
4. `run bootcmdrecovery` — or hold reset ~10 s while powering on.
5. Log in `ubnt:ubnt` or `root:ubnt` (UART, or telnet).
6. `mount /dev/sda1 /mnt`, `gunzip`, `dd if=... of=/dev/boot bs=4M`, `sync`, `reboot`.
   **On older USB-boot units replace `/dev/boot` with the USB device path.**

### UrNVR route (no UART needed)

Buildroot-based, own kernel, installs over SSH:

- Requires stock firmware **2.3.14**.
- SSH in (`root@<ip>`, password `ubnt`).
- **Back up `/dev/mtd*` and `/dev/boot` first** via `ssh ... dd if=/dev/boot | dd of=backup_boot`.
- Mount `/dev/boot1`, drop in `urnvr.conf`, keep `uImage.bkp` of the original `uImage`,
  copy the new `uImage` over, reboot. SSH password becomes `urnvr`.
- Enable setup in the config, reboot, it downloads a rootfs.

### Removal / restore to stock

1. Boot recovery, telnet in.
2. `cat /proc/mtd` and **verify** before erasing. Expected:
   ```
   mtd0: 001c0000 00001000 "u-boot"
   mtd1: 00010000 00001000 "u-boot env"
   mtd2: 00010000 00001000 "u-boot env redundant"
   mtd3: 00010000 00001000 "Factory"
   mtd4: 00010000 00001000 "EEPROM"
   mtd5: 01000000 00001000 "recovery kernel"
   mtd6: 00e00000 00001000 "config"
   ```
3. `dd if=/dev/zero of=/dev/mtd1`, same for `mtd2`.
4. `/sbin/parted -s -- /dev/boot mklabel gpt`
5. Restore stock via the Recovery WebUI.

## Sources

| What | Where |
|---|---|
| **Linux 6.12 LTS port for Alpine V2, UNVR DTS + defconfig, al_eth/al_dma/al_ssm/al_sgpo, RE notes, measured perf** | <https://github.com/bcyangkmluohmars/linux-alpine-v2> |
| **Ubiquiti-distributed GPL kernel source, 4.19.152, full `drivers/net/ethernet/al/`** | <https://github.com/UrNVR/unvr-kernel> |
| `al_eth` standalone driver (upstream of the 6.12 port) | <https://github.com/delroth/al_eth-standalone> |
| Alpine HAL extraction | <https://github.com/delroth/alpine_hal> |
| Alpine V2 work on QNAP TS-x32x (same SoC) | <https://github.com/delroth/linux-qnap-tsx32x> |
| Alpine V2 kernel sources (Codeberg mirror) | <https://codeberg.org/filefly/linux-alpine-v2> |
| Alpine thermal driver for modern kernels | <https://github.com/mornepousse/al_thermal-standalone> |
| UNVR→Debian+OpenMediaVault builder; install/removal, MTD map, UART pinout | <https://github.com/riptidewave93/UNVR-NAS> |
| Buildroot-based UNVR OS; SSH install, no UART | <https://github.com/UrNVR/urnvr> |
| UNVR exploration; hardware specs, serial settings, USB stick removal, boot logs | <https://github.com/NeccoNeko/UNVR-diy-os> |
| UBNT GPL source archive | <https://github.com/NeccoNeko/UBNT-source-code> |
| Emulating UDM firmware into user space (QEMU) | <https://emulatedbox.wordpress.com/2024/12/12/emulating-ubiquity-dream-machine-firmware-booting-into-user-space/> |
| Firmware API (version, size, sha256, URL) | `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr` |
| Recovery Mode procedure | <https://help.ui.com/hc/en-us/articles/360043360253-UniFi-Recovery-Mode> |
| Failed-USB replacement procedure | <https://www.incredigeek.com/home/unifi-unvr-recover-from-failed-usb-drive/> |
| Community thread, 368 replies — USB failure is a known defect | <https://community.ui.com/questions/UNVR-stopped-responding-just-white-flashing-light/a051f869-8349-4a2a-a72a-ce3a8aa8c759> |
| Mainline Alpine support (DTs, irqchip, PCIe) | `/mnt/2tb/git_mirror/linux` |
| QEMU ARM machine models (no Alpine) | `/mnt/2tb/git_mirror/qemu` `hw/arm/` |

Credits chain: Bootlin did the original Alpine V2 upstreaming (2017); delroth
extracted the HAL and the standalone `al_eth`; riptidewave93 documented the boot
chain; linux-alpine-v2 pulled it onto 6.12.

Licences: kernel patches GPL-2.0; out-of-tree modules GPL-2.0 (Annapurna HAL is
dual GPL/commercial); device trees GPL-2.0 OR MIT.

Warranty: sticker over a chassis screw, glued USB stick. Opening voids it.
