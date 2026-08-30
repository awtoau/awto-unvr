#!/usr/bin/env python3
"""Forward-port the Alpine-V2 ea16 build onto AWTO_KERNEL_SRC.

Cross-compiled on the HOST (no docker) with the host aarch64-linux-gnu-gcc (16).
Netboot-ready: legacy uImage (initramfs embedded) + ea16 DTB. No flashing.

Forward-port deltas needed against current AWTO_KERNEL_SRC (7.2.0):
  1. pcie-al-internal.c: pci_host_common_probe/remove declared in the
     PRIVATE header drivers/pci/controller/pci-host-common.h ->
     `#include "pci-host-common.h"`; no `.remove_new` (removed upstream
     6.15), pci_host_common_remove() returns void -> `.remove`.
  2. pcie-al.c: `pci->pp.native_ecam = true;` and the
     `dev_dbg("From DT: controller_base"...)` anchor still present - the
     DBI-at-+0x10000 pre-set is inserted in place (keeps native_ecam).
  3. al_sgpo.c: gpio_chip.set is `int (*set)(gc, offset, value)` ->
     al_sgpo_set returns int.
  * DTS: dts/alpine-v2.dtsi is byte-identical to upstream's own
    arch/arm64/boot/dts/amazon/alpine-v2.dtsi at this tree's HEAD.
  * al_eth/al_dma/al_ssm: built as-is; any new netdev/phylink/dmaengine/crypto
    break is caught per-module (non-fatal) and reported.

Toolchain: host aarch64-linux-gnu-gcc (GCC 16). WERROR disabled defensively.

Order: configure -> `make modules` (builds vmlinux + Module.symvers) ->
out-of-tree al_* -> install into initramfs -> `make Image dtbs` (embeds the
now-populated initramfs) -> pure-python uImage wrap.

Out: /mnt/2tb/unvr-port-refs/build-out-ea16/
"""

import os
import pathlib
import shutil
import struct
import subprocess
import sys
import time
import zlib

from _repo import NPROC, REPO, ea16_build_out, kernel_build_ver  # -j28 host build parallelism (#146); self-locating repo root

SRC = os.environ.get("AWTO_KERNEL_SRC", "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh")
PORT = os.environ.get("AWTO_KERNEL_PORT", "/mnt/2tb/unvr-port-refs/linux-alpine-v2")
# _repo.py's REPO self-locates via git rev-parse, correct wherever this
# script runs from.
OUT = str(ea16_build_out())
# kbuild's own O= output dir, independent of SRC's own in-tree state (a
# build-fedora run against the same SRC needs this to not collide).
KOUT = os.path.join(OUT, "kbuild")
INITRAMFS_BASE = "/mnt/2tb/unvr-port-refs/build-out"  # base busybox initramfs skeleton, kernel-version-agnostic
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
VER = kernel_build_ver()  # baked into output filenames/uImage Image Name
# Empty string for a native build (e.g. running this script ON woomera itself).
CROSS = os.environ.get("AWTO_CROSS_COMPILE", "aarch64-linux-gnu-")
MIN_FREE_GB = 15

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = (
    0x27051956,
    5,
    22,
    2,
    0,
)
LOAD_ADDR = ENTRY_ADDR = 0x08000000

# Production mode: no embedded initramfs, so the kernel runs /sbin/init (systemd)
# from the real root (root=PARTUUID=..., kernel's own rootwait waits for SATA).
# The bring-up initramfs would otherwise hijack the boot into a shell. Output is
# suffixed so it never clobbers the installer uImage. See docs/fedora-on-ssd.md.
# (Audit #43 kernel-config adds are a later rebuild; this is the minimal change
# to get Fedora booting.)
PROD = ("--production" in sys.argv) or os.environ.get("UNVR_PRODUCTION") == "1"
SUFFIX = "-prod" if PROD else ""

KCONFIG_SNIPPET = """
config PCIE_AL_INTERNAL
\tbool "Annapurna Labs Alpine V2 internal PCIe host controller"
\tdepends on OF && (ARM64 || COMPILE_TEST)
\tselect PCI_HOST_COMMON
\tselect IRQ_DOMAIN
\thelp
\t  Host controller for the Alpine V2 internal PCIe bus (on-SoC AHCI,
\t  Ethernet, crypto, DMA). Configures AXI sub-master snoop (SMCC) and
\t  APP_CONTROL so internal bus masters do cache-coherent DMA.
"""
MAKEFILE_LINE = "obj-$(CONFIG_PCIE_AL_INTERNAL) += pcie-al-internal.o\n"

ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE=CROSS)


