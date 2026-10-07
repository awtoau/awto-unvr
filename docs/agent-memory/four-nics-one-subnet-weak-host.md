---
name: four-nics-one-subnet-weak-host
description: "woomera has 4 NICs on ONE subnet - an IP that answers does not say which port carried it; check the MAC, use iperf3 --bind-dev, and never conclude a port is dead from tcpdump silence"
metadata:
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-10-07T06:30:40.116Z
---

woomera's 4 NICs all sit on `192.168.25.0/24` with DHCP leases: `enp0s1` 1G (`74:ac:b9:41:a8:11`), `enp0s2` 10G (`…a8:12`), `enP1p1s0u1u1` (`60:7d:09:4c:39:7b`), `enP1p1s0u1u3` (`80:6d:97:13:a5:6b`). **Weak-host routing: the box replies on whichever interface the ROUTE picks, not the one the address belongs to.**

**Why:** this produced three wrong conclusions in a single day (2026-10-07), each from a true observation:
- `iperf3 -B <10G addr>` measured **94 Mb/s** — the traffic left via a USB NIC. `-B` picks a source address, not an egress port.
- A "TX asymmetry on every interface including USB" that was unpinned iperf3 measuring whatever the route chose.
- `tcpdump -e 'ether host …a8:11'` showed **zero frames**, concluded "the 1G port is on an unreachable segment". The capture was accurate; the port was fine. `ip neigh show 192.168.25.133` → `lladdr …a8:12`, i.e. the box answers for the 1G address out its 10G MAC.

**How to apply:**
- `ip neigh show <ip>` and **read the MAC** before attributing anything to a port.
- Throughput: `iperf3 --bind-dev <iface>`, never `-B <addr>` alone. State the stream count — one A57 core cannot saturate 10GbE (2.3 Gb/s single-stream vs 9.35 with `-P 4`).
- A port's own truth: `ip -s link show <iface>` counters and `ip neigh show dev <iface>`. RX/TX counts and its own ARP entries beat any inference from the dev host.
- `tcpdump` silence on a MAC is the *expected* result here, not evidence of a dead link.
- Separate measurement from conclusion: say what was observed, then what it does and does not establish.
- U-Boot has no route table and no weak-host behaviour — it uses `ethact`, so a port that works under Linux can still fail there. That is a driver question (#253), not a cabling one.

Tracked as #170. Also in `CLAUDE.md` under "Reaching the box". See [[test-every-change-never-assert]], [[dont-blame-hardware-assume-our-code]].
