# Woomera MTD flash snapshots — index

**Canonical flash/partition map: [`docs/mtd.md`](../../docs/mtd.md)** — both chips
(NOR + NAND), all 12 partitions, offsets, sizes, what's raw vs filesystem. Read
that first; this dir is the per-snapshot evidence index.

- Binary dumps **relocated out of the repo on 2026-08-17** to `/mnt/2tb/git_debris/woomera-mtd/`
  (real hexdumps: `xxd` the `mtdNN-*.img` under the matching subdir there).
  - Per-snapshot subdir structure mirrored identically at that path.
  - Reason: ~1.5 GB non-regenerable evidence, too big for the git tree.
- **Per-snapshot `README.md` manifests remain here** — they are the durable, tracked index.
- Dumps are **non-regenerable**: actual flash state captured pre/post each firmware step
  (1.4.9 → 2.3.14 → 3.1.16 → 4.1.22 → 5.1.25). Cannot be re-acquired once the unit was upgraded.
- Move was verified: sha256 of every file recomputed at destination == source before source deletion.
  - Destination manifest: `/mnt/2tb/git_debris/woomera-mtd/SHA256SUMS.txt` (110 files, 1,509,949,480 bytes).
  - Move script: `scripts/move-woomera-mtd.py`; log: `tmp/logs/move-woomera-mtd.log`.
- To read a dump for a snapshot, look under the matching subdir name at the git_debris path.
