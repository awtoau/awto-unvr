# Mainline Linux port — roadmap (UNVR / Alpine V2)

Turn Ubiquiti's frozen 4.19.152 vendor kernel into a **mainline** Linux for this
box (custom NAS: TrueNAS/Fedora/Debian). Strategy owner: Dan. Spine of this doc =
the eight-phase strategy below; every row annotated with the **live hardware
evidence** captured 2026-08-16.

- Ground-truth evidence: [hw-reference/20260816-104601/](hw-reference/20260816-104601/)
  and the decompiled vendor DT [live.dts](hw-reference/20260816-104601/live.dts).
- Refreshed chip table: [hardware.md](hardware.md).
- Do NOT restate the finding docs — links below.

## Goal + why now

- Target: **Alpine V2 solid on Linux 6.18 LTS** (newest LTS, ~Dec 2028), minimal
  Annapurna-specific code, reuse current kernel subsystems. Forward-port the small
  patch set to **7.1** (current stable; a local mainline mirror is on 7.1).
  Upstream generic wins where practical.
- Feasible because two locks are already open:
  - **Boot is UNSIGNED** — U-Boot `dobootm = run bootunsign`; we can boot our own
    kernel. See [bootloader.md](bootloader.md), [boot-flow.md](boot-flow.md).
  - **Module signing / lockdown OFF** in the stock kernel config
    (`CONFIG_MODULE_SIG` unset, no lockdown). See
    [porting-reference.md](porting-reference.md).
  - U-Boot is writeable (mtd0 flags 0xc00) but **frozen** — vendor never rewrites
    it on ea16, so we build ON TOP of the existing U-Boot, not replacing it. See
    [uboot-update-path.md](uboot-update-path.md).
- sysid **ea16** confirmed live ([ubnthal.txt](hw-reference/20260816-104601/ubnthal.txt):
  `systemid=ea16`, `shortname=UNVR4`). (porting-reference.md's `ea1a` guess is
  superseded — use ea16.)
- Prior art = **evidence + test reference, NOT final architecture**: the 6.12
  Alpine V2 port and al_eth standalone. See [prior-art.md](prior-art.md).

## Target architecture — strategy vs. live capture

Each row: hardware → preferred mainline solution → **what the capture shows**.
Compatible strings taken from the vendor DT / live drivers; "unconfirmed" means the
capture does not prove register compatibility.

