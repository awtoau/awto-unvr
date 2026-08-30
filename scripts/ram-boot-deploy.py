#!/usr/bin/env python3
"""Boot a kernel+DTB (and optionally inject a matching module tree) via RAM,
through stock U-Boot, entirely from this dev host - no NAND flash, no SSH
required to get there. Built to replace the ~8-step manual ritual (reset,
setenv, 3x tftpboot, bootm, wait, dd-extract, verify) that was hand-typed
dozens of times during the #131 KASAN investigation, each repetition risking
a fresh mistake (wrong byte count, stale tftpd root, stray buffered input).

Why RAM and not NAND: the KASAN diagnostic kernel is ~40MB compressed,
comfortably over the 18.9MiB NAND kernel partition span (see flash-nand.py) -
this is the general-purpose "test an oversized or throwaway kernel" path,
not just a #131-specific tool.

Usage:
    ./dev.py ram-boot-deploy --kernel PATH --dtb PATH \
        [--modules-tar PATH --kver KVER]

Requires:
    - Box on the same LAN, reachable via the Sonoff TH smart outlet
      (so-th-1.local) for remote power-cycling - see
      docs (memory: unvr-power-cycle-via-hass) for the direct API details.
    - ./dev.py console running (this script does NOT start it).
"""

from __future__ import annotations

import argparse
import hashlib
import os
import socket
import struct
import subprocess
import sys
import tarfile
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tftpd as _tftpd
from _net import UNVR_IPADDR as IPADDR
from _net import detect_server_ip
from _power import power_cycle_verified
from _repo import LOGS, REPO, TFTP_ROOT, log_path

LOG = log_path("ram-boot-deploy")

# RAM addresses - chosen and verified during the #131 session to avoid two
# real collisions hit that night: the kernel's own DECOMPRESSED span (not
# just its compressed tftp size) can be huge (155MB+ for a KASAN build) and
# will silently corrupt whatever else sits in that range mid-boot. Kernel
# decompresses to 0x08000000; giving everything else a wide berth above the
# compressed image's own load address is what actually avoids this, not
# just avoiding the DTB's traditional low address.
KERNEL_ADDR = "0x20000000"
DTB_ADDR = "0x30000000"
MODULES_ADDR = "0xa0000000"  # 2.5GB - well clear of early kernel allocations,
# confirmed safe via /proc/iomem showing System RAM 0x0-0xBFFFFFFF as one bank

def log(msg: str, level: str = "INFO") -> None:
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {level:5s} {msg}"
    print(line)
    with LOG.open("a") as fh:
        fh.write(line + "\n")


def run_devpy(*args: str, timeout: float = 30.0) -> str:
    """One ./dev.py console-send call. Raises on failsafe timeout (the
    '<<MATCHED' marker line is how we know it actually worked, not just
    that the subprocess exited 0)."""
    cmd = ["./dev.py", "console-send", *args]
    log(f"run: {' '.join(cmd)}")
    p = subprocess.run(
        cmd, cwd=REPO, capture_output=True, text=True, timeout=timeout + 10
    )
    out = p.stdout + p.stderr
    if "<<MATCHED" not in out and "--expect" in " ".join(args):
        log(f"no match, output tail:\n{out[-2000:]}", "ERROR")
        raise RuntimeError(f"console-send did not match: {' '.join(args)}")
    return out


# power_cycle_verified() now lives in scripts/_power.py, shared with the
# standalone `./dev.py power-cycle` entry point - was a local copy here,
# duplicated a second time as an inline python3 -c snippet elsewhere in the
# same session; one copy now, imported above.

# Embedded tftpd, in-process (not a separate sudo'd subprocess). Reuses
# tftpd.py's own RRQ/WRQ handlers directly rather than reimplementing TFTP -
# one process means one thing to go stale, not two: a standalone tftpd
# subprocess bound to a DIFFERENT root (from an earlier, unrelated command)
# silently serving the wrong file was a repeat, real mistake tonight (#119).
# rrq_seen[filename] = monotonic() timestamp of the most recent RRQ for that
# file - lets the caller detect "box never even reached us" (a real network
# problem) within a couple of seconds, instead of only finding out after
# U-Boot's own ~55s internal retry-count timeout expires.
_rrq_lock = threading.Lock()
rrq_seen: dict[str, float] = {}


