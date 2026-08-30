#!/usr/bin/env python3
"""Build a Fedora aarch64 rootfs for the UNVR (ea16), bootable by OUR U-Boot+kernel.

The "hack": Fedora arm64 normally boots UEFI -> shim -> GRUB -> dracut. We have
none of those. This rootfs is booted directly by the stock U-Boot + our 7.1.8
kernel with root=/dev/sdaN, no initramfs (kernel has AHCI+ext4 built-in), no
bootloader of its own. So this script produces ONLY a configured userland tar;
kernel + modules + boot config are applied separately.

Built with `dnf --installroot --forcearch=aarch64` directly against this host's
own Fedora repos - no podman, no registry.fedoraproject.org container base
image. A container base image comes with its own already-installed packages
and already-applied presets (systemd-resolved enabled by default, for one -
#124) that we never chose and didn't know we'd inherited; installroot builds
the tree from nothing but the same @core + EXTRAS package set, so there's no
hidden baggage left to discover later. binfmt_misc (already registered on
this host) transparently runs aarch64 scriptlets/systemctl under both dnf's
own package scriptlets and the chroot config step below - the same mechanism
RPM %post macros already rely on, not something new we're introducing.

Output: tmp/fedora-rootfs-ea16.tar (+ .sha256). Slow (dnf under emulation).

Choices baked in (all overridable later in the tar):
- SELinux disabled (selinux=0 also set in bootargs) - no relabel path without dracut.
- serial getty on ttyS0 @115200 enabled.
- NetworkManager (not systemd-networkd - stale claim, actual code below always
  enabled NetworkManager), DHCP on all wired links, so eth0/eth1 come up.
- systemd-resolved disabled (#124) - it fights NetworkManager over
  /etc/resolv.conf otherwise, spinning forever on Permission denied.
- root password set (default 'unvr', CHANGE IT) + sshd permitrootlogin yes.
- our 7.1.8 kernel modules copied into /lib/modules by the deploy step, not here.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, REPO, TMP

REL = "44"
ARCH = "aarch64"

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
    # @core's default weak-dep resolution on this host pulls in
    # fedora-release-container (wrong PRETTY_NAME/VARIANT for a real install -
    # #124 follow-up). Force the server variant instead - closest fit for a
    # headless NAS, no desktop/container baggage.
    "fedora-release-server",
    # Build tools
    "gcc",
    "gcc-c++",
    "make",
    "git",
    "kernel-devel",
    "kernel-headers",
    "binutils",
    "pkgconf-pkg-config",
    "dwarves",
    # Diagnostics
    "strace",
    "ltrace",
    "gdb",
    "tcpdump",
    "ethtool",
    "lsof",
    "iotop",
    "sysstat",
    "pciutils",
    "usbutils",
    "nvme-cli",
    "iperf3",
    "bind-utils",
    "traceroute",
    "perf",
    "dmidecode",
    "trace-cmd",
    "kexec-tools",
]

# Config applied via chroot into the installroot (aarch64, via qemu binfmt).
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
# --- serial console login, auto (owner directive - headless bring-up box,
# no reason to retype root/unvr every time the shared console is attached) ---
systemctl enable serial-getty@ttyS0.service
# --- fan daemon (#44) - staged into /usr/local/bin + /etc/systemd/system
# above, just needs enabling here ---
systemctl enable unvr-fand.service
mkdir -p /etc/systemd/system/serial-getty@ttyS0.service.d
cat > /etc/systemd/system/serial-getty@ttyS0.service.d/autologin.conf << 'GETTYEOF'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin root --noclear %I $TERM
GETTYEOF
# --- sysrq: Fedora's own /usr/lib/sysctl.d/50-default.conf sets kernel.sysrq=16
# (log only) at every boot via systemd-sysctl, overriding the kernel's own
# compiled-in default (#97 - want serial BREAK+key recoverable without a
# physical power-cycle). Higher-sorting drop-in wins. ---
echo 'kernel.sysrq = 1' > /etc/sysctl.d/99-awto-sysrq.conf
# --- reset button: systemd-logind's own default HandleRebootKey= treats the
# gpio-keys KEY_RESTART event (pin 38, and at least one of the undocumented
# SW1/SW2 test switches wired to the same net) as a real reboot trigger with
# zero custom code. Fine for a deployed device, a hazard on a dev box under
# active testing (killed background collectors mid-run, see #docs/gpio-
# switches-leds.md). Disable the auto-action; the button still generates the
# KEY_RESTART evdev event for anyone who wants to handle it deliberately. ---
mkdir -p /etc/systemd/logind.conf.d
printf '[Login]\nHandleRebootKey=ignore\n' > /etc/systemd/logind.conf.d/99-awto-no-reboot-key.conf
# --- network: NetworkManager (Fedora default, installed), auto-DHCPs all wired links ---
systemctl enable NetworkManager.service
# Stock TimeoutStartSec=600 (NetworkManager.service) / systemd's 90s default
# (polkit.service, no override) exist for boxes with "a huge number of
# interfaces" - this box has 2. When al_eth fails to register (#131), boot
# hung 10+ minutes waiting these out before reaching a login prompt at all -
# 1.25x a real single-digit-second local startup, not minutes.
mkdir -p /etc/systemd/system/NetworkManager.service.d /etc/systemd/system/polkit.service.d
printf '[Service]\nTimeoutStartSec=10\n' > /etc/systemd/system/NetworkManager.service.d/override.conf
printf '[Service]\nTimeoutStartSec=10\n' > /etc/systemd/system/polkit.service.d/override.conf
# #131: al_eth was ONE module bound to BOTH the 1G and 10G PCI devices, so
# udev's 2 per-device modprobes raced finit_module() on the same module ->
# soft lockup. Fixed structurally: al_eth now builds as al_eth_1g.ko /
# al_eth_10g.ko (modules/al_eth/Makefile MODULE_VARIANT), each bound to
# only one PCI ID, so no blacklist/modules-load.d ordering hack is needed -
# normal MODALIAS auto-load is safe for both.
# systemd-resolved's own RPM preset enables it by default, but nothing here
# ever configures it. Both enabled + unreconciled means resolved spins
# forever trying to open a file NetworkManager owns at 0700, ~1 failure
# every 0.3s for the life of every boot (#124). Disable it explicitly instead
# of leaving an unreconciled default.
systemctl disable --now systemd-resolved.service 2>/dev/null || true
# NetworkManager writes real DNS content to /run/NetworkManager/resolv.conf
# every boot (confirmed correct on a live box), but does NOT reliably create
# the /etc/resolv.conf symlink pointing at it on a genuinely fresh rootfs -
# confirmed live: a fresh boot had a fully-populated /run/NetworkManager/
# resolv.conf and no /etc/resolv.conf at all, breaking dnf/curl/every
# hostname lookup. Create the symlink at build time instead of trusting
# NetworkManager to create it at runtime.
mkdir -p /etc/NetworkManager/conf.d
printf '[main]\ndns=default\nrc-manager=symlink\n' > /etc/NetworkManager/conf.d/dns.conf
ln -sf /run/NetworkManager/resolv.conf /etc/resolv.conf
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


def sudo_run(*cmd: str, **kw) -> subprocess.CompletedProcess:
    """-n: fail fast if credentials aren't cached, instead of hanging on a
    password prompt this script has no TTY to answer."""
    return run("sudo", "-n", *cmd, **kw)


def verify_not_container_flavored(root: pathlib.Path) -> None:
    """Fail loudly, before packaging/deploying anything, if this build ever
    regresses to a container-flavored rootfs (#114, #150, #164) - dnf's
    @core weak-dep resolution has silently pulled in fedora-release-container
    before despite the --exclude above; catch it here instead of finding out
    from a live box's ConditionVirtualization=!container failures.

    #164 root-caused the deeper mechanism #150 found but couldn't explain:
    on this rootfs's no-initramfs direct-kernel-boot path, PID1 checks
    /run/.containerenv in its very first mount-table pass - BEFORE /run
    gets its own tmpfs mount - so a stale, on-disk /run/.containerenv left
    over from this project's old podman-based build (dropped in c712df1)
    fooled systemd's detect_container() into caching "container" for PID1's
    entire lifetime, even though a live systemd-detect-virt (a fresh
    process, checking AFTER /run's tmpfs is up) always read "none". The
    file is only ever written by a container runtime, never by systemd -
    its presence in a freshly-built installroot before packaging is
    unambiguous evidence of exactly this regression, not a false positive."""
    os_release = root / "etc" / "os-release"
    text = os_release.read_text() if os_release.exists() else ""
    if "container" in text.lower():
        log(f"FATAL: {os_release} mentions 'container': {text}", "ERROR")
        sys.exit(1)
    for marker in (".containerenv", ".dockerenv"):
        for rel in (pathlib.Path("run") / marker, pathlib.Path(marker)):
            p = root / rel
            if p.exists():
                log(f"FATAL: container marker file present in installroot: {p} "
                    f"(#164 - this shadows systemd's real virtualization state "
                    f"on the no-initramfs boot path, not just cosmetic)", "ERROR")
                sys.exit(1)
    for rel in ("run/systemd/container", "run/host/container-manager"):
        p = root / rel
        if p.exists():
            log(f"FATAL: container marker file present in installroot: {p} (#164)", "ERROR")
            sys.exit(1)
    rpm = subprocess.run(
        ["rpm", "--root", str(root), "-q", "fedora-release-container"],
        capture_output=True,
        text=True,
    )
    if rpm.returncode == 0:
        log(
            f"FATAL: fedora-release-container IS installed in {root} "
            f"(dnf's weak-dep resolution won despite --exclude): {rpm.stdout.strip()}",
            "ERROR",
        )
        sys.exit(1)
    rpm = subprocess.run(
        ["rpm", "--root", str(root), "-q", "fedora-release-server"],
        capture_output=True,
        text=True,
    )
    if rpm.returncode != 0:
        log(f"FATAL: fedora-release-server NOT installed in {root}", "ERROR")
        sys.exit(1)
    log("verified: not container-flavored (fedora-release-server present, os-release clean)")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--keep", action="store_true", help="don't rm the installroot dir")
    a = ap.parse_args()

    TMP.mkdir(parents=True, exist_ok=True)
    out = TMP / "fedora-rootfs-ea16.tar"
    root = TMP / "fedora-rootfs-installroot"

    # Fresh, empty installroot - nothing pre-installed, no inherited base-image
    # config (that's the whole point, see the module docstring / #124).
    if root.exists():
        sudo_run("rm", "-rf", str(root))
    root.mkdir(parents=True)
    # --use-host-config: dnf5 --installroot otherwise ignores this host's own
    # /etc/yum.repos.d entirely and sees zero repos.
    dnf_args = [
        "-y",
        "--installroot",
        str(root),
        "--forcearch",
        ARCH,
        "--releasever",
        REL,
        "--use-host-config",
        "--disablerepo=*",
        "--enablerepo=fedora",
        "--enablerepo=updates",
        # @core's weak-dep resolution defaults to fedora-release-container
        # here - exclude it so fedora-release-server (in EXTRAS) wins instead.
        "--exclude=fedora-release-container",
    ]
    try:
        # Stock Fedora base: @core group WITH weak deps (the default), then
        # extras, straight from this host's own fedora/updates repos. This is
        # the slow part under qemu emulation (dnf scriptlets run aarch64 code
        # via binfmt_misc).
        sudo_run("dnf", *dnf_args, "group", "install", CORE_GROUP)
        sudo_run("dnf", *dnf_args, "install", *EXTRAS)
        sudo_run(
            "dnf",
            "-y",
            "--installroot",
            str(root),
            "clean",
            "all",
        )
        # Stage the fan daemon (#44) before the chroot config step enables it.
        sudo_run(
            "install", "-m", "755",
            str(REPO / "scripts" / "unvr-fand.py"),
            str(root / "usr/local/bin/unvr-fand.py"),
        )
        sudo_run(
            "install", "-m", "644",
            str(REPO / "scripts" / "unvr-fand.service"),
            str(root / "etc/systemd/system/unvr-fand.service"),
        )
        # Apply config via chroot - binfmt_misc runs the aarch64 bash/systemctl
        # transparently, same mechanism dnf's own scriptlets just used above.
        sudo_run("chroot", str(root), "bash", "-c", CONFIG_SH)
        verify_not_container_flavored(root)
        # Export the configured rootfs (preserve ownership/perms - needs sudo
        # to read root-owned files dnf --installroot creates).
        log(f"exporting rootfs -> {out}")
        sudo_run(
            "tar",
            "--numeric-owner",
            "-C",
            str(root),
            "-cf",
            str(out),
            ".",
        )
        sudo_run("chown", f"{os.environ['USER']}:", str(out))
    finally:
        if not a.keep:
            sudo_run("rm", "-rf", str(root))

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