def log(msg):
    print(msg, flush=True)


def run(cmd, **kw):
    log("+ " + (cmd if isinstance(cmd, str) else " ".join(cmd)))
    subprocess.run(cmd, check=True, env=ENV, **kw)


def check_space():
    # Was hardcoded to /mnt/2tb (dev host's data mount) - check OUT's own
    # mount instead, correct wherever this actually runs.
    st = os.statvfs(OUT if os.path.isdir(OUT) else os.path.dirname(OUT) or ".")
    free_gb = st.f_bavail * st.f_frsize / 1e9
    log(f"free space at {OUT}: {free_gb:.1f} GB")
    if free_gb < MIN_FREE_GB:
        log(f"ABORT: <{MIN_FREE_GB} GB free")
        sys.exit(2)


def prep_tree():
    os.makedirs(OUT, exist_ok=True)
    amazon = os.path.join(SRC, "arch/arm64/boot/dts/amazon")
    shutil.copy(
        os.path.join(PORT, "configs/unvr_defconfig"),
        os.path.join(SRC, "arch/arm64/configs/unvr_defconfig"),
    )
    # ea16 board DTS = the repo's tracked hardware-of-record (single source of
    # truth, dts/). Reference boards (unvr/udmpro) still come from PORT.
    shutil.copy(os.path.join(REPO, "dts", f"{DTS_NAME}.dts"), amazon)
    for f in ("dts/alpine-v2-ubnt-unvr.dts", "dts/alpine-v2-ubnt-udmpro.dts"):
        s = os.path.join(PORT, f)
        if os.path.exists(s):
            shutil.copy(s, amazon)
    mk = os.path.join(amazon, "Makefile")
    txt = pathlib.Path(mk).read_text()
    if DTS_NAME not in txt:
        pathlib.Path(mk).write_text(
            txt + f"dtb-$(CONFIG_ARCH_ALPINE)\t+= {DTS_NAME}.dtb\n"
        )
    log("prepped tree: defconfig + dts installed")


def integrate_patches():
    ctl = os.path.join(SRC, "drivers/pci/controller")
    # 1. internal PCIe driver (new file) + 7.1 API adaptations (same as 6.18).
    dst = os.path.join(ctl, "pcie-al-internal.c")
    src_txt = pathlib.Path(os.path.join(PORT, "patches/pcie-al-internal.c")).read_text()
    # 7.1: pci_host_common_probe/remove decls live in a private header.
    if '#include "pci-host-common.h"' not in src_txt:
        src_txt = src_txt.replace(
            "#include <linux/pci-ecam.h>\n",
            '#include <linux/pci-ecam.h>\n#include "pci-host-common.h"\n',
            1,
        )
    # 7.1: platform_driver .remove_new removed; .remove is void-returning.
    src_txt = src_txt.replace(
        ".remove_new = pci_host_common_remove", ".remove = pci_host_common_remove"
    )
    pathlib.Path(dst).write_text(src_txt)

    kc = os.path.join(ctl, "Kconfig")
    txt = pathlib.Path(kc).read_text()
    if "PCIE_AL_INTERNAL" not in txt:
        idx = txt.rstrip().rfind("endmenu")
        txt = txt[:idx] + KCONFIG_SNIPPET + "\n" + txt[idx:]
        pathlib.Path(kc).write_text(txt)
    mkf = os.path.join(ctl, "Makefile")
    mtxt = pathlib.Path(mkf).read_text()
    if "pcie-al-internal.o" not in mtxt:
        pathlib.Path(mkf).write_text(mtxt.rstrip("\n") + "\n" + MAKEFILE_LINE)

    # 2. External DWC PCIe DBI fix: insert into 7.1's own pcie-al.c in place,
    #    preserving the `native_ecam = true` line.
    al = os.path.join(SRC, "drivers/pci/controller/dwc/pcie-al.c")
    atxt = pathlib.Path(al).read_text()
    if "controller_res->start + 0x10000" not in atxt:
        anchor = '\tdev_dbg(dev, "From DT: controller_base: %pR\\n", controller_res);\n'
        block = (
            "\n\t/*\n"
            "\t * Alpine V2: DWC DBI registers sit at controller_base + 0x10000.\n"
            '\t * Pre-set dbi_base so dw_pcie_get_resources() skips the "dbi"\n'
            "\t * resource lookup and avoids a resource conflict (forward-port of\n"
            "\t * the 6.12 patches/pcie-al-dbi-fix.c, applied in place for 7.1).\n"
            "\t */\n"
            "\tpci->dbi_base = devm_ioremap(dev, controller_res->start + 0x10000,\n"
            "\t\t\t\t     0x10000);\n"
            "\tif (!pci->dbi_base) {\n"
            '\t\tdev_err(dev, "couldn\'t map DBI base\\n");\n'
            "\t\treturn -ENOMEM;\n"
            "\t}\n"
        )
        if anchor not in atxt:
            raise RuntimeError("pcie-al.c anchor not found; 7.1 source changed")
        atxt = atxt.replace(anchor, anchor + block, 1)
        pathlib.Path(al).write_text(atxt)
    log("integrated patches: pcie-al-internal (7.1 adapted) + pcie-al DBI in-place")


