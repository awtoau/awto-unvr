# [Method] Driver-ancestry register database + structural-compare tooling
labels: porting, phase-2, analysis, tooling
---

Stage 2 of the plan is broader than the ethernet-only ancestry (live #5): build a
**normalised register database across all custom blocks** and compare STRUCTURE, not
names, before any "reuse driver X" decision.
Roadmap: [Stage 2 row](../porting-roadmap.md#14-stage-plan-reconciliation),
[Phase 2](../porting-roadmap.md#phase-2--inventory-the-existing-patch-set).

## Deliverable
- [ ] Script in `scripts/` builds a register DB (output `tmp/`), logs to `tmp/logs/`.
- [ ] Rows normalised as: `(block, reg, offset, width, field, access, source, gen)`.
- [ ] Ancestry report per custom block (finding doc, not chat).

## Sources to ingest
- Annapurna HAL (`delroth/alpine_hal`), UBNT kernel 4.19.152 (`urnvr-kernel`),
  UBNT u-boot, `al_eth`/`al_hal_eth`, `al_dma`, `al_ssm`, `al_nand`, `al_thermal`,
  al-sgpo; internal-PCIe glue.
- Candidate mainline ancestors: Altera, NXP, Synopsys DesignWare, ARM PrimeCell.

## Compare STRUCTURE
- [ ] Register offsets and gaps
- [ ] Reset values
- [ ] Bitfield layouts
- [ ] Descriptor layout
- [ ] Ring producer/consumer semantics

## Decision gate
- [ ] No existing mainline driver gets Alpine support added until register
  compatibility is **proven** by this DB — resemblance is not evidence.

## Relation to existing issues
- Feeds the ethernet ancestry report (live #5) and the patch inventory (live #7):
  #5/#7 consume this DB for the MAC/DMA blocks; this issue is the general engine.

## Acceptance criteria
- [ ] DB regenerates from source trees with one script.
- [ ] Per-block ancestry verdict: mainline-compatible / shared-IP-with-diffs /
  genuinely-custom, each with the offset/bitfield evidence.
