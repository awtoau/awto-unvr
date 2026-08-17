# TianoCore EDK2 UEFI on the UNVR — feasibility + phased plan

Goal: run **EDK2 UEFI as a later-stage payload** on this UNVR (AL-324 / Alpine V2,
quad Cortex-A57, aarch64, sysid **0xea16**). Strategy owner: Dan.

- **Chosen chain:** `ROM → Annapurna S2 → al_boot/stage3 → U-Boot → EDK2 → GRUB/Linux`.
- **Rejected chain:** `ROM → our-EDK2` — needs undocumented AL-324 DDR training / PLL /
  fabric / SATA-PHY init. DDR training is owned by the proprietary CVOS agent
  (`agent_wakeup v2.10`, `exec_via_agent`), not recoverable
  ([preboot-decompile.md](preboot-decompile.md) §DRAM). Do **not** attempt.
- Context, not restated: [hardware.md](hardware.md#mmio-and-address-map) (address map),
  [nor-boot-chain.md](nor-boot-chain.md) (boot chain + trust model),
  [uboot-update-path.md](uboot-update-path.md), [bootloader.md](bootloader.md),
  [boot-flow.md](boot-flow.md), [preboot-decompile.md](preboot-decompile.md),
  [porting-roadmap.md](porting-roadmap.md).

---

## 1. Pivotal question — does U-Boot support EFI chainloading? **NO.**

`bootefi` / `CONFIG_EFI_LOADER` is **absent**. Proven three independent ways:

| Source | Evidence | Result |
|---|---|---|
| GPL U-Boot source (`UNVR-1.3.35-GPL/u-boot`) | Base = denx **v2015.07** (`Makefile VERSION=2015 PATCHLEVEL=07`). **No** `lib/efi_loader/`, **no** `common/cmd_bootefi.c`/`cmd/bootefi.c`, **no** `EFI_LOADER` in any Kconfig. Only `CONFIG_EFI_PARTITION` (GPT table parsing) in `nas_ai/hd/pro` defconfigs — **unrelated to EFI boot** | absent |
| OLD device binary (`tmp/uboot/old-uboot-payload.bin`, build 2020-12-16, THE ONE THAT RUNS) | `strings` → **zero** `bootefi`/`Booting EFI`/`efi_`/`BOOTAA64` | absent |
| NEW 5.1.25 binary (`tmp/uboot/new-uboot-payload.bin`, build 2026-07-09) | `strings` → **zero** EFI-loader strings | absent |

- `EFI_LOADER` first appeared in U-Boot **v2016.09** and matured ~v2017 — v2015.07
  predates it entirely. Rebuilding *this* fork with `CONFIG_EFI_LOADER` is not a
  config flip; the code does not exist in the tree.
- **Our board's `nas_defconfig` is minimal** (`configs/alpine_v2_64_ubnt_nas_defconfig`,
  13 lines) — no `EFI_PARTITION` even. USB/xHCI/DM/USB_STORAGE on; keyed autoboot
  (Esc Esc).

### Command set actually present (both OLD and NEW binaries)

`bootm bootz go source fdt` · `tftpboot dhcp` · `usb pci nand sf ext4load`
· `crc32 cmp cp md mw loadb loady`. NEW adds `scsi`.
**Absent:** `bootefi`, `booti` (raw arm64 Image), `fatload`, `ext2load`.

- `bootm` handles **legacy uImage** including image types `standalone` /
  `firmware` — banner `## Starting application at 0x%08lX ...` present in the binary.
- `go <addr>` = jump to a raw binary at an address. Present.
- Storage from USB is **ext4 only** (`ext4load`, no `fatload`) — or raw `sf`/`nand`
  read, or `tftpboot`/`loady` into RAM.

### Resulting easiest path — chainload EDK2 as a U-Boot "firmware" image or via `go`

No bootefi means EDK2 is not launched as an `.EFI` app **by U-Boot**. Instead
U-Boot loads the **EDK2 flash image (FD)** into RAM and enters its SEC/PrePi
entry. Two mechanisms, both already in the binary:

