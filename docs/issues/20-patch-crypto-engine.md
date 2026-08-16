# [Patch 13] Crypto-engine driver (al_ssm)
labels: porting, crypto, patch
---

Code for the Annapurna crypto engine via the Linux crypto-engine framework. LAST
functional driver.
Roadmap: [patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] crypto-engine driver for PCI `1c36:0022` (crypto class).
- [ ] AES-XTS/CBC (vendor priority 400, beats ARM CE 300 per prior perf notes).
- [ ] Async request queue via crypto-engine; DMA mapping; reset recovery.

## Live note
- Device present but its MMIO shows `[disabled]` in this capture and `al_ssm` was not
  bound ([lspci.txt](../hw-reference/20260816-104601/lspci.txt)) — confirm the block
  powers up under our stack.

## Acceptance criteria
- [ ] AES-XTS via kernel crypto API using the engine; graceful fallback to ARM CE.

Depends: #05, #14.
