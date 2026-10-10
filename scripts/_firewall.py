# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Dev-host firewalld preflight for the TFTP flows (#136).

A zone that drops UDP:69 leaves tftpd up and listening while the box's RRQs
never arrive: silent `T T` timeouts that look like a dead network.

  ensure_tftp_open(port)   preflight for every tftp call site
  ./scripts/_firewall.py [--apply]   check (and with --apply, fix) by hand

Fix is opt-in: --apply or AWTO_FIREWALL_APPLY=1. Otherwise a blocked port
exits with the exact command to run. firewalld not running -> skipped.
"""

from __future__ import annotations

import ipaddress
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _net import detect_server_ip

APPLY_ENV = "AWTO_FIREWALL_APPLY"
# firewall-cmd and ip are local, <50 ms; 5 s is a 100x ceiling against a
# wedged firewalld D-Bus call. Expiry: treated as "cannot check", fail loud.
CMD_LIMIT_S = 5
# Zone targets that accept everything not otherwise matched.
ACCEPT_TARGETS = {"ACCEPT"}


@dataclass
class Verdict:
    status: str  # ok | blocked | skipped
    detail: str
    zone: str = ""
    iface: str = ""
    fix: list[list[str]] = field(default_factory=list)


def iface_of_ip(ip_out: str, ip: str) -> str:
    """Interface holding `ip`, from `ip -o -4 addr show` output."""
    for line in ip_out.splitlines():
        m = re.match(r"\d+:\s+(\S+)\s+inet\s+(\d+\.\d+\.\d+\.\d+)/", line)
        if m and m.group(2) == ip:
            return m.group(1)
    return ""


def zone_of(active_out: str, iface: str, ip: str) -> str:
    """Zone governing `ip`/`iface` from `--get-active-zones`.
    firewalld matches a source (subnet) before an interface."""
    zones: dict[str, dict[str, list[str]]] = {}
    cur = ""
    for line in active_out.splitlines():
        if not line.strip():
            continue
        if not line[0].isspace():
            cur = line.split()[0]
            zones[cur] = {"interfaces": [], "sources": []}
            continue
        key, _, rest = line.strip().partition(":")
        if cur and key in ("interfaces", "sources"):
            zones[cur][key] = rest.split()
    addr = ipaddress.ip_address(ip)
    for name, z in zones.items():
        for src in z["sources"]:
            try:
                if addr in ipaddress.ip_network(src, strict=False):
                    return name
            except ValueError:
                continue
    for name, z in zones.items():
        if iface in z["interfaces"]:
            return name
    return ""


def port_in_list(ports: str, port: int, proto: str = "udp") -> bool:
    """`--list-ports` output ("1025-65535/udp 69/udp") covers `port`?"""
    for tok in ports.split():
        rng, _, p = tok.partition("/")
        if p != proto:
            continue
        lo, _, hi = rng.partition("-")
        if int(lo) <= port <= int(hi or lo):
            return True
    return False


def zone_permits(services: str, ports: str, target: str, port: int) -> bool:
    if target.strip() in ACCEPT_TARGETS:
        return True
    if port == 69 and "tftp" in services.split():
        return True
    return port_in_list(ports, port)


def fix_commands(zone: str, port: int) -> list[list[str]]:
    """Runtime + permanent add, no reload (keeps other runtime-only state)."""
    arg = "--add-service=tftp" if port == 69 else f"--add-port={port}/udp"
    return [
        ["sudo", "firewall-cmd", f"--zone={zone}", arg],
        ["sudo", "firewall-cmd", "--permanent", f"--zone={zone}", arg],
    ]


def _target_of(list_all: str) -> str:
    """`target:` line of `--list-all` (--get-target needs --permanent)."""
    m = re.search(r"^\s*target:\s*(\S+)", list_all, re.M)
    return m.group(1) if m else ""


def _run(cmd: list[str]) -> str:
    r = subprocess.run(
        cmd, capture_output=True, text=True, check=False, timeout=CMD_LIMIT_S
    )
    return r.stdout.strip() if r.returncode == 0 else ""


def check_tftp(server_ip: str, port: int = 69) -> Verdict:
    if not shutil.which("firewall-cmd"):
        return Verdict("skipped", "firewall-cmd not installed")
    if _run(["firewall-cmd", "--state"]) != "running":
        return Verdict("skipped", "firewalld not running")
    iface = iface_of_ip(_run(["ip", "-o", "-4", "addr", "show"]), server_ip)
    zone = zone_of(_run(["firewall-cmd", "--get-active-zones"]), iface, server_ip)
    if not zone:
        return Verdict(
            "blocked", f"no firewalld zone found for {server_ip} ({iface or '?'})"
        )
    z = f"--zone={zone}"
    ok = zone_permits(
        _run(["firewall-cmd", z, "--list-services"]),
        _run(["firewall-cmd", z, "--list-ports"]),
        _target_of(_run(["firewall-cmd", z, "--list-all"])),
        port,
    )
    if ok:
        return Verdict("ok", f"UDP:{port} permitted in zone {zone}", zone, iface)
    return Verdict(
        "blocked",
        f"zone {zone} (iface {iface}) does not permit UDP:{port}",
        zone,
        iface,
        fix_commands(zone, port),
    )


def ensure_tftp_open(
    port: int = 69, server_ip: str | None = None, apply: bool = False, log=print
) -> Verdict:
    """Preflight: exit loudly if the firewall will drop the box's RRQs."""
    apply = apply or os.environ.get(APPLY_ENV) == "1"
    try:
        v = check_tftp(server_ip or detect_server_ip(), port)
    except (subprocess.TimeoutExpired, OSError) as e:
        sys.exit(f"firewall preflight failed ({e}); check `firewall-cmd --state`")
    log(f"firewall: {v.status} - {v.detail}")
    if v.status != "blocked":
        return v
    cmds = [" ".join(c) for c in v.fix]
    if not apply or not v.fix:
        sys.exit(
            f"UDP:{port} is blocked: tftp requests from the box would never "
            f"arrive.\n  fix: {' && '.join(cmds) or 'add the LAN interface to a permitting zone'}"
            f"\n  or rerun with {APPLY_ENV}=1"
        )
    for c in v.fix:
        log(f"firewall: + {' '.join(c)}")
        subprocess.run(c, check=True, timeout=CMD_LIMIT_S * 6)
    return check_tftp(server_ip or detect_server_ip(), port)


if __name__ == "__main__":
    import argparse

    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--port", type=int, default=69)
    ap.add_argument("--apply", action="store_true", help="run the fix (sudo)")
    a = ap.parse_args()
    print(ensure_tftp_open(a.port, apply=a.apply))
