# Modern U-Boot port — UNVR / Annapurna Labs Alpine V2 (AL-324)

Feasibility + staged plan to replace the frozen stock **U-Boot 2015.07-alpine_db-2.21-HAL**
(issue #46) with mainline **U-Boot v2026.07**. Target SoC: AL-324, quad Cortex-A57,
aarch64, sysid **0xea16**.

Marks: ✅ confirmed this session · ⚠ needs on-device test · 📄 from source/docs.

Sources cited by absolute path:
- Mainline: `/mnt/2tb/unvr-port-refs/u-boot-v2026.07` (full mirror at
  `/mnt/2tb/git_mirror/u-boot/u-boot` when cloned).
- Ubiquiti GPL U-Boot: `/mnt/2tb/unvr-port-refs/UBNT-source-code/UNVR-1.3.35-GPL/u-boot`.
- RE'd Annapurna HAL: `/mnt/2tb/unvr-port-refs/delroth-alpine_hal`.
- Board DTS: `/mnt/2tb/unvr-port-refs/linux-alpine-v2/dts/alpine-v2-ubnt-unvr-ea16.dts`;
  Linux dtsi `/mnt/2tb/unvr-port-refs/linux-6.12/arch/arm64/boot/dts/amazon/alpine-v2.dtsi`.
- MMIO map: `docs/hardware.md`. Boot chain: `docs/nor-boot-chain.md`.

---

## 0. Verdict

- **Stage-1 (chainload console) is DONE-feasible and BUILDS ✅.** A minimal mainline
  U-Boot-proper for AL-324 compiles cleanly to a 321 KB aarch64 `u-boot-dtb.bin`
  loadable via the stock `go 0x1100000`. Scaffold created (§9).
- **Stage-2 SATA read (chainload) is feasible, medium effort.** Blocker is porting the
  **Alpine internal-PCIe host** enumeration (no mainline U-Boot driver); generic mainline
  AHCI then binds the `1c36:0031` EPs. Coherency mechanism is the open question (§6).
- **Stage-3 standalone replacement (SPL+DDR from reset) is feasible, HIGH effort.** The
  GPL stock U-Boot does **no** DDR/clock/serdes init (`CONFIG_SKIP_LOWLEVEL_INIT` ✅) — it
  runs after the **proprietary al_boot preboot** brings DRAM up. DDR bring-up + BIST/margins
  must be forward-ported from the **delroth alpine_hal** `ddr/` (the only published copy);
  it is not in any GPL U-Boot. This is the make-or-break for booting from reset (§5).

Nothing about AL-324 exists in mainline U-Boot today (§1) — this is a from-scratch board,
but every standard-IP block (UART, SPI, I2C, AHCI, NAND, DW-PCIe) has a mainline driver.

---

## 1. Mainline v2026.07 support inventory

**Zero** Alpine/Annapurna/AL-324 support ✅ (grep of `arch/ drivers/ board/ configs/ dts/`):
no board, no defconfig, no DTS, no mach dir. (`1c36` hits are unrelated: meson video regs,
riscv DDR, x86 microcode.) So: new board dir + defconfig + DTS + Kconfig target, all new.

Generic drivers that DO exist and are reusable (mainline path → our block):

| Block | Mainline driver present | File |
|---|---|---|
| Console 8250/DW | ✅ `ns16550` (`SYS_NS16550`, DM) | `drivers/serial/ns16550.c` |
| AHCI/SATA | ✅ generic `ahci` + `ahci-pci` + `dwc_ahci` | `drivers/ata/ahci.c`, `ahci-pci.c` |
| SPI (DW SSI) | ✅ `designware_spi` + `spi-mem` + `sf` | `drivers/spi/designware_spi.c` |
| SPI-NOR | ✅ `sf`/`spi-nor` (MX25U class) | `drivers/mtd/spi/` |
| I2C (DW) | ✅ `designware_i2c` | `drivers/i2c/designware_i2c.c` |
| NAND (Denali) | ✅ `denali` (⚠ AL-NAND ≠ Denali — see §4) | `drivers/mtd/nand/raw/denali.c` |
| PCIe DWC | ⚠ `pcie_dw_common` + per-SoC glue only | `drivers/pci/pcie_dw_*.c` |
| GIC-v3 | ✅ | `arch/arm/lib` / `drivers/` |
| PSCI/timer | ✅ arch generic | `arch/arm/cpu/armv8` |
| EFI (`bootefi`) | ✅ full `efi_loader` | `lib/efi_loader/` |

---

## 2. Chainload framing — confirmed ✅

- **Load/entry `0x01100000`** = stock `CONFIG_SYS_TEXT_BASE` (stock
  `include/configs/alpine_v2_64_ubnt_nas.h:13`) ✅. stage3 loads the stock `uboot` TOC object
  here and jumps (`docs/nor-boot-chain.md` §2). We set mainline `CONFIG_TEXT_BASE=0x01100000`.
- **Stock U-Boot itself is U-Boot-proper: `CONFIG_SKIP_LOWLEVEL_INIT`** (stock
  `include/configs/alpine_db_common.h:59`) ✅ — no SPL, no DDR. DRAM is fully up when it (and
  our chainloaded image) runs. Relocation to top-of-RAM works exactly as the stock does.
- **Entry via `go`:** `go <addr>` passes argc/argv, **not** a DTB pointer in x0. So the test
  image carries its **own embedded control DTB** (`OF_CONTROL` + `u-boot-dtb.bin`), not the
  prior stage's — simpler and deterministic. (If we later want the stock DTB, mainline's
  `OF_HAS_PRIOR_STAGE` + `board_fdt_blob_setup` is the hook, cf. total_compute.)
