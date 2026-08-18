// SPDX-License-Identifier: GPL-2.0+
/*
 * Annapurna Labs Alpine V2 (AL-324) / Ubiquiti UNVR board support.
 *
 * Stage-1 (chainload) target: entered via the vendor U-Boot `go 0x1100000`,
 * so DRAM/clocks/serdes are already up (vendor al_boot preboot did them).
 * No SPL, no low-level init here. See docs/uboot-port-plan.md.
 */

#include <config.h>
#include <command.h>
#include <dm.h>
#include <fdtdec.h>
#include <i2c.h>
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
	/* awto plus splash (figlet, standard font) */
	puts("\n"
	"                _                _           \n"
	"  __ ___      _| |_ ___    _ __ | |_   _ ___ \n"
	" / _` \\ \\ /\\ / / __/ _ \\  | '_ \\| | | | / __|\n"
	"| (_| |\\ V  V /| || (_) | | |_) | | |_| \\__ \\\n"
	" \\__,_| \\_/\\_/  \\__\\___/  | .__/|_|\\__,_|___/\n"
	"                          |_|                \n\n");

	/* Assert A57 debug-enable so the JTAG/SWD TAP is live (see above). */
	writel(AL_DBG_ALL, (void __iomem *)AL_CPUS_SECURE);
	return 0;
}

/* The S-35390A transmits every data byte LSB-first (bit-reversed). */
static u8 s35_rev(u8 b)
{
	b = (b >> 4) | (b << 4);
	b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
	b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
	return b;
}

/*
 * Initialize the S-35390A RTC (0x30, behind PCA9546 mux ch0 on i2c bus 0):
 * write STATUS1 RESET (B7) | 24H (B6) per its datasheet. Byte is bit-reversed
 * on the wire. Selects mux ch0, writes, reads back STATUS1, deselects.
 * Invoked by the `rtcinit` command (below), not automatically.
 */
static void rtc_s35390a_init(void)
{
	struct udevice *mux, *rtc;
	u8 v, st;
	int rc, rr, gm, gr, sel;

	gm = i2c_get_chip_for_busnum(0, 0x71, 0, &mux);	/* mux control, no offset */
	if (gm) {
		printf("rtc-s35390a: get_chip mux=%d\n", gm);
		return;
	}

	v = 0x01;					/* select mux ch0 FIRST */
	sel = dm_i2c_write(mux, 0, &v, 1);

	gr = i2c_get_chip_for_busnum(0, 0x30, 0, &rtc);	/* now reachable via ch0 */
	if (gr) {
		printf("rtc-s35390a: sel=%d get_chip rtc=%d\n", sel, gr);
	} else {
		v = s35_rev(0x80 | 0x40);		/* RESET | 24H, bit-reversed */
		rc = dm_i2c_write(rtc, 0, &v, 1);
		rr = dm_i2c_read(rtc, 0, &st, 1);
		printf("rtc-s35390a: sel=%d RESET.wr=%d STATUS1.rd=%d val=0x%02x\n",
		       sel, rc, rr, rr ? 0 : s35_rev(st));
	}

	v = 0x00;					/* deselect mux */
	dm_i2c_write(mux, 0, &v, 1);
}

/*
 * Manual command, NOT auto at boot: while the RTC is stuck (holding SDA on ch0),
 * selecting ch0 + accessing 0x30 wedges the whole pld bus (pca9575 LEDs + bay
 * power then fail). Run `rtcinit` deliberately once the RTC is reachable.
 */
static int do_rtcinit(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	rtc_s35390a_init();
	return 0;
}
U_BOOT_CMD(rtcinit, 1, 0, do_rtcinit,
	   "initialize the s35390a RTC (write RESET via mux ch0)", "");

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
