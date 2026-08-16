# [Phase 4] PCIe — mainline DesignWare host + small Alpine glue
labels: porting, pcie, phase-4
---

Smallest Alpine-specific area, but **gates everything** — eth, DMA, AHCI, crypto and
USB are all PCIe endpoints. Use mainline `pcie-designware-host`; keep only Alpine
glue.
Roadmap: [Phase 4](../porting-roadmap.md#phase-4--pcie).

## Glue points (from live capture + RE notes)
- **Internal PCIe** ECAM `0xfbc00000`, integrated-EP window `0xfe000000`
  (eth/dma/ahci/crypto). Mainline models it as `pci-host-ecam-generic` — **not
  cache-coherent-DMA safe**.
- **AXI SMCC snoop**: regs 0x110/0x130/0x150/0x170, bits `SNOOP_OVR|SNOOP_ENABLE`.
- **APP_CONTROL 0x220 = 0x03FF** or DMA does not work.
- **DBI base = controller_base + 0x10000** (not controller_base).
- **External PCIe0** @ `0xfd800000`, ECAM `0xfb600000`, `cfg-space-offset 0x10000`,
  observed `link up Gen2 x1` → carries ASMedia xHCI.
- RE source: [docs/porting-reference.md](../porting-reference.md) §"Hardware gotchas".
- Evidence: [live.dts](../hw-reference/20260816-104601/live.dts),
  [iomem.txt](../hw-reference/20260816-104601/iomem.txt),
  [dmesg.txt](../hw-reference/20260816-104601/dmesg.txt).

## Tasks
- [ ] Get mainline `pcie-al.c` building for 6.18; assess DBI-offset handling.
- [ ] Internal-controller support with AXI snoop + APP_CONTROL (#14).
- [ ] Confirm MSI routing via `al,alpine-msix` (base SPI 0xa1, count 0x9e live).
- [ ] Verify coherent DMA to al_eth/al_dma once eth driver exists.

## Acceptance criteria
- [ ] Internal + external0 PCIe enumerate under 6.18; xHCI + AHCI probe.
- [ ] Resembles other `pcie-designware-host` platform drivers, not the old private
  Annapurna PCI framework.

Blocks: #02, #14. See also #10.
