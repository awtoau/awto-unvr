#!/usr/bin/env python3
"""Cross-compile the 6.12 ea16 kernel with the HOST's latest toolchain: GCC 16.

Deliberately-risky variant of build-linux-612-ea16.py (which uses the port's
tested gcc-12 Docker image). GCC 16 vs a 6.12 tree may break; if so this reports
where and the gcc-12 artifacts in build-out/gcc12-fallback/ remain the bootable copy.

- Toolchain: host aarch64-linux-gnu-gcc (GCC 16.1.1), binutils 2.46.
- Builds into a SEPARATE objdir (O=) so the gcc-12 in-tree objects are untouched.
- On success, writes gcc-16 artifacts to build-out/ (primary) and keeps a labelled
  copy in build-out/gcc16/.
"""
import os, sys, struct, zlib, subprocess, shutil, pathlib, time

SRC    = "/mnt/2tb/unvr-port-refs/linux-6.12"
PORT   = "/mnt/2tb/unvr-port-refs/linux-alpine-v2"
OUT    = "/mnt/2tb/unvr-port-refs/build-out"
BUILD  = "/mnt/2tb/unvr-port-refs/build-gcc16"
INITRD = "/mnt/2tb/unvr-port-refs/build-out/initramfs-root"
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
ENV = {**os.environ, "ARCH": "arm64", "CROSS_COMPILE": "aarch64-linux-gnu-"}

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = \
    0x27051956, 5, 22, 2, 0
LOAD_ADDR = ENTRY_ADDR = 0x08000000


def run(cmd):
    print("+ " + " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, env=ENV)


def prep_tree():
    amazon = os.path.join(SRC, "arch/arm64/boot/dts/amazon")
    shutil.copy(os.path.join(PORT, "configs/unvr_defconfig"),
                os.path.join(SRC, "arch/arm64/configs/unvr_defconfig"))
    for f in ("dts/alpine-v2-ubnt-unvr-ea16.dts", "dts/alpine-v2-ubnt-unvr.dts",
              "dts/alpine-v2-ubnt-udmpro.dts"):
        s = os.path.join(PORT, f)
        if os.path.exists(s):
            shutil.copy(s, amazon)
    mk = os.path.join(amazon, "Makefile")
    txt = pathlib.Path(mk).read_text()
    if DTS_NAME not in txt:
        pathlib.Path(mk).write_text(
            txt + f"dtb-$(CONFIG_ARCH_ALPINE)\t+= {DTS_NAME}.dtb\n")


def config():
    run(["make", "-C", SRC, f"O={BUILD}", "unvr_defconfig"])
    run(["bash", os.path.join(SRC, "scripts/config"), "--file",
         os.path.join(BUILD, ".config"),
         "--disable", "WERROR",
         "--disable", "TRIM_UNUSED_KSYMS",
         "--enable", "BLK_DEV_INITRD",
         "--set-str", "INITRAMFS_SOURCE", INITRD,
         "--set-val", "INITRAMFS_ROOT_UID", "0",
         "--set-val", "INITRAMFS_ROOT_GID", "0",
         "--enable", "RD_GZIP", "--enable", "INITRAMFS_COMPRESSION_GZIP"])
    run(["make", "-C", SRC, f"O={BUILD}", "olddefconfig"])


def build():
    n = str(os.cpu_count())
    run(["make", "-C", SRC, f"O={BUILD}", f"-j{n}", "Image", "dtbs", "modules"])
    mods = os.path.join(BUILD, "modules")
    if os.path.exists(mods):
        shutil.rmtree(mods)
    os.makedirs(mods)
    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo"):
        dst = os.path.join(mods, m)
        shutil.copytree(os.path.join(PORT, "modules", m), dst)
        run(["make", "-C", BUILD, f"KDIR={BUILD}", f"M={dst}", "modules"])


def mkuimage(img, out, name="unvr-ea16-6.12-gcc16"):
    data = pathlib.Path(img).read_bytes()
    dcrc = zlib.crc32(data) & 0xffffffff
    nm = name.encode()[:31].ljust(32, b"\0")
    def hdr(hc):
        return struct.pack(">IIIIIIIBBBB32s", IH_MAGIC, hc, int(time.time()),
                           len(data), LOAD_ADDR, ENTRY_ADDR, dcrc,
                           IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE, nm)
    h = hdr(zlib.crc32(hdr(0)) & 0xffffffff)
    pathlib.Path(out).write_bytes(h + data)
    print(f"uImage: {out} ({len(h)+len(data)} bytes, load=0x{LOAD_ADDR:08x}, dcrc=0x{dcrc:08x})", flush=True)


def collect():
    g16 = os.path.join(OUT, "gcc16")
    os.makedirs(g16, exist_ok=True)
    img = os.path.join(BUILD, "arch/arm64/boot/Image")
    dtb = os.path.join(BUILD, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    for dst in (OUT, g16):                       # primary + labelled copy
        shutil.copy(img, os.path.join(dst, "Image"))
        shutil.copy(dtb, os.path.join(dst, f"{DTS_NAME}.dtb"))
        shutil.copy(os.path.join(BUILD, ".config"), os.path.join(dst, "unvr-ea16.config"))
        mkuimage(img, os.path.join(dst, "uImage-unvr-ea16"))
        md = os.path.join(dst, "modules")
        if os.path.exists(md):
            shutil.rmtree(md)
        shutil.copytree(os.path.join(BUILD, "modules"), md)
    for root, _, files in os.walk(os.path.join(g16, "modules")):
        for f in files:
            if f.endswith(".ko"):
                p = os.path.join(root, f)
                print(f"  {p}  ({os.path.getsize(p)} bytes)", flush=True)


def main():
    t0 = time.time()
    print("=== GCC-16 host build ===", flush=True)
    subprocess.run(["aarch64-linux-gnu-gcc", "--version"], check=True)
    prep_tree(); config(); build(); collect()
    print(f"GCC16 DONE in {int(time.time()-t0)}s", flush=True)


if __name__ == "__main__":
    sys.exit(main())
