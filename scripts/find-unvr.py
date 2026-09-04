#!/usr/bin/env python3
"""Find the UNVR on the LAN.

Three independent methods, because any one can miss:
  1. Ubiquiti discovery protocol (UDP 10001 broadcast) - definitive: devices
     reply with MAC, model, firmware and hostname.
  2. mDNS via avahi-browse.
  3. TCP sweep for the UniFi OS web UI (443) and SSH (22).

Read-only: sends discovery probes and TCP connects on the local subnet. Nothing
is written to any device.

Run: ./scripts/find-unvr.py [--subnet <cidr>] [--sweep]
"""

import argparse
import socket
import struct
import subprocess
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOGS = REPO / "tmp" / "logs"

UBNT_PORT = 10001
# Probes: v1 and v2 of the discovery request. Devices answer one or both.
PROBES = [b"\x01\x00\x00\x00", b"\x02\x08\x00\x00"]

# Collect window. A LAN device replies in single-digit milliseconds; 3 s is
# ~500x that, covering a retransmit and a busy device. On expiry we simply
# report what answered - there is nothing to fail.
COLLECT = 3

# TLV field types from Ubiquiti's discovery protocol.
FIELDS = {
    0x01: "mac",
    0x02: "mac_ip",
    0x03: "firmware",
    0x0A: "uptime",
    0x0B: "hostname",
    0x0C: "platform",
    0x0D: "essid",
    0x0E: "wmode",
    0x14: "model",
    0x15: "model_short",
    0x16: "version",
}


def log(msg):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "find-unvr.log").open("a") as fh:
        fh.write(line + "\n")


def parse_tlv(payload):
    """Parse the TLV body of a discovery reply."""
    out, i = {}, 4  # skip 4-byte header
    while i + 3 <= len(payload):
        ftype = payload[i]
        flen = struct.unpack(">H", payload[i + 1 : i + 3])[0]
        val = payload[i + 3 : i + 3 + flen]
        name = FIELDS.get(ftype, f"0x{ftype:02x}")
        if name == "mac" and flen == 6:
            out["mac"] = ":".join(f"{b:02x}" for b in val)
        elif name == "mac_ip" and flen == 10:
            out["mac"] = ":".join(f"{b:02x}" for b in val[:6])
            out["ip"] = socket.inet_ntoa(val[6:10])
        elif name == "uptime" and flen == 4:
            out["uptime"] = struct.unpack(">I", val)[0]
        else:
            try:
                out[name] = val.decode("utf-8", "replace").strip("\x00")
            except UnicodeError:
                out[name] = val.hex()
        i += 3 + flen
    return out


def broadcast_addrs():
    addrs = ["255.255.255.255"]
    try:
        p = subprocess.run(
            ["ip", "-4", "-brief", "addr", "show"],
            capture_output=True,
            text=True,
            check=False,
        )
        for line in p.stdout.splitlines():
            for tok in line.split():
                if "/" in tok and not tok.startswith("127."):
                    ip, plen = tok.split("/")
                    if plen == "24":
                        addrs.append(".".join(ip.split(".")[:3]) + ".255")
    except (OSError, ValueError):
        pass
    return sorted(set(addrs))


def discover():
    log("=== Ubiquiti discovery (UDP 10001 broadcast) ===")
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.settimeout(COLLECT)
    try:
        s.bind(("", 0))
    except OSError as e:
        log(f"bind failed: {e}")
    for bcast in broadcast_addrs():
        for probe in PROBES:
            try:
                s.sendto(probe, (bcast, UBNT_PORT))
            except OSError as e:
                log(f"send to {bcast} failed: {e}")
    found = {}
    import time

    deadline = time.monotonic() + COLLECT
    while time.monotonic() < deadline:
        try:
            s.settimeout(max(0.1, deadline - time.monotonic()))
            data, addr = s.recvfrom(4096)
        except TimeoutError:
            break
        except OSError:
            break
        if len(data) < 4:
            continue
        info = parse_tlv(data)
        info.setdefault("ip", addr[0])
        found[info["ip"]] = info
    s.close()
    if not found:
        log("no Ubiquiti devices answered")
    for ip, i in sorted(found.items()):
        log(
            f"  {ip:16s} mac={i.get('mac', '?')} model={i.get('model') or i.get('platform', '?')} "
            f"fw={i.get('firmware', '?')} host={i.get('hostname', '?')}"
        )
    return found


def mdns():
    log("=== mDNS (avahi-browse) ===")
    try:
        p = subprocess.run(
            ["avahi-browse", "-a", "-t", "-r", "-p"],
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
    except (subprocess.TimeoutExpired, OSError) as e:
        log(f"avahi-browse unavailable: {e}")
        return
    hits = [
        l
        for l in p.stdout.splitlines()
        if any(k in l.lower() for k in ("unifi", "ubnt", "ubiquiti", "unvr", "protect"))
    ]
    if not hits:
        log("no UniFi-looking mDNS records")
    for h in hits[:20]:
        log(f"  {h}")


def sweep(subnet):
    log(f"=== TCP sweep {subnet} for 443/22 ===")
    try:
        p = subprocess.run(
            ["nmap", "-n", "-Pn", "-p", "443,22", "--open", "-oG", "-", subnet],
            capture_output=True,
            text=True,
            timeout=600,
            check=False,
        )
    except (subprocess.TimeoutExpired, OSError) as e:
        log(f"nmap failed: {e}")
        return
    for line in p.stdout.splitlines():
        if line.startswith("Host:") and "Ports:" in line:
            log(f"  {line.split('Ignored')[0].strip()}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--subnet", default=None, help="CIDR for the TCP sweep")
    ap.add_argument(
        "--sweep", action="store_true", help="also run the nmap sweep (slower)"
    )
    a = ap.parse_args()

    log("--- find-unvr ---")
    found = discover()
    mdns()
    if a.sweep or a.subnet:
        sub = a.subnet
        if not sub:
            p = subprocess.run(
                ["ip", "-4", "-brief", "addr", "show"],
                capture_output=True,
                text=True,
                check=False,
            )
            for line in p.stdout.splitlines():
                for tok in line.split():
                    if tok.endswith("/24") and not tok.startswith("127."):
                        ip = tok.split("/")[0]
                        sub = ".".join(ip.split(".")[:3]) + ".0/24"
        if sub:
            sweep(sub)
    log("done")