def _serve_forever(sock: socket.socket, root: Path) -> None:
    while True:
        try:
            data, addr = sock.recvfrom(65536)
        except OSError:
            return
        opcode, filename, _mode, opts = _tftpd.parse_request(data)
        if opcode == _tftpd.RRQ:
            with _rrq_lock:
                rrq_seen[filename] = time.monotonic()
            log(f"tftpd: RRQ {filename!r} from {addr[0]}")
            _tftpd.handle_rrq(root, addr, filename, opts)
        elif opcode == _tftpd.WRQ:
            log(f"tftpd: WRQ {filename!r} from {addr[0]}")
            _tftpd.handle_wrq(root, addr, filename, opts)
        else:
            log(f"tftpd: unsupported opcode {opcode} from {addr[0]}", "WARN")


def ensure_tftpd() -> None:
    """Bind port 69 in THIS process and serve on a daemon thread. Binding a
    port <1024 normally needs root, but the interpreter itself carries
    CAP_NET_BIND_SERVICE (granted once: `sudo setcap
    'cap_net_bind_service=+ep' <real python3 binary, resolve the
    sys.executable symlink chain first>`) - runs as the normal user, no sudo,
    no privilege-drop dance, no root-vs-real-user XDG_RUNTIME_DIR mismatch."""
    TFTP_ROOT.mkdir(parents=True, exist_ok=True)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock.bind(("", 69))
    except PermissionError:
        raise RuntimeError(
            "cannot bind UDP port 69 - grant the interpreter CAP_NET_BIND_SERVICE: "
            "sudo setcap 'cap_net_bind_service=+ep' $(readlink -f $(command -v python3))"
        ) from None
    threading.Thread(target=_serve_forever, args=(sock, TFTP_ROOT), daemon=True).start()
    log(f"embedded tftpd on :69, root {TFTP_ROOT}")


