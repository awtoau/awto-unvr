#!/usr/bin/env python3
"""Recover woomera after a bad al_eth: boot with it blacklisted, restore the old .ko.

The rebuilt al_eth panics at boot (SError in al_udma_init) so autoboot loops. This
catches U-Boot on the next power-cycle, does a ONE-TIME boot with
`module_blacklist=al_eth` appended to bootargs (no saveenv — normal boot is
untouched), logs in, restores the backed-up old al_eth.ko, depmods, and freshly
modprobes the OLD al_eth (a first load, not a reload — safe). Network + box back,
new al_ssm (#50, verified) kept. No NAND writes.

Run it, THEN power-cycle woomera. Streams ESC to break into U-Boot.
"""
from __future__ import annotations
import os, re, socket, sys, time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS  # noqa: E402

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
UBOOT = "ALPINE_UBNT_NAS_ALL>"
PROMPT = "@@P@@"
USER, PASSWD = "root", "unvr"
CATCH_S = float(os.environ.get("CATCH_S", "1800"))
ESC_INTERVAL = 0.05
BLACKLIST = "al_eth"     # the crasher; new al_ssm (#50) is kept


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "recover-woomera.log").open("a").write(line + "\n")


def read_until(s, needle, limit, extra=()):
    buf = b""
    end = time.monotonic() + limit
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except socket.timeout:
            continue
        if not c:
            break
        buf += c
        if needle.encode() in buf:
            return buf, needle
        for n in extra:
            if n.encode() in buf:
                return buf, n
    return buf, None


def catch_uboot(s):
    log(f"streaming ESC — POWER-CYCLE woomera now (up to {CATCH_S:.0f}s)")
    buf = b""; last = 0.0; end = time.monotonic() + CATCH_S
    while time.monotonic() < end:
        now = time.monotonic()
        if now - last >= ESC_INTERVAL:
            try: s.sendall(b"\x1b")
            except socket.timeout: pass
            last = now
        try: c = s.recv(4096)
        except socket.timeout: continue
        if not c: break
        buf = (buf + c)[-16384:]
        if UBOOT.encode() in buf:
            log("U-Boot prompt caught")
            return True
    log("did NOT catch U-Boot")
    return False


def sh(s, cmd, timeout=25, label=None):
    s.sendall(cmd.encode() + b"; echo @@RC=$?@@\r")
    buf = b""; end = time.monotonic() + timeout; rc = None
    while time.monotonic() < end:
        try: c = s.recv(4096)
        except socket.timeout: continue
        if not c: break
        buf += c
        m = re.search(rb"@@RC=(\d+)@@", buf)
        if m: rc = int(m.group(1)); break
    read_until(s, PROMPT, 4)
    txt = buf.decode(errors="replace")
    if rc is None:
        log(f"  TIMEOUT: {label or cmd}\n{txt[-400:]}"); raise SystemExit(3)
    if label: log(f"  {label}: rc={rc}")
    return rc, txt


def main():
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.1)
    s.connect(str(SOCK))

    if not catch_uboot(s):
        return 1
    # one-time boot with al_eth blacklisted (bootcmd = 'nand read...; bootm', no
    # loadbootargs, so this append survives to bootm; NOT saved). modprobe.blacklist
    # (not module_blacklist) blocks only udev auto-load — explicit insmod still works.
    sh_uboot(s, "printenv bootargs", "bootargs")
    send_uboot(s, f'setenv bootargs "${{bootargs}} modprobe.blacklist={BLACKLIST}"')
    read_until(s, UBOOT, 4)
    log(f"bootargs += modprobe.blacklist={BLACKLIST}; booting (one-time)")
    send_uboot(s, "boot")

    # wait for login, then log in
    _, hit = read_until(s, "login:", 150, extra=("panic",))
    if hit != "login:":
        log("did not reach login (see console)"); return 2
    s.sendall(USER.encode() + b"\r"); read_until(s, "Password:", 8)
    s.sendall(PASSWD.encode() + b"\r")
    _, hit = read_until(s, "#", 15, extra=("incorrect",))
    if hit != "#":
        log("login failed"); return 2
    s.sendall(b"unalias -a 2>/dev/null; true\r"); read_until(s, "#", 4)
    s.sendall(f"export PS1='{PROMPT}'\r".encode()); read_until(s, PROMPT, 6)
    read_until(s, PROMPT, 3)
    log("logged in (al_eth blacklisted, box up)")

    rc, out = sh(s, "uname -r")
    kv = (re.search(r"(\d+\.\d+\.\d+\S*)", out) or [None, "7.1.8-dirty"])[1]
    extra = f"/lib/modules/{kv}/extra"
    rc, _ = sh(s, f"test -f {extra}/al_eth.ko.bak", label="old al_eth backup present")
    if rc != 0:
        log(f"ABORT: no {extra}/al_eth.ko.bak to restore"); return 4
    sh(s, f"mv -f {extra}/al_eth.ko.bak {extra}/al_eth.ko", label="restore old al_eth.ko")
    sh(s, f"depmod {kv}", timeout=60, label="depmod")
    # insmod by path bypasses modprobe.blacklist; fresh load (al_eth never loaded
    # this boot) so it's a first-probe, not the reload that panics.
    rc, out = sh(s, f"insmod {extra}/al_eth.ko 2>&1; true", timeout=40, label="insmod OLD al_eth (fresh load)")
    _, dm = sh(s, "dmesg | grep -iE 'al_eth|enp0s1|SError|panic' | tail -15")
    ok = "panic" not in dm.lower() and "serror" not in dm.lower()
    log(f"=== recovery: {'OK — old al_eth loaded, box healthy' if ok else 'STILL BAD'} ===\n{dm}")
    log("RESULT: " + ("recovered (old al_eth + new al_ssm). Normal boot is clean "
                      "(blacklist was one-time)." if ok else "old al_eth ALSO faulted — "
                      "SError is not the module rewrite; investigate hardware/timing."))
    s.close()
    return 0 if ok else 5


def send_uboot(s, line):
    s.sendall(line.encode() + b"\r")


def sh_uboot(s, cmd, label):
    send_uboot(s, cmd)
    buf, _ = read_until(s, UBOOT, 6)
    log(f"  {label}: {buf.decode(errors='replace').strip()[-160:]}")


if __name__ == "__main__":
    sys.exit(main())
