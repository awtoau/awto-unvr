<!-- SPDX-License-Identifier: GPL-2.0-or-later -->
<!-- Copyright (C) 2026 Awto / Daniel Tyrrell -->
# serdes-tune

SFP+ decode, link-quality counters and TX-equalisation sweeps for woomera's 10G
port. Single binary, four subcommands, no external crates.

Physics and prior art: `docs/serdes-equalisation.md`. Earlier Python version of
the sweep: `scripts/serdes-tx-sweep.py` (kept; this supersedes its metrics).

## Where it runs — decision

**Runs on the dev host, drives the box over SSH.** `--local` runs the identical
code paths on the box itself.

- Consistent with `scripts/` — everything else here drives the box remotely.
- No cross-toolchain needed to use it; `cargo build` on the dev host is enough.
- The iperf3 *server* is on the dev host anyway, so the sweep needs a dev-host
  process regardless.
- SSH is also the box-absent detector: a missing box is one loud error, not a
  hang or an empty counter set.
- `--local` exists because the same binary cross-builds for the box when direct
  sysfs/i2c access is wanted:
  `cargo build --release --target aarch64-unknown-linux-musl`.

## Where the crate lives — decision

`tools/serdes-tune/`, beside `tools/uboot-diag/`.

- `scripts/` is Python by project rule; `tools/` is where compiled tools live.
- Zero dependencies, same rationale as `uboot-diag`: cross-builds offline, no
  registry access, no version drift. Arg parsing is ~110 lines (`src/args.rs`).

## Subcommands

    serdes-tune env       what is running on the box (print this with any result)
    serdes-tune sfp       full SFF-8472 decode, A0h + A2h
    serdes-tune counters  pcs_* / ec_* / generic PHY counters, with diffing
    serdes-tune params    show or set one serdes_tx_* knob
    serdes-tune sweep     sweep one knob and score every setting

Global: `--host <addr>` `--local` `--password <pw>` `--iface <name>` `--verbose`.
Host resolution order: `--host`, `$WOOMERA_HOST`, `scripts/ssh-woomera.py
--print`. Interface default `enp0s2`; everything else (PCI address, sysfs dir)
is resolved from it, never hardcoded.

| subcommand | options |
|---|---|
| `env` | — (exit 4 if an expected counter is missing) |
| `sfp` | `--via auto\|ethtool\|i2c`, `--i2c-bus <n>` (default 2), `--hex` (exit 3 on checksum mismatch) |
| `counters` | `--json`, `--save <file>`, `--diff <file>`, `--diff <a> --to <b>`, `--window <secs>` |
| `params` | `--set <param>=<value>` (decimal in, hex on the wire), `--no-bounce` |
| `sweep` | `--param`, `--values`, `--metric`, `--duration`, `--repeat`, `--streams`, `--server`, `--port`, `--udp-mbit`, `--no-load`, `--csv <file>`, `--no-restore`, `--dry-run`, `--allow-control-path` |

## Scoring metrics

| `--metric` | scores | resolution |
|---|---|---|
| `pcs` (default) | delta in `pcs_errored_blocks` + `pcs_ber_events` | best available locally; blind once the link is clean |
| `ec` | delta in the 15 `ec_*` drop counters | coarser — frames dropped, not bit errors |
| `udp-loss` | iperf3 UDP at a fixed offered rate, loss % | works through a store-and-forward switch, which discards bad-CRC frames |
| `throughput` | iperf3 TCP Mbit/s | **saturates**; finds window edges only, cannot rank inside them |

All four record pcs and ec deltas per point regardless of which one scores, so
one run yields every counter's view.

## What the tool refuses to do

- **Score on a counter that does not exist.** `--metric pcs`/`ec` abort if the
  counters are absent, naming them, and never fall back to another metric — an
  absent counter summed as zero reads as a perfect link. Kernel/module mismatch
  is this project's most expensive recurring bug (#105 #131 #161).
- **Treat absent as zero anywhere.** Every counter accessor returns `Option`;
  `ABSENT` prints as `ABSENT`, and a counter present in only one of two samples
  gets no delta plus a warning that the module changed underneath.
