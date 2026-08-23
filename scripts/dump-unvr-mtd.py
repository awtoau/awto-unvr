#!/usr/bin/env python3
"""Dump every MTD partition off the UNVR over the wire. No SSH.

The device has no SSH and no open ports - it sits in the initramfs BusyBox shell
because USERDEV is missing. So: commands go in over the tio console socket, bulk
data comes back over netcat. Nothing is written to the device.

Irreplaceable partitions are pulled FIRST, so an interrupted run still leaves the
identity captured:
    Factory, EEPROM  - MAC, board ID, HW rev, device ID. Downloadable nowhere.
    u-boot, env      - the bootloader and its environment, i.e. whether this unit
                       actually boots unsigned (dobootm=bootunsign) or not.
    recovery kernel  - the unbrick path.
Bulk NAND (rootfs, ~1 GB) goes last.

Restore is `flashcp`, NEVER `dd` - MTD needs an erase cycle first.

Run: ./scripts/dump-unvr-mtd.py            # all partitions
     ./scripts/dump-unvr-mtd.py --only 8,9 # just those mtd numbers
     ./scripts/dump-unvr-mtd.py --list     # read /proc/mtd and stop
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import socket
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import IMAGES, LOGS, rel

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"

# Run identity. Compact sortable stamp for paths (house rule: YYYYMMDD-HHMMSS in
# filenames, full ISO 8601 with offset in content). Every dump is named for the
# unit AND the run, so a single .img file remains identifiable on its own after
# being copied somewhere else - which is the whole point of dumping it.
RUN_ID = datetime.now().astimezone().strftime("%Y%m%d-%H%M%S")
RUN_ISO = datetime.now().astimezone().isoformat(timespec="seconds")

# Set once the device has told us who it is.
OUT: Path = IMAGES / "mtd"
BOARD: dict[str, str] = {}

# TFTP transport. Port 69 needs root to bind, so use a high port - both busybox
# tftp and U-Boot accept an explicit port. 1400 keeps each block inside a 1500-byte
# Ethernet MTU while cutting round trips ~2.7x versus the 512-byte default; at
# 512 a 1 GB partition would need ~2M round trips.
TFTP_PORT = 6969
TFTP_BLKSIZE = 1400
TFTP_ROOT = IMAGES / "tftp"

# Order: identity and boot first, bulk last. An interrupted run must still have
# left us the things that cannot be re-obtained.
PRIORITY = [
    "Factory",
    "EEPROM",
    "u-boot",
    "u-boot env",
    "u-boot env redundant",
    "cksum",
    "device_tree",
    "al_boot",
    "config",
    "chike",
    "recovery kernel",
    "linux_kernel",
    "rootfs",
]

# Ships in the firmware .bin, so re-extractable and not worth committing.
# Everything else is unique to this unit and IS committed - see .gitignore.
REOBTAINABLE = {
    "al_boot",
    "device_tree",
    "linux_kernel",
    "chike",
    "u-boot",
    "recovery kernel",
    "rootfs",
}

# Presets for re-backing-up between firmware upgrades.
#
#   identity - the irreplaceable set. ~264 KB, seconds. Nothing here should ever
#              change; if it does, something is wrong and you want to know.
#   state    - identity + config + kernel + u-boot. ~33 MB, under a minute.
#              Everything unique to this unit plus the parts an upgrade rewrites,
#              minus the 1 GB rootfs (which is in the .bin you just flashed).
#   all      - every partition, ~1.1 GB, several minutes.
#
# `upgrade_firmware()` writes only kernel and rootfs, so u-boot/env/EEPROM are
# expected identical across a hop. `state` is the sane per-hop default.
# Selected BY LABEL, never by index. MTD numbering is NOT stable across firmware
# generations: 1.3.35 builds al_nand into the kernel so NAND takes mtd0-4, while
# al324 builds load it as a module so SPI wins the low numbers and NAND starts at
# mtd8. Index presets silently dumped the wrong partitions after the 2.3.14 hop.
PRESETS = {
    "identity": {"u-boot env", "u-boot env redundant", "Factory", "EEPROM", "cksum"},
    "state": {
        "linux_kernel",
        "u-boot",
        "u-boot env",
        "u-boot env redundant",
        "Factory",
        "EEPROM",
        "config",
        "cksum",
    },
    "all": None,  # whatever this firmware exposes
}

# Console read window. The shell echoes and replies in milliseconds; 2 s is ~1000x
# that and still returns promptly when a command produces nothing.
CONSOLE_WAIT = 2.0

# Transfer floor for sizing the wait. Measured NAND-read + 100 Mbit is well over
# 5 MB/s; 1 MB/s is a deliberately pessimistic floor so a slow link is tolerated
# but a wedged transfer still fails instead of hanging forever. On expiry the
# partition is recorded FAILED and the run continues to the next.
MIN_RATE = 1 << 20
MIN_WAIT = 30


def log(msg, level="INFO"):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {level:<5} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "dump-unvr-mtd.log").open("a") as fh:
        fh.write(line + "\n")


def console(cmd: str, wait: float = CONSOLE_WAIT) -> str:
    """Send one line to the device shell via tio's socket, return what came back."""
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}\nStart it with ./dev.py console")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(wait)
    s.connect(str(SOCK))
    s.sendall(cmd.encode() + b"\r")
    got = b""
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            got += chunk
    except TimeoutError:
        pass
    finally:
        s.close()
    return got.decode("utf-8", "replace")


