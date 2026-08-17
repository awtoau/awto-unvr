// SPDX-License-Identifier: GPL-2.0
/*
 * mmio_dump - one-shot, read-only dump of an arbitrary SoC MMIO region to dmesg.
 *
 * Reads hardware registers from kernel space, so it works even when
 * STRICT_DEVMEM/IO_STRICT_DEVMEM lock /dev/mem (stock + hardened kernels).
 * Arsenal tool for register RE on ANY kernel where userspace /dev/mem is denied.
 *
 *   insmod mmio_dump.ko base=0xf0080000 len=0x400   # Alpine V2 DDR uMCTL2 ctrl
 *   insmod mmio_dump.ko base=0xf0088000 len=0x100   # DDR PHY head
 *   dmesg | grep mmio_dump
 *
 * Read-only (readl only). Returns -EINVAL after dumping so the load "fails" and
 * the module auto-unloads — nothing to rmmod, nothing stays mapped. The .ko
 * must be built against the TARGET kernel (vermagic must match the box).
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>

static unsigned long base = 0xf0080000UL;   /* default: Alpine V2 DDR controller */
static unsigned int  len  = 0x400;
module_param(base, ulong, 0);
MODULE_PARM_DESC(base, "physical MMIO base address");
module_param(len, uint, 0);
MODULE_PARM_DESC(len, "bytes to dump (rounded to 16-byte rows)");

static int __init mmio_dump_init(void)
{
	void __iomem *p;
	unsigned int i;

	p = ioremap(base, len);
	if (!p) {
		pr_err("mmio_dump: ioremap %#lx +%#x failed\n", base, len);
		return -ENOMEM;
	}
	pr_info("mmio_dump: === %#lx +%#x ===\n", base, len);
	for (i = 0; i < len; i += 16)
		pr_info("mmio_dump %#010lx: %08x %08x %08x %08x\n", base + i,
			readl(p + i), readl(p + i + 4),
			readl(p + i + 8), readl(p + i + 12));
	iounmap(p);
	pr_info("mmio_dump: done (auto-unloads)\n");
	return -EINVAL;   /* one-shot: dump, then refuse to stay resident */
}

module_init(mmio_dump_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("One-shot read-only MMIO register dump to dmesg");
MODULE_AUTHOR("awto-unvr");
