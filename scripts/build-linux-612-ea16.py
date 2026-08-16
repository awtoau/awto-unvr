#!/usr/bin/env python3
"""Cross-compile the Linux 6.12 Alpine-V2 port for THIS UNVR (sysid ea16).

Host-side (x86 -> arm64) only. Uses the port's tested Docker toolchain
(Debian Bookworm, gcc-aarch64-linux-gnu 12) rather than the host's bleeding-edge
gcc so results match the port authors' proven build.

Produces, in build-out/:
  Image                          - raw arm64 kernel (initramfs embedded)
  uImage-unvr-ea16               - legacy U-Boot image (CRC32, unsigned) for bootm
  alpine-v2-ubnt-unvr-ea16.dtb   - this unit's device tree
  modules/*/al_*.ko              - out-of-tree drivers (al_eth/al_dma/al_ssm/al_sgpo)
  unvr-ea16.config               - the exact kernel .config used

Idempotent / resumable: builds in-place in the kernel tree so an interrupted run
(host reboot) resumes incrementally on re-invocation. Logs to tmp/logs/linux-build.log
when invoked with stdout/stderr redirected there.
"""
import os, sys, struct, zlib, subprocess, shutil, pathlib, time

SRC   = "/mnt/2tb/unvr-port-refs/linux-6.12"
PORT  = "/mnt/2tb/unvr-port-refs/linux-alpine-v2"
OUT   = "/mnt/2tb/unvr-port-refs/build-out"
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
IMAGE_TAG = "alpine-v2-builder"

# U-Boot legacy image constants
IH_MAGIC = 0x27051956
IH_OS_LINUX = 5
IH_ARCH_ARM64 = 22
IH_TYPE_KERNEL = 2
IH_COMP_NONE = 0
# arm64 kernel load/entry for netboot bootm (2 MiB aligned, clear of TFTP staging)
LOAD_ADDR = 0x08080000
ENTRY_ADDR = 0x08080000


def run(cmd, **kw):
    print("+ " + (cmd if isinstance(cmd, str) else " ".join(cmd)), flush=True)
    subprocess.run(cmd, check=True, **kw)


def prep_tree():
    amazon = os.path.join(SRC, "arch/arm64/boot/dts/amazon")
    cfgdir = os.path.join(SRC, "arch/arm64/configs")
    shutil.copy(os.path.join(PORT, "configs/unvr_defconfig"),
                os.path.join(cfgdir, "unvr_defconfig"))
    # DTS: our ea16 variant + the port's originals (so includes resolve identically)
    for f in ("dts/alpine-v2-ubnt-unvr-ea16.dts",
              "dts/alpine-v2-ubnt-unvr.dts",
              "dts/alpine-v2-ubnt-udmpro.dts"):
        src = os.path.join(PORT, f)
        if os.path.exists(src):
            shutil.copy(src, amazon)
    # Register ea16 dtb in the amazon Makefile (idempotent)
    mk = os.path.join(amazon, "Makefile")
    line = f"dtb-$(CONFIG_ARCH_ALPINE)\t+= {DTS_NAME}.dtb\n"
    txt = pathlib.Path(mk).read_text()
    if DTS_NAME not in txt:
        pathlib.Path(mk).write_text(txt + line)
    print(f"prepped tree: defconfig+dts installed, {DTS_NAME} in Makefile", flush=True)


def prep_modules():
    dst = os.path.join(OUT, "modules")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo"):
        shutil.copytree(os.path.join(PORT, "modules", m), os.path.join(dst, m))
    print("copied out-of-tree modules to build-out/modules", flush=True)


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
  --set-str INITRAMFS_SOURCE /initramfs-root \
  --set-val INITRAMFS_ROOT_UID 0 \
  --set-val INITRAMFS_ROOT_GID 0 \
  --enable RD_GZIP --enable INITRAMFS_COMPRESSION_GZIP
make olddefconfig
make -j"$(nproc)" Image dtbs modules
for m in al_eth al_dma al_ssm al_sgpo; do
  echo "=== building out-of-tree $m ==="
  make -C /src KDIR=/src M=/modules/$m modules
done
echo "=== container build complete ==="
"""


def docker_run():
    run(["docker", "run", "--rm",
         "-v", f"{SRC}:/src",
         "-v", f"{OUT}/modules:/modules",
         "-v", f"{OUT}/initramfs-root:/initramfs-root:ro",
         IMAGE_TAG, "bash", "-c", CONTAINER_SCRIPT])


def mkuimage(image_path, out_path, name="unvr-ea16-6.12"):
    data = pathlib.Path(image_path).read_bytes()
    dcrc = zlib.crc32(data) & 0xffffffff
    nm = name.encode()[:31].ljust(32, b"\0")
    hdr = struct.pack(">IIIIIIIBBBB32s",
                      IH_MAGIC, 0, int(time.time()), len(data),
                      LOAD_ADDR, ENTRY_ADDR, dcrc,
                      IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE, nm)
    hcrc = zlib.crc32(hdr) & 0xffffffff
    hdr = struct.pack(">IIIIIIIBBBB32s",
                      IH_MAGIC, hcrc, int(time.time()), len(data),
                      LOAD_ADDR, ENTRY_ADDR, dcrc,
                      IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE, nm)
    pathlib.Path(out_path).write_bytes(hdr + data)
    print(f"uImage: {out_path} ({len(hdr)+len(data)} bytes, "
          f"load/entry=0x{LOAD_ADDR:08x}, dcrc=0x{dcrc:08x})", flush=True)


def collect():
    image = os.path.join(SRC, "arch/arm64/boot/Image")
    dtb = os.path.join(SRC, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}.dtb"))
    shutil.copy(os.path.join(SRC, ".config"), os.path.join(OUT, "unvr-ea16.config"))
    mkuimage(image, os.path.join(OUT, "uImage-unvr-ea16"))
    # gather .ko
    kdir = os.path.join(OUT, "modules")
    kos = []
    for root, _, files in os.walk(kdir):
        for f in files:
            if f.endswith(".ko"):
                kos.append(os.path.join(root, f))
    print("modules built: " + ", ".join(sorted(os.path.basename(k) for k in kos)), flush=True)
    for k in kos:
        print(f"  {k}  ({os.path.getsize(k)} bytes)", flush=True)


def main():
    t0 = time.time()
    prep_tree()
    prep_modules()
    docker_build()
    docker_run()
    collect()
    print(f"DONE in {int(time.time()-t0)}s", flush=True)


if __name__ == "__main__":
    sys.exit(main())
