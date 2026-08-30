# Network throughput testing methodology

The box has **two DHCP-leased IPs, one per NIC** (`enp0s1` 1G RJ45, `enp0s2` 10G
SFP+). Every throughput number in this project's history that turned out to be
wrong was wrong because of this - not a driver bug. Read this before trusting
any iperf3 result, box-side or test-machine-side.

## The trap, in two parts

1. **Wrong port on the box.** "Connect to the box's IP" silently means "connect
   to whichever port answered host discovery" - not necessarily the 10G one
   you meant to test. `enp0s1` and `enp0s2` are different addresses.
2. **Wrong NIC on the test machine.** Even once you connect to the *correct*
   box IP, if the machine running iperf3 has more than one NIC on the same
   subnet as the box, Linux route selection is **destination-driven** - it
   picks whichever local NIC's route table entry matches, independent of
   which NIC you intended. Binding the iperf3 client's source address
   (`-B`) does **not** control this - it's a common, wrong assumption.
   Verify with `ip route get <box IP>` before trusting a result; if it picks
   the wrong local NIC, add a temporary host route to force it:
   `sudo ip route add <box's 10G IP>/32 dev <your 10G nic>` (remove after).

Real incident, from #121: a "10G" result of 939/218 Mbit/s turned out to
still be entirely a 1G-class measurement, on the *second* attempt at fixing
it - the first fix (binding source address) didn't actually address the
route-selection problem at all.

## Use `scripts/test-eth.py`, don't hand-roll `iperf3` commands

`scripts/test-eth.py` implements this correctly: it queries `enp0s2`'s own
live IP over SSH (not host-discovery's address) and reports which local
interface+speed `ip route get` will actually use for the test, so a
wrong-NIC result is flagged instead of silently mislabeled. Its own
docstring is the maintained, authoritative explanation of this - this doc
summarizes it, refer to the script for detail as it evolves.

```
./scripts/test-eth.py                    # full run, ~30s iperf3 each way
./scripts/test-eth.py --skip-iperf        # quick check, no throughput test
./scripts/test-eth.py --iperf-duration 60 # longer throughput test
```

It still cannot force your *own* machine's egress interface for you (that
needs the temporary host route above) - it only reports what route
selection will actually do, so you can catch a wrong-NIC run before trusting
the number.

## A third trap: the box itself can pick the wrong egress NIC (#170)

Found 2026-08-30, box-side, not test-tooling: the box is multi-homed on the
same `/24` (`enp0s1`/.149 metric 101, `enp0s2`/.133 metric 100). Its own
routing table picks `enp0s2` for *any* destination in that subnet regardless
of which interface's source IP the connection actually uses -
`ip route get 192.168.25.117 from 192.168.25.149` returns `dev enp0s2`, not
`enp0s1`. A connection accepted on `enp0s1` has its reply traffic silently
egress via `enp0s2` instead. A `tcpdump` capture taken on the "wrong" (not
actually used) interface during such a test shows zero packets in the tested
direction - looks like total loss, is actually a capture on the wrong NIC.
To genuinely test `enp0s1` specifically, pin the source with a policy route
first: `ip rule add from <box 1G IP> table 101; ip route add default via
<gw> dev enp0s1 table 101; ip route add <subnet> dev enp0s1 table 101 src
<box 1G IP>` - remove afterward (`ip rule del ...`; a reboot also clears it).

Use `scripts/eth-tx-capture-diag.py` for this now (see below) - it checks
and warns about this automatically before running.

## Current known state (2026-08-30 - root cause found and partially fixed)

With the methodology above correctly applied, isolated (non-`--bidir`)
unidirectional tests, clean interface routing confirmed:

| Link | Direction | Throughput | Retransmits |
|---|---|---|---|
| 1G | host→box (RX) | 945 Mbit/s | 0 |
| 1G | box→host (TX) | 41.9 Mbit/s | 3,301 |
| 10G | host→box (RX) | 9.19-9.41 Gbit/s | low |
| 10G | box→host (TX) | 19.8 Mbit/s | 1,870 |

**Root cause #1, found and fixed**: `al_eth_select_queue()`
(`modules/al_eth/al_eth_main.c`) picked the TX hardware queue via
`qid = smp_processor_id()` under `CONFIG_ARCH_ALPINE` - re-evaluated per
packet, not per-flow. With `AL_ETH_NUM_QUEUES`=4 matching the box's 4 cores,
segments of the same TCP flow scattered across independently-drained
hardware queues as the sending thread migrated cores. Fixed: use
`netdev_pick_tx()` (the modern exported core helper, `net/core/dev.c`) -
stable per-flow, XPS-aware. (The obvious swap, `skb_tx_hash()`, doesn't work
- it's `static`/core-internal in this kernel version, not callable by an
out-of-tree module; this was discovered the hard way, mid-build-failure -
verify a symbol is actually exported before reaching for it, don't assume
vendor-authored code that's never been build-tested on this kernel is
still valid.)

Confirmed real improvement, same clean 10G path: 19.8 -> 97.6 Mbit/s (4 TX
queues) -> **172 Mbit/s with 1 TX queue forced** (`ethtool -L enp0s2 tx 1`,
via `set_channels` support added this session).

**Root cause #2, NOT yet found**: even with exactly one hardware TX queue
and one TCP stream - no queue selection, no scheduling, nothing left to
"thrash" - TX is still ~55x below RX line rate with heavy loss. Packet
capture (single queue, single flow, 8s): 1,366 retransmissions, 1,047
fast-retransmissions, 3,543 duplicate ACKs, **zero out-of-order, zero
zero-window**. Rules out both wire-level reordering and receiver
backpressure - this is genuine, silent packet loss. `ethtool -S` exposes no
TX error/drop counter at all (the field doesn't exist for this driver), and
`dmesg` has zero TX-related warnings across every test run. Neither the
driver nor ethtool can see whatever is dropping these packets. Most likely
location: UDMA TX completion tracking or the MAC/PCS TX path itself - same
*class* of bug as #23's completion-descriptor-stride mismatch (RX side,
al_dma), just not yet root-caused for TX/al_eth. Full detail and evolving
findings: #121.

Confirmed NOT the cause: #90 (looked related by name, traced the exact log
strings to U-Boot's own separate `al_eth` driver copy
(`uboot-port/drivers/net/al_eth/al_eth_dm_10g.c`) - a different codebase
entirely from the Linux kernel module `modules/al_eth/al_eth_main.c`. Do not
conflate the two.

Related: #116 (use `ip -s link` for real hardware MAC error counters on
this driver - `ethtool -S` only exposes driver/queue counters, not hardware
CRC/FCS/drop counts), #122 (RX errors appearing after repeated
rmmod/modprobe cycling specifically, a different trigger condition from a
single clean boot).

## Use `scripts/eth-tx-capture-diag.py` for capture-based diagnosis

Wraps the isolated-iperf3 + simultaneous-tcpdump + tshark-TCP-analysis
methodology above into one command instead of re-typing the SSH/tcpdump/
tshark chain by hand each time (expensive in tokens, error-prone - a
malformed multi-line hand-typed SSH command hung for 2+ minutes during this
investigation). Checks the box's routing (#170) before running and warns if
the test won't exercise the interface you think it will.

```
./scripts/eth-tx-capture-diag.py --box-ip 192.168.25.133 --box-iface enp0s2 \
    --bind-ip 192.168.25.147 --bind-iface enp7s0 --port 5603 --direction tx
```
