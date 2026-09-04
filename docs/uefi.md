# TianoCore EDK2 UEFI on the UNVR — feasibility + phased plan

Goal: run **EDK2 UEFI as a later-stage payload** on this UNVR (AL-324 / Alpine V2,
quad Cortex-A57, aarch64, sysid **0xea16**). Strategy owner: Dan.

- **Chosen chain:** `ROM → Annapurna S2 → al_boot/stage3 → U-Boot → EDK2 → GRUB/Linux`.
- **Rejected chain:** `ROM → our-EDK2` — needs AL-324 DDR training / PLL / fabric /
  SATA-PHY init from cold. DDR training's algorithm is open
  (`al_hal_ddr_init_alpine_v2.c`) and per-unit config is decoded
  ([ddr-config-reverse.md](ddr-config-reverse.md) §6-7) — porting it is feasible, just
  real work not yet done. PLL/fabric/SATA-PHY cold-init remain genuinely undocumented.
  Do **not** attempt without that work first.
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
```

**No DeviceTree/ directory.** It held a byte-identical copy of
`docs/hw-reference/20260816-104601/live.dts`, referenced by nothing - not the
.dsc, not the .fdf, no `gFdtTableGuid` install, no build rule. Removed
2026-09-04 rather than wired up: it is the *vendor's* decompiled live DTB
(`compatible = "annapurna-labs,alpine"`, vendor bootargs baked in), not our
maintained Linux DT (`dts/alpine-v2-ubnt-unvr-ea16.dts`, `compatible =
"ubnt,unvr"`). Installing it as a config table would hand GRUB the wrong tree.
When P4 needs DT handoff, the source is the ea16 DTS, compiled and installed as
`gFdtTableGuid` - a real build-rule change, not a file that was already sitting
there.

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
  map** → **GIC-v3** → **EFI DXE core + boot services** → EFI shell. (DT
  install deferred to P4 - nothing at P0 consumes it; see status below.)
- Entry: `tftpboot 0x20000000 UNVR.fd; go 0x20000000` (or `bootm` firmware image).
- Success = EDK2 banner + UEFI Shell prompt on `ttyS0` at 115200.
- Confirms: entry EL, UART, timer, DRAM map — the software side closes
  once the actual chainload probe runs (§5's dry-probe, owner-only).
- **No PCIe/USB/SATA/net yet** — Shell over serial only.

**Status (2026-09-02): P0 fully achieved - UEFI Interactive Shell
running and responsive on real hardware.** `./dev.py uefi-chainload-probe`
(docs/uefi.md §5's dry probe) tftp'd `UNVR.fd`, crc32-verified it, and
`go 0x20000000`'d into it. First attempt ever, and it worked past
everything §2/§6 flagged as unconfirmed. The doc's own original success
criterion ("EDK2 banner + UEFI Shell prompt on `ttyS0` at 115200") is
met - see the hotkey fix below for the final piece.

```
## Starting application at 0x20000000 ...
SEC: UNVR ArmPlatformInitialize (P0 - no board init needed)
UEFI firmware (version UNVR EDK2 P0 built at 08:30:30 on Sep  2 2026)
Tianocore/EDK2 firmware version UNVR EDK2 P0
Press ESCAPE for boot options ...
[Bds] Unable to boot!
Please select boot device:
  UEFI Non-Block Boot Device
  UEFI Non-Block Boot Device 2