def prep_initramfs():
    """Reuse the 6.12 initramfs content, minus its lib/modules; refresh banner."""
    dst = os.path.join(OUT, "initramfs-root")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    # symlinks=True is REQUIRED: the alpine rootfs is hundreds of busybox
    # symlinks; dereferencing them balloons 11 MB -> 400+ MB (and the Image).
    shutil.copytree(
        os.path.join(INITRAMFS_BASE, "initramfs-root"),
        dst,
        symlinks=True,
        ignore=shutil.ignore_patterns("modules"),
    )
    # copytree above keeps lib/modules/<ver> dirs; drop the whole lib/modules.
    libmod = os.path.join(dst, "lib/modules")
    if os.path.exists(libmod):
        shutil.rmtree(libmod)
    # refresh the cosmetic banner (module loading itself is $(uname -r)-driven).
    init = os.path.join(dst, "init")
    it = pathlib.Path(init).read_text()
    for old in ("Linux 6.12 first boot", "Linux 6.18 first boot"):
        it = it.replace(old, "Linux 7.1 first boot")
    pathlib.Path(init).write_text(it)
    # devnode list (real /dev/console for PID1 controlling tty).
    shutil.copy(
        os.path.join(INITRAMFS_BASE, "initramfs-devnodes"),
        os.path.join(OUT, "initramfs-devnodes"),
    )
    log("prepped initramfs-root (reused 6.12, modules cleared)")


def configure():
    os.makedirs(KOUT, exist_ok=True)
    run(["make", "-C", SRC, f"O={KOUT}", "unvr_defconfig"])
    cfg = os.path.join(SRC, "scripts/config")
    args = [
        cfg,
        "--file",
        os.path.join(KOUT, ".config"),
        "--disable",
        "WERROR",
        "--disable",
        "TRIM_UNUSED_KSYMS",
        "--enable",
        "PCIE_AL",
        "--enable",
        "PCIE_AL_INTERNAL",
        "--enable",
        "EXPERT",
        "--enable",
        "GPIO_SYSFS",
        # unrestricted /dev/mem for SoC MMIO poking from userspace (DDR/reg RE)
        "--enable",
        "DEVMEM",
        "--disable",
        "STRICT_DEVMEM",
        "--disable",
        "IO_STRICT_DEVMEM",
        # 4K-sector erase so the 4K-aligned NOR config/cksum partitions aren't
        # force-read-only (they don't align to 64K erase blocks).
        "--enable",
        "MTD_SPI_NOR_USE_4K_SECTORS",
    ]
    if PROD:
        # Production: no embedded initramfs -> kernel runs systemd from
        # root=PARTUUID. Add the exact Fedora/systemd-required symbols the audit
        # found missing (docs/improvements-audit.md dim 2) + cheap wins, so stock
        # Fedora boots with nothing degraded.
        args += [
            "--set-str",
            "INITRAMFS_SOURCE",
            "",
            "--enable",
            "PSI",
            "--enable",
            "CGROUP_BPF",
            "--enable",
            "BPF_JIT",
            "--module",
            "BINFMT_MISC",
            "--enable",
            "HUGETLBFS",
            "--enable",
            "CRYPTO_SHA1_ARM64_CE",
            "--enable",
            "CRYPTO_SHA2_ARM64_CE",
            "--enable",
            "CRYPTO_SHA512_ARM64_CE",
            "--enable",
            "XFS_FS",
            "--enable",
            "BTRFS_FS_POSIX_ACL",
            "--enable",
            "ZRAM",
            "--enable",
            "SECURITY_YAMA",
            "--enable",
            "SLAB_FREELIST_HARDENED",
        ]
    else:
        initramfs = f"{OUT}/initramfs-root {OUT}/initramfs-devnodes"
        args += [
            "--enable",
            "BLK_DEV_INITRD",
            "--set-str",
            "INITRAMFS_SOURCE",
            initramfs,
            "--set-val",
            "INITRAMFS_ROOT_UID",
            "0",
            "--set-val",
            "INITRAMFS_ROOT_GID",
            "0",
            "--enable",
            "RD_GZIP",
            "--enable",
            "INITRAMFS_COMPRESSION_GZIP",
        ]
    run(args)
    run(["make", "-C", SRC, f"O={KOUT}", "olddefconfig"])
    dotcfg = pathlib.Path(os.path.join(KOUT, ".config")).read_text()
    for sym in ("CONFIG_PCIE_AL_INTERNAL=y", "CONFIG_PCIE_AL=y"):
        if sym not in dotcfg:
            log(f"FATAL: {sym} not set after olddefconfig")
            sys.exit(1)
    log("configured: PCIE_AL + PCIE_AL_INTERNAL confirmed =y")


