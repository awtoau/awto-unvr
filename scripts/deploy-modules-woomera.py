#!/usr/bin/env python3
"""Deploy rebuilt al_* modules to woomera and live-reload them — no NAND, no reboot.

The #52 (al_eth UBSAN) and #50 (al_ssm crypto) fixes are in OUT-OF-TREE modules,
not the kernel Image/DTB — so no NAND reflash is needed. This logs into woomera
over the serial console (root/unvr), pulls the fresh .ko over HTTP from selwyn,
installs them into /lib/modules/<kv>/extra, depmods, then reloads al_ssm and
al_eth live and captures the verification lines:

  - al_ssm reload -> re-runs the XTS self-test. PASS = no "test failed" (#50).
    al_ssm IS reload-safe (crypto/DMA engine tears down cleanly).
  - al_eth is NOT reload-safe: re-register panics in al_eth_init (4 PCI funcs +
    internal-PCIe + UDMA). It's deployed to disk only and loads on the next boot,
    where UBSAN absence (#52) is verified from the boot log. The fix is a
    register-overlay type change (never allocated), so it can't panic at probe.

Console-only (works even though `reboot` hangs, #51). Reversible: old .ko are
kept as *.ko.bak on the device.

Prereqs: build-out present (scripts/build-linux-fedora.py), console socket up
(./dev.py console), woomera at the `fedora login:` prompt on ttyS0.
"""

from __future__ import annotations

import http.server
import re
import socketserver
import sys
import threading
from datetime import datetime, timezone
from functools import partial
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _fedora_deploy as fd

import _console
from _net import detect_server_ip
from _repo import LOGS

# fd.OUT/fd.KVER are the single source of truth (kernel_build_out() +
# the modroot's own real kernelrelease) - a hardcoded copy here silently
# drifted from the actual build-out path/kernel version once already.
EXTRA = fd.OUT / "modroot" / "lib" / "modules" / fd.KVER / "extra"
MODS = ["al_eth.ko", "al_ssm.ko"]  # the two with fixes; al_dma/al_sgpo unchanged
HTTP_PORT = 8099


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "deploy-modules-woomera.log").open("a").write(line + "\n")


def sh(s, cmd, timeout=20, label=None):
    """_console.sh with this script's two extras: a timeout is FATAL (a
    half-installed module tree must not be silently continued past), and an
    optional per-step label in the log."""
    rc, txt = _console.sh(s, cmd, timeout=timeout)
    if rc is None:
        log(f"  TIMEOUT: {label or cmd} ({timeout}s)\n{txt[-500:]}")
        raise SystemExit(3)
    if label:
        log(f"  {label}: rc={rc}")
    return rc, txt


def serve():
    """Serve EXTRA over HTTP in a background thread; return the server to stop."""
    handler = partial(http.server.SimpleHTTPRequestHandler, directory=str(EXTRA))
    httpd = socketserver.TCPServer(("0.0.0.0", HTTP_PORT), handler)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    return httpd


def main():
    if not _console.SOCK.exists():
        sys.exit(f"console socket absent: {_console.SOCK} — start ./dev.py console")
    for m in MODS:
        if not (EXTRA / m).exists():
            sys.exit(f"missing module {EXTRA / m} — run build-linux-fedora.py")

    selwyn_ip = detect_server_ip()
    log(f"dev host IP (auto-detected): {selwyn_ip}")

    httpd = serve()
    log(f"serving {EXTRA} on {selwyn_ip}:{HTTP_PORT}")
    s = _console.connect()
    try:
        try:
            _console.login(s)
        except RuntimeError as e:
            log(f"FAIL: {e}")
            raise SystemExit(2)
        log("logged in")

        rc, out = sh(s, "uname -r", label="kernel")
        kv_match = re.search(r"(\d+\.\d+\.\d+\S*)", out)
        if not kv_match:
            log(f"ABORT: couldn't parse kernel version from `uname -r` output: {out!r}")
            raise SystemExit(6)
        kv = kv_match.group(1)
        log(f"  device kver: {kv}")

        # reachability
        rc, _ = sh(s, f"ping -c1 -W2 {selwyn_ip} >/dev/null 2>&1", label="ping selwyn")
        if rc != 0:
            rc, out = sh(s, "ip -4 addr show scope global | grep inet")
            log(f"ABORT: woomera can't reach {selwyn_ip}. Its addrs:\n{out}")
            raise SystemExit(4)

        extra = f"/lib/modules/{kv}/extra"
        sh(s, f"mkdir -p {extra}", label="ensure extra dir")
        for m in MODS:
            url = f"http://{selwyn_ip}:{HTTP_PORT}/{m}"
            dst = f"{extra}/{m}"
            sh(s, f"cp -af {dst} {dst}.bak 2>/dev/null; true", label=f"backup {m}")
            rc, out = sh(
                s, f"curl -fsS {url} -o {dst}.new", timeout=40, label=f"fetch {m}"
            )
            if rc != 0:
                log(f"ABORT: fetch {m} failed:\n{out[-400:]}")
                raise SystemExit(5)
            sh(s, f"mv -f {dst}.new {dst}", label=f"install {m}")
        sh(s, f"ls -l {extra}/al_eth.ko {extra}/al_ssm.ko")
        sh(s, f"depmod {kv}", timeout=60, label="depmod")

        # --- verify al_ssm (#50): reload re-runs the XTS self-test ---
        sh(s, "dmesg -C", label="clear dmesg")
        sh(s, "modprobe -r al_ssm 2>/dev/null; true", label="rmmod al_ssm")
        rc, out = sh(
            s, "modprobe al_ssm 2>&1; true", timeout=30, label="modprobe al_ssm"
        )
        _, dm = sh(
            s,
            "dmesg | grep -iE 'al.?ssm|xts|skcipher|self-test' | tail -30",
            label="al_ssm dmesg",
        )
        ssm_fail = "test failed" in dm.lower() or "wrong result" in dm.lower()
        log(
            f"=== al_ssm (#50): {'STILL FAILING' if ssm_fail else 'no self-test failure seen'} ===\n{dm}"
        )

        # al_eth (#52): do NOT live-reload. al_eth is not rmmod/reprobe-safe —
        # re-register panics in al_eth_init -> __pci_register_driver (4 PCI funcs +
        # internal-PCIe + UDMA don't tear down cleanly). The new .ko is on disk and
        # depmod'd, so it loads at the NEXT boot; verify UBSAN is absent in the boot
        # log then. The fix is a register-overlay type change only (never allocated),
        # so it cannot panic at probe.
        log(
            "=== al_eth (#52): new module deployed to disk; NOT reloaded "
            "(al_eth is not reload-safe — panics on re-register). Loads on next "
            "boot; verify UBSAN absent in the boot log. ==="
        )

        log(
            "RESULT: "
            f"al_ssm #50 {'FAIL' if ssm_fail else 'OK (self-test clean on reload)'} ; "
            "al_eth #52 deployed — verify UBSAN-free on next boot"
        )
        return 0
    finally:
        httpd.shutdown()
        s.close()


if __name__ == "__main__":
    sys.exit(main())