```

- **Entry EL was correct** (or at least compatible) - PeilessSec's own
  platform hook ran and printed, closing the §6 "entry EL unconfirmed"
  risk without needing to add a `CurrentEL` print first.
- **UART, ARM generic timer, GIC-v3, DRAM map all correct** - DXE Core
  loaded, ran, and reached BDS console output on the real serial link.
- **Reached the interactive Boot Manager Menu** ("Please select boot
  device") - not a hang, not a crash. Confirmed still-alive-and-
  responsive twice more: selecting either listed option, and pressing
  ESC, all produce sensible (if not yet successful) responses rather
  than silence.
- **`BmDriverHealth.c(553)` assert: root-caused and fixed.**
  `BmRepairAllControllers()` (in `UefiBootManagerLib`, linked into
  `BdsDxe` - no formal `[Depex]`, so this wasn't visible as a protocol
  dependency) unconditionally looks up `gEfiFormBrowser2ProtocolGuid`
  unless `PcdDriverHealthConfigureForm` is `ZeroGuid` - documented in
  the source itself as the intended way to disable the check. P0 has no
  `SetupBrowserDxe`/`FormBrowser2` at all (no Setup UI), so the lookup
  failed "Not Found" on every boot attempt. Set
  `PcdDriverHealthConfigureForm` to `ZeroGuid` in `Unvr.dsc` - confirmed
  live, this assert no longer fires.
  - Tried the RELEASE-build route first (`./dev.py build-uefi-p0
    --target RELEASE`) - **did not help**: `PcdDebugPropertyMask`
    (baked in as `0x07`, a `PcdsFixedAtBuild` value) controls `ASSERT()`
    behavior, not the DEBUG/RELEASE build target. Same assert, same
    output, either way.
  - **A tempting wrong turn, worth recording**: also tried dropping
    `HiiDatabaseDxe` from the component list entirely, on the theory
    that an empty HII database was the root cause of *both* remaining
    asserts. This was wrong and made things strictly worse - it turned
    the benign, loop-back-to-menu `Database.c` assert into a genuine
    `Synchronous Exception` (`DxeMain.c(578)`: "Bds Arch Protocol not
    present!!" - `BdsDxe`/`UefiBootManagerLib`'s C code calls HII
    functions directly regardless of DEPEX). Reverted; `HiiDatabaseDxe`
    stays in the component list.
- **`Database.c(3599/3626)` assert: fixed.** Confirmed to be a
  genuinely separate issue from the one above (fixing
  `PcdDriverHealthConfigureForm` alone didn't touch it). Root cause:
  `HiiGetDatabaseInfo()`'s `ReadyToBoot` callback expects
  `HiiExportPackageLists()` to return `EFI_BUFFER_TOO_SMALL` (the normal
  "ask for size first" pattern) but got `EFI_NOT_FOUND` because the HII
  database was genuinely empty - nothing in P0's component list
  registered any HII package. Removing the *consumer* (`HiiDatabaseDxe`)
  was the wrong fix (see above); added a minimal HII package *provider*
  instead: `Platform/Ubiquiti/UNVR/Drivers/MinimalHiiDxe/` (`DXE_DRIVER`,
  not an application - those only register once actually launched as a
  boot option, which is after `ReadyToBoot` already fired), registers
  one trivial string via `HiiAddPackages()`, `[Depex]` on
  `gEfiHiiDatabaseProtocolGuid AND gEfiHiiConfigRoutingProtocolGuid`.
  Confirmed live 2026-09-02: **both asserts gone, box reaches the boot
  menu completely cleanly.**
- **The interactive menu's own 2 entries are NOT the real boot
  options - root-caused.** Earlier guess (Shell.c's `UefiMain()` runs
  partway then fails silently) was **wrong** - confirmed with a
  temporary `DEBUG_ERROR` print at the very top of `UefiMain()`: it
  never fires when selecting either menu entry. Widening
  `PcdDebugPrintErrorLevel` to include `DEBUG_LOAD`/`DEBUG_INFO`
  (`0x800000CF`, kept in `Unvr.dsc` going forward - genuinely useful)
  revealed the real story: `BootManagerMenuApp`'s own interactive menu
  auto-enumerates generic "non-block boot devices" from whatever's
  currently connected, which for us is just the raw memory-mapped FV
  region (`MemoryMapped(0xB,0x20000000,0x207FFFFF)`) - selecting either
  entry logs `[Bds] Expand MemoryMapped(...) -> <null string>`: there is
  no specific file identified in that path, so it can never resolve to
  anything bootable. Neither entry was ever going to reach `Shell.inf`.
  - **The real, correctly-configured boot options are Boot0000/
    Boot0001 in NVRAM** (dumped at every boot: `Boot0000:
    BootManagerMenuApp`, `Boot0001: UEFI Shell`), each with a proper
    `FvFile(GUID)` device path - confirmed `Boot0000` resolves and loads
    correctly (`[Bds] Expand MemoryMapped(...)/FvFile(EEC25BDC-...) ->
    MemoryMapped(...)/FvFile(EEC25BDC-...)`, `Loading driver at
    0x...BootManagerMenuApp.efi`). BDS's own automatic pre-menu phase
    already tries these directly, in order - it's the interactive menu
    (triggered only because `Boot0000` = the menu app itself) that never
    gets to `Boot0001`.
  - **Tried dropping `BootManagerMenuApp` entirely** so BDS's automatic
    phase would fall through past a (still-failing) `Boot0000` straight
    to `Boot0001` (Shell) - **the box hung instead**: `PlatformRecovery0000`
    fails the same way (`\EFI\BOOT\BOOTAA64.EFI -> <null string>`), then
    `"BootManagerMenu FFS section can not be found, skip its boot option
    registration"`, then nothing - no crash, no further log output,
    unresponsive to input. Recovered with a power-cycle (RAM payload,
    never touched flash - the doc's own safety guarantee held). Reverted;
    `BootManagerMenuApp` stays in the component list, confirmed back to
    the known-good clean-menu state afterward.
- **Reaching `Boot0001` (Shell): solved via the 's' hotkey, not a
  component-list or NVRAM change.** `ArmPkg/Library/
  PlatformBootManagerLib` (the generic lib P0 reuses, see above) already
  registers a hotkey for `Boot0001` - `Key.UnicodeChar = 's'` - live
  during `BdsDxe`'s ~3s `BdsWait` countdown
  (`PcdPlatformBootTimeOut`). Spamming 's' during that window makes BDS
  boot `Boot0001` directly, bypassing `BootManagerMenuApp`'s broken menu
  entirely - confirmed live: `[Bds]BmHotkeyCallback: 0000:0073` →
  `[Bds]Hotkey for Boot0001 pressed - Success` → `[Bds]Booting UEFI
  Shell` → `Loading driver at ...Shell.efi` → full `UEFI Interactive
  Shell` banner with an `FS0:` mapping table and a live `Shell>` prompt.
  Verified genuinely interactive (not just banner text) by running `ver`
  in a follow-up connection: `UEFI Interactive Shell v2.2 / EDK II /
  UEFI v2.70 (EDK II, 0x00010000)`.
  - `scripts/uefi-chainload-probe.py` now does this itself: sends `go`
    over a raw console socket (not `dev.py console-send`'s blocking
    subprocess wrapper - too slow to land inside the ~3s window once
    tftp+crc32 overhead is accounted for) and spams `s` for the first 8s
    while reading continuously, watching for the literal string `UEFI
    Interactive Shell` as the success signal (not `UEFI Shell`/`Shell>`
    substrings - those also appear, as a false positive, in BDS's own
    boot-options-dump trace before anything has actually launched).
    `--no-hotkey` skips the spam to reproduce the old "lands on
    Boot0000's own menu" behavior if ever needed for comparison.
  - This is now unattended and reproducible: `./dev.py
    uefi-chainload-probe` runs power-cycle → catch U-Boot → tftp → crc32
    → `go` + hotkey race → Shell detection → exits 0 end to end, no
    manual intervention.

Real fixes found getting the FD to build in the first place, in case
they bite the next phase too:

- **`ArmLib` moved from `ArmPkg` to `MdePkg`** upstream since the
  reference/doc were written (`MdePkg/Library/ArmLib/ArmBaseLib.inf`,
  confirmed against `edk2-stable202608` and cross-checked against
  ArmVirtPkg's own DSC, which is actively maintained).
- **`CpuExceptionHandlerLib`** is now the unified
  `UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf`
  - no separate `DefaultExceptionHandlerLib` needed anymore.
- **No `PcdArmArchTimerFreqInHz` or `PcdArmPsciMethod` PCD exists** in
  this edk2 - `ArmArchTimerLib` reads `CNTFRQ_EL0` directly at runtime
  (so the live DT's 58333312 Hz just works, no override needed), and
  `ArmMonitorLib`'s `PcdMonitorConduitHvc` defaults to FALSE (=SMC, what
  we want) - neither PCD needed an entry in `Unvr.dsc`.
- **`PciLib`/`PciExpressLib`/`PciSegmentLib` still needed even with no
  PCIe driver running** - `BaseSerialPortLib16550` has an (unused, since
  we set `PcdSerialUseMmio=TRUE`) PCI-config-space code path that must
  still resolve statically at link time.
- **`gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType` must be `4`
  (TTYTERM)** - `ArmPkg/Library/PlatformBootManagerLib`'s `PlatformBm.c`
  has a build-time `STATIC_ASSERT` on this; it's the generic, board-
  agnostic ARM boot-manager lib (falls back to the internal shell with no
  boot option configured, which is exactly P0's need), reused as-is
  rather than writing our own for P0.
- **Pin to a stable edk2 tag, not tip.** An earlier attempt against
  tianocore/edk2's dev tip hit the same `ArmLib` move mid-refactor -
  `edk2-stable202608` (latest stable as of this session) is what actually
  works; `scripts/build-uefi-p0.py` clones that tag specifically.
- Small submodules needed (not the network/crypto/TPM ones - no
  components in P0 pull those in): `BaseTools/Source/C/BrotliCompress/
  brotli`, `MdePkg/Library/MipiSysTLib/mipisyst`, `MdeModulePkg/Library/
  BrotliCustomDecompressLib/brotli`, `SecurityPkg/DeviceSecurity/SpdmLib/
  libspdm` - all pulled in by `scripts/build-uefi-p0.py` automatically.

### P1 — PCIe

- Internal PCIe host: ECAM `0xfbc00000`; external0: `0xfd800000`, ECAM `0xfb600000`,
  `cfg-space-offset 0x10000`, link Gen2 x1.
- Reuse `MdeModulePkg` PCI host bridge + a small Alpine glue (AXI snoop
  `0x110/0x130/0x150/0x170`, APP_CONTROL `0x220=0x03FF`, DBI = ctrl+`0x10000`) —
  same glue the Linux port needs ([porting-roadmap.md](porting-roadmap.md) §Phase 4).
- Gates USB/SATA/eth (all PCIe EPs).

**Status (2026-09-03): internal PCIe done, verified live.** Scoped
deliberately to internal-only first (flat ECAM, no link/PHY concept at
all - no risk of repeating issue #140's link-retrain stall); external
PCIe0 (ASM1042A xHCI) is a separate follow-up, see below.

- **`Platform/Ubiquiti/UNVR/Library/PciHostBridgeLib/`**: adapted
  directly from `imbushuo/ccr2004-uefi`'s own working `PciHostBridgeLib`
  (same SoC family, same internal-PCIe layout - bus 0-0, ECAM
  `0xFBC00000` already set as `PcdPciExpressBaseAddress` since P0, MMIO
  window `0xFE000000-0xFEFFFFFF`, no I/O space). Values cross-checked
  against `docs/hardware.md`'s register table - identical.
- **`Platform/Ubiquiti/UNVR/Drivers/AlPcieSnoopFixDxe/`**: a `DXE_DRIVER`
  with `[Depex] gEfiPciEnumerationCompleteProtocolGuid`, walking
  `gEfiPciIoProtocolGuid` handles post-enumeration and writing the same
  SMCC (`cfg 0x110` + `0x20`×{1,2,3} for slot≤5, `SNOOP_OVR|SNOOP_EN` =
  `0x3`) and `APP_CONTROL` (`cfg 0x220` low-16 = `0x3ff`) registers
  `al_pcie_snoop_fix()`/`al_snoop_one()` in our U-Boot fork
  (`board/annapurna/alpine/alpine.c`) already applies every boot - not
  just matching values, both now `#include hal/pcie-al-alpine-regs.h`
  directly (2026-09-03), a byte-identical mirror of our Linux fork's
  canonical copy (`drivers/pci/controller/pcie-al-internal.c`,
  `awto-au/linux`) - see docs/audits/audit-edk2-pcie-glue.md. Deliberately
  excludes `1c36:0001`/`0002` (al_eth), mirroring U-Boot's own exclusion
  (applying this fixup to al_eth broke UDMA TX and persisted across a warm reset,
  #74/#90).
- Added `ArmPkg/Drivers/ArmPciCpuIo2Dxe`, `MdeModulePkg/Bus/Pci/
  PciHostBridgeDxe`, `MdeModulePkg/Bus/Pci/PciBusDxe` to the component
  list (CpuIo2/PciHostBridgeDxe/PciBusDxe standard trio) - no PCD
  overrides needed beyond what P0 already set.
- **Confirmed live, first attempt**: `AlPcieSnoopFix: applied to 4
  internal PCIe device(s)` in the boot log, then the Shell's own `pci`
  command independently confirms all 6 internal devices enumerated
  correctly with the right vendor/device/class:
  ```
  00/00/01/00  1C36:0001  Ethernet controller       (al_eth  - excluded)
  00/00/02/00  1C36:0002  Ethernet controller       (al_eth  - excluded)
  00/00/04/00  1C36:0022  Encrypt/Decrypt           (al_ssm  - fixed)
  00/00/05/00  1C36:0022  RAID controller           (al_dma  - fixed)
  00/00/08/00  1C36:0031  Serial ATA controller     (AHCI 0  - fixed)
  00/00/09/00  1C36:0031  Serial ATA controller     (AHCI 1  - fixed)
  ```
  4 fixed + 2 excluded = 6 total, exactly matching U-Boot's own current
  (post-#90-fix) behavior on this hardware.
### P1.5 — external PCIe0 (ASM1042A xHCI) - devfn-alias crash fixed, xHCI controller inits cleanly

Motivation beyond completeness: EDK2's `XhciDxe` is a fully independent
codebase from U-Boot's own xHCI driver (`uboot-port/drivers/usb/host/
xhci.c`) - issue #140's U-Boot-side investigation has been stuck for a
long time on `Cannot allocate device context to get SLOT_ID`, unresolved
after dozens of rounds. Standing up a second, independent driver against
the *same* hardware is a differential test: if it also fails hard, that's
real evidence the bug is silicon/coherency-level, not specific to
U-Boot's implementation.

**Built this (2026-09-03), following docs/hardware.md's "never retrain
an already-linked port" rule throughout:**
- `Library/PciSegmentInfoLib/` - two ECAM segments (segment 0 = internal
  `0xfbc00000`, segment 1 = external `0xfb600000`), both genuinely flat
  single-bus ECAM (no bus-shift bits in the address at all - see
  `docs/hardware.md`), so the stock `MdePkg/Library/
  PciSegmentLibSegmentInfo/BasePciSegmentLibSegmentInfo.inf` works
  unmodified - no custom PciSegmentLib needed for *this* part.
- `PciHostBridgeLib` extended: reads LTSSM (`0xfd802080` bits `[8:3]`)
  before adding the external root bridge at all - if `< L0`, the segment
  is simply never presented to `PciBusDxe` (no cold bring-up attempted,
  ever). If already linked (confirmed live: `LTSSM 0x11` = L0), applies
  `AlPcieExt0PortConfigFixup()` - `CFG_TARGET_BUS`/AXI-snoop/RC-mode
  `COMMAND`, register-for-register identical to `alpine.c`'s
  `al_pcie_ext0_port_config_fixup()` - deliberately *without*
  `axi_slave_err_resp` (see below).
- `PlatformLibMem.c`: added the missing MMU region for external PCIe0's
  MMIO/BAR window (`0xC0010000-0xC7FFFFFF`) - it sits in the gap between
  DRAM0's end and the SoC device band, wasn't covered by any existing
  descriptor.
- Added `XhciDxe`/`UsbBusDxe`/`UsbMassStorageDxe` to the component list.

**Result: a real, reproducible hard crash - SError inside `XhciDxe.efi`
itself** (`XhcReadCapReg8`, `MdeModulePkg/Bus/Pci/XhciDxe/XhciReg.c:48`
- the very first xHCI capability-register read), not a graceful
timeout. Root cause, from the full boot trace: **the ASM1042A
(`1B21:1142`) aliases across multiple devfns on the external segment**
(`PciBus: Discovered PCI @ [00|00|00]` through `[00|09|00]`, all
identical VID/DID) - the exact "same device on every devfn" symptom
issue #140 spent many rounds on for U-Boot. `PciBusDxe` has no
protection against this (it isn't a DWC-aware driver), so it treats each
alias as a separate real device, assigns each a BAR, and `XhciDxe`
binds to more than one - at least one of which gets a BAR that doesn't
correspond to real hardware, faulting on first touch.

**Fixed (2026-09-03): `Library/PciSegmentLib/`**, a small UNVR fork of
the stock `MdePkg/Library/PciSegmentLibSegmentInfo` (copied verbatim,
one change) - every mainline DesignWare PCIe host driver
(`pcie-designware-host.c`'s `dw_pcie_rd_other_conf()`) has a built-in
guard where config-space accesses to any devfn other than 0 on the root
complex's own bus are answered in software as "no device," never
actually issued to hardware; EDK2's generic `PciSegmentLibSegmentInfo`
has no equivalent, so this fork adds it: `PciSegmentLibGetEcamAddress()`
redirects any `Device != 0 || Function != 0` access on segment 1 to a
static, all-`0xFF`-filled dummy buffer instead of real MMIO - reads come
back as the standard PCI "no device" response, writes land harmlessly.
See `PciSegmentLibCommon.c`'s file header and
`docs/audits/audit-edk2-pcie-glue.md`.

**Confirmed live: the crash is gone.** `PciBus: Discovered PCI @
[00|00|00] [VID = 0x1B21, DID = 0x1142]` now appears **exactly once**
(no more devfn 1-9 aliases), boot reaches the interactive Shell cleanly,
and the Shell's own `pci` command independently confirms one clean
entry: `Bus 01 Dev 00 Func 00 - Serial Bus Controllers - USB, Vendor
1B21 Device 1142 Prog Interface 30` (0x30 = XHCI). `XhciDxe`'s own
controller-level init (reset, capability read, operational register
setup) completes with no crash, no hang, no timeout.

**Tested (2026-09-03) with a real device attached: differential test
confirmed positive.** A Realtek RTL8153 USB-Ethernet dongle
(`0bda:8153`) was plugged into the external port. EDK2's `XhciDxe`
fails the exact same way U-Boot's driver does in #140:

```
UsbEnumeratePort: new device connected at port 2
XhcUsbPortReset!
XhcInitializeDeviceSlot: Enable Slot Failed, Status = Time out
UsbEnumerateNewDev: failed to set device address - Device Error
```

Two independent xHCI implementations (U-Boot's `drivers/usb/host/
xhci.c` and EDK2's `MdeModulePkg/Bus/Pci/XhciDxe`, completely separate
codebases) both fail with an Enable-Slot-command timeout on the same
hardware, against two different real device types (a USB-SATA/SSD
enclosure previously, this RTL8153 now). EDK2 retries a few times then
continues gracefully (reaches the Shell fine, unlike U-Boot's long
`usb start` retry loop).

**Correction, caught right after the initial "silicon-level" framing
was posted to #140**: Linux's own xHCI driver works fine on this exact
hardware (#157), so this can't be a plain hardware defect - two drivers
failing only rules out "bug in one driver's protocol implementation,"
not the chip. The better-fitting, already-open #140 hypothesis: AL-324
is a 2x2 A57 cluster part; Linux brings up both clusters via SMP,
neither U-Boot nor EDK2 does (`PcdCoreCount|1` here, matching U-Boot's
own single-core execution at this stage) - if the CCU cluster1/slave4
snoop-routing path is gated on cluster1 actually being powered, that
explains both bootloader failures and Linux's success with one
variable. Untested, cheap, decisive next step: bring up EDK2's second
core cluster (PSCI `cpu_on`, §6's other still-unconfirmed item) and see
if that alone changes the SLOT_ID result. Full writeup: issue #140.

**Also confirmed working, from the earlier crash-chasing rounds, still
correct**: LTSSM-gated root bridge addition (never retrains an
already-linked port), the config-space fixup, and the missing-MMU-region
fix in `PlatformLibMem.c` are all real, verified-necessary pieces.

**Also confirmed NOT to help**: enabling `axi_slave_err_resp` (matching
alpine.c) turns a failed downstream access into an immediate SError
instead of a normal `0xFFFFFFFF` "no device" response - tried first,
made the earlier crash *less* informative (identical crash address
regardless), left disabled.

Box power-cycled clean after every test, crash or success alike
(RAM-payload only, docs/uefi.md §1's safety guarantee held throughout -
never touched flash).

### P2 — SATA

- 2× AHCI EPs behind internal PCIe (`1c36:0031`, abar `0xfe154000` / `0xfe158000`,
  4 ports each). Stock `AtaAtapiPassThru` + `AhciBus` (generic AHCI).

### P3 — network

- 1G `al_eth` `1c36:0001` (RGMII → AR8031 addr 4) and/or 10G `1c36:0002` (SFP+).
- **Reference**: `imbushuo/ccr2004-uefi`'s `Drivers/AlEthNextDxe/` — a real SNP
  driver for the same SoC (AL324/Alpine V2) on the same delroth-vintage HAL (2.9)
  ([hal-provenance-and-cross-system.md](hal-provenance-and-cross-system.md) §3, #85).
  **BSD-2-Clause-Patent** (per-file SPDX headers; the repo carries no LICENSE file),
  so copying is fine with the MikroTik copyright line kept.

**Status (2026-09-04): binds on hardware, confirmed live. TX/RX untested.**

```
Loading driver at 0x000BB2A2000 EntryPoint=0x000BB2AB408 AlEth1gSnpDxe.efi
AlEth1g: BARs UDMA 0xFE000000 MAC 0xFE15C000 EC 0xFE150000
AlEth1g: MAC 74:AC:B9:41:A8:11
AlEth1g: SNP installed on 1c36:0001
```

- All three BARs match [hardware.md](hardware.md#mmio-and-address-map)'s recorded
  map exactly (eth0 `0xfe000000`, eth3 `0xfe150000`, eth4 `0xfe15c000`).
- MAC read from the EC filter matches what U-Boot printed the same boot
  (`al_eth1: MAC 74:ac:b9:41:a8:11`) — the EC hand-off works.
- `connect -r` binds cleanly, installs DevicePath + SNP, no exception or hang.
- Reproduce: `./dev.py uefi-chainload-probe`, then
  `./dev.py uefi-shell-cmd "connect -r"`. Log `tmp/logs/uefi-p3-connect.log`.

Two new pieces, both inside `Platform/Ubiquiti/UNVR/`:

- **`Library/AlpineHalLib/`** — the HAL compiled for EDK2. 12 `.c` (eth + UDMA +
  IOFIC) and 47 headers, copied **byte-identical from our own U-Boot tree**
  (`uboot-port/drivers/net/al_eth/hal/` plus 3 from `al_hal_shim/`), not from the
  CCR2004 reference: same board, our maintained fixes, and the exact subset the
  1G path needs (no v3/v4 MAC, no serdes/KR closure). The porting layer is the two
  force-included shims `al_hal_plat_types.h` / `al_hal_plat_services.h`, adapted
  from the reference's — MMIO, barriers, delays, `al_memset`, logging onto MdePkg.
  - EDK2 INFs cannot reference sources outside their own package, so sharing the
    U-Boot copy in place was impossible. This is a **fifth** HAL copy (#218) and is
    now a registered `hal-drift-check.py` tree (`edk2`), starting at zero drift.
- **`Drivers/AlEth1gSnpDxe/`** — the SNP driver. Binds `1c36:0001` only.

Board deltas vs the CCR2004 reference, each traceable to our working U-Boot driver
(`uboot-port/drivers/net/al_eth/al_eth_dm.c`):

| Delta | Why |
|---|---|
| PHY addr fixed at **4**, no MDIO scan | Hardware-of-record; a scan can latch a stale/aliasing address |
| **No** AR8035 RGMII TX-delay poke | UNVR's delay is board-strapped; our U-Boot driver does not touch it |
| **No** `al_eth_mac_link_config()` | Stock skips it for external-PHY RGMII — the AR8033 drives the link, MAC follows in-band |
| MAC from the **EC filter register** | No MikroTik BoardInfo protocol, and EDK2 has no SPI-NOR driver. U-Boot writes it at probe from NOR `0x1f0000` (`al_eth_hwaddr.c`); a zero reading falls back to a locally-administered address |
| **No** EmbeddedGpio PHY reset | No PL061/GPIO driver on this platform |

- `AlEth1gHalStubs.c` stubs `al_eth_mac_v3/v4_handle_init` and
  `al_unit_adapter_init` — the same three symbols, for the same reason, as
  U-Boot's own `al_eth_stubs.c`. Avoids dragging in the SerDes/KR closure.
- DMA is **uncached + explicit cache maintenance**: `AlPcieSnoopFixDxe`
  deliberately excludes both al_eth functions from the AXI snoop fixup (applying
  it broke UDMA TX, #74/#90), so the device is not coherent with CPU caches.

**Trap, cost an hour: a LibraryClass can silently un-dispatch a driver.**
Declaring `NetLib` for two *header macros* (`NET_ETHER_ADDR_LEN`,
`NET_IFTYPE_ETHERNET`) linked `NetworkPkg/DxeNetLib`, whose own
`[Depex] gEfiRngProtocolGuid` is folded into the module's auto-generated depex.
Nothing here installs an RNG **protocol** (`RngLib` is a library, not
`RngDxe`), so the AND chain evaluated FALSE forever:

```
Evaluate DXE DEPEX for FFS(3696B990-...)
  PUSH GUID(3152BCA5-EADE-433D-862E-C01CDC291F44) = FALSE   <- gEfiRngProtocol
  ...  RESULT = FALSE
```

- Failure mode is **silence**: the driver is present in the FV, never loads, and
  nothing says why unless `DEBUG_DISPATCH` tracing is read.
- Fix: define the two macros locally, drop `NetLib`. The module then emits **no
  `.depex` at all**, which is correct for a `UEFI_DRIVER` bound by `PciBusDxe`.
- Diagnosing it: decode `Build/.../OUTPUT/<mod>.depex` (opcode `0x02` = PUSH
  GUID) and check each GUID against the boot trace.
- **Second trap**: an incremental build reuses a cached `.depex`. `rm -rf` the
  module's Build dir when changing `[LibraryClasses]`/`[Depex]`.

**Still unproven:**

- link state off `MAC_GEN_RGMII_STAT` (`0x91C` bit 4) — never read, since
  nothing calls `SnpInitialize()` without a network stack driving it;
- `SnpInitialize()` itself: the HAL adapter init, queue config, `mac_config`,
  MDIO/PHY bring-up. All that ran was `Start()` (bind + BAR/MAC read);
- TX/RX. **TX is the known risk**: `#90`'s UDMA M2S DRAM-read hang is open and
  unexplained on the 1G port under Linux. If it reproduces here it is the same
  bug in a second, independent driver — useful evidence either way.

Next step is a small UEFI Shell app calling `Initialize()` + `GetStatus()` on the
SNP handle, which forces the whole init path and prints link state.

### P4 — GRUB → Linux

- EDK2 installs `live.dtb` as `gFdtTableGuid` config table; `grubaa64.efi` from the
  ext4 USB or SATA; GRUB loads the mainline `Image` + initramfs; Linux enters at
  the EL EDK2 hands off (EL2 per current chain).
- This is the payoff: a standard UEFI+ACPI/DT boot for TrueNAS/Fedora/Debian
  without the stock initramfs.

---

## 5. Bench step results (2026-09-02, `./dev.py uboot-bench-check`)

Read-only U-Boot queries, run via `scripts/uboot-bench-check.py` (power-cycle +
catch-uboot.py race, then `version`/`help`/`help bootm`/`printenv`/`bdinfo`/
`help go` — no writes, no chainload jump).

- **`version` is absent** — `Unknown command 'version' - try 'help'`. Doc
  previously expected this to work; it doesn't on this build. No build-date
  string obtained this way; not blocking.
- **`go`, `bootm` confirmed present**; `bootefi`, `booti`, `fatload` confirmed
  absent from `help`'s full command list — matches the binary-string analysis
  in §1.
- **DRAM banks (`bdinfo`)**: 4 banks, each 1 GiB — `0x0`, `0x40000000`,
  `0x80000000` (contiguous, = the doc's "DRAM0 0x0/0xC0000000" 3 GiB region)
  and `0x200000000` (= the doc's "DRAM1" 1 GiB region). Reconciles exactly
  with §3's PCD table — no change needed there.
- **`relocaddr = 0x03F1E000`** — U-Boot relocates itself to ~66 MiB, well
  clear of the chosen EDK2 FD base `0x20000000` (512 MiB). Closes the §6
  "FD load base collision" risk.
- **`bootcmd`** confirms the live NAND boot flow: kernel read from
  `0x1300000` (4-byte length prefix at `0x300000` via the partition-offset
  dance), dtb from `0x2800000`, `bootm 0x02000000 - 0x04078000` — matches
  [nand-boot-layout-recovery] (memory).

Entry EL is still unconfirmed (needs EDK2's own boot log, not a U-Boot
command — see §6). Next: the dry chainload probe below, once a P0 FD exists.

```
setenv ipaddr <unit>; setenv serverip <host>
tftpboot 0x20000000 UNVR.fd     # host serves it via scripts/tftpd.py
crc32 0x20000000 ${filesize}    # verify against host crc32 before jumping
go 0x20000000                   # transfer control to EDK2 SEC
```

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
- ~~FD load base collision~~ — resolved. `relocaddr = 0x03F1E000` (§5,
  confirmed on bench), well clear of `0x20000000`.
- **Arch-timer frequency conflict.** U-Boot config hardcodes `COUNTER_FREQUENCY
  50000000` ([bootloader.md](bootloader.md)) but the **live DT says 58 333 312 Hz**
  and Linux runs at 58.33 MHz. Trust the DT / read `CNTFRQ_EL0` at runtime; a wrong
  value = all EDK2 timeouts/delays wrong by 1.17×.
- **al_eth port still needed for our board** (P3) — a reference now exists
  (`AlEthNextDxe` in `imbushuo/ccr2004-uefi`, see §P3) but our DTS/board params
  still need porting; not a drop-in reuse.
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
