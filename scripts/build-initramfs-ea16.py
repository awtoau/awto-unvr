#!/usr/bin/env python3
"""Build a minimal aarch64 initramfs for UNVR ea16 first-Linux bring-up.

- Pulls arm64 Alpine via docker (binfmt/qemu-aarch64 already registered), exports
  its rootfs, adds an /init that mounts pseudo-fs, loads the out-of-tree drivers,
  powers on the SATA bays, prints diagnostics, then drops to a serial shell.
- Emits: the initramfs directory (build-out/initramfs-root, used as the kernel's
  CONFIG_INITRAMFS_SOURCE), a gen_init_cpio device-node list (build-out/
  initramfs-devnodes, gives a real /dev/console so PID1 gets a controlling tty),
  and a baseline gzip'd newc cpio.

The out-of-tree modules are added to initramfs-root LATER, by build-linux-612-ea16.py,
before the kernel Image (which embeds this initramfs) is linked. Host-side only.
"""
import os, subprocess, sys, shutil, pathlib

OUT   = "/mnt/2tb/unvr-port-refs/build-out"
WORK  = "/mnt/2tb/unvr-port-refs/build-out/initramfs-root"
DEVNODES = "/mnt/2tb/unvr-port-refs/build-out/initramfs-devnodes"
IMAGE = "alpine:3.21"

INIT = r"""#!/bin/sh
mount -t proc     proc /proc    2>/dev/null
mount -t sysfs    sys  /sys     2>/dev/null
mount -t devtmpfs dev  /dev     2>/dev/null
mount -t tmpfs    tmp  /tmp     2>/dev/null
mkdir -p /dev/pts && mount -t devpts devpts /dev/pts 2>/dev/null

KVER=$(uname -r)
# pcie-al-internal / ahci / pca953x / designware-i2c are built-in (already up).
# Load the out-of-tree drivers in dependency order.
for m in al_dma al_ssm al_eth al_sgpo; do
    modprobe $m 2>/dev/null || insmod /lib/modules/$KVER/extra/$m.ko 2>/dev/null \
        && echo "loaded $m" || echo "FAILED $m"
done

# Power on the SATA bays (no mainline hdd-pwrctl driver in this build).
[ -x /bin/hdd-power-on ] && /bin/hdd-power-on

echo
echo "==================================================================="
echo " UNVR (sysid ea16) - Linux 6.12 first boot"
echo "-------------------------------------------------------------------"
uname -a
printf "model : "; cat /sys/firmware/devicetree/base/model 2>/dev/null; echo
echo "--- al_* modules ---"; lsmod 2>/dev/null | awk 'NR==1 || /^al_/'
echo "--- PCI devices (internal PCIe must be non-empty) ---"
n=0
for d in /sys/bus/pci/devices/*; do
    [ -e "$d" ] || continue; n=$((n+1))
    echo "  $(basename $d)  ven=$(cat $d/vendor) dev=$(cat $d/device) class=$(cat $d/class)"
done
echo "  ($n PCI devices)"
echo "--- ATA / block ---"; ls /sys/class/ata_port 2>/dev/null; cat /proc/partitions 2>/dev/null
echo "--- net links ---"; ip -o link 2>/dev/null | awk -F': ' '{print $2}'
echo "==================================================================="
echo " serial shell on ttyS0. 'reboot -f' to exit."
echo

exec setsid -c /bin/sh 0<>/dev/console 1>&0 2>&0
"""

HDD_POWER = r"""#!/bin/sh
# The DTB gpio-hog drives PCA9575 @0x21 pwren lines 0-3 high at boot, so the
# bays should already be powered. This is a diagnostic + manual fallback for a
# DTB without the hog. Uses libgpiod (v2) on the /dev/gpiochipN char device
# (CONFIG_GPIO_SYSFS is not required), with an i2c-tools / sysfs fallback.
echo "--- gpiodetect ---"; gpiodetect 2>/dev/null || echo "(gpiodetect missing)"
# pca953x char-dev label looks like "0-0021" (i2c bus N, addr 0x21).
CHIP=$(gpiodetect 2>/dev/null | awk '/0021/{print $1; exit}')
if [ -n "$CHIP" ]; then
    echo "--- $CHIP (pca9575 @0x21) lines 0-3 ---"
    gpioinfo "$CHIP" 2>/dev/null | grep -E 'line +[0-3][^0-9]' || gpioinfo "$CHIP" 2>/dev/null | head -6
    # If the hog holds the lines this is a busy no-op (bays already powered);
    # if the hog is absent this drives them high and holds in the background.
    gpioset -c "$CHIP" 0=1 1=1 2=1 3=1 >/dev/null 2>&1 &
    echo "requested $CHIP lines 0-3 = high (hog holds them if present)"
else
    echo "pca9575@0x21 gpiochip not found via gpiodetect"
fi
# sysfs convenience path (only if CONFIG_GPIO_SYSFS is enabled)
if [ -w /sys/class/gpio/export ]; then
    B=""
    for c in /sys/class/gpio/gpiochip*; do
        [ -e "$c" ] || continue
        case "$(cat $c/label)" in *0021) B=$(cat $c/base);; esac
    done
    if [ -n "$B" ]; then
        for p in 0 1 2 3; do g=$((B+p))
            echo $g > /sys/class/gpio/export 2>/dev/null
            echo out > /sys/class/gpio/gpio$g/direction 2>/dev/null
            echo 1   > /sys/class/gpio/gpio$g/value 2>/dev/null
        done
        echo "sysfs: drove base $B lines 0-3 high"
    fi
fi
"""

