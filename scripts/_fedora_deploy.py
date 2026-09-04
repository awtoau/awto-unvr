"""Shared state for deploying a build-out-fedora build to woomera.

2026-08-20: flashed a kernel binary into NAND that was ~50 min OLDER than the
module tree synced alongside it - images/tftp/ was a manually-copied cache
nobody re-ran mkuimage.py against after the last rebuild. Same failure class
as the #104/#105 module-mismatch incident (kernel Oops in resolve_symbol /
ref_module), just hitting the kernel binary instead of /lib/modules. Fix: the
kernel+DTB the box actually reads must be regenerated from build-out on every
publish, never trusted as a standing cache, and flash-nand.py must refuse to
flash anything older than the build it should have come from.

Splits the deploy into two phases because they need the box in two mutually
exclusive states - trying to do both from one invocation is what caused the
sequencing mistake that led to this:
  publish-fedora.py  - box must be running Fedora (needs SSH)
  flash-nand.py       - box must be at the U-Boot prompt
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box
from _repo import IMAGES, REPO, kernel_build_out, kernel_build_ver, log_path

OUT = kernel_build_out()
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
VER = kernel_build_ver()


def _detect_kver() -> str:
    """Read the real kernelrelease from whatever build actually landed in
    modroot, rather than hardcoding it - a hardcoded "7.1.8-dirty" here sent
    KASAN-build modules to the wrong /lib/modules/ path on the box even after
    the KASAN build itself got a distinct LOCALVERSION (#131 module-mismatch
    incident, the part of that fix that was still missing). Picks the most
    recently built dir in case a stale one from an earlier build variant is
    still sitting alongside it."""
    modules_dir = OUT / "modroot" / "lib" / "modules"
    candidates = (
        [d for d in modules_dir.iterdir() if d.is_dir()] if modules_dir.is_dir() else []
    )
    if not candidates:
        sys.exit(
            f"FATAL: no kernel release directory under {modules_dir} - build first"
        )
    return max(candidates, key=lambda d: d.stat().st_mtime).name


KVER = _detect_kver()

BUILD_IMAGE = OUT / "Image"
BUILD_DTB = OUT / f"{DTS_NAME}-{VER}.dtb"
MODROOT = OUT / "modroot" / "lib" / "modules" / KVER

TFTP_DIR = IMAGES / "tftp"
TFTP_KIMG = TFTP_DIR / f"uImage-unvr-ea16-{VER}-fedora-gz"
TFTP_DIMG = TFTP_DIR / f"{DTS_NAME}-{VER}-fedora.dtb"

ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
SSH_OPTS = (
    "ssh -o StrictHostKeyChecking=accept-new"
    " -o PreferredAuthentications=password"
    " -o PubkeyAuthentication=no"
)

LOG = log_path("fedora-deploy")


def log(m: str) -> None:
    from datetime import datetime, timezone

    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOG.open("a").write(line + "\n")


def publish_artifacts() -> None:
    """Regenerate the tftp-served uImage+DTB from build-out, UNCONDITIONALLY -
    no mtime heuristics that can be fooled, always fresh from what's actually
    on disk right now. Must run before sync_modules(): both this and the
    module tree need to trace to the exact same build-out-fedora build."""
    if not BUILD_IMAGE.is_file():
        sys.exit(f"ABORT: no build at {BUILD_IMAGE} - build first")
    TFTP_DIR.mkdir(parents=True, exist_ok=True)
    rc = subprocess.run(
        [
            sys.executable,
            "scripts/mkuimage.py",
            str(BUILD_IMAGE),
            str(TFTP_KIMG),
            "--gzip",
            "--name",
            f"unvr-ea16-{VER}-fedora",
        ],
        cwd=REPO,
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(f"ABORT: mkuimage failed (rc={rc})")
    import shutil

    shutil.copy2(BUILD_DTB, TFTP_DIMG)
    log(f"published tftp artifacts from {BUILD_IMAGE} (built {_mtime(BUILD_IMAGE)})")


def sync_modules() -> None:
    """Full clean (--delete) module sync onto the CURRENTLY-RUNNING box, over
    SSH, from the SAME build-out this run's publish_artifacts() just used.
    Aborts the whole deploy if this fails."""
    if not MODROOT.is_dir():
        sys.exit(f"ABORT: no module tree at {MODROOT} - build first")
    host = _box.locate()
    if not host:
        sys.exit(
            "ABORT: woomera not reachable - can't sync modules, refusing to deploy"
        )
    log(f"syncing module tree ({MODROOT}) -> woomera:/lib/modules/{KVER}/ (--delete)")
    rc = subprocess.run(
        [
            "sshpass",
            "-p",
            ROOT_PASSWORD,
            "rsync",
            "-az",
            "--delete",
            "-e",
            SSH_OPTS,
            f"{MODROOT}/",
            f"root@{host}:/lib/modules/{KVER}/",
        ],
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(
            f"ABORT: module rsync failed (rc={rc}) - refusing to deploy a mismatched kernel"
        )
    rc = subprocess.run(
        [
            "sshpass",
            "-p",
            ROOT_PASSWORD,
            "ssh",
            "-o",
            "StrictHostKeyChecking=accept-new",
            "-o",
            "PreferredAuthentications=password",
            "-o",
            "PubkeyAuthentication=no",
            f"root@{host}",
            f"depmod -a {KVER}",
        ],
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(f"ABORT: depmod on woomera failed (rc={rc}) - refusing to deploy")
    _verify_sync_integrity(host)
    _deploy_kernel_module_check(host)
    # 2026-08-27: found via #162 testing - a file transferred via scp/rsync
    # can report rc=0 while its data is still only in the remote page cache,
    # not yet fsync'd to the physical SSD. This box is routinely reset via
    # the SP805 watchdog (no plain `reboot` - #51) within seconds of a
    # deploy for iterative testing; an abrupt hardware reset that soon after
    # can lose that unflushed data even though the transfer "succeeded" -
    # confirmed live: a just-scp'd 305-byte marker file read back as 305
    # zero bytes after exactly that sequence (ext4 journaled the file's
    # size/existence, not its never-flushed content). Explicit sync here
    # protects every file this function just wrote, including the module
    # tree itself, not just the marker - covers a real gap in #161's own
    # fix too, not only #162's addition.
    rc = subprocess.run(
        [
            "sshpass",
            "-p",
            ROOT_PASSWORD,
            "ssh",
            "-o",
            "StrictHostKeyChecking=accept-new",
            "-o",
            "PreferredAuthentications=password",
            "-o",
            "PubkeyAuthentication=no",
            f"root@{host}",
            "sync",
        ],
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(f"ABORT: sync on woomera failed (rc={rc}) - files may not be durable")
    log("module sync + depmod OK")


def _kernel_banner() -> str:
    """Extract the `Linux version ...` banner directly from the raw kernel
    Image binary - this differs per actual build even when the KVER string
    itself doesn't (e.g. "7.1.8-dirty" for every build regardless of date),
    since it embeds the build timestamp. Confirmed present via plain
    `strings` on the uncompressed arm64 Image - no need to boot it.

    The binary contains this string twice: a truncated copy missing the
    build number/date (`... # SMP PREEMPT_DYNAMIC` with no trailing text -
    some other symbol/template sharing a prefix, not investigated further)
    and the real, complete one. Take the longest match, not the first."""
    out = subprocess.run(
        ["strings", "-a", str(BUILD_IMAGE)], capture_output=True, text=True, check=True
    ).stdout
    matches = [
        line.strip() for line in out.splitlines() if line.startswith("Linux version")
    ]
    if not matches:
        sys.exit(
            f"ABORT: no 'Linux version' banner found in {BUILD_IMAGE} - can't record #162 provenance"
        )
    return max(matches, key=len)


def _deploy_kernel_module_check(host: str) -> None:
    """#162: write a marker recording which exact kernel build this module
    sync came from, and ship the boot-time checker that compares it against
    whatever kernel is actually running - see scripts/check-kernel-module-
    match.py's own docstring for the full rationale. Ships on every sync
    (idempotent - same content each time) so it self-heals onto any rootfs
    built before #162 landed, without needing a full rootfs rebuild."""
    from datetime import datetime, timezone

    banner = _kernel_banner()
    marker = (
        f"banner={banner}\n"
        f"build_out={OUT}\n"
        f"kver={KVER}\n"
        f"synced_at={datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}\n"
    )
    ssh_base = [
        "sshpass",
        "-p",
        ROOT_PASSWORD,
        "ssh",
        "-o",
        "StrictHostKeyChecking=accept-new",
        "-o",
        "PreferredAuthentications=password",
        "-o",
        "PubkeyAuthentication=no",
        f"root@{host}",
    ]
    # Piping content via `input=` through an sshpass-wrapped ssh landed as a
    # silently-empty 0-byte file (sshpass's own pty handling for the
    # password prompt apparently interferes with forwarding stdin to the
    # remote command) - scp of a real local file, like the two below, is
    # the proven-working path in this function; use that instead.
    import tempfile

    with tempfile.NamedTemporaryFile("w", suffix=".deployed-from", delete=False) as tf:
        tf.write(marker)
        marker_path = Path(tf.name)
    try:
        rc = subprocess.run(
            [
                "sshpass",
                "-p",
                ROOT_PASSWORD,
                "scp",
                "-o",
                "StrictHostKeyChecking=accept-new",
                "-o",
                "PreferredAuthentications=password",
                "-o",
                "PubkeyAuthentication=no",
                str(marker_path),
                f"root@{host}:/lib/modules/{KVER}/.deployed-from",
            ],
            check=False,
        ).returncode
    finally:
        marker_path.unlink()
    if rc != 0:
        sys.exit(f"ABORT: writing #162 provenance marker failed (rc={rc})")

    script = Path(__file__).resolve().parent / "check-kernel-module-match.py"
    unit = Path(__file__).resolve().parent / "check-kernel-module-match.service"
    for src, dst in (
        (script, "/usr/local/bin/check-kernel-module-match.py"),
        (unit, "/etc/systemd/system/check-kernel-module-match.service"),
    ):
        rc = subprocess.run(
            [
                "sshpass",
                "-p",
                ROOT_PASSWORD,
                "scp",
                "-o",
                "StrictHostKeyChecking=accept-new",
                "-o",
                "PreferredAuthentications=password",
                "-o",
                "PubkeyAuthentication=no",
                str(src),
                f"root@{host}:{dst}",
            ],
            check=False,
        ).returncode
        if rc != 0:
            sys.exit(f"ABORT: shipping {src.name} failed (rc={rc})")
    # Zero-byte check on the marker specifically: this exact function just
    # proved (2026-08-27) that a 0-length remote file is possible even with
    # rc=0 from the transfer command, the same failure class
    # _verify_sync_integrity below exists to catch for modules. Confirming
    # non-empty AND grep for the expected key, not just "exists".
    check = subprocess.run(
        [*ssh_base, f"grep -q '^banner=' /lib/modules/{KVER}/.deployed-from"],
        check=False,
    ).returncode
    if check != 0:
        sys.exit(
            "ABORT: #162 marker landed but is empty/malformed on woomera - refusing to continue"
        )
    rc = subprocess.run(
        [
            *ssh_base,
            "chmod +x /usr/local/bin/check-kernel-module-match.py "
            "&& systemctl daemon-reload "
            "&& systemctl enable check-kernel-module-match.service",
        ],
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(f"ABORT: enabling check-kernel-module-match.service failed (rc={rc})")
    log(f"#162 provenance marker + checker deployed (banner: {banner})")


def _verify_sync_integrity(host: str) -> None:
    """2026-08-20: rsync exited 0 and depmod exited 0, but al_eth's declared
    dependency phylink.ko landed on the box at 0 bytes (145K locally) - a
    silent, undetected transfer failure. rsync's own exit code alone isn't
    proof the files are intact; check for zero-byte .ko files, which should
    never legitimately exist, as a cheap post-sync integrity signal."""
    out = subprocess.run(
        [
            "sshpass",
            "-p",
            ROOT_PASSWORD,
            "ssh",
            "-o",
            "StrictHostKeyChecking=accept-new",
            "-o",
            "PreferredAuthentications=password",
            "-o",
            "PubkeyAuthentication=no",
            f"root@{host}",
            f"find /lib/modules/{KVER} -name '*.ko' -size 0",
        ],
        capture_output=True,
        text=True,
        check=False,
    ).stdout.strip()
    if out:
        bad = out.splitlines()
        sys.exit(
            f"ABORT: {len(bad)} zero-byte .ko file(s) after sync (transfer "
            f"silently failed) - refusing to deploy: {', '.join(bad[:5])}"
        )


def _mtime(p: Path) -> str:
    from datetime import datetime, timezone

    return (
        datetime.fromtimestamp(p.stat().st_mtime, tz=timezone.utc)
        .astimezone()
        .isoformat(timespec="seconds")
    )


def assert_fresh() -> None:
    """flash-nand.py's guard: refuse to flash a tftp-served kernel/DTB that
    is older than the build-out it should have been regenerated from. This
    is exactly the check that would have caught the 2026-08-20 incident -
    a 12:57 tftp cache flashed against a 13:46 build/module tree."""
    for served, src, label in (
        (TFTP_KIMG, BUILD_IMAGE, "uImage"),
        (TFTP_DIMG, BUILD_DTB, "DTB"),
    ):
        if not served.is_file():
            sys.exit(
                f"ABORT: no published {label} at {served} - run './dev.py publish-fedora' first"
            )
        if not src.is_file():
            sys.exit(f"ABORT: no build-out {label} at {src} - build first")
        if served.stat().st_mtime < src.stat().st_mtime:
            sys.exit(
                f"ABORT: published {label} ({served}, {_mtime(served)}) is OLDER than "
                f"the build-out it should match ({src}, {_mtime(src)}) - "
                f"run './dev.py publish-fedora' first"
            )
