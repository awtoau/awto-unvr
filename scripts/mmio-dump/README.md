# mmio_dump — read SoC registers on a locked-down kernel

One-shot, read-only kernel module: `ioremap`s an MMIO region and prints it to
`dmesg`, then auto-unloads. The way to peek at hardware registers when
`/dev/mem` is blocked by `STRICT_DEVMEM`/`IO_STRICT_DEVMEM` (stock + hardened
kernels — including the stock 4.19 and a fresh Fedora before our rebuild).

## Build (against the TARGET kernel's tree — vermagic must match the box)
- `make KDIR=/mnt/2tb/unvr-port-refs/linux-v7.1.8 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-`
- Do NOT build against a tree that's mid-`make` (artifacts change under you).

## Run on the box
- `insmod mmio_dump.ko base=0xf0080000 len=0x400` — Alpine V2 DDR uMCTL2 controller
- `insmod mmio_dump.ko base=0xf0088000 len=0x100` — DDR PHY head (impedance/ODT/vref)
- `dmesg | grep mmio_dump`
- Auto-unloads (`init` returns `-EINVAL` after dumping) — nothing to `rmmod`, nothing stays mapped.

## Notes
- Read-only (`readl` only). Safe.
- **Not needed** on a kernel with `STRICT_DEVMEM=n` — use `/dev/mem` / `devmem` directly.
  Kept because it works on ANY kernel where userspace `/dev/mem` is denied.
- General tool: `base`/`len` dump any MMIO region, not just DDR.
