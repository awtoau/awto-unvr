# [Patch 12] RAID dmaengine driver (al_dma)
labels: porting, raid, dmaengine, patch
---

Code for Phase 6: the Annapurna RAID/XOR engine as a dmaengine provider.
Roadmap: [Phase 6](../porting-roadmap.md#phase-6--raid-acceleration),
[patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] dmaengine driver for PCI `1c36:0022` (RAID class).
- [ ] Capabilities: memcpy, XOR, PQ gen, PQ validate (if supported).
- [ ] IRQ completion, DMA mapping, residue/status, runtime PM + reset recovery.
- [ ] Wire MD async_tx offload.

## Hard requirement
- [ ] CPU-parity fallback preserved (stock already runs `raid456` + async_* —
  [modules.txt](../hw-reference/20260816-104601/modules.txt)). Absent/failed driver
  must not break arrays.

## Acceptance criteria
- [ ] MD offloads parity to the engine; forced-disable falls back to CPU cleanly.

Depends: #05, #14.
