#!/usr/bin/env python3
"""Build a Fedora aarch64 rootfs for the UNVR (ea16), bootable by OUR U-Boot+kernel.

The "hack": Fedora arm64 normally boots UEFI -> shim -> GRUB -> dracut. We have
none of those. This rootfs is booted directly by the stock U-Boot + our 7.1.8
kernel with root=/dev/sdaN, no initramfs (kernel has AHCI+ext4 built-in), no
bootloader of its own. So this script produces ONLY a configured userland tar;
kernel + modules + boot config are applied separately.

Runs on the x86 host via podman + qemu-aarch64 binfmt (already registered).
Output: tmp/fedora-rootfs-ea16.tar (+ .sha256). Slow (dnf under emulation).

Choices baked in (all overridable later in the tar):
- SELinux disabled (selinux=0 also set in bootargs) - no relabel path without dracut.
- serial getty on ttyS0 @115200 enabled.
- systemd-networkd, DHCP on all ethernet (match eth*/en*), so eth0/eth1 come up.
- root password set (default 'unvr', CHANGE IT) + sshd permitrootlogin yes.
- our 7.1.8 kernel modules copied into /lib/modules by the deploy step, not here.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, TMP

REL = "44"
IMAGE = f"registry.fedoraproject.org/fedora:{REL}"
CTR = "unvr-fedora-build"

# Stay as close to a stock Fedora install as possible: install the standard
# @core group (with weak deps, the Fedora default) as the base, then add only
# the NAS/admin extras. Deviations from stock live in CONFIG_SH and are the ONLY
# ones (SELinux/firewall/abrt off, serial console, ssh root) - all requested.
# No kernel/GRUB/dracut - we boot it ourselves (our U-Boot + 7.1.8 kernel).
CORE_GROUP = "core"
EXTRAS = [
    "openssh-server",
    "e2fsprogs",
    "dosfstools",
    "python3",
    "rsync",
    "htop",
    "smartmontools",
    "mdadm",
    "hdparm",
    "libgpiod-utils",
    "lm_sensors",
]

# Config applied inside the container (aarch64, via qemu) before export.
CONFIG_SH = r"""
set -eux
# --- SELinux off (config absent in minimal container; selinux=0 in bootargs too) ---
[ -f /etc/selinux/config ] && sed -i 's/^SELINUX=.*/SELINUX=disabled/' /etc/selinux/config || true
# --- firewall off (device sits on the user's trusted LAN, single-tenant NAS) ---
systemctl disable firewalld.service 2>/dev/null || true
dnf -y remove firewalld 2>/dev/null || true
# --- ABRT off (no crash-report daemon; it thrashed the host earlier w/ big cores) ---
systemctl disable abrtd.service abrt-ccpp.service abrt-oops.service \
    abrt-journal-core.service abrt-xorg.service 2>/dev/null || true
dnf -y remove 'abrt*' 2>/dev/null || true
# --- serial console login ---
systemctl enable serial-getty@ttyS0.service
# --- sysrq: Fedora's own /usr/lib/sysctl.d/50-default.conf sets kernel.sysrq=16
# (log only) at every boot via systemd-sysctl, overriding the kernel's own
# compiled-in default (#97 - want serial BREAK+key recoverable without a
# physical power-cycle). Higher-sorting drop-in wins. ---
echo 'kernel.sysrq = 1' > /etc/sysctl.d/99-awto-sysrq.conf
# --- network: NetworkManager (Fedora default, installed), auto-DHCPs all wired links ---
systemctl enable NetworkManager.service
# --- ssh ---
systemctl enable sshd.service
sed -i 's/^#\?PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config
# --- root password (CHANGE THIS) ---
echo 'root:unvr' | chpasswd
# --- fstab: root by label, no swap; the deploy step labels sdaN 'unvr-root' ---
cat > /etc/fstab <<'EOF'
LABEL=unvr-root  /  ext4  defaults,noatime  0 1
EOF
# --- hostname (device is 'woomera'; dev work self-hosts here post-milestone) ---
echo woomera > /etc/hostname
# --- don't wait forever for a nonexistent initramfs/rootdev ---
systemctl mask systemd-firstboot.service || true
# --- machine-id will be generated first boot ---
rm -f /etc/machine-id
"""


def log(msg: str) -> None:
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "build-fedora-rootfs.log").open("a") as fh:
        fh.write(line + "\n")


def run(*cmd: str, **kw) -> subprocess.CompletedProcess:
    log("+ " + " ".join(cmd))
    return subprocess.run(cmd, check=True, **kw)


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--keep", action="store_true", help="don't rm the build container")
    a = ap.parse_args()

    TMP.mkdir(parents=True, exist_ok=True)
    out = TMP / "fedora-rootfs-ea16.tar"

    # Fresh container, aarch64 platform (qemu binfmt does the emulation).
    subprocess.run(
        ["podman", "rm", "-f", CTR],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,  # ok if CTR doesn't exist yet
    )
    run(
        "podman",
        "create",
        "--platform",
        "linux/arm64",
        "--name",
        CTR,
        IMAGE,
        "sleep",
        "infinity",
    )
    run("podman", "start", CTR)
    try:
        # Stock Fedora base: @core group WITH weak deps (the default), then extras.
        # This is the slow part under qemu emulation.
        run("podman", "exec", CTR, "dnf", "-y", "group", "install", CORE_GROUP)
        run("podman", "exec", CTR, "dnf", "-y", "install", *EXTRAS)
        run("podman", "exec", CTR, "dnf", "-y", "clean", "all")
        # Apply config inside the container.
        run("podman", "exec", CTR, "bash", "-c", CONFIG_SH)
        # Export the configured rootfs.
        log(f"exporting rootfs -> {out}")
        with out.open("wb") as fh:
            subprocess.run(["podman", "export", CTR], check=True, stdout=fh)
    finally:
        if not a.keep:
            subprocess.run(
                ["podman", "rm", "-f", CTR],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,  # best-effort cleanup
            )

    sha = run("sha256sum", str(out), capture_output=True, text=True).stdout.split()[0]
    (out.with_suffix(".tar.sha256")).write_text(f"{sha}  {out.name}\n")
    size_mb = out.stat().st_size / 1e6
    log(f"DONE: {out} ({size_mb:.0f} MB) sha256={sha[:16]}...")
    log(
        "Next: enhanced initramfs -> format sdaN ext4 (label unvr-root) -> "
        "rsync this tar's contents -> add 7.1.8 modules -> U-Boot env -> saveenv"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
