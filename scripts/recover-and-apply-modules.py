#!/usr/bin/env python3
"""Recover woomera (al_eth wedged, network down) and load the clean-built modules.

The box's network died from repeated al_eth reloads (mdiobus_register -22). This:
 1. watchdog-resets the box (SP805, the ONLY safe reset here — plain reboot hangs,
    #51) -> Fedora boots fresh, al_eth loads cleanly, network back;
 2. deploys the clean-built al_eth/al_ssm/al_dma .ko (iofic UBSAN + crypto fixes)
    to /lib/modules, sha-verified on-disk;
 3. watchdog-resets again -> the clean modules load on boot -> UBSAN gone;
 4. verifies UBSAN=0 + crypto pass + network up.
NAND untouched; kernel/DTB unchanged (this is modules-only).
"""
from __future__ import annotations
import hashlib, http.server, socketserver, sys, threading, time
sys.path.insert(0, "scripts")
import _console as con  # noqa: E402
from pathlib import Path

OUT = "/mnt/2tb/unvr-port-refs/build-out-71-fedora"
SEL, P = "192.168.25.145", 8094
MODS = ["al_eth", "al_ssm", "al_dma"]
WDT = (r'''python3 -c "import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); '''
       r'''fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')"''')


class Hnd(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, p): return OUT + "/" + p.lstrip("/")
    def log_message(self, *a): pass


def log(m): print(f"{time.strftime('%H:%M:%S')}  {m}", flush=True)


def watch_login(s, cap=240):
    buf = b""; end = time.monotonic() + cap
    while time.monotonic() < end:
        try: c = s.recv(4096)
        except Exception: continue
        if not c: continue
        buf += c
        if b"login:" in buf[-3000:]: return True
        if b"Kernel panic" in buf[-3000:]: return False
    return False


def wdt_reset(s, tag):
    log(f"{tag}: arming SP805 watchdog -> SoC reset")
    s.sendall(WDT.encode() + b"\r")
    if not watch_login(s):
        log(f"{tag}: FAILED to reach login after reset"); return False
    log(f"{tag}: booted to login"); con.login(s); return True


def main():
    lh = {m: hashlib.sha256(Path(f"{OUT}/{m}/{m}.ko").read_bytes()).hexdigest() for m in MODS}
    for m in MODS: log(f"clean {m}.ko = {lh[m][:12]}")
    s = con.connect(); con.login(s)

    if not wdt_reset(s, "reset-1"):
        s.close(); return 1
    r, net = con.sh(s, f"ping -c1 -W3 {SEL} >/dev/null 2>&1 && echo OK || echo NO", 20)
    log(f"network after reset-1: {net.strip()}")
    if "OK" not in net:
        log("still no network after fresh boot — cannot deploy"); s.close(); return 2

    httpd = socketserver.TCPServer(("0.0.0.0", P), Hnd)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    md = "/lib/modules/" + con.sh(s, "uname -r")[1].strip() + "/extra"
    ok = True
    for m in MODS:
        con.sh(s, f"curl -fsS http://{SEL}:{P}/{m}/{m}.ko -o {md}/{m}.ko", 60)
        r, h = con.sh(s, f"sha256sum {md}/{m}.ko", 30)
        good = lh[m] in h
        log(f"deploy {m}: {'OK' if good else 'MISMATCH'}")
        ok = ok and good
    httpd.shutdown()
    if not ok:
        log("deploy failed"); s.close(); return 3
    con.sh(s, "depmod -a; sync", 40)
    log("clean modules installed + depmod")

    if not wdt_reset(s, "reset-2"):
        s.close(); return 4
    def n(c): return con.sh(s, c, 30)[1].strip().splitlines()[-1]
    log("=== FINAL VERIFY (clean boot) ===")
    log("UBSAN (expect 0)      : " + n("dmesg | grep -c UBSAN"))
    log("iofic OOB (expect 0)  : " + n("dmesg | grep -c al_hal_iofic"))
    log("crypto FAIL (expect 0): " + n("dmesg | grep -ciE 'self-tests.*(cbc|xts).*failed'"))
    log("network               : " + n(f"ping -c1 -W3 {SEL} >/dev/null 2>&1 && echo UP || echo DOWN"))
    log("al_eth link           : " + n("ip -br link | grep -c UP"))
    s.close(); return 0


if __name__ == "__main__":
    sys.exit(main())
