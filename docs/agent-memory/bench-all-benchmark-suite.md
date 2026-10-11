---
name: bench-all-benchmark-suite
description: scripts/bench-all.py is THE test harness for woomera (health/eth/crypto/disk/usb/dma, opt-in ssm/stress, --idle errors-only loop); extend it, never add another test script
metadata: 
  node_type: memory
  type: reference
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-10-11T00:00:00.000Z
---

- `./scripts/bench-all.py` is the one harness. New check = new `bench_*` subtest taking a `Report`; metrics named in `report.metrics` are picked up by `--compare` and `--idle` automatically.
- Its docstring is the maintained reference: subtests, baselines (date + kernel), tolerances, idle rules, and which scripts are deliberately standalone.
- `--idle`: unattended loop, terminal gets findings only (`FAIL`/`REGRESSION`/`RECOVERED`/`STARVED`/`WEDGED`); detail in `tmp/logs/bench-all.log`. Baseline `bench-history/baseline.json`; re-baseline with an attended `--save-baseline`.
- Folded in 2026-10-11 and retired to `debris/scripts/`: al-dma-verify, test-eth, al-ssm-stress, mem-stress-woomera, hwverify-woomera.
- dma was excluded until 2026-10-11 on the belief that UDMA state survives a reload as hardware ([[al-dma-udma-state-survives-reload]]); that was al_dma not resetting its queues (#289). Still never `modprobe -r al_dma`; `--dma-reload` is opt-in.
- AF_ALG gotcha: `socket.settimeout()` on the AF_ALG parent socket breaks `accept()` after ~128 calls; only time out the per-op socket. AF_ALG reaches CE, never al_ssm (#267): crypto numbers are CE numbers.
