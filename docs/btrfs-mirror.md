# Btrfs RAID1 mirror on woomera (#372)

- Measured 2026-11-17, Fedora, kernel 7.4.0-rc2-perf-9b19c94+, 4x A57 (CE aes/pmull/sha2/crc32), 3.8G RAM.
- Disks: 2x WD82PURZ (serials VGG8B0LG, VGG8ASZG) on SATA; whole-disk btrfs, no partitions.
- `-d raid1 -m raid1`, default nodesize, `noatime`; mounted at `/srv/mirror` via fstab by UUID (docs/fedora-shares.md).
- Previous tank ZFS pool on these disks is gone (Dan's decision; rebuild later).
- Decisions + raw data: #372, `bench-history/bench-20261117-*.json`, harness `scripts/bench-all.py`.

## Harness
- `--btrfs-mirror`: fio on the mounted fs; SKIP line if nothing mounted.
- `--btrfs-config CSUM[:mountopts] --btrfs-serials A B --i-accept-wipe`: the only wipe path.
  - Refuses protected serials, non-WD82PURZ, anything mounted; zeroes first/last 4 MiB of each partition + disk, wipefs, mkfs, mount, bench.
  - Repeatable; `--btrfs-buffered` for compression (btrfs O_DIRECT never compresses; fio default fallocate makes prealloc extents, also never compressed: harness uses `--fallocate=none` when buffered).
- `--btrfs-raw SERIAL --i-accept-wipe`: single-disk ceiling (writes first 8G of the raw disk).
- `--hash`: AF_ALG hash MB/s + btrfs on two zram devices per csum (no disk).
- Jobs: 8G file (> 2x RAM), `direct=1`, incompressible data: seq 1M QD4 write/read, 4k QD16 randread/randwrite; reads after `drop_caches`; read jobs 30 s timed. CPU % = whole box from /proc/stat.
- No loop device in this kernel (`/dev/loop*` absent): RAM block devices are zram.

## Single disk (raw, `direct=1`, first 8G)
| disk | seq write 1M | seq read 1M | randread 4k QD16 | randwrite 4k QD16 |
|---|---|---|---|---|
| VGG8B0LG | 248.8 MB/s | 254.7 MB/s | 271 iops | 758 iops |
| VGG8ASZG | 272.2 MB/s | 273.9 MB/s | 279 iops | 909 iops |
- CPU 1-2 % throughout.

## Btrfs RAID1 per checksum (one run each, incompressible, direct)
| csum | seq write MB/s (cpu%) | seq read MB/s (cpu%) | randread iops (cpu%) | randwrite iops (cpu%) |
|---|---|---|---|---|
| crc32c | 253.7 (7) | 271.0 (2) | 326 (1) | 16314 (55) |
| xxhash | 252.2 (7) | 271.0 (2) | 329 (1) | 16592 (55) |
| sha256 | 251.1 (13) | 268.2 (9) | 324 (1) | 15443 (53) |
| blake2b | 207.8 (26) | 252.8 (24) | 311 (1) | 13325 (49) |
| crc32c, `nodatasum` | 254.1 (3) | 257.2 (1) | 328 (1) | 17717 (55) |
- Mirror reads ~= one disk (no read striping across the pair at QD4 sequential): 271 vs 255-274 single.
- Seq write ~= one disk (both mirrors written in parallel).
- Randwrite iops are ~20x the raw disk's (writes land in fio-preallocated extents and are coalesced); not comparable to the raw 758-909 iops.
- Repeat of crc32c later (final mirror, same jobs): 251.5 / 254.6 MB/s, 181 randread iops, 13043 randwrite iops. Run-to-run spread on randread is ~2x; treat sub-10 % differences as noise.

## Checksum cost without the disk (btrfs on 2x zram, 900 MiB file, 4 loops, direct, 2026-11-17)
| csum | seq write MB/s (cpu%) | seq read MB/s (cpu%) |
|---|---|---|
| crc32c | 195.8 (27) | 1482 (47) |
| xxhash | 197.1 (26) | 1475 (43) |
| sha256 | 165.2 (26) | 1374 (68) |
| blake2b | 117.0 (26) | 796 (88) |
- Write is zram-bound (compress attempt on random data), not checksum-bound: read is the checksum signal.
- Reads verify every block: sha256 costs 7 % of RAM-speed read and +21 CPU points; blake2b costs 46 % and +41 points.

## Raw hash speed (AF_ALG, 2026-11-17)
| alg | driver (prio) | 128 KiB chunks | 4 KiB per-call digest |
|---|---|---|---|
| sha256 | sha256-lib (300) | 677 MB/s | 346 MB/s |
| crc32c | crc32c-lib (100) | n/a | n/a |
| xxhash64 | xxhash64-generic (100) | n/a | n/a |
| blake2b-256 | blake2b-256-lib (300) | n/a | n/a |
- n/a: AF_ALG `bind` returns ENOENT for these (not exposed to userspace); in-kernel cost is the zram table above.
- sha256 on arm64 is the `lib` implementation: it dispatches to CE at boot. `sha256-ce`/`-neon`/`-generic` are not separate drivers and AF_ALG binds by generic name only: no runtime on/off.
- 4 KiB column is dominated by syscall overhead.

## Compression (crc32c, buffered fio, `--fallocate=none`, `compress=zstd:1`)
| data | compress | seq write MB/s (cpu%) | seq read MB/s (cpu%) | randread iops | randwrite iops |
|---|---|---|---|---|---|
| incompressible | off | 241.7 (9) | 270.4 (7) | 173 | 10496 |
| incompressible | zstd:1 | 243.9 (9) | 267.0 (7) | 172 | 10304 |
| zero-filled | off | 246.7 (5) | 257.2 (7) | 170 | 10276 |
| zero-filled | zstd:1 | 1079.4 (78) | 1153.0 (68) | 237 | 6308 |
- Incompressible: btrfs's heuristic skips compression, no cost.
- Zero-filled is the upper bound (32x ratio seen with dd); real data lands between.
- Direct IO and fio's default prealloc both bypass compression: first attempts showed zstd with no effect for that reason.

## Hardware on/off
| engine | applies to btrfs RAID1? | on | off | status |
|---|---|---|---|---|
| CE aes (xts-aes-ce) | no (no encryption) | AF_ALG xts(aes): 534.7 MB/s | not selectable via AF_ALG (al_ssm masked) | measured, bench-all `crypto` |
| CE sha2 / pmull / crc32 | yes (csum) | see zram table | cannot be disabled at runtime | on only |
| NEON raid6 | no (no parity) | raid6 `neonx8` gen() 5151 MB/s, recovery neon | int64x8 2384 MB/s | boot-time dmesg selection |
| xor | no | `8regs` 15593 MB/s selected | n/a | boot dmesg |
| al_dma (copy/xor) | no | dmatest dma0chan0 20000 it: copy 65769 iops, xor 62080 iops, 0 fail | engine not in the btrfs path | isolated, bench-all `dma` |
| al_ssm RAID/XOR | no (RAID1 has no parity) | not exercised: md raid5/6 async_tx not tested (not trivial here) | | n/a |
| dm-crypt (any impl) | n/a | skipped per Dan | | |
- Baseline snapshot of every bench-all section: `bench-history/bench-20261117-134843.json`.
  - 1G port `enp0s1` has no carrier: `health` + `eth 1G` FAIL (no cable, not a regression).
  - dma iops 65769/62080 vs the 2026-10-11 baseline 13716/12743: kernel/CPU state differs, not re-baselined.

## Answer: sha256 vs crc32c on this box
- On the two HDDs: no throughput difference (251-268 vs 254-271 MB/s, within run noise); cost is CPU only, +6 points write, +7 points read.
- At RAM speed reads: -7 % and +21 CPU points.
- The disks (~255-275 MB/s each) are the limit, not the hash: sha256 holds ~1.37 GB/s on 4 cores.
- blake2b is the only variant that slows disk writes (-18 %).

## Final configuration
- crc32c, raid1/raid1, no compression, no `nodatasum` (lowest CPU at disk speed, default).
- sha256 is a free upgrade for disk-bound loads if stronger checksums are wanted; remake with `--btrfs-config sha256`.

## Never wipe the mirror
- `/srv/mirror` (sda+sdb) holds the music consolidation data (awto-music#22); no mkfs/wipefs/`--i-accept-wipe` on it.
- Wipe path above is for benchmark disks only, by serial; the mirror serials are protected.
- Dedupe: bees owns the mirror (#372); duperemove timer covers `single` only.
