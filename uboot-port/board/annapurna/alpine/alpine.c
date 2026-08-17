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

DECLARE_GLOBAL_DATA_PTR;

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

/* Reset is handled by PSCI (sysreset-psci); nothing to do here. */
void reset_cpu(void)
{
}