def host_ip() -> str:
    """Our address on the device's subnet, learned from the routing table rather
    than assumed - the lab has several interfaces on the same segment."""
    # Skip loopback. In the initramfs `lo` is unconfigured so the first inet line
    # is the real NIC, but on a fully booted system 127.0.0.1/8 comes first -
    # taking it makes the device TFTP to itself and every transfer times out at
    # 0 bytes. Skip link-local too.
    out = console("ip -4 addr show", wait=2.0)
    cands = [
        ip
        for ip in re.findall(r"inet (\d+\.\d+\.\d+\.\d+)/", out)
        if not ip.startswith(("127.", "169.254."))
    ]
    if not cands:
        sys.exit(f"no routable IPv4 on the device. Console said:\n{out}")
    dev_ip = cands[0]
    p = subprocess.run(
        ["ip", "-o", "route", "get", dev_ip],
        capture_output=True,
        text=True,
        check=False,
    )
    m = re.search(r"\bsrc (\d+\.\d+\.\d+\.\d+)", p.stdout)
    if not m:
        sys.exit(f"no route to the device at {dev_ip}")
    log(f"device {dev_ip}, this host {m.group(1)}")
    return m.group(1)


def read_board() -> dict[str, str]:
    """`ubnt-tools id` - the unit's own identity, used to name the dump set."""
    out = console("ubnt-tools id", wait=3.0)
    board = {}
    for line in out.splitlines():
        m = re.match(r"^board\.([a-z.]+)=(.*)$", line.strip())
        if m:
            board[m.group(1)] = m.group(2).strip()
    if not board:
        log("ubnt-tools id returned nothing - naming with 'unknown'", "WARN")
    return board


def read_proc_mtd() -> list[dict]:
    out = console("cat /proc/mtd", wait=3.0)
    parts = []
    for line in out.splitlines():
        m = re.match(r"^(mtd\d+):\s+([0-9a-f]+)\s+([0-9a-f]+)\s+\"(.*)\"", line.strip())
        if m:
            parts.append(
                {
                    "dev": m.group(1),
                    "num": int(m.group(1)[3:]),
                    "size": int(m.group(2), 16),
                    "erase": int(m.group(3), 16),
                    "name": m.group(4),
                }
            )
    if not parts:
        sys.exit(f"no MTD partitions parsed. Console said:\n{out}")
    return parts