# gen_init_cpio device-node list: gives the embedded initramfs a real /dev/console
# so PID1 gets a controlling tty (busybox lacks cttyhack). /dev already exists as a
# mountpoint from initramfs-root, so declare only the nodes (no `dir /dev`).
DEVNODE_LIST = """\
nod /dev/console 0600 0 0 c 5 1
nod /dev/null 0666 0 0 c 1 3
nod /dev/tty 0666 0 0 c 5 0
nod /dev/ttyS0 0660 0 0 c 4 64
nod /dev/kmsg 0644 0 0 c 1 11
"""


def run(cmd, **kw):
    print("+ " + " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, **kw)


def main():
    pathlib.Path(OUT).mkdir(parents=True, exist_ok=True)
    if os.path.exists(WORK):
        try:
            shutil.rmtree(WORK)
        except PermissionError:
            # A prior kernel build wrote root-owned files (lib/modules/…) that
            # this (non-root) process can't unlink — remove them as root.
            run(["docker", "run", "--rm", "--platform=linux/arm64",
                 "-v", f"{OUT}:/out", IMAGE, "rm", "-rf", "/out/initramfs-root"])
    os.makedirs(WORK, exist_ok=True)

    run(["docker", "pull", "--platform=linux/arm64", IMAGE])
    # Install libgpiod (gpioset/gpioinfo) + i2c-tools into the rootfs, then export.
    cid = subprocess.check_output(
        ["docker", "create", "--platform=linux/arm64", IMAGE,
         "sh", "-c", "apk add --no-cache libgpiod i2c-tools"]).decode().strip()
    try:
        run(["docker", "start", "-a", cid])   # runs apk add (needs network)
        tar = os.path.join(OUT, "alpine-arm64.tar")
        with open(tar, "wb") as f:
            run(["docker", "export", cid], stdout=f)
    finally:
        run(["docker", "rm", cid])
    run(["tar", "-xf", tar, "-C", WORK])
    os.remove(tar)

    # Docker export writes /dev/console as a regular file -> breaks devtmpfs.
    # Keep an empty /dev as a mountpoint; real nodes come from the devnode list.
    dev = os.path.join(WORK, "dev")
    if os.path.exists(dev):
        shutil.rmtree(dev)
    os.makedirs(dev)

    for name, body, mode in (("init", INIT, 0o755),
                             ("bin/hdd-power-on", HDD_POWER, 0o755)):
        p = os.path.join(WORK, name)
        pathlib.Path(os.path.dirname(p)).mkdir(parents=True, exist_ok=True)
        with open(p, "w") as f:
            f.write(body)
        os.chmod(p, mode)

    pathlib.Path(DEVNODES).write_text(DEVNODE_LIST)
    print(f"devnode list: {DEVNODES}", flush=True)

    make_cpio()


def make_cpio():
    cpio_gz = os.path.join(OUT, "initramfs-ea16.cpio.gz")
    find = subprocess.Popen(["find", "."], cwd=WORK, stdout=subprocess.PIPE)
    cpio = subprocess.Popen(["cpio", "-o", "-H", "newc", "--quiet"],
                            cwd=WORK, stdin=find.stdout, stdout=subprocess.PIPE)
    with open(cpio_gz, "wb") as f:
        gz = subprocess.Popen(["gzip", "-9"], stdin=cpio.stdout, stdout=f)
        gz.communicate()
    find.wait(); cpio.wait()
    print(f"initramfs dir : {WORK}", flush=True)
    print(f"initramfs cpio: {cpio_gz} ({os.path.getsize(cpio_gz)//1024} KiB)", flush=True)


if __name__ == "__main__":
    sys.exit(main())
