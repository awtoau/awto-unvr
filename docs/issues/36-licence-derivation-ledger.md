# [Upstream] Clean-source / derivation ledger + submission (Stage 14)
labels: porting, upstream, licensing
---

Stage 14. Per-file provenance ledger so the series is upstream-submittable, plus the
submission order. Complements the meta tracker (live #4) and defconfig+docs (live #25).
Roadmap: [Stage 14 row](../porting-roadmap.md#14-stage-plan-reconciliation).

## Per-file ledger (every new/modified file)
- [ ] Source(s) consulted; copied-vs-written; licence; copyright holder;
  transformations applied.
- [ ] **Decompiled firmware = behavioural research only** — never copied as source.
- [ ] SPDX headers + `Signed-off-by` on every commit; `checkpatch` clean.

## Submission order (least-controversial first)
- [ ] DT bindings → SoC/board DT → PCIe → small drivers (SGPO, reset) → ethernet →
  DMA/RAID → crypto.
- [ ] Each patch: hardware description, rationale, derivation note, test hardware +
  procedure + results, build + DT-schema check.

## Acceptance criteria
- [ ] Ledger covers 100% of new files; no unexplained copied code.
- [ ] First binding/DT slice passes checkpatch + `dt_binding_check` and is ready to post.

Relates: meta (live #4), rebase (live #13), defconfig+docs (live #25).