def console_capture(cmd: str, sentinel: str, max_wait: float) -> str:
    """Send cmd, read until `sentinel` appears. Used for bulk reads where the
    fixed CONSOLE_WAIT is far too short."""
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}\nStart it with ./dev.py console")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(5.0)
    s.connect(str(SOCK))
    s.sendall(cmd.encode() + b"\r")
    buf = ""
    end = time.monotonic() + max_wait
    try:
        while time.monotonic() < end:
            try:
                chunk = s.recv(1 << 16)
            except TimeoutError:
                continue
            if not chunk:
                break
            buf += chunk.decode("utf-8", "replace")
            if sentinel in buf:
                break
    finally:
        s.close()
    return buf


def dump_serial(part: dict) -> bytes | None:
    """Read a partition over the serial console as hex.

    Why not netcat: `cat /dev/mtdN | nc` transfers 0 bytes on this box, while
    `cat /proc/mtd | nc` and `echo | nc` both work - so pipes and the network are
    fine and it is the MTD char device that will not feed a pipe. `od` reads it
    correctly (as `wc -c` does), so the console is the reliable path.

    Cost: hex is ~3.06x the payload (16 bytes -> "xx " * 16 + newline) and the
    line is 115200 8N1 = ~11.5 kB/s, so ~3.7 kB/s of payload. Fine for the 64 KB
    identity partitions (~20 s each); hopeless for the 1004 MB rootfs.
    """
    size = part["size"]
    # 3.06 bytes on the wire per payload byte at ~11.5 kB/s, x4 headroom for the
    # device's own read speed and console echo. Floor of 30 s for tiny reads.
    budget = max(30.0, (size * 3.06 / 11500) * 4)
    marker = f"OD-END-{part['num']}"
    log(f"  reading over serial as hex (budget {budget:.0f}s)")
    out = console_capture(
        f"od -An -v -tx1 /dev/{part['dev']}; echo {marker}", marker, budget
    )
    if marker not in out:
        log(f"  FAILED: sentinel {marker} not seen within {budget:.0f}s", "ERROR")
        return None
    body = out.split(marker)[0]
    blob = bytearray()
    for line in body.splitlines():
        toks = line.split()
        for t in toks:
            if len(t) == 2:
                try:
                    blob.append(int(t, 16))
                except ValueError:
                    pass
    if len(blob) != size:
        log(f"  FAILED: decoded {len(blob)} B, expected {size}", "ERROR")
        return None
    return bytes(blob)


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as fh:
        for b in iter(lambda: fh.read(4 << 20), b""):
            h.update(b)
    return h.hexdigest()


