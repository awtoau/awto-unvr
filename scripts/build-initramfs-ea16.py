#!/usr/bin/env python3
"""Build a minimal aarch64 initramfs for UNVR ea16 first-Linux bring-up.

- Pulls arm64 Alpine via docker (binfmt/qemu-aarch64 already registered), exports
  its rootfs, adds an /init that mounts pseudo-fs and drops to a serial shell.
- Emits both a directory (for CONFIG_INITRAMFS_SOURCE) and a gzip'd newc cpio.

Not for building on the device; host-side only. Logs to tmp/logs/linux-build.log.
"""
import os, subprocess, sys, shutil, pathlib

OUT   = "/mnt/2tb/unvr-port-refs/build-out"
WORK  = "/mnt/2tb/unvr-port-refs/build-out/initramfs-root"
LOG   = "/mnt/2tb/git/awto-unvr/tmp/logs/linux-build.log"
IMAGE = "alpine:3.21"

INIT = r"""#!/bin/sh
# UNVR ea16 first-Linux initramfs init
mount -t proc     none /proc      2>/dev/null
mount -t sysfs    none /sys       2>/dev/null
mount -t devtmpfs none /dev       2>/dev/null
mount -t tmpfs    none /tmp       2>/dev/null
echo 0 > /proc/sys/kernel/printk 2>/dev/null

echo
echo "==================================================================="
echo " UNVR (sysid ea16) - first Linux on the device"
echo "-------------------------------------------------------------------"
echo -n " kernel : "; uname -a
echo -n " model  : "; cat /sys/firmware/devicetree/base/model 2>/dev/null; echo
echo -n " cmdline: "; cat /proc/cmdline
echo -n " cpus   : "; grep -c ^processor /proc/cpuinfo
echo -n " memtot : "; grep MemTotal /proc/meminfo
echo "==================================================================="
echo " Loading out-of-tree modules (if present)..."
for m in al_eth al_dma al_ssm al_sgpo; do
    modprobe $m 2>/dev/null && echo "  loaded $m" || true
done
echo " Type 'poweroff -f' or 'reboot -f' to exit. Dropping to shell."
echo

# Prefer a controlling tty via cttyhack; fall back to a plain console shell.
setsid cttyhack /bin/sh </dev/console >/dev/console 2>&1
exec /bin/sh </dev/console >/dev/console 2>&1
"""

def run(cmd, **kw):
    print("+ " + " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, **kw)

def main():
    pathlib.Path(os.path.dirname(LOG)).mkdir(parents=True, exist_ok=True)
    pathlib.Path(OUT).mkdir(parents=True, exist_ok=True)
    if os.path.exists(WORK):
        shutil.rmtree(WORK)
    os.makedirs(WORK)

    run(["docker", "pull", "--platform=linux/arm64", IMAGE])
    cid = subprocess.check_output(
        ["docker", "create", "--platform=linux/arm64", IMAGE]).decode().strip()
    try:
        tar = os.path.join(OUT, "alpine-arm64.tar")
        with open(tar, "wb") as f:
            run(["docker", "export", cid], stdout=f)
    finally:
        run(["docker", "rm", cid])

    run(["tar", "-xf", tar, "-C", WORK])
    os.remove(tar)

    # Docker export writes /dev/console as a regular file -> breaks devtmpfs.
    dev = os.path.join(WORK, "dev")
    if os.path.exists(dev):
        shutil.rmtree(dev)
    os.makedirs(dev)

    init_path = os.path.join(WORK, "init")
    with open(init_path, "w") as f:
        f.write(INIT)
    os.chmod(init_path, 0o755)
    # Alpine's /sbin/init is busybox; ensure cttyhack exists (busybox applet).

    # Build newc cpio + gzip (device nodes created by /init via devtmpfs).
    cpio_gz = os.path.join(OUT, "initramfs-ea16.cpio.gz")
    find = subprocess.Popen(["find", "."], cwd=WORK, stdout=subprocess.PIPE)
    cpio = subprocess.Popen(["cpio", "-o", "-H", "newc", "--quiet"],
                            cwd=WORK, stdin=find.stdout, stdout=subprocess.PIPE)
    with open(cpio_gz, "wb") as f:
        gz = subprocess.Popen(["gzip", "-9"], stdin=cpio.stdout, stdout=f)
        gz.communicate()
    find.wait(); cpio.wait()

    sz = os.path.getsize(cpio_gz)
    print(f"initramfs dir : {WORK}")
    print(f"initramfs cpio: {cpio_gz} ({sz//1024} KiB)")

if __name__ == "__main__":
    sys.exit(main())
