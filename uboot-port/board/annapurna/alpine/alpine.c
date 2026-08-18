// SPDX-License-Identifier: GPL-2.0+
/*
 * Annapurna Labs Alpine V2 (AL-324) / Ubiquiti UNVR board support.
 *
 * Stage-1 (chainload) target: entered via the vendor U-Boot `go 0x1100000`,
 * so DRAM/clocks/serdes are already up (vendor al_boot preboot did them).
 * No SPL, no low-level init here. See docs/uboot-port-plan.md.
 */

#include <config.h>
#include <dm.h>
#include <fdtdec.h>
#include <init.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * A57 CoreSight debug-enable. cpus_secure @ nb-service +0x8:
 *   bit0 DBGEN, bit1 NIDEN, bit2 SPIDEN, bit3 SPNIDEN.
 * RW, NOT fuse-locked on this board (the secure-boot debug-kill latch at
 * 0xfd8a81e0 is unwritten — unsigned boot), resets to 0, and no vendor code
 * ever asserts it — so the SWJ-DP pads (the unpopulated 2-row header at the
 * SoC top edge) are dead unless we set it. We default it ON so JTAG/SWD is
 * live for bring-up/debug. See docs/unvr-access-research.md, #48.
 */
#define AL_CPUS_SECURE		0xf0070008UL
#define AL_DBG_ALL		0xf	/* DBGEN|NIDEN|SPIDEN|SPNIDEN */

/*
 * Flat MMU map:
 *  - 0x00000000..0xC0000000  DRAM bank0 (normal, cacheable)
 *  - 0xC0000000..0x100000000 device (all SoC MMIO: fbxxxxxx/fdxxxxxx PBS,
 *    PCIe ECAM/windows, GIC f0xxxxxx, MSI-X fbe00000)
 *  - 0x200000000..0x240000000 DRAM bank1 (normal, cacheable)
 *
 * Device range covers every peripheral in docs/hardware.md MMIO map. Kept as
 * NGnRnE so AHCI/PCIe register access is strongly ordered.
 */
static struct mm_region alpine_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xC0000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE,
	}, {
		.virt = 0xC0000000UL,
		.phys = 0xC0000000UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN,
	}, {
		.virt = 0x200000000UL,
		.phys = 0x200000000UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE,
	}, {
		/* list terminator */
		0,
	}
};

struct mm_region *mem_map = alpine_mem_map;

int board_init(void)
{
	/* Assert A57 debug-enable so the JTAG/SWD TAP is live (see above). */
	writel(AL_DBG_ALL, (void __iomem *)AL_CPUS_SECURE);
	return 0;
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

/*
 * `reset` via the SP805 watchdog — the simple, working AL-324 reset.
 * PSCI SYSTEM_RESET does NOT work here (Linux `reboot` hangs, #51); stock
 * U-Boot and the al_reboot driver both reset through the SP805. Arm wdt0 with
 * a tiny timeout and spin: RESEN fires a SoC reset in ~2 counts.
 * wdt0 @0xfd88c000: Load 0x000, Control 0x008 (INTEN|RESEN=0x3), Lock 0xC00.
 */
#define SP805_WDT_BASE	0xfd88c000UL
void reset_cpu(void)
{
	writel(0x1ACCE551, (void __iomem *)(SP805_WDT_BASE + 0xC00)); /* unlock */
	writel(0x100,      (void __iomem *)(SP805_WDT_BASE + 0x000)); /* load */
	writel(0x3,        (void __iomem *)(SP805_WDT_BASE + 0x008)); /* INTEN|RESEN */
	while (1)
		;
}
