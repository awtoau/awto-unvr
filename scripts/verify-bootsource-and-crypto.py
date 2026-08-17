#!/usr/bin/env python3
"""One reboot: read the U-Boot bootcmd (resolve WHERE the kernel loads from), then
boot and TEST that al_ssm crypto is actually gone (blacklisted). No assertions.

Catches the stock U-Boot via SP805 watchdog reset (the only safe reset, #51),
prints bootcmd + the kernel/dtb load lines, then `boot`s and verifies on the
running Fedora: al_ssm NOT loaded + zero crypto self-test failures in dmesg.
"""
from __future__ import annotations
import socket, sys, time
sys.path.insert(0, "scripts")
import _console as con  # noqa: E402

UB = "ALPINE_UBNT_NAS_ALL>"
WDT = (r'''python3 -c "import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); '''
       r'''fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')"''')


def log(m): print(f"{time.strftime('%H:%M:%S')}  {m}", flush=True)


def read_until(s, needle, cap):
    buf = b""; end = time.monotonic() + cap
    while time.monotonic() < end:
        try: c = s.recv(4096)
        except socket.timeout: continue
        except Exception: continue
        if c: buf += c
        if needle.encode() in buf: return buf, True
    return buf, False


def main():
    s = con.connect(); con.login(s)
    log("arming watchdog -> reset, then spamming ESC to catch stock U-Boot")
    s.sendall(WDT.encode() + b"\r")
    caught = False
    end = time.monotonic() + 90
    while time.monotonic() < end:
        s.sendall(b"\x1b")
        buf, hit = read_until(s, UB, 0.3)
        if hit: caught = True; break
    if not caught:
        log("did NOT catch stock U-Boot (may have booted past). Aborting."); s.close(); return 1
    log("AT STOCK U-BOOT")

    # THE question: where does the kernel/DTB come from?
    s.sendall(b"printenv bootcmd\r"); b, _ = read_until(s, UB, 8)
    txt = b.decode(errors="replace")
    for line in txt.splitlines():
        if "bootcmd=" in line or "nand" in line.lower() or "ext4load" in line or "fatload" in line or "scsi" in line.lower():
            log("BOOTCMD> " + line.strip())
    s.sendall(b"printenv baudrate\r"); read_until(s, UB, 5)

    # let it boot to Fedora
    log("booting...")
    s.sendall(b"boot\r")
    b, ok = read_until(s, "login:", 240)
    if not ok:
        log("did not reach Fedora login after boot"); s.close(); return 2
    con.login(s)

    def sh(c, to=30): return con.sh(s, c, to)[1].strip().splitlines()[-1] if con.sh(s, c, to)[1].strip() else ""
    log("=== TESTED (booted) crypto-removal ===")
    log("al_ssm loaded? (expect 0)       : " + con.sh(s, "lsmod | grep -c '^al_ssm'")[1].strip())
    log("crypto self-test FAIL (expect 0): " + con.sh(s, "dmesg | grep -ciE 'self-tests.*(cbc|xts).*failed'")[1].strip())
    log("al_ssm blacklist in effect      : " + con.sh(s, "cat /etc/modprobe.d/al_ssm-parked.conf 2>/dev/null | grep -c blacklist")[1].strip())
    log("UBSAN (expect 0)                : " + con.sh(s, "dmesg | grep -c UBSAN")[1].strip())
    s.close(); return 0


if __name__ == "__main__":
    sys.exit(main())