| Hardware | Mainline solution | Live evidence | Status |
|---|---|---|---|
| 4× Cortex-A57 | ARM64, PSCI, generic timer | `CPU part 0xd07` ×4 ([cpuinfo](hw-reference/20260816-104601/cpuinfo.txt)); `psci: PSCIv0.2`, `arch_timer 58.33MHz`, 4 CPUs SMP ([dmesg](hw-reference/20260816-104601/dmesg.txt)); DT `psci` `method="smc"` | **Confirmed.** Mainline `alpine-v2.dtsi` already models these (`arm,cortex-a57`) |
| GIC | ARM GIC driver | `arm,gic-v3` @ `0xf0200000`; GICR `0xf0280000`; `GICv3: split EOI/Deactivate` ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Confirmed.** In mainline dtsi |
| MSI | mainline alpine-msix | DT `al,alpine-msix` @ `0xfbe00000`; live SPI base 0xa1(161)/num 0x9e(158) vs mainline dtsi 160/160 | **Confirmed.** `drivers/irqchip/irq-alpine-msi.c` in mainline; SPI base/count differ — set per-board in DT |
| SATA | mainline AHCI | 2× `1c36:0031` `prog-if 01 [AHCI 1.0]`, `ahci` driver bound, `AHCI 0001.0300 4 ports 6Gbps` ([lspci](hw-reference/20260816-104601/lspci.txt), [dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Confirmed** — stock uses stock `ahci`. Integrated PCIe EPs, not a platform node |
| USB | mainline xHCI + DW PCIe | ASMedia **ASM1042A** `1b21:1142` xHCI behind external-PCIe0; `xhci_hcd` bound ([lspci](hw-reference/20260816-104601/lspci.txt)) | **Confirmed** — xHCI is the ASMedia chip on a PCIe link, not an Alpine block. Needs external-PCIe glue (see PCIe row) |
| SPI-NOR + MTD | mainline SPI-NOR + fixed-partitions | DT `snps,dw-apb-ssi`+`amazon,alpine-dw-apb-ssi` @ `0xfd882000`; `m25p80 spi0.0: found mx25u25635f (32768 KB)` ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Confirmed** — mainline `spi-dw` + `m25p80` already bind it |
| I2C | mainline Synopsys DW I2C | DT `snps,designware-i2c` @ `0xfd880000`; live `i2c-0/name=Synopsys DesignWare I2C adapter` ([i2c-devices](hw-reference/20260816-104601/i2c-devices.txt)) | **Confirmed DW** — mainline `i2c-designware` binds. (Strategy's "if registers confirm it" → confirmed.) |
| UART | mainline 8250/DW | DT `ns16550a` @ `0xfd883000`, reg-shift 2, reg-io-width 4, clk 500MHz; console `ttyS0` IRQ49 ([interrupts](hw-reference/20260816-104601/interrupts.txt)) | **Confirmed** — mainline dtsi models these (as `serial@1883000` via io-bus) |
| RTC | mainline s35390a | DT `sii,s35390a` @ i2c 0x30; `rtc-s35390a 1-0030` registered ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Confirmed** — mainline `rtc-s35390a` |
| Fans / hwmon | mainline adt7475 | DT `adi,adt7475` @ i2c 0x2e; `adt7475 4-002e: revision 1`, fan4+pwm2 ([dmesg](hw-reference/20260816-104601/dmesg.txt), [hwmon](hw-reference/20260816-104601/hwmon.txt)) | **Confirmed** — mainline `adt7475`. 3 fans reading 938–949 RPM |
| GPIO expanders | mainline pca953x | DT `nxp,pca9575` @ 0x20/0x21/0x29 behind `pca9546` mux @ 0x71; live `pca9575`/`pca9546` ([i2c-devices](hw-reference/20260816-104601/i2c-devices.txt), [gpio](hw-reference/20260816-104601/gpio.txt)) | **Confirmed** — mainline `pca953x` (pca9575) + `pca954x` (mux). Note DT calls mux `pca9546` |
| SoC GPIO | mainline PL061 | DT `arm,pl061` ×6 @ `0xfd887000`+; `gpiochip2..7` bound ([gpio](hw-reference/20260816-104601/gpio.txt)) | **Confirmed** — mainline `pl061` |
| Watchdog | mainline SP805 | DT `arm,sp805` ×4 @ `0xfd88c000`+ (wdt0 enabled) | **Confirmed** — mainline `sp805_wdt` |
| PCIe | mainline DesignWare + small Alpine glue | Internal: DT `annapurna-labs,alpine-internal-pcie` ECAM `0xfbc00000`; External0: `annapurna-labs,alpine-external-pcie` @ `0xfd800000` ECAM `0xfb600000`, `link up Gen2 x1` ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Glue REQUIRED.** Mainline models internal as `pci-host-ecam-generic` — **not DMA-coherent-safe** (missing AXI snoop / APP_CONTROL). `pcie-al.c` exists in mainline; needs DBI-offset + internal-controller work. See [Phase 4](#phase-4--pcie) |
| Ethernet MAC/DMA | investigate Altera/NXP reuse | `al_eth v3.5.3`; PCI `1c36:0001` (1G) + `1c36:0002` (10G SFP+); binds by **PCI ID not DT** ("Unable to find compatible OF node") ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Unconfirmed relationship** — ancestry test decides (Phase 3). No mainline al_eth exists |
| MDIO + PHY | phylink/phylib | 1G port: RGMII to **Atheros AR8031** PHY addr 4 (`driver Atheros 8031`); shared MDIO bus ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Confirmed PHY** — mainline `at803x`. Note: `at803x` `depends on REGULATOR` |
| SFP+ | mainline SFP + phylink + PCS | 10G port `1c36:0002`: `SFP connected Yes`, `AL_ETH_LM_MODE_10G_OPTIC`, mdio 2500kHz, no ext PHY ([dmesg](hw-reference/20260816-104601/dmesg.txt)); DT `serdes-grp 3 lane 0`, `10g-serial force-1000base-x` | **Confirmed SFP optic path.** SFP module EEPROM NOT captured (`ethtool` returned "No data available") — module type unconfirmed. See [Phase 5](#phase-5--10gbe-pcs-serdes) |
| RAID/XOR engine | dmaengine + async_tx | PCI `1c36:0022` RAID + crypto; `al_dma` bound; stock already loads `async_pq/async_xor/raid456` ([modules](hw-reference/20260816-104601/modules.txt)) | **CPU-parity fallback proven live** — accel is genuinely last. No mainline al_dma |
| Crypto engine | crypto-engine framework | PCI `1c36:0022` "Network and computing encryption device"; `al_ssm` (not bound in this capture — MMIO `[disabled]`) ([lspci](hw-reference/20260816-104601/lspci.txt)) | No mainline driver. Last patch |
| HDD LEDs | LED subsystem + SGPO | DT `annapurna-labs,alpine-sgpo` @ `0xfd8b4000`; `al-sgpo ... probed`, `gpiochip8` 64 lines; `sata_sw_leds` maps 8 bay LEDs ([gpio](hw-reference/20260816-104601/gpio.txt)) | **Custom.** SGPO is RE'd out-of-tree; write a small SGPO gpio/LED driver |
| Clocks/resets | CCF + reset framework | DT has only `fixed-clock` (refclk 100MHz, sbclk 500MHz, nbclk, cpuclk); no clock controller node | Likely **fixed-clocks in DT** suffice; no CCF driver needed unless a block demands gating |
| Thermal/DVFS | thermal/hwmon/cpufreq | DT `annapurna-labs,al-thermal` @ `0xfd860a00`; `al_thermal_probe: Thermal Sensor Loaded`, zone `cpu-thermal` 50°C ([thermal](hw-reference/20260816-104601/thermal.txt)) | **Custom** SoC thermal sensor (al_thermal-standalone exists for modern kernels). cpufreq/DVFS not present in stock DT |
| NAND | (mainline gap) | DT `annapurna-labs,al-nand` @ `0xfa100000`; `nand: Micron MT29F8G08ABBCAH4, 1024MiB SLC, page 4096, OOB 224` ([dmesg](hw-reference/20260816-104601/dmesg.txt)) | **Custom `al_nand`** — no mainline driver. Boot media is USB (`/dev/sdq`), so NAND can stay read-only/last |

## Boot media reality

- Root runs from the **glued-down USB stick**: SanDisk Ultra 58.9GiB → `/dev/sdq`,
  on the ASM1042A xHCI ([blockdev](hw-reference/20260816-104601/blockdev.txt),
  [lsusb](hw-reference/20260816-104601/lsusb.txt)). Overlay: squashfs `ram0`
  (`/mnt/.rofs`) + ext4 `/dev/sdq` (`/mnt/.rwfs`) ([mounts](hw-reference/20260816-104601/mounts.txt)).
- **No SATA drives linked** at capture time — all 8 AHCI ports `SATA link down`
  ([dmesg](hw-reference/20260816-104601/dmesg.txt)). Storage bring-up tests need
  drives fitted.

## Phases

Each phase lists concrete **entry criteria** grounded in the capture. Milestone
order within Phase 1 is the bring-up order.

### Phase 1 — establish Linux 6.18 LTS

Boot 6.18 on the existing (frozen) U-Boot. Use the 6.12 port as a hardware test
reference, not final architecture. Milestones, in order:

1. Boot 6.18 `Image` via existing U-Boot `bootunsign` path; serial console on
   `ttyS0,115200` (uart @ `0xfd883000`).
2. Timers, interrupts (GIC-v3), all **4 CPUs** (PSCI) — mainline dtsi already covers.
3. RO recovery rootfs (initramfs) to iterate without touching stock.
4. SPI-NOR/MTD **read-only** (mainline `spi-dw`+`m25p80`, fixed-partitions).
5. **PCIe** (internal + external0) — gates USB/SATA/eth (all are PCIe EPs).
6. **USB** (xHCI on external-PCIe0) and **SATA** (AHCI on internal PCIe).
7. One Ethernet iface (1G RGMII / AR8031).
8. 10GbE/SFP+.
9. LEDs, fans (adt7475), watchdog (sp805), sensors (al_thermal).
10. RAID/XOR and crypto **last** — working NAS on CPU parity before accel.

Entry criteria met from capture: PSCI/GIC/timer/UART confirmed; MMIO map known
from [live.dts](hw-reference/20260816-104601/live.dts) + [iomem](hw-reference/20260816-104601/iomem.txt).

### Phase 2 — inventory the existing patch set

For every file in the Ubiquiti tree, QNAP tree, Annapurna HAL and the 6.12 port,
classify as one of: already-mainline / mainline-needs-compatible-string /
mainline-needs-Alpine-quirks / shared-third-party-IP-with-register-diffs /
genuinely-Annapurna-specific / obsolete-board-policy-→-DT / not-needed-for-UNVR.

- Compatible strings must describe **actually-compatible** hardware — do not label
  the MAC "Altera"/"NXP" on resemblance.
- Sources to inventory: `UrNVR/unvr-kernel` (4.19.152 GPL), `linux-alpine-v2`
  (6.12), `delroth/al_eth-standalone`, `delroth/alpine_hal`,
  `delroth/linux-qnap-tsx32x`. See [prior-art.md](prior-art.md).
- Live baseline of what's genuinely custom (no mainline driver): `al_eth`,
  `al_dma`, `al_ssm`, `al_nand`, `al_thermal`, `al-sgpo`, internal-PCIe glue.

### Phase 3 — Ethernet ancestry test (BEFORE choosing a driver)

**This is the first engineering task — see [FIRST TASK](#first-task--ethernet-ancestry-report).**

Compare the Annapurna MAC mechanically against mainline Altera TSE/mSGDMA, NXP
DPAA/DPAA2/FMan, Synopsys DesignWare Ethernet, and Annapurna al_eth/al_hal_eth:
register offsets & bit meanings, descriptor formats, ring producer/consumer
semantics, interrupt regs, reset sequence, MDIO regs, MAC stats layout,
checksum/TSO/RSS metadata, PCS/SerDes interface, 1G-RGMII vs 10G-SFP+ paths.

Live constraints the driver must satisfy:
- Binds by **PCI ID** (`1c36:0001`/`0002`), not DT — al_eth said "Unable to find
  compatible OF node". Board params come from EEPROM/board-data today.
- **Shared MDIO** — one bus for both MACs (`al mdio bus: probed` once).
- 1G = RGMII → AR8031 (addr 4); 10G = SFP+ optic, LM mode, no ext PHY.

Outcomes: **A)** register-compatible → extend existing driver via `of_device_id`
match data + per-variant layouts + small hw-op structs, no duplicated datapath;
**B)** shared DMA / different MAC-PCS → extract a common DMA-ring lib, separate MAC
front-ends; **C)** only concepts similar → write a modern al_eth around
phylink/phylib/SFP/page_pool/NAPI/ethtool-netlink/DT topology. Old al_eth's
hardcoded board data, internal SerDes impl and obsolete ethtool are the pieces to
remove. Copying a whole Altera/NXP driver and changing offsets is **worse** than a
clean driver unless the relationship is demonstrated.

### Phase 4 — PCIe

Smallest Alpine-specific area. Use mainline `pcie-designware-host`; keep only
Alpine glue.

Live evidence pins the glue points:
- **Internal PCIe** ECAM `0xfbc00000` (1MB), integrated EPs window `0xfe000000`
  (eth/dma/ahci) — mainline dtsi currently models this as
  `pci-host-ecam-generic`, which is **not** cache-coherent-DMA safe.
- Glue must program: **AXI SMCC snoop** (regs 0x110/0x130/0x150/0x170, bits
  `SNOOP_OVR|SNOOP_ENABLE`), **APP_CONTROL 0x220 = 0x03FF**, **DBI base =
  ctrl_base + 0x10000**, internal-vs-external resources, MSI routing
  (`al,alpine-msix`), reset/clock. Source of these: linux-alpine-v2 RE notes in
  [porting-reference.md](porting-reference.md) §"Hardware gotchas".
- **External PCIe0** @ `0xfd800000`, ECAM `0xfb600000`, `cfg-space-offset 0x10000`,
  `link up Gen2 x1` → carries the ASMedia xHCI.
- Should resemble other `pcie-designware-host` platform drivers, **not**
  Annapurna's old private PCI framework.

### Phase 5 — 10GbE, PCS, SerDes

Do not leave link management in al_eth. Move to: phylink (link state), mainline
SFP cage, `phylink_pcs` for the MAC/PCS boundary, a small Alpine SerDes/PCS
provider only for genuinely proprietary ops, standard module EEPROM handling, DT
props for lane/mode/polarity.

Live evidence:
- DT `serdes` @ `0xfd8c0000`; board-cfg serdes groups: **group3 = 10gbe, lane 0,
  156.25MHz**; port2 `serdes-grp 3 serdes-lane 0`, `10g-serial force-1000base-x`.
- LM mode observed: `AL_ETH_LM_MODE_10G_OPTIC`. Retimer `br410` present-but-disabled
  in DT (`retimer exist="disabled"`), but the UNVR al_eth tree carries
  `al_mod_eth_lm_retimer_ds125.c` — [porting-reference.md](porting-reference.md)
  warns a straight swap to the UDM revision is unsafe for this reason.
- **SFP module EEPROM not captured** — decode it on hardware before finalising.
- Most likely place vendor reuse helps — but only **after** matching PCS/SerDes
  registers.

### Phase 6 — RAID acceleration

Expose the Annapurna RAID engine via **dmaengine**, not a private API. Ops: memcpy,
XOR, PQ gen, PQ validate (if supported), IRQ completion, DMA mapping,
residue/status, runtime PM + reset recovery. MD RAID consumes via async_tx.

- **Array MUST keep working on CPU parity** if the hw driver is absent/fails —
  already true: stock loads `raid456 + async_pq/async_xor/async_raid6_recov`
  ([modules](hw-reference/20260816-104601/modules.txt)).
- NXP RAID/DMA drivers may offer framework/descriptor/test patterns; reuse the
  register backend only if the descriptor engine is genuinely related.

### Phase 7 — Device Tree cleanup

Clean SoC hierarchy: `alpine-v2.dtsi` (extend the existing mainline one), optional
shared Ubiquiti include, `alpine-v2-ubnt-unvr.dts`, separate UDM-Pro and QNAP board
files.

DT describes: MMIO/interrupts, clocks/resets, PHY connections, SFP cages, GPIO
ownership, LEDs/fans, MTD partitions, PCIe topology, SATA ports, fixed regulators.
It must **not** encode driver impl details or values copied blindly from old board
files. Get the **partition map right from the start**:

- Live vendor DT already has the **corrected** NAND map — NO spurious `device_tree`
  partition, and NO `partition@1`: `al_boot@0`, `linux_kernel@2` (0x300000),
  `rootfs@3` (0x1300000), `chike@4` (0x3ff00000). Confirms the persistent
  `fdt rm /soc/nand-flash/partition@1` fix is live. See
  [nand-1.3.35.md](nand-1.3.35.md), [firmware-5.1.25.md](firmware-5.1.25.md).
- SPI-NOR map (mtd0–11) exact in [live.dts](hw-reference/20260816-104601/live.dts)
  and [mtd.txt](hw-reference/20260816-104601/mtd.txt).

### Phase 8 — 6.18 → 7.1

Rebase the series onto 7.1, resolve API changes one subsystem at a time, boot after
each group, compare boot logs / PCI enum / DT state, run identical storage+network
tests. Keep **6.18 = conservative deployment branch**, **7.1 = upstream-dev**.
(A local mainline mirror is already on 7.1.)

## Patch order

Each patch boots, or adds one reviewable function. Tracked as issues (NN):

| # | Patch | Issue | Depends |
|---|---|---|---|
| 1 | ARM64 DT binding for Alpine V2 | 11 | — |
| 2 | Alpine V2 SoC `.dtsi` (extend mainline) | 12 | 11 |
| 3 | UNVR board DT | 13 | 12 |
| 4 | DWC PCIe Alpine quirks | 14 | 12 |
| 5 | Alpine internal PCIe controller | 14 | 4 |
| 6 | clock/reset additions (if required) | 12 | — |
| 7 | Ethernet DMA core / modern al_eth | 15 | 01, 04 |
| 8 | MDIO / phylink integration | 16 | 15 |
| 9 | Alpine PCS/SerDes | 16 | 15 |
| 10 | UNVR Ethernet & SFP+ DT | 17 | 13, 16 |
| 11 | SGPO LED controller | 18 | 12 |
| 12 | RAID dmaengine driver | 19 | 05 |
| 13 | crypto-engine driver | 20 | 05 |
| 14 | defconfig & docs | 21 | all |

## FIRST TASK — Ethernet ancestry report

**Immediate next step, not coding.** Generate register & descriptor comparisons
between al_eth, Annapurna HAL, Altera TSE/mSGDMA, relevant NXP Ethernet/DMA
drivers, and Linux 6.18 implementations. That report decides
extend-vs-extract-vs-clean-frontend for the driver. Without it, "reuse NXP and
Altera" is a hypothesis, not a safe decision. Tracked as **issue 01**.

## Evidence — the live capture

Location: [hw-reference/20260816-104601/](hw-reference/20260816-104601/). The
vendor kernel's own DT decompiled to
[live.dts](hw-reference/20260816-104601/live.dts) (from `live.dtb`, 28672 B).
Kernel: `4.19.152-alpine-unvr`, firmware `UNVR4.al324.v5.1.25`.

Key nodes / facts the port depends on:

- **SoC MMIO** — full address map (bases/sizes/compatibles) in
  [hardware.md](hardware.md#mmio-and-address-map). Port-relevant blocks: GIC-v3, msix,
  DW i2c/spi/uart, PL061 gpio, sp805 wdt, sgpo, serdes, al-nand, al-thermal; custom
  SoC service blocks al-ccu/al-nb-service/al-pbs/alpine-mc.
- **al_eth nodes**: driver binds by PCI ID (`1c36:0001` 1G, `1c36:0002` 10G), MMIO
  `0xfe000000`/`0xfe020000` under the internal-PCIe window. Bare `eth0..eth3`
  platform nodes exist in vendor DT (`0xfc000000`+, irq 61–64) but are **unused**
  by the driver.
- **PCIe controllers**: internal `annapurna-labs,alpine-internal-pcie` ECAM
  `0xfbc00000`; external0 `annapurna-labs,alpine-external-pcie` `0xfd800000` (ECAM
  `0xfb600000`, `cfg-space-offset 0x10000`), external1–3 present but `disabled`.
- **SATA/AHCI**: 2× `1c36:0031` integrated EPs, `ahci` driver, 4 ports each, 6Gbps,
  all links down at capture.
- **DW I2C**: `snps,designware-i2c` @ `0xfd880000` — confirmed DesignWare live.
  Behind it: `pca9546` mux @ 0x71 → RTC s35390a (0x30), adt7475 (0x2e); pca9575
  GPIO expanders @ 0x20/0x21/0x29.
- **GPIO/PCA9575**: three `nxp,pca9575` expanders; SoC PL061 ×6; custom SGPO 64-line
  controller drives the 8 SATA bay LEDs (`sata_sw_leds`).
- **SFP**: SerDes group3 = 10gbe lane0 156.25MHz; port2 `serdes-grp 3`, DAC-length
  3, `force-1000base-x`, retimer `br410` disabled. Module EEPROM not captured.
- **MTD map**: NAND (al_boot/linux_kernel/rootfs/chike, corrected — no
  device_tree/partition@1) + SPI-NOR (u-boot/env/env-redundant/Factory/EEPROM/
  recovery-kernel/config/cksum). Exact offsets in
  [live.dts](hw-reference/20260816-104601/live.dts).
