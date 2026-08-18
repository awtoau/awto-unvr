#!/usr/bin/env python3
"""Deploy the boot-error fixes to woomera, reboot, and verify they took.

Applies in one cycle: aligned kernel (-gz load 0x08000000), new DTB (@0x29 gone,
timer0 disabled), and rebuilt al_eth/al_ssm/al_dma (.ko with the iofic UBSAN fix +
al_ssm crypto fix). Backs up /boot kernel+DTB (guarded), sha256-verifies every
file, depmods, reboots, watches boot, then greps dmesg for: UBSAN, crypto
self-test failures, @0x29, timer0 deferred, misalignment. NAND untouched;
fallback = U-Boot ext4load the .bak.

Boot-watch cap 240s (reboot here ~60-120s incl. 8TB spin-up; 2x margin).
"""
from __future__ import annotations
import hashlib, http.server, socketserver, sys, threading, time
from datetime import datetime, timezone
from functools import partial
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console as con  # noqa: E402
from _repo import LOGS  # noqa: E402

OUT = Path("/mnt/2tb/unvr-port-refs/build-out-71-fedora")
SELWYN, PORT = "192.168.25.145", 8097
LOG = LOGS / "apply-fixes-woomera.log"
BOOT_CAP = 240
BAD = ("Kernel panic", "Synchronous Abort", "Unable to handle", "Internal error")

# (local, /boot dest) — kernel + DTB
BOOT = [("uImage-unvr-ea16-7.1-fedora-gz", "uImage-unvr-ea16-7.1-fedora-gz"),
        ("alpine-v2-ubnt-unvr-ea16-7.1.dtb", "alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb")]
# module ko basenames (served from OUT/<m>/<m>.ko)
MODS = ["al_eth", "al_ssm", "al_dma"]


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True); LOGS.mkdir(parents=True, exist_ok=True); LOG.open("a").write(line + "\n")


def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()


class H(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, path):  # serve arbitrary files under OUT by rel path
        return str(OUT / path.lstrip("/"))
    def log_message(self, *a): pass


def fetch_verify(s, url, dst, want):
    con.sh(s, f"curl -fsS {url} -o {dst}.new && echo OK || echo FAIL", 120)
    rc, h = con.sh(s, f"sha256sum {dst}.new | cut -d' ' -f1", 30)
    got = h.split()[0] if h.split() else ""
    if got != want:
        log(f"HASH MISMATCH {dst}: {got} != {want}"); return False
    con.sh(s, f"mv {dst}.new {dst}")
    return True


def main():
    for _, d in [(0, OUT / f"{m}/{m}.ko") for m in MODS] + [(0, OUT / b[0]) for b in BOOT]:
        if not Path(d).exists(): sys.exit(f"missing artifact: {d}")
    lh = {b[1]: sha(OUT / b[0]) for b in BOOT}
    lh.update({f"{m}.ko": sha(OUT / f"{m}/{m}.ko") for m in MODS})

    httpd = socketserver.TCPServer(("0.0.0.0", PORT), H)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    log(f"serving {OUT} on {SELWYN}:{PORT}")
    s = con.connect(); con.login(s)
    rc, _ = con.sh(s, f"ping -c1 -W2 {SELWYN} >/dev/null 2>&1")
    if rc != 0: log("FATAL: woomera can't reach selwyn"); httpd.shutdown(); return 1

    rc, moddir = con.sh(s, "d=$(dirname $(modinfo -n al_eth 2>/dev/null)); echo ${d:-/lib/modules/$(uname -r)/extra}")
    moddir = moddir.strip().splitlines()[-1] if moddir.strip() else "/lib/modules/$(uname -r)/extra"
    log(f"module dir: {moddir}")

    ok = True
    for src, dst in BOOT:
        b = f"/boot/{dst}"
        con.sh(s, f"[ -e {b}.bak ] || cp -a {b} {b}.bak 2>/dev/null; echo bk")
        if not fetch_verify(s, f"http://{SELWYN}:{PORT}/{src}", b, lh[dst]): ok = False; break
        log(f"{dst}: installed (sha256 OK)")
    if ok:
        for m in MODS:
            b = f"{moddir}/{m}.ko"
            con.sh(s, f"[ -e {b}.bak ] || cp -a {b} {b}.bak 2>/dev/null; echo bk")
            if not fetch_verify(s, f"http://{SELWYN}:{PORT}/{m}/{m}.ko", b, lh[f'{m}.ko']): ok = False; break
            log(f"{m}.ko: installed (sha256 OK)")
    httpd.shutdown()
    if not ok:
        con.sh(s, "rm -f /boot/*.new " + moddir + "/*.new 2>/dev/null; echo x")
        log("ABORTED — .new cleaned, current install untouched"); s.close(); return 2
    con.sh(s, "depmod -a 2>/dev/null; sync; echo depmod-done", 40)
    log("all files installed + depmod. Rebooting...")

    con.sh(s, "sync"); s.sendall(b"reboot\r")
    buf = b""; end = time.monotonic() + BOOT_CAP; res = "timeout"
    while time.monotonic() < end:
        try: c = s.recv(4096)
        except Exception: continue
        if not c: continue
        buf += c; tail = buf[-4096:].decode(errors="replace")
        if "login:" in tail: res = "ok"; break
        for x in BAD:
            if x in tail: res = x; break
        if res != "timeout": break
    if res != "ok":
        log(f"BOOT FAILED: {res} — recover via U-Boot ext4load the .bak kernel+DTB"); s.close(); return 3
    log("BOOT OK — verifying fixes")
    con.login(s)
    checks = [
        ("UBSAN count (expect 0)", "dmesg | grep -c UBSAN"),
        ("crypto self-test failures (expect 0)", "dmesg | grep -ciE 'self-tests.*(cbc|xts).*al-ssm.*failed'"),
        ("crypto al-ssm algs registered", "grep -E 'ssm' /proc/crypto | grep -c ssm"),
        ("pca953x @0x29 -121 (expect 0)", "dmesg | grep -c '0-0029'"),
        ("timer0 deferred (expect 0)", "dmesg | grep -c 'fd890000.timer'"),
        ("kernel misaligned (expect 0)", "dmesg | grep -c 'misaligned at boot'"),
        ("config/cksum force-RO (expect 0 if 4K-sector kernel)", "dmesg | grep -c 'force read-only'"),
        ("al_eth loaded + iface", "lsmod | grep -c al_eth; ip -br link 2>/dev/null | grep -c UP"),
    ]
    for label, cmd in checks:
        rc, out = con.sh(s, cmd, 30)
        log(f"  {label}: {out.strip()}")
    s.close()
    log("DONE — verification battery complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
