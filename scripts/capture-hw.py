#!/usr/bin/env python3
"""Capture the live hardware picture from the running UNVR over the serial console.

Reference material for custom-Linux bring-up on the AL-324: the stock kernel's
own view of the SoC - DTB, drivers, PCI/SATA topology, SFP, sensors, GPIO, memory
map. Text goes straight over the console; small binaries (live DTB, SFP EEPROM)
come back base64-encoded on the same channel, so there is no TFTP dependency.

Output: docs/hw-reference/<RUN_ID>/  (one file per probe) + capture.log
"""

from __future__ import annotations

import base64
import fcntl
import re
import socket
import sys
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console  # noqa: E402
from _repo import REPO

SOCK = _console.SOCK
LOGS = REPO / "tmp" / "logs"
LOG = LOGS / "capture-hw.log"
RUN_ID = datetime.now().astimezone().strftime("%Y%m%d-%H%M%S")
OUT = REPO / "docs" / "hw-reference" / RUN_ID

LOGIN_USER = "root"
LOGIN_PASSWORDS = ["ui", "ubnt"]  # al324 = ui, older = ubnt

_SEQ = [0]
_CONN: list[socket.socket] = []  # one persistent console connection


def log(msg: str, level: str = "INFO") -> None:
    line = f"{datetime.now().astimezone().strftime('%Y-%m-%dT%H:%M:%S%z')}  {level:5s} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with LOG.open("a", encoding="utf-8") as fh:
        fh.write(line + "\n")


def _sock() -> socket.socket:
    """One long-lived connection for the whole run. Reconnecting per command
    races the shell: a fast command's output flies past between close and the
    next connect, so small probes came back empty while slow ones survived."""
    if not _CONN:
        if not SOCK.exists():
            sys.exit(f"console socket absent: {SOCK} - start ./dev.py console")
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(1.0)
        s.connect(str(SOCK))
        _CONN.append(s)
    return _CONN[0]


def _drain(s: socket.socket, quiet: float = 0.3) -> None:
    """Swallow anything still in flight before issuing the next command."""
    end = time.monotonic() + quiet
    while time.monotonic() < end:
        try:
            if not s.recv(65536):
                break
        except TimeoutError:
            break


def run(cmd: str, wait: float = 8.0) -> str:
    """Run one command; return exactly the text between START and END markers.

    Bracketing with two distinct markers (not just a trailing sentinel) makes the
    output boundaries unambiguous regardless of echo, prompt or line wrapping.
    """
    s = _sock()
    _drain(s)
    _SEQ[0] += 1
    a, b = f"__HWa{_SEQ[0]}zz__", f"__HWb{_SEQ[0]}zz__"
    s.sendall(f"echo {a}; {cmd}; echo {b}$?".encode() + b"\r")
    got, end = "", time.monotonic() + wait
    while time.monotonic() < end:
        try:
            chunk = s.recv(65536)
        except TimeoutError:
            continue
        if not chunk:
            break
        got += chunk.decode("utf-8", "replace")
        if re.search(rf"{b}\d", got):
            break
    got = got.replace("\x1b[?2004l", "").replace("\x1b[?2004h", "")
    # Text strictly between the START marker's own echo-output line and the END
    # marker. The START token appears twice (command echo, then its output); the
    # real output begins after the SECOND occurrence.
    m_end = re.search(rf"{b}\d", got)
    if not m_end:
        return ""  # timed out without the end marker
    head = got[: m_end.start()]
    parts = head.split(a)
    body = parts[-1] if len(parts) >= 2 else head  # after last START occurrence
    return (
        "\n".join(body.splitlines()[1:]).strip("\r\n") if "\n" in body else body.strip()
    )


def login() -> None:
    if "OK__" in run("echo OK__", wait=6.0):
        return
    for pw in LOGIN_PASSWORDS:
        s = _sock()
        s.sendall(b"\r")
        time.sleep(0.5)
        s.sendall(LOGIN_USER.encode() + b"\r")
        time.sleep(1.0)
        s.sendall(pw.encode() + b"\r")
        time.sleep(2.0)
        _drain(s)
        if "OK__" in run("echo OK__", wait=6.0):
            log(f"logged in (root:{pw})")
            return
    sys.exit("could not authenticate at console")


def capture(name: str, cmd: str, wait: float = 8.0) -> None:
    out = run(cmd, wait=wait)
    (OUT / name).write_text(out + "\n", encoding="utf-8")
    n = len(out.splitlines())
    log(f"  {name:28s} <- {cmd[:50]:50s} ({n} lines)")


def capture_bin(name: str, devpath: str, wait: float = 30.0) -> None:
    """Pull a small binary back base64 over the console (no TFTP needed)."""
    out = run(f"base64 -w0 {devpath} 2>/dev/null", wait=wait)
    b64 = re.sub(r"[^A-Za-z0-9+/=]", "", out)
    try:
        blob = base64.b64decode(b64, validate=False)
    except Exception as e:  # noqa: BLE001
        log(f"  {name}: base64 decode failed ({e})", "WARN")
        return
    (OUT / name).write_bytes(blob)
    log(f"  {name:28s} <- {devpath:50s} ({len(blob)} bytes)")


