# UNVR repurpose — project status (2026-08-16)

Single-glance current state. Detail lives in the linked docs/issues; this is the map.
Device: Ubiquiti UNVR → custom Linux/NAS. Annapurna Alpine V2 / AL-324, quad
Cortex-A57 aarch64, 4 GiB, sysid **0xea16**. Host name for the box: **woomera**.

## Done (verified on hardware)

- **Boot chain fully reversed** — ROM→S2→al_boot(stage2/3 CVOS HAL)→U-Boot→kernel.
  [nor-boot-chain.md](nor-boot-chain.md), [preboot-decompile.md](preboot-decompile.md).
- **Firmware ladder** to 5.1.25 (vendor). [firmware-5.1.25.md](firmware-5.1.25.md).
- **Kernel port, 3 versions, all netboot-verified full-platform**:
  6.12.103 → 6.18.44 LTS → **7.1.8 (latest stable)**. Each: 8 internal-PCIe
  devices, 2×8TB SATA @6G + Samsung SSD, eth0 (1G) + eth1 (10G SFP), al_ssm
  crypto, al_dma 4ch, xHCI SuperSpeed. 6.18→7.1 = **zero new API deltas**.
  [linux-71-build.md](linux-71-build.md), [porting-roadmap.md](porting-roadmap.md).
- **Patch series published** — `kernel-patches/` (6 patches, base 6.18.44, apply
  clean to 6.12/6.18/7.1). Owner-chosen format.
- **Tooling**: `scripts/netboot.py` (atomic catch-U-Boot + tftp + bootm, watchdog-safe),
  `scripts/build-linux-{612,618,71}-ea16.py`, `scripts/build-initramfs-ea16.py`.
- **Drive power root-caused** — PCA9575 @0x21 pwren, gpio-hog auto-powers bays.
- **Security analysis** — exposed secrets, unsigned boot, data remanence (#1,#2).

## In flight

- **Fedora aarch64 on the SSD** (#40) — stock Fedora 44, booted by our U-Boot +
  7.1.8 kernel (no UEFI/GRUB/dracut). Rootfs building on host now.
  [fedora-on-ssd.md](fedora-on-ssd.md). Then dev work self-hosts on woomera.
- **System-improvements audit** — kernel-config / perf / thermal / RAID / Fedora-compat.

## Storage layout (current)

| Dev* | Disk | Role |
|---|---|---|
| sda | SanDisk 64 GB USB (vendor USERDEV) | **to be unplugged** → then SSD becomes sda |
| sdb | Samsung SSD 850 EVO 1 TB (ata3) | **Fedora boot/root** (ESP + 931 GB) |
| sdc | WD82PURZ 8 TB (ata5) | work store ("unvr-work") |
| sdd | WD82PURZ 8 TB (ata7) | spare / RAID pair (raw) |

\* naming shifts by one once the USB is removed. Full USB backup: `images/unvr-usb-*.img`.

## Next

1. Deploy Fedora to SSD (#40): unplug USB → enhanced initramfs → format `sda2` →
   extract rootfs + 7.1.8 modules → U-Boot env `root=/dev/sda2` → saveenv.
   Verify vendor U-Boot can read SATA (kernel on SSD vs NAND).
2. Move dev toolchain + git onto woomera; build kernels on-device.
3. **UEFI/EDK2** as a U-Boot payload (#39) — [uefi.md](uefi.md).
4. Apply improvements from the audit (kernel config, perf, thermal, overclock #29).
5. Benchmarks on 7.1 (#41) + network throughput (#42).

## Operational notes (gotchas)

- **U-Boot has a ~50 s prompt watchdog** — catch + netboot must be atomic
  (`netboot.py` handles it). Send `reboot` from a shell BEFORE streaming ESC
  (streaming at a bash prompt triggers tab-completion, not U-Boot capture).
- **`panic=15`** in bootargs — a console/host drop kills the PID-1 initramfs shell
  → "Attempted to kill init" panic; panic=15 auto-reboots instead of stranding.
- Console creds (vendor): root:ui (al324 gen). See [credentials.md](credentials.md).

## Issue index (by theme)

- **Meta/tracking**: #4 (port), #37 (repo structure), #36 (upstream ledger).
- **Boot independence**: #3 (U-Boot decompile), #26 (bootROM), #27/#28 (flash layout / DDR),
  #30 (MTD for UEFI), **#39 (EDK2/UEFI)**.
- **Porting phases/patches**: #5–#25 (per-subsystem), #31–#35.
- **Milestones**: #32 (MD RAID), **#40 (Fedora on SSD)**.
- **Perf/OC/validation**: **#29 (overclock)**, **#41 (7.1 matrix)**, **#42 (network)**.
- **Chips/datasheets**: #38.
- **Security**: #1, #2.
