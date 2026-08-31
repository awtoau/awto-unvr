# UNVR repurpose — project status (2026-08-17)

Single-glance current state. Detail lives in the linked docs/issues; this is the map.
**Kernel/build status below is dated - see [build.md](build.md) for the current,
accurate pipeline map and kernel version (this repo tracks `torvalds/linux`
mainline directly, not a pinned "latest stable" release).**
Device: Ubiquiti UNVR → custom Linux/NAS. Annapurna Alpine V2 / AL-324, quad
Cortex-A57 aarch64, 4 GiB, sysid **0xea16**. Host name for the box: **woomera**.

## Done (verified on hardware)

- **Fedora 44 aarch64 boots standalone on woomera** — no host, no netboot, no
  UEFI/GRUB/dracut. Kernel in NAND, rootfs on the SATA SSD.
  [fedora-on-ssd.md](fedora-on-ssd.md) §PERSISTENT. #40 closed.
  - U-Boot **can't read SATA** (`scsi init` → 0 devices; no AXI-snoop), so the
    18.5 MB gzip Fedora uImage lives in NAND: kernel @`0x1300000`, DTB @`0x2800000`
    (the dead stock rootfs region). Stock kernel @`0x300000` left intact as recovery.
  - `bootcmd`: `nand read 0x02000000 0x1300000 0x1200000; nand read 0x04078000 0x2800000 0x20000; bootm 0x02000000 - 0x04078000`.
    Root `root=PARTUUID=…` ext4 on SSD `sda2`. Reversible — stock NAND kernel untouched.
- **Boot chain fully reversed** — ROM→S2→al_boot(stage2/3 CVOS HAL)→U-Boot→kernel.
  Canonical: [nor-boot-chain.md](nor-boot-chain.md), [preboot-decompile.md](preboot-decompile.md).
- **Firmware ladder** to 5.1.25 (stock). [firmware-5.1.25.md](firmware-5.1.25.md).
- **Kernel port, netboot-verified full-platform, now tracking mainline HEAD
  directly** (pull forward, port whatever breaks - not pinned to a release):
  8 internal-PCIe devices, 2×8TB SATA @6G + Samsung SSD, eth0 (1G) + eth1
  (10G SFP), al_ssm crypto, al_dma 4ch, xHCI SuperSpeed.
  [porting-roadmap.md](porting-roadmap.md). All build variants (which
  script/kernel-ver/output for what purpose, including the Fedora
  daily-driver + its KASAN twin + on-box native + awto-uboot): [build.md](build.md).
- **Board support is real commits in the kernel tree's own git history**
  (DTS, `pcie-al-internal.c`, the `pcie-al.c` DBI fix, `unvr_defconfig`) -
  the old `kernel-patches/` series was inert (applied by nothing) and has
  been removed.
- **Tooling**: `scripts/netboot.py`, `scripts/flash-nand.py` (standalone-boot flash,
  verified), `scripts/build-linux-ea16.py` (netboot installer),
  `scripts/build-linux-fedora.py` (daily-driver + KASAN variant),
  `scripts/build-fedora-rootfs.py`,
  `./dev.py ram-boot-deploy` (RAM-boot test without flashing NAND).