def kver():
    return (
        subprocess.check_output(
            ["make", "-s", "-C", SRC, f"O={KOUT}", "kernelrelease"], env=ENV
        )
        .decode()
        .strip()
    )


def build_modules_intree():
    run(["make", "-C", SRC, f"O={KOUT}", f"-j{NPROC}", "modules"])


def adapt_module(m, mpath):
    """Apply 7.1 API forward-port fixes to a copied out-of-tree module.
    al_eth/al_dma/al_ssm build unchanged; only al_sgpo needs a fix."""
    if m == "al_sgpo":
        # 6.15+/7.1: gpio_chip.set (and set_multiple) return int, not void.
        # gcc-14+ makes incompatible-pointer-types a hard error.
        f = os.path.join(mpath, "al_sgpo.c")
        t = pathlib.Path(f).read_text()
        t = t.replace(
            "static void al_sgpo_set(struct gpio_chip *gc, unsigned int offset, int value)\n{",
            "static int al_sgpo_set(struct gpio_chip *gc, unsigned int offset, int value)\n{",
        )
        # the early-return inside the now-int function + a trailing return 0.
        t = t.replace(
            "\tif (group >= sgpo->num_groups)\n\t\treturn;\n\n\tspin_lock_irqsave(&sgpo->lock, flags);\n\n\twritel(value ? (1 << bit) : 0,\n\t       al_sgpo_group_reg(sgpo, group, GRP_VEC(1 << bit)));\n\n\tspin_unlock_irqrestore(&sgpo->lock, flags);\n}",
            "\tif (group >= sgpo->num_groups)\n\t\treturn -EINVAL;\n\n\tspin_lock_irqsave(&sgpo->lock, flags);\n\n\twritel(value ? (1 << bit) : 0,\n\t       al_sgpo_group_reg(sgpo, group, GRP_VEC(1 << bit)));\n\n\tspin_unlock_irqrestore(&sgpo->lock, flags);\n\n\treturn 0;\n}",
        )
        pathlib.Path(f).write_text(t)
        log("adapted al_sgpo: .set callback returns int (7.1 gpio_chip API)")


