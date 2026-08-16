# [Phase 6] RAID acceleration via dmaengine (not a private API)
labels: porting, raid, dmaengine, phase-6
---

Expose the Annapurna RAID engine through **dmaengine**; MD RAID consumes via
async_tx.
Roadmap: [Phase 6](../porting-roadmap.md#phase-6--raid-acceleration).

## Ops to expose
- [ ] memcpy
- [ ] XOR
- [ ] PQ gen
- [ ] PQ validate (if supported)
- [ ] IRQ completion
- [ ] proper DMA mapping
- [ ] residue/status
- [ ] runtime PM + reset recovery

## Hard requirement
- [ ] Array MUST keep working on **CPU parity** if the hw driver is absent/fails.
  Already true live — stock loads `raid456` + `async_pq/async_xor/async_raid6_recov`
  ([modules.txt](../hw-reference/20260816-104601/modules.txt)). So this is genuinely
  the LAST bring-up item.

## Notes
- Device: PCI `1c36:0022` (RAID class), vendor driver `al_dma`
  ([lspci.txt](../hw-reference/20260816-104601/lspci.txt)).
- NXP RAID/DMA drivers may offer framework/descriptor/test patterns; reuse the
  register backend only if the descriptor engine is genuinely related.

## Acceptance criteria
- [ ] dmaengine driver registers memcpy/XOR/PQ; MD offloads parity to it.
- [ ] Removing/failing the driver falls back to CPU parity with no data loss.

Depends: #05.
