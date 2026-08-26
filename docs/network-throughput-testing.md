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

## Current known state (not a test artifact - see #121)

With the methodology above correctly applied (confirmed real 10G-path
traffic, not 1G masquerading as 10G):
- **Host → box (box RX): reaches near line rate** (~9.19 Gbit/s measured).
- **Box → host (box TX): severely degraded** (~140 Mbit/s - 1.87 Gbit/s
  across different runs, thousands of retransmits). This is a real,
  asymmetric loss problem on the box's transmit path or the link back to
  the test host, confirmed on genuine 10G traffic - not fixed by any test-
  tooling change, not yet root-caused.

Related: #116 (use `ip -s link` for real hardware MAC error counters on
this driver - `ethtool -S` only exposes driver/queue counters, not hardware
CRC/FCS/drop counts), #122 (RX errors appearing after repeated
rmmod/modprobe cycling specifically, a different trigger condition from a
single clean boot).
