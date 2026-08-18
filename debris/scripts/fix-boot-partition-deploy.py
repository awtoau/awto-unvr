#!/usr/bin/env python3
"""Deploy kernel+DTB to the REAL /boot (ext4 root), not the ESP automount.

Root cause of "fixes don't take": sda1 (200M vfat ESP) is auto-mounted at /boot as
an autofs (systemd-gpt-auto-generator), shadowing the ext4 root's /boot. But U-Boot
does `ext4load scsi 0:2 /boot/...` = reads sda2 (ext4 root)'s /boot directory. So
deploys written while Linux runs land in the ESP, never seen by U-Boot.

Fix: stop+mask the /boot automount so /boot = the ext4 dir U-Boot reads, deploy the
aligned kernel + fixed DTB there (backup + sha-verify), then watchdog-reset. Verify
misalign/@0x29/timer0 gone. Modules already correct on the ext4 root.
"""
from __future__ import annotations
import hashlib, http.server, socketserver, sys, threading, time
sys.path.insert(0, "scripts")
import _console as con  # noqa: E402
from pathlib import Path

OUT = "/mnt/2tb/unvr-port-refs/build-out-71-fedora"
SEL, P = "192.168.25.145", 8093
FILES = [("uImage-unvr-ea16-7.1-fedora-gz", "uImage-unvr-ea16-7.1-fedora-gz"),
         ("alpine-v2-ubnt-unvr-ea16-7.1.dtb", "alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb")]
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
    return False


def main():
    lh = {d: hashlib.sha256(Path(f"{OUT}/{sfn}").read_bytes()).hexdigest() for sfn, d in FILES}
    s = con.connect(); con.login(s)
    def sh(c, to=40): return con.sh(s, c, to)[1].strip()

    log("BEFORE: /boot is " + sh("findmnt -no FSTYPE,SOURCE /boot 2>/dev/null || echo '(unmounted)'"))
    # stop + mask the ESP automount so /boot = the ext4 root dir U-Boot reads
    sh("systemctl stop boot.automount boot.mount 2>/dev/null; umount /boot 2>/dev/null; "
       "systemctl mask boot.automount 2>/dev/null; echo done")
    log("AFTER stop+mask: /boot is " + sh("findmnt -no FSTYPE,SOURCE /boot 2>/dev/null || echo '(now the ext4 root dir)'"))
    log("real /boot old kernel sha: " + sh("sha256sum /boot/uImage-unvr-ea16-7.1-fedora-gz 2>/dev/null | cut -c1-16 || echo MISSING")
        + "  (want new: " + lh["uImage-unvr-ea16-7.1-fedora-gz"][:16] + ")")

    httpd = socketserver.TCPServer(("0.0.0.0", P), Hnd)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    ok = True
    for sfn, d in FILES:
        b = f"/boot/{d}"
        sh(f"[ -e {b}.realbak ] || cp -a {b} {b}.realbak 2>/dev/null; echo bk")
        sh(f"curl -fsS http://{SEL}:{P}/{sfn} -o {b}")
        good = lh[d] in sh(f"sha256sum {b}")
        log(f"deploy->real /boot {d}: {'OK' if good else 'MISMATCH'}")
        ok = ok and good
    httpd.shutdown()
    if not ok:
        log("deploy failed — not rebooting"); s.close(); return 1
    sh("sync")
    log("real /boot updated + synced. watchdog reset...")
    s.sendall(WDT.encode() + b"\r")
    if not watch_login(s):
        log("did not reach login after reset — recover via U-Boot .realbak"); s.close(); return 2
    con.login(s)
    log("=== VERIFY (booted from the real /boot now) ===")
    log("misaligned (expect 0): " + sh("dmesg | grep -c 'misaligned at boot'"))
    log("gpio@29    (expect 0): " + sh("dmesg | grep -c '0-0029'"))
    log("timer0 dfr (expect 0): " + sh("dmesg | grep -c 'fd890000.timer'"))
    log("config RO  (expect 0): " + sh("dmesg | grep -c 'force read-only'"))
    log("UBSAN      (expect 0): " + sh("dmesg | grep -c UBSAN"))
    log("crypto FAIL(0 or 1xts): " + sh("dmesg | grep -ciE 'self-tests.*(cbc|xts).*failed'"))
    s.close(); return 0


if __name__ == "__main__":
    sys.exit(main())
