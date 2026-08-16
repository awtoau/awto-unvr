#!/usr/bin/env python3
"""Walk the UNVR up the firmware ladder, backing up between every hop.

Per hop: back up -> fetch+verify -> push -> signature-check -> flash -> reboot ->
wait -> confirm version -> confirm the video array -> back up again.

Stops dead on the first failure. Every hop is resumable: re-run and it skips
versions already installed.

Transport is TFTP (scripts/tftpd.py); commands go over the tio console socket.
No SSH. See docs/upgrade-path.md for why this ladder and not a direct jump.

Run: ./scripts/upgrade-ladder.py --dry-run      # show the plan
     ./scripts/upgrade-ladder.py --go
     ./scripts/upgrade-ladder.py --go --only 1.4.9
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import socket
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, REPO, SOURCES, rel  # noqa: E402

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
TFTP_PORT = 6969
TFTP_BLKSIZE = 1400
TFTP_ROOT = REPO / "images" / "tftp"

# Staging path on the device. /mnt/.rwfs is USERDEV (58 GB stick, 55 GB free);
# /tmp is tmpfs at 971 MB and the later images are ~790 MB, too close to fit.
STAGE_DIR = "/mnt/.rwfs/upgrade"

# Firmware signing public key, per THIS generation's scripts/ubnt (1.3.35):
#   FIRMWARE_PUB_KEY=/etc/ssl/unas.pub
# 5.1.25 renamed it to /etc/ssl/fw.pub - do not copy that path backwards.
PUB_KEY = "/etc/ssl/unas.pub"

# Vendor default console credentials, recorded in secrets.yaml (gitignored).
# The password changed across generations: 1.3.35/1.4.9 = "ubnt", 2.3.14 and
# every al324 build after = "ui" (verified against each firmware's baked-in
# /etc/shadow: $5$ SHA-256 crypt, cracked to "ui"). Try the newer one first.
LOGIN_USER = "root"
LOGIN_PASSWORDS = ["ui", "ubnt"]

# The ladder. Reasoning in docs/upgrade-path.md:
#   1.4.9  - crosses the arm64 -> al324 platform boundary alone
#   2.3.14 - the only release any community project pins exactly (UrNVR)
#   3.1.16 / 4.1.22 - late-in-line builds of each major
#   5.1.25 - current
LADDER = ["1.4.9", "2.3.14", "3.1.16", "4.1.22", "5.1.25"]

API = ("https://fw-update.ubnt.com/api/firmware"
       "?filter=eq~~platform~~UNVR&filter=eq~~channel~~release&sort=created&limit=200")

# --- timeouts, all derived ------------------------------------------------
# Console command echo: the shell replies in milliseconds; 3 s is ~1000x that.
CONSOLE_WAIT = 3.0
# A hop's TFTP push: 300-790 MB at the measured 3.1 MB/s is 95-255 s. 900 s is
# ~3.5x the worst case. On expiry the hop fails rather than hanging the ladder.
PUSH_WAIT = 900
# Flash + reboot + full UniFi OS boot. Observed cold boot to login prompt was
# ~85 s on 1.3.35; a flash adds NAND erase+write of kernel and rootfs. 600 s is
# generous and still bounded.
BOOT_WAIT = 600
# Poll interval while waiting for the box to come back. It is off the network
# for minutes; polling faster just burns cycles.
POLL = 5.0


def log(msg, level="INFO"):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {level:<5} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "upgrade-ladder.log").open("a") as fh:
        fh.write(line + "\n")


_SEQ = [0]


def console(cmd: str, wait: float = CONSOLE_WAIT) -> str:
    """Run one command and return its output, delimited by a unique sentinel.

    NOT a quiet-timeout read. The kernel logs to this console, and while the
    SFP link flaps it emits a message every ~7 s - a "read until the socket
    goes quiet" loop then never returns, which hung an entire upgrade hop.
    Instead: append `; echo <sentinel>$?`, read until the sentinel appears, and
    give up at a HARD deadline regardless of how chatty the console is.
    """
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK} - start ./dev.py console")
    _SEQ[0] += 1
    tag = f"__UL{_SEQ[0]}__"
    line = f"{cmd}; echo {tag}$?" if cmd else f"echo {tag}$?"

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(1.0)                     # poll interval, not the deadline
    s.connect(str(SOCK))
    s.sendall(line.encode() + b"\r")
    got = ""
    end = time.monotonic() + wait
    try:
        while time.monotonic() < end:
            try:
                chunk = s.recv(65536)
            except socket.timeout:
                continue
            if not chunk:
                break
            got += chunk.decode("utf-8", "replace")
            # The echoed command also contains the tag; require the form the
            # shell produces on execution (tag immediately followed by digits).
            if re.search(rf"{tag}(\d+)", got):
                break
    finally:
        s.close()
    return got


def console_raw(text: str, wait: float = 2.0, until: str | None = None) -> str:
    """Send a literal line with NO sentinel appended.

    console() appends `; echo <tag>$?` to everything. Right for a shell, wrong
    for a getty - there the sentinel gets typed as the username. That bug ran
    ~20 consecutive "Login incorrect" cycles against a box that had upgraded
    perfectly well: the check never authenticated, so it never read a version,
    so a successful hop was reported as a failure.
    """
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK} - start ./dev.py console")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(1.0)
    s.connect(str(SOCK))
    s.sendall(text.encode() + b"\r")
    got = ""
    end = time.monotonic() + wait
    try:
        while time.monotonic() < end:
            try:
                chunk = s.recv(65536)
            except socket.timeout:
                continue
            if not chunk:
                break
            got += chunk.decode("utf-8", "replace")
            if until and until in got:
                break
    finally:
        s.close()
    return got


def login() -> bool:
    """Authenticate at the getty if one is prompting. True once a shell answers.

    Proof of success is a sentinel command completing, not the absence of
    "Login incorrect" - the getty reprints its banner on a timer, so matching
    on prompt text alone races with it.
    """
    # Already at a shell? (getty not prompting.) Confirm and return.
    if console_rc(console("true", wait=10.0)) == 0:
        return True
    for pw in LOGIN_PASSWORDS:
        out = console_raw("", wait=3.0)
        if "login:" not in out.lower():
            # A partial prompt or a stale password line - nudge with a newline.
            out = console_raw("", wait=3.0)
        if "login:" in out.lower():
            console_raw(LOGIN_USER, wait=5.0, until="assword")
            console_raw(pw, wait=10.0)
        if console_rc(console("true", wait=10.0)) == 0:
            log(f"  shell available (root:{pw})")
            return True
    log("  could not authenticate at console", "ERROR")
    return False


def console_rc(out: str) -> int | None:
    """Exit status carried back by the sentinel, or None if it never arrived."""
    m = re.findall(r"__UL\d+__(\d+)", out)
    return int(m[-1]) if m else None


def quiet_console() -> None:
    """Stop kernel messages spamming the serial console.

    The SFP link-manager logs on every up/down transition; during a flash that
    noise both obscures fwupdate's own output and slows every read. dmesg -n 1
    leaves only emergency messages. Reverted implicitly by the next reboot.
    """
    console("dmesg -n 1", wait=10.0)
    log("  console kernel logging set to emergency-only for the flash")


def api_rows() -> list[dict]:
    """Release list, via playwrong-equivalent plain fetch is NOT used here -
    the project rule routes web access through playwrong. This reads a cached
    copy written by the caller, or fetches with curl only if the user has
    already approved that for this repo (they did, for firmware downloads)."""
    cache = SOURCES / "unvr-releases.json"
    if cache.exists():
        return json.loads(cache.read_text())["_embedded"]["firmware"]
    SOURCES.mkdir(parents=True, exist_ok=True)
    p = subprocess.run(["curl", "-fsSL", "--max-time", "60", API],
                       capture_output=True, text=True)
    if p.returncode != 0:
        sys.exit(f"could not fetch the release list: {p.stderr.strip()}")
    cache.write_text(p.stdout)
    return json.loads(p.stdout)["_embedded"]["firmware"]


def pick(rows: list[dict], version: str) -> dict:
    for r in rows:
        if r["version"].lstrip("v").split("+")[0] == version:
            return r
    sys.exit(f"version {version} not in the release list")


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as fh:
        for b in iter(lambda: fh.read(4 << 20), b""):
            h.update(b)
    return h.hexdigest()


def fetch(row: dict) -> Path:
    ver = row["version"].lstrip("v").split("+")[0]
    dest = SOURCES / f"UNVR-{ver}.bin"
    want = row["sha256_checksum"]
    if dest.exists() and dest.stat().st_size == row["file_size"]:
        if sha256(dest) == want:
            log(f"  already have {rel(dest)}, sha256 OK")
            return dest
        log("  cached copy failed sha256 - refetching", "WARN")
        dest.unlink()
    url = row["_links"]["data"]["href"]
    log(f"  GET {url}")
    p = subprocess.run(["curl", "-fL", "--progress-bar", "--speed-limit", "10240",
                        "--speed-time", "30", "--max-time", "3600", "-o", str(dest), url])
    if p.returncode != 0:
        dest.unlink(missing_ok=True)
        sys.exit(f"download failed for {ver}")
    got = sha256(dest)
    if got != want:
        dest.unlink()
        sys.exit(f"sha256 MISMATCH for {ver}: got {got}, want {want}")
    log(f"  {dest.stat().st_size} B, sha256 verified")
    return dest


def device_version() -> str | None:
    out = console("cat /usr/lib/version 2>/dev/null || cat /etc/version 2>/dev/null", wait=3.0)
    m = re.search(r"UNVR4\.\w+\.v(\d+\.\d+\.\d+)", out)
    return m.group(1) if m else None


def backup(tag: str) -> bool:
    log(f"  backup: --preset state --tag {tag}")
    p = subprocess.run([sys.executable, str(REPO / "scripts" / "dump-unvr-mtd.py"),
                        "--preset", "state", "--tag", tag], cwd=REPO)
    return p.returncode == 0


def ensure_tftpd() -> subprocess.Popen | None:
    r = subprocess.run(["ss", "-lun", "sport = :%d" % TFTP_PORT],
                       capture_output=True, text=True)
    if "UNCONN" in r.stdout:
        return None                      # already listening
    log("  starting tftpd")
    TFTP_ROOT.mkdir(parents=True, exist_ok=True)
    proc = subprocess.Popen([sys.executable, str(REPO / "scripts" / "tftpd.py"),
                             "--root", "images/tftp", "--port", str(TFTP_PORT)],
                            cwd=REPO, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            start_new_session=True)
    for _ in range(40):
        r = subprocess.run(["ss", "-lun", "sport = :%d" % TFTP_PORT],
                           capture_output=True, text=True)
        if "UNCONN" in r.stdout:
            return proc
        time.sleep(0.05)
    sys.exit("tftpd did not bind")


def host_ip() -> str:
    out = console("ip -4 addr show", wait=3.0)
    cands = [ip for ip in re.findall(r"inet (\d+\.\d+\.\d+\.\d+)/", out)
             if not ip.startswith(("127.", "169.254."))]
    if not cands:
        sys.exit(f"no routable IPv4 on the device:\n{out}")
    dev = cands[0]
    p = subprocess.run(["ip", "-o", "route", "get", dev], capture_output=True, text=True)
    m = re.search(r"\bsrc (\d+\.\d+\.\d+\.\d+)", p.stdout)
    if not m:
        sys.exit(f"no route to {dev}")
    return m.group(1), dev


def wait_for_boot(expect: str) -> bool:
    """Poll the console until the box reports the expected version."""
    log(f"  waiting for reboot into {expect} (limit {BOOT_WAIT}s)")
    end = time.monotonic() + BOOT_WAIT
    seen_login = False
    while time.monotonic() < end:
        time.sleep(POLL)
        out = console_raw("", wait=3.0)
        if "login:" in out.lower():
            if not seen_login:
                seen_login = True
                log("  login prompt is back")
            if not login():
                continue
        v = device_version()
        if v == expect:
            log(f"  device reports {v}")
            return True
        if v:
            log(f"  device reports {v}, waiting for {expect}")
    return False


def last_kernel_banner() -> str | None:
    """Newest `Linux version ...` line from the console log.

    Read with errors="replace": the log carries raw serial line noise, and the
    same binary bytes make plain `grep` skip the file entirely unless given -a.
    """
    p = REPO / "tmp" / "logs" / "unvr-console.log"
    if not p.exists():
        return None
    lines = p.read_text(errors="replace").splitlines()
    # U-Boot's "Image Name:" is the reliable one - it prints every boot and names
    # the build date of the kernel actually in NAND. The kernel's own "Linux
    # version" is emitted before the console is up and often never reaches the log.
    for key in ("Image Name:", "Linux version"):
        hits = [ln.strip() for ln in lines if key in ln]
        if hits:
            return hits[-1][:160]
    return None


def volume_ok() -> bool:
    out = console("df -h /volume1 2>/dev/null | tail -1", wait=3.0)
    if "/volume1" not in out:
        log("  /volume1 NOT MOUNTED", "ERROR")
        return False
    log(f"  /volume1: {out.strip().splitlines()[-2] if len(out.splitlines()) > 1 else out.strip()}")
    return True


def hop(row: dict, ver: str) -> bool:
    log(f"=== hop -> {ver} ===")
    if not backup(f"pre-{ver}"):
        log("  pre-hop backup FAILED - stopping", "ERROR")
        return False
    img = fetch(row)

    ensure_tftpd()
    hostip, devip = host_ip()
    log(f"  device {devip}, host {hostip}")

    served = TFTP_ROOT / img.name
    if not served.exists():
        served.hardlink_to(img) if hasattr(served, "hardlink_to") else os.link(img, served)

    console(f"mkdir -p {STAGE_DIR} && rm -f {STAGE_DIR}/fw-image.bin", wait=3.0)
    log(f"  pushing {img.name} ({img.stat().st_size} B) - limit {PUSH_WAIT}s")
    console(f"cd {STAGE_DIR} && tftp -b {TFTP_BLKSIZE} -g -l fw-image.bin "
            f"-r {img.name} {hostip} {TFTP_PORT}", wait=1.0)

    end = time.monotonic() + PUSH_WAIT
    want = img.stat().st_size
    while time.monotonic() < end:
        time.sleep(POLL)
        out = console(f"stat -c %s {STAGE_DIR}/fw-image.bin 2>/dev/null", wait=3.0)
        m = re.search(r"^(\d+)$", out.strip().splitlines()[-2] if len(out.strip().splitlines()) > 1 else "", re.M)
        sizes = [int(x) for x in re.findall(r"\b(\d{6,})\b", out)]
        if sizes and max(sizes) >= want:
            log(f"  pushed {max(sizes)} B")
            break
    else:
        log("  push did not complete in time", "ERROR")
        return False

    quiet_console()

    local_md5 = hashlib.md5(img.read_bytes()).hexdigest()
    out = console(f"md5sum {STAGE_DIR}/fw-image.bin", wait=300.0)
    if local_md5 not in out:
        log(f"  MD5 MISMATCH on device (want {local_md5})", "ERROR")
        return False
    log(f"  md5 {local_md5} matches on device")

    # Verify with the tool THIS firmware actually uses.
    #
    # Do NOT use `ubnt-tools fwupdate -dc` here: that is the 5.1.25 initramfs's
    # invocation. On 1.3.35 fwupdate is v1.1 and accepts only -d and -h, so -c
    # is an unrecognised option and it exits 254 with NO output - which reads
    # exactly like a signature failure and is not one.
    #
    # This generation's own scripts/ubnt.nand does:
    #   fwsplit -s /etc/ssl/unas.pub -o <prefix> <image>
    # Note -o is a PREFIX: it writes <prefix>.txt/.kernel/.rootfs, not a dir.
    # Verification and extraction are the same step, so a clean exit proves the
    # signature. We extract to tmpfs then delete - /tmp is 971 MB and the rootfs
    # section alone is ~300 MB, so leaving it there would matter.
    # Pre-verify ONLY if this generation keeps the signing key in the rootfs.
    #
    # 1.3.35 has /etc/ssl/unas.pub and fwsplit verifies against it. al324 builds
    # (1.4.9+) have no firmware key in the rootfs at all - it ships inside the
    # initramfs - so `fwsplit -s <missing>` prints its usage and exits 255,
    # which is indistinguishable from a bad signature. That false alarm stopped
    # the 2.3.14 hop on an image that was perfectly good.
    #
    # Skipping it loses nothing: the download is sha256-checked, the staged copy
    # md5-checked on the device, and the initramfs verifies the signature with
    # its own key before writing a single block.
    if "K=0" in console(f"test -f {PUB_KEY}; echo K=$?", wait=30.0):
        log(f"  pre-verifying with fwsplit against {PUB_KEY}")
        out = console(f"rm -f /tmp/.fwchk.*; fwsplit -s {PUB_KEY} -o /tmp/.fwchk "
                      f"{STAGE_DIR}/fw-image.bin 2>&1; echo CHK=$?", wait=300.0)
        for line in out.splitlines():
            if line.strip() and not line.startswith("__UL"):
                log(f"    | {line.rstrip()}")
        if "CHK=0" not in out:
            log("  SIGNATURE CHECK FAILED - not flashing", "ERROR")
            return False
        console("rm -f /tmp/.fwchk.*", wait=30.0)
        log("  signature verified")
    else:
        log(f"  no {PUB_KEY} in rootfs - al324 keeps it in the initramfs, "
            "which verifies before writing")

    # Hand the flash to the vendor's own path rather than driving nandwrite
    # ourselves. `mount_premount` finds ${MNT_RWFS}/upgrade/fw-image.bin before
    # UniFi OS starts, runs this generation's upgrade_firmware (fwsplit ->
    # nandwrite, including the 4-byte LE length prefix the kernel partition
    # needs), removes the image and reboots. The file is already staged and
    # md5-verified, so all that remains is to restart.
    log("  handing off to the device's own upgrade path; rebooting")
    console("sync; sync", wait=30.0)
    console("reboot", wait=5.0)

    if not wait_for_boot(ver):
        log(f"  did not come back as {ver} within {BOOT_WAIT}s", "ERROR")
        # Say WHICH kernel is running. A timeout means either the flash did not
        # apply or the version read failed, and those need opposite responses -
        # guessing wrong cost a whole hop on 1.4.9, which had upgraded fine.
        log(f"  last kernel banner seen: {last_kernel_banner() or 'none in log'}", "ERROR")
        return False
    # Advisory only. overlay_cleanup runs on every upgrade boot, and a missing
    # /volume1 is not a reason to abandon a ladder that is otherwise healthy.
    volume_ok()
    if not backup(f"post-{ver}"):
        log("  post-hop backup FAILED", "ERROR")
        return False
    log(f"=== {ver} OK ===")
    return True


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--go", action="store_true", help="actually do it")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--only", help="single version, e.g. 1.4.9")
    a = ap.parse_args()

    rows = api_rows()
    # Authenticate first. Reading a version at a getty prompt returns nothing
    # and looks identical to "the upgrade did not apply".
    login()
    cur = device_version()
    log(f"device currently reports {cur or 'UNKNOWN'}")

    def ver_key(v):
        return tuple(int(x) for x in v.split("."))

    if a.only:
        plan = [a.only]
    else:
        # ONLY hop forward. The default plan is the ladder, but the loop below
        # once started at LADDER[0] and tried to DOWNGRADE 3.1.16 -> 1.4.9.
        # That is not a no-op: with the fdt-rm layout, 1.4.9's initramfs reads
        # rootfs from the wrong mtd and boot-loops. Cut everything at or below
        # the running version.
        if not cur:
            sys.exit("cannot read current version; refusing to guess a plan")
        plan = [v for v in LADDER if ver_key(v) > ver_key(cur)]

    # A downgrade via --only is almost never intended; make it deliberate.
    if a.go and cur and any(ver_key(v) < ver_key(cur) for v in plan):
        sys.exit(f"refusing to downgrade from {cur} to {plan}. "
                 "Downgrading past the al324 boundary boot-loops on this unit.")

    if not plan:
        log(f"already at or past the ladder top ({cur}); nothing to do")
        sys.exit(0)

    if not a.go:
        log("PLAN:")
        for v in plan:
            r = pick(rows, v)
            log(f"  {v:8s} {r['file_size']:>10} B  {r['tags']['ubnt_version']}")
        log("re-run with --go")
        sys.exit(0)

    for v in plan:
        if cur == v:
            log(f"already on {v}, skipping")
            continue
        if not hop(pick(rows, v), v):
            log(f"LADDER STOPPED at {v}", "FATAL")
            sys.exit(1)
        cur = v
    log("ladder complete", "INFO")