- **Report a winner when the metric read zero everywhere.** It says so instead,
  with the dwell arithmetic: at 10.3125 Gb/s, 100 errors take ~10 s at BER 1e-9
  and ~2.7 h at 1e-12.
- **Bounce the link the SSH session rides on.** Checked via the box's route to
  `$SSH_CLIENT`; override with `--allow-control-path`.
- **Drive a lane with no main cursor.** `main = total_driver_units −
  (c_minus_1 + c_plus_1 + c_plus_2)` is computed and shown for every point;
  points with `main <= 0` are skipped, not written.

Every sweep prints kernel release, module name/srcversion/version, resolved
interface + PCI address, and the present/missing counter inventory first — a
result without provenance is not reproducible.

## Facts built in

- sysfs values are **written as hex** (`kstrtoul(buf, 16, …)`) and **read back
  as decimal** — writing `10` sets 16. Every write is verified.
- A write only sets `tx_param_dirty`; `al_eth_serdes_static_tx_params_set()`
  runs from phylink `pcs_config`, so **each change needs a link bounce**. The
  bounce polls carrier and gives up loudly rather than waiting blind.
- `total_driver_units` is a **shared budget**, not an independent knob.
- Over-emphasis produces an **error floor, not a plateau** (802.3dj), so a
  winner at either edge of the swept range is flagged: extend the sweep.
- PCS counters are clear-on-read at MDIO 3.33 and saturate per poll (255 / 63) —
  a large delta is a lower bound. Do not read MDIO 3.33 directly from userspace;
  it races the driver's 1 Hz poll and both readers get partial counts.

## Build and test

    cargo build --offline
    cargo clippy --offline --all-targets
    cargo test --offline                     # 24 tests, no hardware needed

Cross-build for the box:

    cargo build --release --target aarch64-unknown-linux-musl

## Running it

Counters and provenance (no traffic, no writes):

    ./target/debug/serdes-tune env
    ./target/debug/serdes-tune counters
    ./target/debug/serdes-tune counters --save tmp/before.json
    ./target/debug/serdes-tune counters --diff tmp/before.json
    ./target/debug/serdes-tune counters --window 30

Module:

    ./target/debug/serdes-tune sfp
    ./target/debug/serdes-tune sfp --hex --via i2c --i2c-bus 2

Knobs:

    ./target/debug/serdes-tune params
    ./target/debug/serdes-tune params --set c_plus_1=5

Sweeps — start the server on the dev host first: `iperf3 -s -p 5701`

    ./target/debug/serdes-tune sweep --param c_plus_1 --values 0,1,2,3,4,5,6,7 --dry-run
    ./target/debug/serdes-tune sweep --param c_plus_1 --values 0,1,2,3,4,5,6,7 \
        --metric pcs --duration 20 --csv tmp/c_plus_1-pcs.csv
    ./target/debug/serdes-tune sweep --param amp --values 1,2,3,4,5,6,7 \
        --metric udp-loss --udp-mbit 2000 --duration 10
    ./target/debug/serdes-tune sweep --param c_plus_1 --values 4,5,6 \
        --metric pcs --no-load --duration 60      # score live traffic, no iperf3

On the box itself (cross-built binary), an explicit server is required:

    ./serdes-tune sweep --local --server <dev-host-ip> --param c_plus_1 --values 4,5,6

## Not verified without hardware

Everything that needs the box. Verified here: SFF-8472 decode against a
synthetic module and its checksums, `ethtool -S` parsing, iperf3 TCP/UDP output
parsing, snapshot round-trip, main-cursor arithmetic, arg handling, absent-vs-
zero behaviour, and the whole `--local` path against the dev host's own NIC
(env/counters/params/sweep error paths, and the SSH failure path against an
unreachable address). Not verified: reading real `serdes_tx_*` values, the link
bounce, `ethtool -m` on a module that supports it, `i2cdump` output shape from
this box, and every end-to-end sweep.
