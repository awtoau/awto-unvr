#!/usr/bin/env python3
"""Production kernel: Fedora 44 aarch64 config + our Alpine V2 patches, no UEFI.

"Take Fedora stock and patch like we did to 7.1" — Fedora 44 ships kernel 7.1.8,
the exact version we patched. This builds the SAME source tree (our Alpine
patches already integrated by build-linux-71-ea16.py) but with **Fedora's own
aarch64 kernel config** as the base, so every systemd/fs/net symbol is correct
by construction (no hand-curation). Booted by U-Boot (no UEFI/GRUB) via
root=PARTUUID, no embedded initramfs -> the kernel runs Fedora's systemd.

Prereq: run build-linux-71-ea16.py first (integrates the patches into the tree)
and have tmp/fedora-kernel/fedora-aarch64.config extracted.

Out: build-out-71-fedora/  (uImage-unvr-ea16-7.1-fedora, dtb, config, modroot/).
Deploy modroot/lib/modules/<kv> -> the Fedora rootfs /lib/modules; boot the uImage.
"""

import os, sys, struct, zlib, subprocess, shutil, pathlib, time, multiprocessing

SRC   = "/mnt/2tb/unvr-port-refs/linux-v7.1.8"
PORT  = "/mnt/2tb/unvr-port-refs/linux-alpine-v2"
OUT   = "/mnt/2tb/unvr-port-refs/build-out-71-fedora"
FEDORA_CONFIG = "/mnt/2tb/git/awto-unvr/tmp/fedora-kernel/fedora-aarch64.config"
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
VER = "7.1"
CROSS = "aarch64-linux-gnu-"
NPROC = str(multiprocessing.cpu_count())

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = \
    0x27051956, 5, 22, 2, 0
LOAD_ADDR = ENTRY_ADDR = 0x08080000
ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE=CROSS)


def log(m): print(m, flush=True)
def run(cmd, **kw):
    log("+ " + (cmd if isinstance(cmd, str) else " ".join(cmd)))
    subprocess.run(cmd, check=True, env=ENV, **kw)


def configure():
    shutil.copy(FEDORA_CONFIG, os.path.join(SRC, ".config"))
    cfg = os.path.join(SRC, "scripts/config")
    run([cfg, "--file", os.path.join(SRC, ".config"),
         # our platform + drivers (off in stock Fedora)
         "--enable", "ARCH_ALPINE",
         "--enable", "PCIE_AL", "--enable", "PCIE_AL_INTERNAL",
         "--enable", "EXPERT", "--enable", "GPIO_SYSFS",
         # no embedded initramfs -> kernel runs systemd from root=PARTUUID
         "--set-str", "INITRAMFS_SOURCE", "",
         # drop build-complexity we don't need: BTF (needs matching pahole),
         # module signing (our OOT al_* are unsigned), Fedora key/cert requirements,
         # and full debuginfo (huge/slow). None affect booting Fedora.
         "--disable", "DEBUG_INFO_BTF", "--disable", "DEBUG_INFO_BTF_MODULES",
         "--disable", "MODULE_SIG", "--disable", "MODULE_SIG_ALL",
         "--disable", "MODULE_SIG_FORCE", "--disable", "SYSTEM_REVOCATION_KEYS",
         "--disable", "SECURITY_LOCKDOWN_LSM",
         # unrestricted /dev/mem: read/write SoC MMIO from userspace (DDR
         # controller/PHY regs, live hardware RE). Our own box, owner directive.
         "--enable", "DEVMEM",
         "--disable", "STRICT_DEVMEM", "--disable", "IO_STRICT_DEVMEM",
         "--enable", "DEBUG_INFO_NONE",
         "--disable", "WERROR"])
    run(["make", "-C", SRC, "olddefconfig"])
    dotcfg = pathlib.Path(os.path.join(SRC, ".config")).read_text()
    for sym in ("CONFIG_ARCH_ALPINE=y", "CONFIG_PCIE_AL_INTERNAL=y", "CONFIG_PCIE_AL=y"):
        if sym not in dotcfg:
            log(f"FATAL: {sym} not set after olddefconfig")
            sys.exit(1)
    log("configured: Fedora base + ARCH_ALPINE + PCIE_AL_INTERNAL confirmed")


def kver():
    return subprocess.check_output(["make", "-s", "-C", SRC, "kernelrelease"],
                                   env=ENV).decode().strip()