- **Hardware fully catalogued** (this session):
  - **Master BOM** — [components.md](components.md): every part/connector/test-point
    reconciled from a 130-photo sweep (SoC silk = **U2**; `U1` is a separate unresolved QFP).
  - **RPS/PSE subsystem reversed** — [rps-subsystem.md](rps-subsystem.md): RPS is
    **populated** on this 4-bay unit (not Pro-only); `ttyS2` = RPS UART via MAX3221
    (**not Bluetooth**; no BT on board); sense = gpio 33/34.
  - **GPIO/switch/LED map** — [gpio-switches-leds.md](gpio-switches-leds.md): reset=gpio38,
    `ulogo_white`=gpio37; gpio 33/34 = RPS (not SW1/SW2 — those are unknown, need a probe).
  - **I2C/SPI scan** — active `i2c_pld` bus has no INA/ISL power monitor; `i2c_gen`
    (stock "bus 11") is **disabled** in the ea16 DTS; unidentified 0x57 device (#62).
  - **JTAG-candidate header** lead — unpopulated 2-row PTH at the SoC top edge
    ([components.md](components.md) test-points); needs a macro.
- **Drive power root-caused** — PCA9575 @0x21 pwren; per-bay `regulator-fixed` auto-powers
  the bays and orders AHCI probe behind them ([sata-bay-power-ordering.md](sata-bay-power-ordering.md)).
- **Security analysis** — exposed secrets, unsigned boot, data remanence (#1,#2).

## In flight / open

- **`al_reboot` SP805 restart driver** — **written, NOT built or tested** (#51).
  Fixes the Linux `reboot` hang (mainline 7.1 has no AL-324 restart handler); pokes
  the SP805 watchdog to reset to U-Boot. Manual SP805 reset already validated.
  [reboot-driver-handover.md](reboot-driver-handover.md). Until proven, reboot hangs
  → power-cycle.
- **System-improvements audit** — kernel-config / perf / thermal (#44 al_thermal) /
  RAID / Fedora-compat. [improvements-audit.md](improvements-audit.md).

## Storage layout (current — USB removed, Fedora on SSD)

| Dev | Disk | Role |
|---|---|---|
| sda | Samsung SSD 1 TB (ata3) | **Fedora boot/root** — sda1 100 MB ESP, sda2 931 GB ext4 (`unvr-root`, PARTUUID `dcdc291e-…`) |
| sdb | WD82PURZ 8 TB | work store / RAID pair |
| sdc | WD82PURZ 8 TB | work store / RAID pair |

Stock USERDEV USB fully backed up (`images/unvr-usb-*.img`), then unplugged — that
shifted SSD/HDD from sdb/… down to sda/…. Two 8 TB disks → repurpose plan
(analyse read-only → wipe → one Linux boot, one work store).

## Next

1. Build + test `al_reboot` (#51) → clean `reboot`; then add to the OOT module list so
   a NAND reflash carries it.
2. Move dev toolchain + git onto woomera; build kernels on-device (self-host).
3. **UEFI/EDK2** as a U-Boot payload (#39) — [uefi.md](uefi.md).
4. Apply improvements from the audit (kernel config, perf, thermal, overclock #29).
5. Benchmarks on 7.1 (#41) + network throughput (#42).
6. Physical chases (macro shots): SoC-top-edge JTAG-candidate header; `U1` (Marvell
   switch vs ASM1061 bridge); continuity-probe JB4 rail map + SW1/SW2 GPIO.

## Operational notes (gotchas)

- **U-Boot has a ~50 s prompt watchdog** — catch + netboot must be atomic
  (`netboot.py` handles it). Send `reboot` from a shell BEFORE streaming ESC
  (streaming at a bash prompt triggers tab-completion, not U-Boot capture).
- **`panic=15`** in bootargs — a console/host drop kills the PID-1 initramfs shell
  → "Attempted to kill init" panic; panic=15 auto-reboots instead of stranding.
- **Reboot from Linux hangs** (#51, no restart driver yet) — power-cycle to reboot.
- Console creds (stock): root:ui (al324 gen). Fedora root pw `unvr` (CHANGE).
  See [credentials.md](credentials.md).

## Issue index (by theme)

- **Meta/tracking**: #4 (port), #37 (repo structure), #36 (upstream ledger).
- **Boot independence**: #3 (U-Boot decompile), #26 (bootROM), #27/#28 (flash layout / DDR),
  #30 (MTD for UEFI), **#39 (EDK2/UEFI)**.
- **Porting phases/patches**: #5–#25 (per-subsystem), #31–#35.
- **Milestones**: #32 (MD RAID), **#40 (Fedora on SSD — CLOSED)**.
- **Reboot/reset**: **#51 (reboot hang — al_reboot, in flight)**, #60 (early watchdog),
  **#47 (reset button — CLOSED)**.
- **Perf/OC/validation**: **#29 (overclock)**, **#41 (7.1 matrix)**, **#42 (network)**,
  **#44 (al_thermal — CLOSED)**, #50 (crypto), #125 (SFP+ thermal zone).
- **Chips/datasheets**: **#38 (DRAM & SFP identified — CLOSED)**, **#62 (i2c 0x57 device — CLOSED)**.
- **Security**: #1, #2.