def dump(part: dict, ip_host: str) -> bool:
    safe = re.sub(r"[^A-Za-z0-9_.-]", "_", part["name"])
    model = BOARD.get("shortname", "UNVR")
    serial = BOARD.get("serialno", "unknown")
    sysid = BOARD.get("sysid", "unknown").replace("0x", "")
    # Fully self-describing: model, unit serial, sysid, partition number and
    # name, exact byte size, and the run stamp. Zero-padded mtd number so a
    # directory listing sorts correctly past mtd9.
    out = OUT / (
        f"{model}-{serial}-{sysid}-mtd{part['num']:02d}-{safe}"
        f"-{part['size']}B-{RUN_ID}.img"
    )
    wait = max(MIN_WAIT, int(part["size"] / MIN_RATE))
    log(
        f'{part["dev"]:6s} "{part["name"]}" {part["size"]} B -> {rel(out)} (limit {wait}s)'
    )

    # TFTP, NOT netcat. BusyBox nc on this box cannot move bulk data: piping
    # /dev/mtdN into it yields 0 bytes, and even a staged regular file truncates
    # DETERMINISTICALLY at 57351 of 65536 - same figure on every retry, so not a
    # race, just unusable. TFTP is ACKed per block, so a transfer completes or
    # fails loudly. Verified byte-perfect on mtd8 (md5 matches the raw device).
    #
    # Stage to tmpfs first regardless: the MTD char device will not feed a pipe,
    # and tftp wants a plain file. /tmp is tmpfs on 4 GB of RAM.
    stage = f"/tmp/.mtddump-{part['num']}"
    remote = out.name
    t0 = time.monotonic()
    console(
        f"dd if=/dev/{part['dev']} of={stage} bs=64k && "
        f"tftp -b {TFTP_BLKSIZE} -p -l {stage} -r {remote} {ip_host} {TFTP_PORT}; "
        f"rm -f {stage}",
        wait=0.3,
    )

    # Poll for the file to reach full size in the tftpd root, then move it into
    # the run directory. Polling the size is how we learn the transfer finished
    # without coupling to the server process.
    landed = TFTP_ROOT / remote
    deadline = time.monotonic() + wait
    while time.monotonic() < deadline:
        if landed.exists() and landed.stat().st_size >= part["size"]:
            break
        time.sleep(0.2)
    if not landed.exists() or landed.stat().st_size != part["size"]:
        got = landed.stat().st_size if landed.exists() else 0
        log(f"  FAILED: {got} of {part['size']} B within {wait}s", "ERROR")
        return False
    landed.replace(out)
    dt = max(time.monotonic() - t0, 0.001)

    got = out.stat().st_size
    ok = got == part["size"]
    rate = got / dt / (1 << 20)
    log(
        f"  {got} B in {dt:.1f}s ({rate:.1f} MB/s) "
        + ("OK" if ok else f"SIZE MISMATCH, expected {part['size']}"),
        "INFO" if ok else "ERROR",
    )
    if not ok:
        return False

    local = sha256(out)
    remote = console(
        f"md5sum /dev/{part['dev']}", wait=max(10.0, part["size"] / (8 << 20))
    )
    m = re.search(r"\b([0-9a-f]{32})\b", remote)
    md5_local = hashlib.md5(out.read_bytes()).hexdigest()
    if m and m.group(1) == md5_local:
        log(f"  md5 {m.group(1)} matches device")
    elif m:
        log(f"  MD5 MISMATCH device={m.group(1)} local={md5_local}", "ERROR")
        return False
    else:
        log("  device md5 not returned - size verified only", "WARN")
    part["sha256"] = local
    part["file"] = out.name
    return True