def adapt_sgpo(mpath):
    """7.1 gpio_chip.set returns int (same fix as build-linux-71-ea16.py)."""
    f = os.path.join(mpath, "al_sgpo.c")
    t = pathlib.Path(f).read_text()
    t = t.replace(
        "static void al_sgpo_set(struct gpio_chip *gc, unsigned int offset, int value)\n{",
        "static int al_sgpo_set(struct gpio_chip *gc, unsigned int offset, int value)\n{")
    t = t.replace(
        "\tif (group >= sgpo->num_groups)\n\t\treturn;\n\n\tspin_lock_irqsave(&sgpo->lock, flags);\n\n\twritel(value ? (1 << bit) : 0,\n\t       al_sgpo_group_reg(sgpo, group, GRP_VEC(1 << bit)));\n\n\tspin_unlock_irqrestore(&sgpo->lock, flags);\n}",
        "\tif (group >= sgpo->num_groups)\n\t\treturn -EINVAL;\n\n\tspin_lock_irqsave(&sgpo->lock, flags);\n\n\twritel(value ? (1 << bit) : 0,\n\t       al_sgpo_group_reg(sgpo, group, GRP_VEC(1 << bit)));\n\n\tspin_unlock_irqrestore(&sgpo->lock, flags);\n\n\treturn 0;\n}")
    pathlib.Path(f).write_text(t)


def build():
    os.makedirs(OUT, exist_ok=True)
    configure()
    kv = kver()
    log(f"KVER={kv}")
    run(["make", "-C", SRC, f"-j{NPROC}", "Image", "dtbs", "modules"])

    # module tree for the Fedora rootfs
    modroot = os.path.join(OUT, "modroot")
    if os.path.exists(modroot):
        shutil.rmtree(modroot)
    os.makedirs(modroot)
    run(["make", "-C", SRC, f"INSTALL_MOD_PATH={modroot}",
         "INSTALL_MOD_STRIP=1", "modules_install"])

    # out-of-tree al_* modules into the same tree
    extra = os.path.join(modroot, f"lib/modules/{kv}/extra")
    os.makedirs(extra, exist_ok=True)
    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo"):
        mpath = os.path.join(OUT, m)
        if os.path.exists(mpath):
            shutil.rmtree(mpath)
        shutil.copytree(os.path.join(PORT, "modules", m), mpath)
        if m == "al_sgpo":
            adapt_sgpo(mpath)
        try:
            run(["make", "-C", SRC, f"KDIR={SRC}", f"M={mpath}", f"-j{NPROC}", "modules"])
            shutil.copy(os.path.join(mpath, f"{m}.ko"), extra)
            log(f"OOT {m}: ok")
        except subprocess.CalledProcessError as e:
            log(f"OOT {m}: FAILED ({e})")
    subprocess.run(["depmod", "-b", modroot, kv], check=False)

    # collect boot artifacts
    image = os.path.join(SRC, "arch/arm64/boot/Image")
    dtb = os.path.join(SRC, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}-{VER}.dtb"))
    shutil.copy(os.path.join(SRC, ".config"), os.path.join(OUT, f"unvr-ea16-{VER}-fedora.config"))
    mkuimage(image, os.path.join(OUT, f"uImage-unvr-ea16-{VER}-fedora"), kv)
    log(f"DONE. modroot: {modroot} ; uImage: {OUT}/uImage-unvr-ea16-{VER}-fedora")


def mkuimage(image_path, out_path, kv):
    data = pathlib.Path(image_path).read_bytes()
    dcrc = zlib.crc32(data) & 0xffffffff
    nm = f"unvr-ea16-{VER}-fedora".encode()[:31].ljust(32, b"\0")
    def hdr(hc):
        return struct.pack(">IIIIIIIBBBB32s", IH_MAGIC, hc, int(time.time()),
                           len(data), LOAD_ADDR, ENTRY_ADDR, dcrc,
                           IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL,
                           IH_COMP_NONE, nm)
    h = hdr(zlib.crc32(hdr(0)) & 0xffffffff)
    pathlib.Path(out_path).write_bytes(h + data)
    log(f"uImage: {out_path} ({len(h)+len(data)} bytes, load/entry=0x{LOAD_ADDR:08x})")


if __name__ == "__main__":
    t0 = time.time()
    build()
    log(f"TOTAL {int(time.time()-t0)}s")
