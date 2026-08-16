# [Phase 1] Establish Linux 6.18 LTS on UNVR
labels: porting, phase-1, bringup
---

Boot mainline 6.18 on the existing (frozen) U-Boot. Use the 6.12 port as a hardware
test reference, not final architecture.
Roadmap: [Phase 1](../porting-roadmap.md#phase-1--establish-linux-618-lts).

## Milestones (in order)
- [ ] Boot 6.18 `Image` via U-Boot `bootunsign`; serial console `ttyS0,115200`
  (uart @ 0xfd883000).
- [ ] Timers, GIC-v3 interrupts, all 4 CPUs via PSCI (mainline `alpine-v2.dtsi`
  already covers).
- [ ] RO recovery rootfs (initramfs) to iterate without touching stock.
- [ ] SPI-NOR / MTD **read-only** (`spi-dw` + `m25p80` + fixed-partitions).
- [ ] PCIe internal + external0 up (#05) — gates USB/SATA/eth.
- [ ] USB (xHCI on ext-PCIe0) and SATA (AHCI on internal PCIe).
- [ ] One Ethernet iface (1G RGMII / AR8031) (#15).
- [ ] 10GbE / SFP+ (#16).
- [ ] LEDs, fans (adt7475), watchdog (sp805), sensors (al_thermal).
- [ ] RAID/XOR (#19) and crypto (#20) **last**.

## Entry criteria (met from capture)
- PSCI/GIC/timer/UART confirmed live; MMIO map known from
  [live.dts](../hw-reference/20260816-104601/live.dts) +
  [iomem.txt](../hw-reference/20260816-104601/iomem.txt).
- Boot media is the USB stick (`/dev/sdq`), so NAND can stay RO/last.

## Acceptance criteria
- [ ] Box boots mainline 6.18 to a shell over serial, off the USB stick.
- [ ] Working NAS (network + SATA storage) before RAID/crypto acceleration.

Depends: #05, #14. Feeds: #21.