def manifest(parts: list[dict]) -> None:
    lines = [
        f"# UNVR MTD dumps — {RUN_ID}\n",
        f"Taken {RUN_ISO} over the serial console + netcat "
        "(no SSH; the device has no open ports).\n",
        "## Unit\n",
        "| Field | Value |",
        "|---|---|",
    ]
    for k in ("shortname", "name", "sysid", "serialno", "hwrev", "bom", "uuid", "qrid"):
        if k in BOARD:
            lines.append(f"| `board.{k}` | `{BOARD[k]}` |")
    lines += [
        f"| kernel | `{BOARD.get('_uname', 'unknown')}` |",
        "\n## Partitions\n",
        "Partitions unique to this unit (Factory, EEPROM, u-boot env, config,",
        "cksum) are **committed** - they exist nowhere else. Partitions that ship",
        "in the firmware `.bin` and can be re-extracted (al_boot, device_tree,",
        "linux_kernel, chike, u-boot, recovery kernel, rootfs) are gitignored;",
        "rootfs alone is 1004 MB, 10x GitHub's per-file limit.\n",
        "| Device | Name | Bytes | Erase | Tracked | sha256 | File |",
        "|---|---|---|---|---|---|---|",
    ]
    for p in sorted(parts, key=lambda x: x["num"]):
        tracked = "no" if p["name"] in REOBTAINABLE else "**yes**"
        if "sha256" not in p:
            lines.append(
                f"| `{p['dev']}` | {p['name']} | {p['size']} | {p['erase']} | "
                f"{tracked} | **NOT CAPTURED** | — |"
            )
        else:
            lines.append(
                f"| `{p['dev']}` | {p['name']} | {p['size']} | {p['erase']} | "
                f"{tracked} | `{p['sha256'][:16]}…` | `{p['file']}` |"
            )
    lines += [
        "\n## Restoring\n",
        "`flashcp` erases first; `dd` to a char MTD does not and leaves a corrupt",
        "partition.\n",
        "```",
        "flashcp -v <file>.img /dev/mtdN",
        "```\n",
    ]
    (OUT / "README.md").write_text("\n".join(lines) + "\n")
    log(f"wrote {rel(OUT / 'README.md')}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--list", action="store_true", help="read /proc/mtd and stop")
    ap.add_argument("--only", help="comma-separated mtd numbers, e.g. 8,9")
    ap.add_argument(
        "--preset",
        choices=sorted(PRESETS),
        help="identity (264 KB) | state (33 MB, per-upgrade default) | all (1.1 GB)",
    )
    ap.add_argument(
        "--tag", default="", help="label folded into the dump dir name, e.g. pre-1.4.9"
    )
    a = ap.parse_args()

    # Single-instance lock. Two dumps sharing one console + TFTP interleave their
    # reads and silently corrupt each other - which is exactly what happened when
    # a killed ladder orphaned its dump child and the next run collided with it
    # (a good env md5 came back as the pre-saveenv value). flock releases on exit,
    # including SIGKILL, so a crashed run never wedges the next one.
    import fcntl

    LOGS.mkdir(parents=True, exist_ok=True)
    _lock = (LOGS / ".dump-unvr-mtd.lock").open("w")
    try:
        fcntl.flock(_lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        sys.exit("another dump-unvr-mtd is already running (console is single-user)")

    log("--- dump-unvr-mtd (console + TFTP) ---")
    BOARD.update(read_board())
    BOARD["_uname"] = (
        console("uname -r", wait=2.0).splitlines()[-2].strip()
        if len(console("uname -r", wait=2.0).splitlines()) > 1
        else "unknown"
    )
    model = BOARD.get("shortname", "UNVR")
    serial = BOARD.get("serialno", "unknown")
    sysid = BOARD.get("sysid", "unknown").replace("0x", "")
    tag = f"-{re.sub(r'[^A-Za-z0-9_.-]', '_', a.tag)}" if a.tag else ""
    OUT = IMAGES / "mtd" / f"{model}-{serial}-sysid{sysid}-{RUN_ID}{tag}"
    globals()["OUT"] = OUT
    OUT.mkdir(parents=True, exist_ok=True)
    log(f"unit   : {model} serial={serial} sysid={sysid}")
    log(f"dumping to {rel(OUT)}/")

    parts = read_proc_mtd()
    for p in parts:
        log(f'  {p["dev"]:6s} {p["size"]:>10} B  erase {p["erase"]:>7}  "{p["name"]}"')
    if a.list:
        sys.exit(0)

    if a.only:
        want = {int(x) for x in a.only.split(",")}
        parts = [p for p in parts if p["num"] in want]
    elif a.preset:
        names = PRESETS[a.preset]
        if names is None:
            log(f"preset {a.preset}: all {len(parts)} partitions")
        else:
            # A label absent from this generation is worth saying out loud - it
            # means the preset no longer covers what its name claims.
            missing = names - {p["name"] for p in parts}
            if missing:
                log(
                    f"preset {a.preset}: not on this firmware: {sorted(missing)}",
                    "WARN",
                )
            parts = [p for p in parts if p["name"] in names]
            log(f"preset {a.preset}: {[p['dev'] for p in parts]}")

    order = {n: i for i, n in enumerate(PRIORITY)}
    parts.sort(key=lambda p: order.get(p["name"], len(PRIORITY)))

    ip = host_ip()
    failed = []
    for p in parts:
        if not dump(p, ip):
            failed.append(p["dev"])
    manifest(parts)
    log(
        f"done. {len(parts) - len(failed)}/{len(parts)} captured"
        + (f", FAILED: {', '.join(failed)}" if failed else ""),
        "ERROR" if failed else "INFO",
    )
    sys.exit(1 if failed else 0)