# name -> (command, wait). Text probes.
PROBES: list[tuple[str, str, float]] = [
    ("uname.txt", "uname -a", 5),
    (
        "version.txt",
        "cat /usr/lib/version 2>/dev/null; cat /etc/version 2>/dev/null",
        5,
    ),
    ("cmdline.txt", "cat /proc/cmdline", 5),
    ("cpuinfo.txt", "cat /proc/cpuinfo", 5),
    ("meminfo.txt", "cat /proc/meminfo", 5),
    ("iomem.txt", "cat /proc/iomem", 6),
    ("interrupts.txt", "cat /proc/interrupts", 6),
    ("mtd.txt", "cat /proc/mtd", 5),
    ("mdstat.txt", "cat /proc/mdstat", 5),
    ("modules.txt", "lsmod", 6),
    ("partitions.txt", "cat /proc/partitions", 5),
    ("mounts.txt", "cat /proc/mounts", 5),
    # PCI: prefer lspci, fall back to raw sysfs id list.
    (
        "lspci.txt",
        "lspci -vnn 2>/dev/null || for d in /sys/bus/pci/devices/*; do echo $d $(cat $d/vendor) $(cat $d/device) class=$(cat $d/class); done",
        10,
    ),
    (
        "lsusb.txt",
        "lsusb 2>/dev/null || for d in /sys/bus/usb/devices/*/idVendor; do echo $(dirname $d) $(cat $d):$(cat $(dirname $d)/idProduct); done",
        8,
    ),
    ("ip-link.txt", "ip -d link show", 6),
    ("ip-addr.txt", "ip -4 addr show", 5),
    ("net-ifaces.txt", "ls -l /sys/class/net", 5),
    ("ubnthal.txt", "cat /proc/ubnthal/system.info 2>/dev/null", 6),
    (
        "i2c-devices.txt",
        "i2cdetect -l 2>/dev/null; echo ---; for n in /sys/bus/i2c/devices/*/name; do echo $n=$(cat $n); done",
        8,
    ),
    (
        "hwmon.txt",
        "for h in /sys/class/hwmon/hwmon*; do echo == $h $(cat $h/name 2>/dev/null); for f in $h/temp*_input $h/in*_input $h/fan*_input; do [ -e $f ] && echo $f=$(cat $f); done; done 2>/dev/null",
        8,
    ),
    (
        "gpio.txt",
        "cat /sys/kernel/debug/gpio 2>/dev/null || (mount -t debugfs none /sys/kernel/debug 2>/dev/null; cat /sys/kernel/debug/gpio 2>/dev/null)",
        8,
    ),
    (
        "thermal.txt",
        "for t in /sys/class/thermal/thermal_zone*; do echo $t type=$(cat $t/type 2>/dev/null) temp=$(cat $t/temp 2>/dev/null); done",
        6,
    ),
    (
        "blockdev.txt",
        "lsblk -o NAME,SIZE,TYPE,FSTYPE,MODEL,SERIAL 2>/dev/null || ls -l /sys/block",
        8,
    ),
    ("dt-nodes.txt", "find /proc/device-tree -maxdepth 2 -type d | sort", 8),
    ("dmesg.txt", "dmesg", 20),
]

# SFP: decoded EEPROM per port that has a module. eth ports are al_eth; try both.
SFP_PROBES = [
    (
        "ethtool-eth0.txt",
        "ethtool eth0 2>/dev/null; echo ---MODULE---; ethtool -m eth0 2>/dev/null",
        10,
    ),
    (
        "ethtool-eth1.txt",
        "ethtool eth1 2>/dev/null; echo ---MODULE---; ethtool -m eth1 2>/dev/null",
        10,
    ),
]

# Binaries pulled base64 over the console.
BINARIES = [
    ("live.dtb", "/sys/firmware/fdt"),
]


if __name__ == "__main__":
    LOGS.mkdir(parents=True, exist_ok=True)
    _lock = (LOGS / ".console-user.lock").open("w")
    try:
        fcntl.flock(_lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        sys.exit("another console user is active (console is single-user)")

    OUT.mkdir(parents=True, exist_ok=True)
    log(f"--- capture-hw -> {OUT.relative_to(REPO)} ---")
    login()

    for name, cmd, wait in PROBES + SFP_PROBES:
        try:
            capture(name, cmd, wait=wait)
        except Exception as e:  # noqa: BLE001
            log(f"  {name}: FAILED ({e})", "WARN")

    for name, dev in BINARIES:
        try:
            capture_bin(name, dev)
        except Exception as e:  # noqa: BLE001
            log(f"  {name}: FAILED ({e})", "WARN")

    log(f"done. {len(list(OUT.iterdir()))} files in {OUT.relative_to(REPO)}")
