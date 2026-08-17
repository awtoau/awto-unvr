#!/usr/bin/env python3
"""Cross-compile the Linux 6.12 Alpine-V2 port for THIS UNVR (sysid ea16),
WITH the community port's PCIe patch integration and out-of-tree modules
installed into the embedded initramfs.

Host-side (x86 -> arm64), using the port's tested Docker toolchain (Debian
Bookworm, gcc-aarch64-linux-gnu 12). GCC 16 on the host does NOT build 6.12.

Patch integration (this is what makes internal PCIe enumerate SATA/al_dma/al_ssm):
  1. drivers/pci/controller/pcie-al-internal.c  (NEW driver) + Kconfig + Makefile,
     so CONFIG_PCIE_AL_INTERNAL=y (already in unvr_defconfig) actually builds and
     binds to the "annapurna-labs,alpine-internal-pcie" DT node. It sets AXI
     sub-master snoop (SMCC 0x110..0x170) + APP_CONTROL (0x220) for coherent DMA.
  2. drivers/pci/controller/dwc/pcie-al.c  <- patches/pcie-al-dbi-fix.c (external
     PCIe DBI base at controller+0x10000; needed by the xHCI/USB port).

Modules (al_eth/al_dma/al_ssm/al_sgpo) are built, installed into
build-out/initramfs-root/lib/modules/<KVER>/extra, depmod'd, THEN the kernel Image
is linked so they ride inside the embedded initramfs (/init auto-loads them).

Produces in build-out/: uImage-unvr-ea16, Image, alpine-v2-ubnt-unvr-ea16.dtb,
initramfs-ea16.cpio.gz (now with modules), modules/*/al_*.ko, unvr-ea16.config.
"""
import os, sys, struct, zlib, subprocess, shutil, pathlib, time

SRC   = "/mnt/2tb/unvr-port-refs/linux-6.12"
PORT  = "/mnt/2tb/unvr-port-refs/linux-alpine-v2"
OUT   = "/mnt/2tb/unvr-port-refs/build-out"
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
IMAGE_TAG = "alpine-v2-builder"

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = \
    0x27051956, 5, 22, 2, 0
LOAD_ADDR = ENTRY_ADDR = 0x08000000

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


def run(cmd, **kw):
    print("+ " + (cmd if isinstance(cmd, str) else " ".join(cmd)), flush=True)
    subprocess.run(cmd, check=True, **kw)


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
        pathlib.Path(mk).write_text(txt + f"dtb-$(CONFIG_ARCH_ALPINE)\t+= {DTS_NAME}.dtb\n")
    print("prepped tree: defconfig + dts installed", flush=True)


def integrate_patches():
    ctl = os.path.join(SRC, "drivers/pci/controller")
    # 1. internal PCIe driver (new file)
    shutil.copy(os.path.join(PORT, "patches/pcie-al-internal.c"),
                os.path.join(ctl, "pcie-al-internal.c"))
    # Kconfig: add PCIE_AL_INTERNAL just before the closing endmenu
    kc = os.path.join(ctl, "Kconfig")
    txt = pathlib.Path(kc).read_text()
    if "PCIE_AL_INTERNAL" not in txt:
        idx = txt.rstrip().rfind("endmenu")
        txt = txt[:idx] + KCONFIG_SNIPPET + "\n" + txt[idx:]
        pathlib.Path(kc).write_text(txt)
    # Makefile: add build rule
    mkf = os.path.join(ctl, "Makefile")
    mtxt = pathlib.Path(mkf).read_text()
    if "pcie-al-internal.o" not in mtxt:
        pathlib.Path(mkf).write_text(mtxt.rstrip("\n") + "\n" + MAKEFILE_LINE)
    # 2. external PCIe DBI fix (overwrite mainline pcie-al.c)
    shutil.copy(os.path.join(PORT, "patches/pcie-al-dbi-fix.c"),
                os.path.join(SRC, "drivers/pci/controller/dwc/pcie-al.c"))
    print("integrated patches: pcie-al-internal (+Kconfig/Makefile) + pcie-al DBI fix",
          flush=True)


def prep_modules():
    dst = os.path.join(OUT, "modules")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo"):
        shutil.copytree(os.path.join(PORT, "modules", m), os.path.join(dst, m))


def docker_build():
    run(["docker", "build", "-t", IMAGE_TAG, "-f",
         os.path.join(PORT, "configs/Dockerfile"), PORT])


