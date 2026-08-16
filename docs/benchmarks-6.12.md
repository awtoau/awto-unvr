# Storage benchmarks — UNVR on our Linux 6.12 (ea16)

Setup: netbooted 6.12.103 (patched: internal-PCIe + AHCI), minimal initramfs, `dd` only
(no hdparm/fio). Sequential, 2 GiB, `bs=1M`; reads with `drop_caches` first, writes with
`conv=fsync`. Not random-IOPS, not concurrent-multi-drive — a first sanity pass.

Drives: `sdb`/`sdc` = WDC WD82PURZ-85T (WD Purple 8 TB, 5640 RPM CMR), SATA link **6.0 Gbps**;
`sda` = SanDisk Ultra 64 GB (USB 3.0 SuperSpeed).

## Results vs theoretical

| Device | Test | Measured | Theoretical / rated | Verdict |
|---|---|---|---|---|
| WD82PURZ `sdc` (raw) | seq read | **254.9 MB/s** | platter ~180–215; SATA-6G link ~600 | **at/above platter rating** — link only ~42% used, not the limit |
| WD82PURZ `sdb` (raw) | seq read | **232.0 MB/s** | same | at platter rating |
| WD82PURZ `sdb` (ext4 file) | seq write | **216.7 MB/s** | ~platter | full platter; ext4 overhead ~small |
| WD82PURZ `sdb` (ext4 file) | seq read | **220.2 MB/s** | ~platter | ~5% under raw = normal fs overhead |
| SanDisk Ultra `sda` (USB3) | seq read | **95.0 MB/s** | flash ~130 rated; USB-SS link ~500 | flash-limited (~73% of rated); USB link far from limit |

## Analysis — does it match theory?
- **WD drives: yes, at the platter ceiling.** 232–255 MB/s raw is at/above the WD82PURZ's ~180–215 MB/s sustained rating. The SATA-6G link (~600 MB/s) and the internal-PCIe→AHCI path (with the AXI-snoop patch) have **headroom to spare** — storage is bound by the spinning platter, not by our port. i.e. the new driver stack delivers full drive bandwidth.
- **ext4 overhead ~5%** (220 vs 232 read) — expected, healthy.
- **USB: flash-limited, not link-limited.** 95 MB/s vs the SuperSpeed link's ~500 MB/s ceiling — the SanDisk's flash/controller is the limit; ~73% of its ~130 MB/s rating (a fair result for a cache-class stick).

## Not yet measured (follow-ups)
- Random IOPS / mixed R-W (needs `fio` in the initramfs).
- **Concurrent 2-drive** aggregate — checks the internal-PCIe fabric ceiling (2×~250 ≈ 490 MB/s).
- Network throughput (1 G RJ45 / 10 G SFP) — needs an iperf peer.
- CPU at stock 1.7 GHz vs the overclock target (#29).

Bottom line: on our own 6.12 the storage path performs to the **drives' physical limit** — the port introduces no bottleneck.
