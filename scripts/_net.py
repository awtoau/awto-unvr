"""Dev host network address detection for the UNVR's TFTP flows.

Shared by scripts/ram-boot-deploy.py, dev.py (uboot-test), and any other
tftpboot-driving path. ONE copy of the detection logic, not hardcoded IPs
that can drift stale in every caller separately.
"""

from __future__ import annotations

import socket

UNVR_IPADDR = "192.168.25.140"


def detect_server_ip(peer: str = UNVR_IPADDR, port: int = 69) -> str:
    """This host's local IP on the route to `peer`, via a UDP connect()
    (SOCK_DGRAM connect() only picks a route/local address, sends no packet).

    Was hardcoded (192.168.25.145 in one place, .147 in another) - this
    host's DHCP lease drifts, so a hardcoded serverip silently goes stale:
    every tftpboot request goes to an address nothing is listening on, zero
    RRQs ever arrive, and it looks exactly like a TX hang (all-T retries)
    but isn't. Always detect fresh, right before use - never cache across a
    box power-cycle (U-Boot's own env resets too, so both sides need a
    fresh value each time)."""
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.connect((peer, port))
        return s.getsockname()[0]


if __name__ == "__main__":
    print(detect_server_ip())