- **Init SP** `0x05000000` (low DRAM, above image); stock used `0x01800000`. Either is valid
  once DRAM is up.

**Two distinct replace targets** (do not conflate):
1. **Replace the `uboot` TOC object only** — keep the proprietary preboot (S2+al_boot+stage3)
   doing DDR/clock/serdes. This is what the stock chain does; **no SPL needed**, DRAM already up.
   This is the realistic flashable replacement, and the chainload image is a superset test of it.
2. **Replace the whole boot chain** (S2 loader at SPI 0 + al_boot) with U-Boot SPL doing DDR —
   requires reproducing the DDR training from the HAL and the boot-ROM S2 format. Much larger;
   only needed if the goal is to eliminate the proprietary preboot entirely.

The DRAM-overclock goal (#29) does **not** require target 2: DDR margin/BIST/ECC commands run
from U-Boot against the already-trained controller (§5); only *retraining at a new frequency*
needs SPL-from-SRAM.

---

## 3. Device tree approach

- **Stage-1 DTS:** hand-written minimal, self-contained (no Linux include chain):
  `arch/arm/dts/awto-alpine-v2-unvr-uboot.dts` — cpus(4×A57)+PSCI+GIC-v3+arch-timer+UART0+memory.
  All addresses from `docs/hardware.md` / the Linux `amazon/alpine-v2.dtsi`. Built ✅.
- **UART0** absolute `0xfd883000`, `ns16550a`, `reg-shift=2`, `reg-io-width=4`,
  `clock-frequency=500000000` (from `amazon/alpine-v2.dtsi:160`; the Linux node sits at bus
  `0x1883000` under an io-fabric `ranges` to `0xfc000000`, = `0xfd883000` absolute).
- **Memory** 4 GiB: `<0x0 0x0 0x0 0xC0000000>` + `<0x2 0x0 0x0 0x40000000>` (hardware.md).
- **GIC-v3** GICD `0xf0200000`/0x10000, GICR `0xf0280000`/0x200000.
- **Later stages:** grow the same DTS with the internal-PCIe host, AHCI, DW-SSI+NOR, DW-I2C.
  Do **not** reuse the stock 2015.07 control DTB (different bindings); grow from our own or
  trim the Linux ea16 DTS. The ea16 DTS's `dma-coherent` on `pci@fbc00000` + `ccu` +
  `nb-service` nodes document what the coherent path needs.

---

## 4. Per-peripheral gap table (chainloaded U-Boot-proper on AL-324)

driver | mainline status | glue/DT needed | port from
---|---|---|---
**UART console** | ✅ `ns16550` binds our DT node | none — DT `clock-frequency`/`reg-shift` | built ✅ Stage-1
**SATA/AHCI** (2× `1c36:0031`) | ✅ generic `ahci` binds AHCI-class PCI | **internal-PCIe host enum + DMA coherency (§6)** | Annapurna `board/annapurna-labs/common/pci.c` + `al_init_pcie`; Linux `pcie-al.c`
**Internal PCIe host** (`fbc00000` ECAM) | ❌ no Alpine host driver | new host driver (custom ECAM `addr=(bus<<20)|(dev<<15)|(fn<<12)|reg` — Annapurna `pci.c:86`) + AXI snoop | Annapurna `pci.c`/`al_init_pcie*`; delroth `drivers/pcie/`; Linux `drivers/pci/controller/pcie-al.c`
**External PCIe** (`fd800000`) | ⚠ DWC common only | Alpine external-PCIe glue | Annapurna `pci.c`; delroth `drivers/pcie/`
**SPI-NOR** (DW SSI `fd882000`, MX25U25635F) | ✅ `designware_spi`+`sf` | DT node `snps,dw-apb-ssi` + `jedec,spi-nor`; verify 1.8V/4KiB/256B | mainline binds directly; stock board only for env layout
**I2C** (DW `fd880000`,`fd894000`) | ✅ `designware_i2c` | DT `snps,designware-i2c` | direct
**NAND** (`fa100000`, Micron MT29F8G08) | ❌ AL-NAND is a **custom** controller, not Denali | new raw-NAND driver | Annapurna `board/annapurna-labs/common/al_nand.c` + `early_nand.c`; kernel `al_nand`
**GIC-v3 / timer / PSCI** | ✅ | DT nodes (have them) | direct
**Reset (`reset`)** | ✅ done | `reset_cpu()` pokes SP805 `wdt0` @`0xfd88c000` (unlock/load/control INT\|RESET). NOT PSCI `SYSTEM_RESET` (unimplemented in the resident EL3 monitor — #51) nor `fabric_software_reset` (sub-block only). RE-confirmed, see [reboot-driver-handover.md](reboot-driver-handover.md) | stock `al_board.c:190`
**MSI-X** (`al,alpine-msix`) | ❌ (not needed for U-Boot polled I/O) | skip for U-Boot | n/a
**SerDes** (`fd8c0000`) | ❌ custom | only needed if we bring up PCIe/SATA PHY from reset (target 2); chainload inherits preboot's serdes | Annapurna `board/.../common/cmd_serdes*.c`; delroth `drivers/serdes/`
**Thermal** (`fd860a00`) | ❌ custom | optional cmd | Annapurna `cmd_thermal.c`; delroth
**SGPO bay LEDs** (`fd8b4000`) | ❌ custom | optional | Annapurna `dt_based_sgpo_init.c`
**EFI `bootefi`** | ✅ `efi_loader` | enable `CONFIG_EFI_LOADER` once RAM/console up | direct (feeds #39 EDK2 path)

---

## 5. DDR bring-up + BIST/margins — make-or-break for STANDALONE (target 2)

**Where DDR init lives:**
- **NOT** in the GPL stock U-Boot: `arch/arm/cpu/armv8/alpine_v2/` contains only a 105-byte
  Kconfig ✅; `CONFIG_SKIP_LOWLEVEL_INIT` set. The `al_hal_ddr.h` that Annapurna
  `board/.../common/cmd_ddr.c` includes is **absent from the GPL drop** ✅ (the HAL was an
  external/prebuilt module).
- DDR training is done by the **proprietary al_boot preboot** (`stage2_loader v2.22.3`,
  `agent_wakeup v2.10`; strings `DDR size not supported!`, `DRAM frequency violation!` —
  `docs/nor-boot-chain.md` §2).
- The **only published DDR init source** is `delroth-alpine_hal/ddr/src/` — has
  `al_hal_ddr_init_alpine_v2.c`, `al_hal_ddr_alpine_v2.c`, phy/ctrl regs for v2, JEDEC DDR3/DDR4
  headers, and `al_hal_ddr_pmu.c` (margin/BIST). Our DRAM is **DDR4** (Samsung K4A8G165WB ×4).

**Plan (target 2 SPL):**
1. Create `arch/arm/mach-alpine/` + SPL that runs from SoC SRAM (`s2_sram 0xf2200000`, hardware.md).
2. Forward-port `al_hal_ddr` (delroth v2 files) into the SPL as the DDR driver: controller cfg,
   PHY training, DDR4 JEDEC init. Feed it the board's SPD (read over I2C `0x57` — Annapurna
   `stage2_loader` does exactly this).
3. Reproduce clock/PLL + serdes bring-up (§7) before DDR.
4. Boot-ROM handoff: the mask ROM loads the **S2 SPI loader** (`"S2\0\0"` header, SPI off 0)
   into SRAM `0xf2200000`. To boot from reset without the Annapurna S2, U-Boot SPL must match that
   ROM-expected format — **needs the S2/boot-ROM contract confirmed** ⚠ (`docs/nor-boot-chain.md`
   §1 has the header layout).

**BIST / margins / ECC — first-class feature, works WITHOUT SPL (against a live controller):**
- Annapurna exposes, all in `board/annapurna-labs/common/cmd_ddr.c` + `cmd_dram_margins.c`:
  `ddr_ecc_stats`, `ddr_ecc_poison`, `dram_margins`, `ddr_training_results` — all call the HAL
  (`al_ddr_cfg_init`, `al_ddr_ecc_status_get`, `al_ddr_ecc_data_poison_enable`,
  `al_ddr_address_translate_sys2dram`).
- Port these as U-Boot commands backed by delroth `al_hal_ddr` + `al_hal_ddr_pmu.c`. They read
  the already-trained controller regs — safe from a running (chainloaded) U-Boot, and give the
  #29 overclock effort its measurement tools immediately, before any SPL work.

---

## 6. SATA plan — priority for the chainload path

**Topology:** SATA controllers are **PCI endpoints** `1c36:0031` (×2, 4 ports each — ata1–4 @
abar `0xfe154000`, ata5–8 @ `0xfe158000`, hardware.md) behind the **internal PCIe host**
(`annapurna-labs,alpine-internal-pcie`, ECAM `0xfbc00000`). They are enumerated, not DT-bound.

**What mainline gives free:** generic `ahci.c` binds an AHCI-class PCI device and does its own
`flush_dcache_range`/`invalidate_dcache_range` around command list / FIS / PRDT / data. The
stock U-Boot proves the ID pairing works: `CONFIG_SCSI_AHCI` +
`CONFIG_SCSI_DEV_LIST={PCI_VENDOR_ID_ANNAPURNALABS, PCI_DEVICE_ID_AL_SATA}` (stock
`alpine_db_common.h:230,237`).

**The gap = the internal-PCIe host bridge.** Mainline U-Boot has no driver for Alpine's internal
PCIe. Port the enumeration from Annapurna `board/annapurna-labs/common/pci.c` (custom ECAM
addressing `(bus<<20)|(dev<<15)|(fn<<12)|reg`, per-controller windows `AL_PCIE_*`, link-up
retry) and/or Linux `drivers/pci/controller/pcie-al.c`. Once the host enumerates the bus,
generic AHCI + `scsi scan` should find the drives.

**Coherency — the real make-or-break, ⚠ needs test.** The Linux internal-PCIe driver
"configures **AXI sub-master snoop** registers for cache-coherent DMA" and marks the node
`dma-coherent` (ea16 DTS lines 92–98). Two outcomes to test on-device:
- **(a)** Mainline U-Boot AHCI's explicit cache maintenance is sufficient with our MMU map
  (DRAM = `MT_NORMAL` inner-shareable; MMIO = `MT_DEVICE_NGnRnE`) → SATA works with no snoop
  config. **Try this first** — it's the low-effort win.
- **(b)** If DMA returns stale/garbage, port the **AXI snoop enable** from the Annapurna HAL /
  Linux `pcie-al.c` (CCU `io_coherency=1` @ `f0090000`, hardware.md) into the host driver's
  probe. The ea16 DTS `ccu@f0090000 { io_coherency=1 }` + `nb-service` nodes name the block.

**Stock U-Boot-can't-read-SATA claim** ⚠: the stock config *has* AHCI+SCSI, so "can't read
the SSD" is either an enumeration/timing gap or SSD-specific. Confirm on-device with stock
`scsi scan` before assuming; the modern port's value is a *working* `scsi`/`sata` + coherent DMA.
This is the #1 functional goal.

---

## 7. PLL / clock overclock hooks (#29)

- Annapurna PLL/clock code: `board/annapurna-labs/common/pll_init.c` + `cmd_cpu_misc.c`
  (`cpu_set_speed`), `alpine_ubnt/board_cfg.h`. CPU is A57 @ 1.7 GHz stock.
- delroth HAL `services/` + `drivers/sys_services/` hold the PLL/VCO register access.
- Port `cpu_set_speed` + the PLL-VCO path as U-Boot commands; combine with the DDR margin
  tooling (§5) so CPU and DRAM overclock are measurable from the same U-Boot. Chainload-safe
  for CPU PLL; DRAM-frequency change needs the SPL retrain path (§5).

---

## 8. Staged build/test plan

1. **Stage-1 — console (DONE, builds ✅).** Chainload `go 0x1100000` the 321 KB
   `u-boot-dtb.bin`; expect the mainline banner + `awto-nas#` prompt on ttyS0 (115200 8N1). ⚠ boot-test
   on device.
2. **Stage-1b — SPI-NOR + I2C.** Add DW-SSI+`sf` and DW-I2C DT nodes; `sf probe` should detect
   MX25U25635F; `i2c dev`/`i2c probe` the SPD @0x57, PCA9546 mux, etc. Low effort (direct mainline).
3. **Stage-2 — SATA (priority).** Port internal-PCIe host enum (§6); `pci enum` → `scsi scan`;
   test coherency path (a) then (b). **Make-or-break for chainload usefulness.**
4. **Stage-3 — EFI.** `CONFIG_EFI_LOADER`; `bootefi` — feeds #39 EDK2.
5. **Stage-4 — NAND.** Port AL-NAND custom driver for `nand` read (boot medium).
6. **Stage-5 — standalone SPL+DDR (target 2).** Forward-port delroth `al_hal_ddr` (DDR4) +
   clocks/serdes into a `mach-alpine` SPL; confirm the boot-ROM S2 contract; **make-or-break for
   from-reset boot / flashable replacement.**
7. **Stage-6 — feature parity.** multiboot / flash-TOC A/B/C/D, board table, env, `bootupd`, LCD,
   thermal, SGPO — port from Annapurna `board/annapurna-labs/` as needed.

Between attempts: `scripts/uboot-build.py --clean` (distclean + un-stage) — disk is ~39 GB free.

---

## 9. Scaffold created + build result ✅

Scaffold lives in-repo at **`uboot-port/`** (source of truth); staged into the U-Boot tree by
**`scripts/uboot-build.py`** (idempotent stage + `arch/arm/Kconfig` patch + build; `--clean`
reverts). Build log: `tmp/logs/uboot-port.log`; notes `tmp/logs/uboot-port.md`.

Files:
- `uboot-port/board/annapurna/alpine/` — `Kconfig`, `Makefile`, `MAINTAINERS`, `alpine.c`
  (mem_map: DRAM `MT_NORMAL` inner-share + MMIO `MT_DEVICE_NGnRnE`; `dram_init` via FDT;
  `reset_cpu` = PSCI).
- `uboot-port/include/configs/alpine.h` — minimal.
- `uboot-port/configs/alpine_v2_unvr_defconfig` — `TARGET_ALPINE_V2_UNVR`, `TEXT_BASE=0x01100000`,
  `INIT_SP=0x05000000`, 2 DRAM banks, `ns16550`+`DM_SERIAL`, `GIC_V3`, `NO_NET`.
- `uboot-port/arch/dts/awto-alpine-v2-unvr-uboot.dts` — the Stage-1 DTS (§3).

**Build result (v2026.07, `aarch64-linux-gnu-gcc 16.1.1`):**
- `alpine_v2_unvr_defconfig` accepted ✅ (board Kconfig wiring valid).
- Full build **OK ✅** → `u-boot` = ARM aarch64 ELF; `u-boot-dtb.bin` = **321 KB** (the
  `go 0x1100000` payload); our `alpine.o` + `awto-alpine-v2-unvr-uboot.dtb` (5.7 KB) compiled in;
  `TEXT_BASE=0x01100000`, `ARM64`, `SYS_NS16550`, `DM_SERIAL` all set.

**Toolchain/env gotcha (blocked the first two builds):** OpenSSL **3.5** (Fedora 44) removed
`engine.h`; U-Boot v2026.07 host tools `lib/rsa/rsa-sign.c` + `lib/aes/aes-encrypt.c` still
`#include <openssl/engine.h>` → fatal. `TOOLS_KWBIMAGE` `select`s `TOOLS_LIBCRYPTO`, so the
defconfig must disable **both**: `# CONFIG_TOOLS_LIBCRYPTO is not set` +
`# CONFIG_TOOLS_KWBIMAGE is not set` (+ `# CONFIG_FIT_SIGNATURE is not set`). These are **host
mkimage** features only — no effect on target features. (Worth an upstream note; not our bug.)