1. **`go <FD_BASE>`** — load the raw EDK2 `.fd` to its build-time base, jump to the
   SEC reset vector. Simplest. Volatile, no flash write.
2. **`bootm`** — wrap the `.fd` as a legacy image:
   `mkimage -A arm64 -O u-boot -T firmware -C none -a <FD_BASE> -e <SEC_entry> -d UNVR.fd uEdk2`
   then `bootm`. U-Boot's `## Starting application` path transfers control.

Load transport for the `.fd`: `tftpboot` into RAM (best for iterate-fast, mirrors
the proven netboot recipe in [boot-flow.md](boot-flow.md)), or `ext4load usb`, or
`loady` over serial. **The chainload never touches flash** — power-cycle returns
to stock. This is the same safety envelope as the existing unsigned-kernel netboot.

- BOOTAA64.EFI (the `.EFI` PE payload) is what **EDK2's own** BDS/GRUB launches
  *after* EDK2 is up — not what U-Boot launches. U-Boot launches the FD.

---

## 2. "Do we have all the parts?" checklist

| # | Requirement | Status | Source / value |
|---|---|---|---|
| 1 | Serial console | **HAVE** | UART `ttyS0` `0xfd883000`, 115200 8N1, header behind SFP+ cage ([hardware.md](hardware.md#connectors)); `scripts/unvr-console.py` |
| 2 | Full flash backups | **HAVE** | 13 byte-identical `mtd00` dumps under `images/mtd/…`; sha256 `8af165bc…48d9`. mtd00 backup verified, stored off-device ([nor-boot-chain.md](nor-boot-chain.md) §0) |
| 3 | SoC memory map | **HAVE** | [hardware.md](hardware.md#mmio-and-address-map) — DRAM, UART, GIC, timers, PCIe ECAM, SATA/USB/eth windows, all cross-checked to live.dts + iomem |
| 4 | Device tree (for pass-through) | **HAVE** | `docs/hw-reference/20260816-104601/live.dts` = the running DTB (28672 B `live.dtb`); Linux + custom Debian already boot from it |
| 5 | Entry EL + DRAM-ready + handoff | **PARTIAL** | DRAM already trained by CVOS agent (poll `_DAT_fbff4150 == 0x31415926`, [preboot-decompile.md](preboot-decompile.md)); Linux is entered at **EL2** (dmesg: 4 cores start EL2). The **EL U-Boot hands off at is unconfirmed on bench** — see §5, §6 |
| 6 | aarch64 toolchain | **HAVE** | `aarch64-linux-gnu-*` in use across the repo; EDK2 needs GCC5/GCC aarch64 + `edk2` + `edk2-platforms`. NEED = clone edk2 (not yet in-repo) |
| 7 | Boot storage / transport | **HAVE** | `tftpboot`+`dhcp` (netboot proven, [boot-flow.md](boot-flow.md)); `ext4load usb`; internal USB `/dev/sdq`; `sf`/`nand` |
| 8 | Chainload mechanism | **HAVE** | `go` + `bootm`(firmware image) present in the running U-Boot (§1). `dobootm=run bootunsign` ⇒ unsigned; **no signature obstacle** ([nor-boot-chain.md](nor-boot-chain.md) §5) |
| 9 | GIC / timer / PSCI values for EDK2 | **HAVE** | GIC-v3 dist `0xf0200000`, GICR `0xf0280000`; ARMv8 generic timer CNTFRQ **58 333 312 Hz** (live.dts `clock-frequency=0x37a1880`); PSCI smc `cpu_on=0x84000003` |
| 10 | Unsigned-payload path | **HAVE** | Whole OS boot is unverified; `bootunsign` is the default. EDK2 FD boots with no signing ([nor-boot-chain.md](nor-boot-chain.md) §5) |
| 11 | UART divisor / input clock | **PARTIAL** | live.dts uart clock **500 MHz**, reg-shift 2 (stride 4), io-width 4. U-Boot already inits UART at 115200 — EDK2 P0 can skip re-init. Exact NS16550 divisor read from strapping at runtime, not compiled ([bootloader.md](bootloader.md)) |

**Summary:** 8 HAVE, 3 PARTIAL, 0 hard NEED (except cloning EDK2 itself). Every
PARTIAL is resolvable on the bench in P0 and none blocks starting.

---

## 3. EDK2 platform package skeleton — `Platform/Ubiquiti/UNVR/`

Filled with our known values. Flow = **PrePi single-blob** (ArmPlatformPkg/PrePi):
DRAM is already up, so skip the DDR-init PEI path — EDK2 is a RAM payload, mirroring
how it is entered.

```
Platform/Ubiquiti/UNVR/
├── Unvr.dsc                 # platform build description
├── Unvr.fdf                 # flash description → UNVR.fd
├── Library/
│   ├── UnvrPlatformLib/     # ArmPlatformLib: mem map, early init, SEC hooks
│   ├── UnvrSerialPortLib/   # thin wrapper over MdeModulePkg 16550 (see below)
│   ├── UnvrMemoryInitLib/   # ArmPlatformGetVirtualMemoryMap (table below)
│   └── UnvrResetSystemLib/  # ArmPsciResetSystemLib (PSCI SMC)
└── DeviceTree/
    └── unvr.dts             # = docs/hw-reference/20260816-104601/live.dts (verbatim)
```

### Known values to bake into the PCDs

| Concern | PCD / setting | Value | Source |
|---|---|---|---|
| CPU | — | Cortex-A57 r1p3, MIDR `0x411fd073`, ARMv8-A aarch64 | [hardware.md](hardware.md) |
| DRAM0 | `PcdSystemMemoryBase` / `…Size` | base `0x00000000`, size `0xC0000000` (3 GiB) | [hardware.md](hardware.md#dram-two-banks) |
| DRAM1 | extra mem region (add to map) | base `0x200000000`, size `0x40000000` (1 GiB) | " |
| EDK2 FD load base | `PcdFdBaseAddress` / `PcdFvBaseAddress` | **`0x20000000`** (512 MiB, clear of U-Boot: TEXT_BASE `0x1100000`, loadaddr `0x08000000`, loadaddr_dt `0x04078000`) | this doc; [boot-flow.md](boot-flow.md) |
| UART base | `PcdSerialRegisterBase` | `0xfd883000` | [hardware.md](hardware.md) |
| UART type | **SerialPortLib16550, NOT PL011** | 8250/DesignWare `ns16550a` | [hardware.md](hardware.md) §standard-IP |
| UART stride | `PcdSerialRegisterStride` | `4` (reg-shift 2, io-width 4) | live.dts |
| UART baud | `PcdSerialBaudRate` | `115200` (or skip init; U-Boot already set) | loadbootargs |
| UART clock | `PcdSerialClockRate` | `500000000` | live.dts uart |
| GIC dist | `PcdGicDistributorBase` | `0xf0200000` | [hardware.md](hardware.md#soc-service--fabric-blocks) |
| GIC redistributor | `PcdGicRedistributorsBase` | `0xf0280000` | " |
| Arch timer freq | `PcdArmArchTimerFreqInHz` | **`58333312`** (0x37a1880) — or read CNTFRQ_EL0 at runtime; do NOT use U-Boot's stale 50 MHz | live.dts arch-timer |
| PSCI | `PcdArmPsciMethod` = smc | `cpu_on=0x84000003`, `cpu_suspend=0x84000001` | live.dts psci |
| DT hand-off | install live.dtb as `gFdtTableGuid` config table | pass `live.dts` to GRUB/Linux | [porting-roadmap.md](porting-roadmap.md) |

### `ArmPlatformGetVirtualMemoryMap` regions

| Region | Base | Size | Attr |
|---|---|---|---|
| DRAM0 | `0x00000000` | `0xC0000000` | WB cacheable |
| SoC service/fabric (GIC, ccu, nb-service, memctl) | `0xf0000000` | `0x01000000` | Device-nGnRnE |
| PBS peripherals (uart/i2c/spi/gpio/wdt/timer/pbs) | `0xfd800000` | `0x00800000` | Device |
| PCIe int-ECAM + integrated-EP window | `0xfbc00000` | `0x00440000` | Device (covers ecam `0xfbc00000`, msix `0xfbe00000`) |
| integrated-EP MMIO (eth/dma/ahci) | `0xfe000000` | `0x01000000` | Device |
| DRAM1 | `0x200000000` | `0x40000000` | WB cacheable |

Cover the whole `0xf0000000..0xff000000` device band as Device-nGnRnE if simpler
for P0; tighten later. Exact sub-block bases in
[hardware.md](hardware.md#mmio-and-address-map).

### SerialPortLib note

Use `MdeModulePkg/Library/BaseSerialPortLib16550` (not `PL011SerialPortLib`) —
the AL HAL confirms 8250-style DesignWare register layout
(`rbr_thr/dll/ier…`), **not** PL011 ([hardware.md](hardware.md) §standard-IP).
Wrong lib = silent no console, the classic first-boot dead end.

---

## 4. Phased plan

Each phase boots and is reverted by power-cycle (RAM payload, no flash write).

### P0 — minimal `BOOTAA64.EFI`-capable EDK2, chainloaded to RAM

- Build `Platform/Ubiquiti/UNVR/Unvr.dsc` PrePi FD.
- Bring-up order inside EDK2: **UART console** → **ARM generic timer** → **RAM/HOB
  map** → **GIC-v3** → **DT install** → **EFI DXE core + boot services** → EFI shell.
- Entry: `tftpboot 0x20000000 UNVR.fd; go 0x20000000` (or `bootm` firmware image).
- Success = EDK2 banner + UEFI Shell prompt on `ttyS0` at 115200.
- Confirms: entry EL, UART, timer, DRAM map, DT hand-off — closes the §2 PARTIALs.
- **No PCIe/USB/SATA/net yet** — Shell over serial only.

### P1 — PCIe

- Internal PCIe host: ECAM `0xfbc00000`; external0: `0xfd800000`, ECAM `0xfb600000`,
  `cfg-space-offset 0x10000`, link Gen2 x1.
- Reuse `MdeModulePkg` PCI host bridge + a small Alpine glue (AXI snoop
  `0x110/0x130/0x150/0x170`, APP_CONTROL `0x220=0x03FF`, DBI = ctrl+`0x10000`) —
  same glue the Linux port needs ([porting-roadmap.md](porting-roadmap.md) §Phase 4).
- Gates USB/SATA/eth (all PCIe EPs).

### P2 — SATA

- 2× AHCI EPs behind internal PCIe (`1c36:0031`, abar `0xfe154000` / `0xfe158000`,
  4 ports each). Stock `AtaAtapiPassThru` + `AhciBus` (generic AHCI).

### P3 — network

- 1G `al_eth` `1c36:0001` (RGMII → AR8031 addr 4) and/or 10G `1c36:0002` (SFP+).
- **No EDK2 al_eth driver exists** — largest P3 unknown. Fallback: keep netboot in
  U-Boot, do EDK2 network later, or use the ASMedia-path USB NIC. UEFI SNP/MNP over
  a fresh al_eth UEFI driver is a real write, not a reuse.

### P4 — GRUB → Linux

- EDK2 installs `live.dtb` as `gFdtTableGuid` config table; `grubaa64.efi` from the
  ext4 USB or SATA; GRUB loads the mainline `Image` + initramfs; Linux enters at
  the EL EDK2 hands off (EL2 per current chain).
- This is the payoff: a standard UEFI+ACPI/DT boot for TrueNAS/Fedora/Debian
  without the vendor initramfs.

---

## 5. Exact next bench step (owner runs — do NOT run here)

At the U-Boot prompt (Esc Esc within ~2 s of power-on; `ALPINE_UBNT_NAS_ALL>`),
run these to confirm chainload capability and entry state. **Read-only, no writes.**

```
version                 # confirm 2015.07-alpine_db-2.21-HAL, build date
help                    # full command list; confirm go / bootm present
help bootm              # confirm 'standalone'/'firmware' image handling
printenv                # bootcmd, bootfrom(=bootnand), loadaddr(0x08000000), loadaddr_dt
bdinfo                  # DRAM banks, relocaddr (where U-Boot moved to — avoid it)
help go                 # confirm 'go' jump-to-address
```

Expected: `go`, `bootm` present; `bootefi`, `booti`, `fatload` **absent**
(already proven in the binary — this is bench confirmation).

Then a **dry chainload probe** (still no flash write) once an EDK2 FD is built:

```
setenv ipaddr <unit>; setenv serverip <host>
tftpboot 0x20000000 UNVR.fd     # host serves it via scripts/tftpd.py
crc32 0x20000000 ${filesize}    # verify against host crc32 before jumping
go 0x20000000                   # transfer control to EDK2 SEC
```

To learn the **entry EL** before trusting `go`: the cleanest read is EDK2's own
first log (build ArmPlatformLib to print `CurrentEL`), since U-Boot 2015.07 has no
`currentel` command. Until then assume **EL2** (what Linux is handed) and build EDK2
to run at EL2.

---

## 6. Gaps + risks

- **Entry EL unconfirmed.** Chain hands Linux EL2; whether `go`/`bootm` enters EDK2
  at EL2 or EL1 is not read on bench. EDK2 must match. Mitigation: print `CurrentEL`
  in the P0 SEC log; rebuild for the observed EL. (§2 #5, §5.)
- **FD load base collision.** U-Boot relocates itself to top of DRAM at start-up
  (`bdinfo relocaddr`). `0x20000000` is chosen clear of TEXT_BASE `0x1100000`,
  loadaddr `0x08000000`, loadaddr_dt `0x04078000` — but confirm against `relocaddr`
  before committing.
- **Arch-timer frequency conflict.** U-Boot config hardcodes `COUNTER_FREQUENCY
  50000000` ([bootloader.md](bootloader.md)) but the **live DT says 58 333 312 Hz**
  and Linux runs at 58.33 MHz. Trust the DT / read `CNTFRQ_EL0` at runtime; a wrong
  value = all EDK2 timeouts/delays wrong by 1.17×.
- **No EDK2 al_eth driver** (P3). Networking inside UEFI is a genuine new driver,
  not a reuse — same conclusion as the Linux port's Phase 3.
- **PCIe internal host is not cache-coherent-DMA-safe** without the Alpine AXI-snoop
  glue ([porting-roadmap.md](porting-roadmap.md) §Phase 4). P1 must program it or
  DMA (USB/SATA/eth) corrupts silently.
- **PSCI source.** `method=smc` implies an EL3 secure monitor (Annapurna al_boot).
  EDK2 MpInitLib/PSCI reset relies on it responding to `cpu_on=0x84000003`. Untested
  from an EDK2 context; P0 can run single-core (secondaries parked by
  `power_down_secondary_cpus`).
- **UART re-init.** live.dts UART clock 500 MHz but the NS16550 divisor is set from
  PBS strapping at runtime, not compiled ([bootloader.md](bootloader.md)). Safest P0:
  **skip UART init** in EDK2 (U-Boot already left it at 115200); only reprogram once
  the divisor is confirmed.
- **Untested end-to-end.** Nobody has run EDK2 on this unit. Every value here is from
  static analysis + the live capture; P0 is the first real test.
- **Value of this vs. plain Linux.** The Linux mainline port
  ([porting-roadmap.md](porting-roadmap.md)) boots via `bootunsign` with **no EDK2 at
  all**. EDK2 buys standard UEFI/GRUB provisioning (TrueNAS installer, distro
  installers expecting UEFI), not raw capability. Weigh P3's al_eth-in-UEFI cost
  against that.
```
