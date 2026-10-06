---
name: bench-all-benchmark-suite
description: scripts/bench-all.py — A/B regression benchmark suite (eth/crypto/SATA/USB) with JSON snapshots and --compare; use this instead of ad-hoc iperf3/hdparm runs
metadata: 
  node_type: memory
  type: reference
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-29T04:47:35.927Z
---

`./scripts/bench-all.py` (added 2026-08-29) is the quick before/after regression
check for woomera: bidirectional iperf3 on both NICs (SO_BINDTODEVICE-bound,
per #121's lesson about a plain `-B` bind silently picking the wrong NIC),
AES-256-XTS crypto throughput via AF_ALG (generic `xts(aes)`
name only — binding by specific driver name like `xts-aes-al-ssm` is NOT
supported by this kernel's algif_skcipher, fails ENOENT), hdparm `-t --direct`
on every SATA disk, and the USB-attached SSD.

Usage: `./scripts/bench-all.py --out tmp/before.json`, make a change, `--out
tmp/after.json`, then `./scripts/bench-all.py --compare tmp/before.json
tmp/after.json` — flags any metric that dropped ≥10%.

**Deliberately excludes al_dma/dmatest** — see [[al-dma-udma-state-survives-reload]]
for why (its hardware state isn't safely re-runnable without a reboot between
runs, unlike everything else this suite covers).

AF_ALG gotcha worth remembering if extending this: `socket.settimeout()` on
the AF_ALG *parent* socket breaks `accept()` after ~128 calls (hangs) —
confirmed live, only set a timeout on the per-op socket returned by
`accept()`, never on the parent.