def tftp_and_verify(local_path: Path, addr: str) -> int:
    """Stage into tmp/tftp, tftpboot to `addr`, and wait for U-Boot's own
    'Bytes transferred = N' line to report the EXACT expected byte count -
    this is what actually catches a stale-tftpd-root or interrupted-transfer
    mistake, not just checking the command didn't error. Baking the expected
    size into the --expect pattern itself (rather than checking a separately
    captured `out` string after the fact) avoids a real race: console-send's
    --expect returns the instant "Bytes transferred" appears, but the byte
    count on the same U-Boot printf can land in a LATER read chunk - a
    transfer that had genuinely already succeeded (confirmed 39908441 bytes
    in the console log) was reported as a verification failure because of
    this, tonight."""
    dest = TFTP_ROOT / local_path.name
    dest.write_bytes(local_path.read_bytes())
    size = dest.stat().st_size
    with _rrq_lock:
        rrq_seen.pop(local_path.name, None)

    # Run the blocking console-send call on a thread so this can ALSO poll
    # the embedded server's own RRQ log in the foreground - a real network
    # problem (box never reached us at all) is now visible within a few
    # seconds via "no RRQ", not only after sitting out the full 90s bound
    # or U-Boot's own ~55s internal retry-count timeout. ONE attempt only,
    # no in-place retry: a same-command retry still pays that ~55s cost
    # before trying again, but a full power-cycle back to a fresh U-Boot
    # prompt takes ~15-20s - power-cycling is the FASTER recovery from
    # #90's TX hang. The outer retry loop in main() does that; this
    # function just reports pass/fail for one attempt.
    result: dict[str, str] = {}

    def _do_tftpboot() -> None:
        result["out"] = run_devpy(
            "--expect",
            f"Bytes transferred = {size} |Retry count exceeded",
            "--timeout",
            "90",
            f"tftpboot {addr} {local_path.name}",
            timeout=90,
        )

    t = threading.Thread(target=_do_tftpboot, daemon=True)
    t.start()
    rrq_deadline = time.monotonic() + 8.0
    while time.monotonic() < rrq_deadline:
        with _rrq_lock:
            if local_path.name in rrq_seen:
                break
        time.sleep(0.2)
    else:
        log(
            f"WARN: no RRQ for {local_path.name} within 8s of issuing tftpboot - "
            f"box may not be reaching the server at all (network/ARP problem, "
            f"not #90's mid-transfer hang)",
            "WARN",
        )
    t.join()
    out = result.get("out", "")
    if f"Bytes transferred = {size} " not in out:
        raise RuntimeError(
            f"tftpboot of {local_path.name} hit #90's TX hang (Retry count exceeded) "
            f"- output tail:\n{out[-500:]}"
        )
    log(f"tftp OK: {local_path.name} ({size} bytes) -> {addr}")
    return size


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--kernel", required=True, help="path to the gzip'd uImage")
    ap.add_argument("--dtb", required=True, help="path to the DTB")
    ap.add_argument(
        "--modules-tar", help="optional: tar.gz of the kernel-version modules dir"
    )
    ap.add_argument(
        "--modules-dir",
        help="optional: a build's modroot dir (containing lib/modules/<kver>/) - "
        "tarred internally, kver auto-derived. Mutually exclusive with "
        "--modules-tar/--kver (which stay for a pre-built tar from elsewhere). "
        "Exists because 'cd .../lib/modules && tar -czf out.tar.gz <kver>' by "
        "hand is exactly the double-nesting trap this script's own module "
        "extraction warns about if you tar the wrong directory level.",
    )
    ap.add_argument(
        "--kver", help="kernel release string, required if --modules-tar is given"
    )
    ap.add_argument(
        "--skip-power-cycle", action="store_true", help="box is already at a fresh boot"
    )
    ap.add_argument(
        "--bootargs",
        help="override U-Boot's persisted bootargs for this boot only (setenv, "
        "not saveenv - reverts on the next real reset). For one-off diagnostic "
        "boots needing a different root=/dyndbg=/loglevel= than the daily driver.",
    )
    a = ap.parse_args()

    if a.modules_tar and a.modules_dir:
        log("FATAL: --modules-tar and --modules-dir are mutually exclusive", "ERROR")
        return 2
    if a.modules_tar and not a.kver:
        log("FATAL: --modules-tar needs --kver", "ERROR")
        return 2

    kernel = Path(a.kernel).resolve()
    dtb = Path(a.dtb).resolve()
    modules_tar = Path(a.modules_tar).resolve() if a.modules_tar else None
    kver = a.kver

    if a.modules_dir:
        modroot = Path(a.modules_dir).resolve()
        modules_lib = modroot / "lib" / "modules"
        if not modules_lib.is_dir():
            log(f"FATAL: no lib/modules/ under {modroot}", "ERROR")
            return 1
        kvers = [p.name for p in modules_lib.iterdir() if p.is_dir()]
        if len(kvers) != 1:
            log(f"FATAL: expected exactly one kver dir under {modules_lib}, found {kvers}", "ERROR")
            return 1
        kver = kvers[0]
        modules_tar = TFTP_ROOT / f"ram-boot-modules-{kver}.tar.gz"
        TFTP_ROOT.mkdir(parents=True, exist_ok=True)
        with tarfile.open(modules_tar, "w:gz") as tf:
            tf.add(modules_lib / kver, arcname=kver)
        log(f"tarred {modules_lib / kver} -> {modules_tar} (kver={kver}, auto-derived)")

    for p in (kernel, dtb, *([modules_tar] if modules_tar else [])):
        if not p.exists():
            log(f"FATAL: not found: {p}", "ERROR")
            return 1

    ensure_tftpd()
    server_ip = detect_server_ip()
    log(f"tftp server IP (auto-detected): {server_ip}")

    def catch_uboot_prompt() -> None:
        """Race catch-uboot.py against the power-cycle to land at the stock
        U-Boot prompt. Stock currently autoboots straight to Fedora (see
        memory: chainload-vs-stock-boot-state) - a plain post-power-cycle
        poll for the prompt loses that race and types our setenv/tftpboot
        commands into a live Linux shell instead (confirmed: al_eth kernel
        log spam in place of "Bytes transferred")."""
        log("starting catch-uboot.py to win the autoboot race")
        catch = subprocess.Popen(
            [sys.executable, "scripts/catch-uboot.py", "--seconds", "60"],
            cwd=REPO,
        )
        power_cycle_verified(log=log)
        try:
            rc = catch.wait(timeout=70)
        except subprocess.TimeoutExpired:
            catch.kill()
            raise RuntimeError("catch-uboot.py hung waiting for the U-Boot prompt")
        if rc != 0:
            raise RuntimeError(
                "catch-uboot.py did not reach the U-Boot prompt (autoboot won)"
            )
        log("U-Boot prompt reached, autoboot stopped")

    def boot_to_login() -> None:
        run_devpy(
            "--expect",
            "ALPINE_UBNT_NAS_ALL>",
            "--timeout",
            "8",
            f"setenv ipaddr {IPADDR}",
        )
        run_devpy(
            "--expect",
            "ALPINE_UBNT_NAS_ALL>",
            "--timeout",
            "8",
            f"setenv serverip {server_ip}",
        )
        tftp_and_verify(kernel, KERNEL_ADDR)
        tftp_and_verify(dtb, DTB_ADDR)
        if modules_tar:
            tftp_and_verify(modules_tar, MODULES_ADDR)
        if a.bootargs:
            run_devpy(
                "--expect",
                "ALPINE_UBNT_NAS_ALL>",
                "--timeout",
                "8",
                f"setenv bootargs '{a.bootargs}'",
            )
            log(f"bootargs overridden for this boot only: {a.bootargs}")
        run_devpy(
            "--expect",
            "Uncompressing|Starting kernel",
            "--timeout",
            "50",  # measured: consistently ~39s bootm->"Starting kernel" for this
            # 148 MiB uncompressed KASAN image (tmp/logs/ram-boot-embedded-tftpd-
            # test5.log) - 1.25x that, not a round-number guess.
            f"bootm {KERNEL_ADDR} - {DTB_ADDR}",
        )
        log("waiting for login...")
        subprocess.run(["./dev.py", "wait-for-boot"], cwd=REPO, timeout=310)

    # #90's TX hang can strike any tftpboot, and a same-command in-place
    # retry still pays U-Boot's own ~55s internal retry-count timeout before
    # trying again - a full power-cycle back to a fresh prompt (~15-20s) is
    # the FASTER recovery (explicit user direction: "stop, reboot, try
    # again, it is quicker power cycling than waiting"). Retry the whole
    # catch+boot sequence from scratch, not just the one failed file, since
    # a mid-sequence failure leaves U-Boot's own network stack in an unknown
    # state anyway.
    skip_cycle_this_round = a.skip_power_cycle
    last_exc: Exception | None = None
    for attempt in range(1, 4):
        try:
            if not skip_cycle_this_round:
                catch_uboot_prompt()
            skip_cycle_this_round = False  # only the very first round may skip
            boot_to_login()
            last_exc = None
            break
        except (RuntimeError, subprocess.TimeoutExpired) as e:
            last_exc = e
            log(f"attempt {attempt}/3 failed ({e}); power-cycling and retrying", "WARN")
    if last_exc is not None:
        log(f"FATAL: boot-to-login failed 3/3 attempts: {last_exc}", "ERROR")
        return 1

    # A bare "#" is a trap: the standing shell prompt already contains "#", so
    # --expect could match the OLD prompt instead of waiting for the command
    # to finish. Echo a marker instead - console-send now skips its own input
    # echo before searching, so the marker being textually present in the
    # command line (as here) no longer risks a premature match either.
    run_devpy("--expect", "SHELL_READY", "--timeout", "10", "echo SHELL_READY")

    if modules_tar:
        modules_size = modules_tar.stat().st_size
        modules_md5 = hashlib.md5(modules_tar.read_bytes()).hexdigest()
        skip_bytes = int(MODULES_ADDR, 16)
        log(
            f"extracting modules from /dev/mem (skip={skip_bytes}, count={modules_size})"
        )
        # Expect the ACTUAL computed hash, not a synthetic "DONE" marker - a
        # marker word is a trap here too: it's textually present in the
        # command line itself, and this console's tty echoes typed input
        # back before the command executes, so --expect can match on that
        # echo (empty/near-instant "output") rather than genuine completion.
        # The real md5 can't appear until dd+md5sum actually ran, so matching
        # on it is race-proof by construction (same pattern already used
        # manually, successfully, earlier tonight).
        out = run_devpy(
            "--expect",
            f"{modules_md5}|No such file|cannot",
            "--timeout",
            "20",
            f"dd if=/dev/mem of=/root/rbd-modules.tar.gz bs=1M skip={skip_bytes} "
            f"count={modules_size} iflag=skip_bytes,count_bytes 2>&1; "
            f"md5sum /root/rbd-modules.tar.gz",
        )
        if modules_md5 not in out:
            log(
                f"FATAL: /dev/mem extraction checksum mismatch. Output:\n{out}", "ERROR"
            )
            return 1
        log("checksum verified, extracting into place")
        # Expected .ko names come from the LOCAL archive we already have, so
        # this is a real correctness check, not just a completion marker -
        # and it's sent as a fresh, separate round-trip after extraction had
        # time to run, so it can't fall into the same input-echo race as a
        # same-line "; echo DONE" marker would.
        # Only the OOT al_* modules land in extra/ - in-tree modules (adt7475,
        # at24, mtd, ...) are elsewhere under kernel/, so checking the whole
        # archive's .ko list against `ls extra/` alone falsely flagged a
        # complete, correct extraction as "missing" everything but the 4 OOT
        # modules tonight.
        expected_kos = sorted(
            os.path.basename(n)
            for n in tarfile.open(modules_tar).getnames()
            if n.endswith(".ko") and "/extra/" in n
        )
        # No --expect here: the real completion+correctness gate is the ls
        # check right after, sent as a genuinely separate round-trip. A
        # same-line marker would just reintroduce the input-echo race for
        # no benefit - this small archive extracts well within the plain
        # 1.5s read window console-send uses when --expect is omitted.
        # Extract to a scratch dir first, not straight into /lib/modules/{kver} -
        # this Fedora rootfs has no `tar` (found live tonight: extraction silently
        # no-op'd after an earlier `rm -rf` had already deleted the previous,
        # working tree, losing it for nothing). python3 is a hard dependency of
        # this rootfs (systemd/dnf need it) so `python3 -m tarfile` is the one
        # extractor guaranteed present. Only rm+swap the real target after the
        # scratch extraction is confirmed to contain what we expect.
        # --filter fully_trusted: PEP 706's default `data` filter refuses any
        # absolute-target symlink and aborts the WHOLE extraction on the first
        # one - found live tonight via modroot's own standard
        # /lib/modules/<kver>/build -> <host kbuild path> symlink. This is a
        # fully-trusted, self-generated archive (both ends are this script),
        # not third-party content, so the safety filter has nothing to guard
        # against here.
        run_devpy(
            "rm -rf /root/rbd-modules-new && mkdir -p /root/rbd-modules-new && "
            "python3 -m tarfile -e /root/rbd-modules.tar.gz /root/rbd-modules-new "
            "--filter fully_trusted"
        )
        # `ls` output is alphabetically sorted, so matching on ANY of the
        # names via OR (dev.py's --expect returns on the FIRST alternative
        # found) cut this off mid-stream at the first name tonight, before
        # the rest of the genuinely-complete listing had even arrived.
        # Matching on just the alphabetically-LAST expected name instead
        # only satisfies once the whole listing has streamed through.
        ls_out = run_devpy(
            "--expect",
            f"{expected_kos[-1]}|No such file",
            "--timeout",
            "10",
            f"ls /root/rbd-modules-new/{kver}/extra/",
        )
        missing = [k for k in expected_kos if k not in ls_out]
        if missing:
            log(
                f"FATAL: extraction incomplete, missing {missing}. ls output:\n{ls_out}",
                "ERROR",
            )
            return 1
        log(f"extraction verified: {len(expected_kos)} module(s) present - swapping into place")
        # Only now, with the new tree confirmed complete, replace the live one.
        run_devpy(
            f"rm -rf /lib/modules/{kver} && "
            f"mv /root/rbd-modules-new/{kver} /lib/modules/{kver} && "
            f"depmod -a {kver}"
        )

    log("DONE - box booted, at a shell.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