CONTAINER_SCRIPT = r"""
set -e
export ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
cd /src
make unvr_defconfig
./scripts/config --file .config \
  --disable WERROR \
  --disable TRIM_UNUSED_KSYMS \
  --enable BLK_DEV_INITRD \
  --enable PCIE_AL_INTERNAL \
  --enable EXPERT --enable GPIO_SYSFS \
  --set-str INITRAMFS_SOURCE "/initramfs-root /initramfs-devnodes" \
  --set-val INITRAMFS_ROOT_UID 0 \
  --set-val INITRAMFS_ROOT_GID 0 \
  --enable RD_GZIP --enable INITRAMFS_COMPRESSION_GZIP
make olddefconfig
grep -q 'CONFIG_PCIE_AL_INTERNAL=y' .config || { echo "FATAL: PCIE_AL_INTERNAL not set"; exit 1; }
KVER=$(make -s kernelrelease)
echo "KVER=$KVER"
# Build modules FIRST so they can be embedded in the initramfs.
make -j"$(nproc)" modules
for m in al_eth al_dma al_ssm al_sgpo; do
  echo "=== out-of-tree $m ==="
  make -C /src KDIR=/src M=/modules/$m modules
done
MDIR=/initramfs-root/lib/modules/$KVER
mkdir -p $MDIR/extra
for m in al_eth al_dma al_ssm al_sgpo; do cp /modules/$m/$m.ko $MDIR/extra/; done
cp modules.builtin modules.order $MDIR/ 2>/dev/null || true
cp modules.builtin.modinfo $MDIR/ 2>/dev/null || true
depmod -b /initramfs-root $KVER || true
# Now link the kernel (embeds initramfs-root, which now contains the modules).
make -j"$(nproc)" Image dtbs
echo "=== container build complete (KVER=$KVER) ==="
"""


def docker_run():
    run(["docker", "run", "--rm",
         "-v", f"{SRC}:/src",
         "-v", f"{OUT}/modules:/modules",
         "-v", f"{OUT}/initramfs-root:/initramfs-root",
         "-v", f"{OUT}/initramfs-devnodes:/initramfs-devnodes:ro",
         IMAGE_TAG, "bash", "-c", CONTAINER_SCRIPT])


def mkuimage(image_path, out_path, name="unvr-ea16-6.12"):
    data = pathlib.Path(image_path).read_bytes()
    dcrc = zlib.crc32(data) & 0xffffffff
    nm = name.encode()[:31].ljust(32, b"\0")
    def hdr(hc):
        return struct.pack(">IIIIIIIBBBB32s", IH_MAGIC, hc, int(time.time()),
                           len(data), LOAD_ADDR, ENTRY_ADDR, dcrc,
                           IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE, nm)
    h = hdr(zlib.crc32(hdr(0)) & 0xffffffff)
    pathlib.Path(out_path).write_bytes(h + data)
    print(f"uImage: {out_path} ({len(h)+len(data)} bytes, load/entry=0x{LOAD_ADDR:08x}, dcrc=0x{dcrc:08x})", flush=True)


def regen_initramfs_cpio():
    """Rebuild the standalone cpio.gz now that initramfs-root has the modules."""
    work = os.path.join(OUT, "initramfs-root")
    cpio_gz = os.path.join(OUT, "initramfs-ea16.cpio.gz")
    find = subprocess.Popen(["find", "."], cwd=work, stdout=subprocess.PIPE)
    cpio = subprocess.Popen(["cpio", "-o", "-H", "newc", "--quiet"],
                            cwd=work, stdin=find.stdout, stdout=subprocess.PIPE)
    with open(cpio_gz, "wb") as f:
        gz = subprocess.Popen(["gzip", "-9"], stdin=cpio.stdout, stdout=f)
        gz.communicate()
    find.wait(); cpio.wait()
    print(f"standalone initramfs (with modules): {cpio_gz} "
          f"({os.path.getsize(cpio_gz)//1024} KiB)", flush=True)


def collect():
    image = os.path.join(SRC, "arch/arm64/boot/Image")
    dtb = os.path.join(SRC, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}.dtb"))
    shutil.copy(os.path.join(SRC, ".config"), os.path.join(OUT, "unvr-ea16.config"))
    mkuimage(image, os.path.join(OUT, "uImage-unvr-ea16"))
    regen_initramfs_cpio()
    for root, _, files in os.walk(os.path.join(OUT, "modules")):
        for f in files:
            if f.endswith(".ko"):
                p = os.path.join(root, f)
                print(f"  module {os.path.basename(p)}  ({os.path.getsize(p)} bytes)", flush=True)


def main():
    t0 = time.time()
    prep_tree()
    integrate_patches()
    prep_modules()
    docker_build()
    docker_run()
    collect()
    print(f"DONE in {int(time.time()-t0)}s", flush=True)


if __name__ == "__main__":
    sys.exit(main())