def build_oot_modules():
    """Build al_eth/al_dma/al_ssm/al_sgpo/al_thermal against 7.1. Non-fatal per
    module so a module API break does not deny us a bootable kernel; failures
    are reported."""
    kv = kver()
    mdst = os.path.join(OUT, "modules")
    if os.path.exists(mdst):
        shutil.rmtree(mdst)
    os.makedirs(mdst)
    results = {}
    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo", "al_thermal"):
        mpath = os.path.join(mdst, m)
        # imported source-of-truth: repo modules/ (carries iofic + crypto fixes).
        shutil.copytree(os.path.join(REPO, "modules", m), mpath)
        adapt_module(m, mpath)
        log(f"=== out-of-tree {m} ===")
        try:
            subprocess.run(
                [
                    "make",
                    "-C",
                    KOUT,
                    f"M={mpath}",
                    f"-j{NPROC}",
                    "modules",
                ],
                check=True,
                env=ENV,
            )
            results[m] = os.path.exists(os.path.join(mpath, f"{m}.ko"))
        except subprocess.CalledProcessError as e:
            log(f"MODULE BUILD FAILED: {m} ({e})")
            results[m] = False
    # install the ones that built into the embedded initramfs.
    extra = os.path.join(OUT, f"initramfs-root/lib/modules/{kv}/extra")
    os.makedirs(extra, exist_ok=True)
    for m, ok in results.items():
        if ok:
            shutil.copy(os.path.join(mdst, m, f"{m}.ko"), extra)
    # module metadata for modprobe.
    mroot = os.path.join(OUT, f"initramfs-root/lib/modules/{kv}")
    for meta in ("modules.builtin", "modules.order", "modules.builtin.modinfo"):
        s = os.path.join(KOUT, meta)
        if os.path.exists(s):
            shutil.copy(s, mroot)
    subprocess.run(
        ["depmod", "-b", os.path.join(OUT, "initramfs-root"), kv], check=False
    )
    log(
        "out-of-tree module results: "
        + ", ".join(f"{m}={'ok' if ok else 'FAIL'}" for m, ok in results.items())
    )
    return results


def build_image():
    run(["make", "-C", SRC, f"O={KOUT}", f"-j{NPROC}", "Image", "dtbs"])


def mkuimage(image_path, out_path, name=f"unvr-ea16-{VER}"):
    data = pathlib.Path(image_path).read_bytes()
    dcrc = zlib.crc32(data) & 0xFFFFFFFF
    nm = name.encode()[:31].ljust(32, b"\0")

    def hdr(hc):
        return struct.pack(
            ">IIIIIIIBBBB32s",
            IH_MAGIC,
            hc,
            int(time.time()),
            len(data),
            LOAD_ADDR,
            ENTRY_ADDR,
            dcrc,
            IH_OS_LINUX,
            IH_ARCH_ARM64,
            IH_TYPE_KERNEL,
            IH_COMP_NONE,
            nm,
        )

    h = hdr(zlib.crc32(hdr(0)) & 0xFFFFFFFF)
    pathlib.Path(out_path).write_bytes(h + data)
    log(
        f"uImage: {out_path} ({len(h) + len(data)} bytes, "
        f"load/entry=0x{LOAD_ADDR:08x}, dcrc=0x{dcrc:08x})"
    )


def regen_initramfs_cpio():
    work = os.path.join(OUT, "initramfs-root")
    cpio_gz = os.path.join(OUT, f"initramfs-ea16-{VER}.cpio.gz")
    find = subprocess.Popen(["find", "."], cwd=work, stdout=subprocess.PIPE)
    cpio = subprocess.Popen(
        ["cpio", "-o", "-H", "newc", "--quiet"],
        cwd=work,
        stdin=find.stdout,
        stdout=subprocess.PIPE,
    )
    with open(cpio_gz, "wb") as f:
        gz = subprocess.Popen(["gzip", "-9"], stdin=cpio.stdout, stdout=f)
        gz.communicate()
    find.wait()
    cpio.wait()
    log(f"standalone initramfs: {cpio_gz} ({os.path.getsize(cpio_gz) // 1024} KiB)")


def collect():
    image = os.path.join(KOUT, "arch/arm64/boot/Image")
    dtb = os.path.join(KOUT, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}-{VER}.dtb"))
    shutil.copy(
        os.path.join(KOUT, ".config"), os.path.join(OUT, f"unvr-ea16-{VER}.config")
    )
    mkuimage(image, os.path.join(OUT, f"uImage-unvr-ea16-{VER}"))
    regen_initramfs_cpio()
    for root, _, files in os.walk(os.path.join(OUT, "modules")):
        for f in files:
            if f.endswith(".ko"):
                p = os.path.join(root, f)
                log(f"  module {os.path.basename(p)}  ({os.path.getsize(p)} bytes)")


def main():
    t0 = time.time()
    check_space()
    prep_tree()
    integrate_patches()
    prep_initramfs()
    configure()
    build_modules_intree()
    check_space()
    build_oot_modules()
    build_image()
    collect()
    check_space()
    log(f"DONE in {int(time.time() - t0)}s (KVER={kver()})")


if __name__ == "__main__":
    sys.exit(main())
