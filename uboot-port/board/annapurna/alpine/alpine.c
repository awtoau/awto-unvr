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
#include <pci.h>
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

/* S-35390A STATUS1 flags (normal bit order, after s35_rev). */
#define S35_POC		0x01	/* B0 power-on */
#define S35_BLD		0x02	/* B1 battery-low */
#define S35_24H		0x40	/* B6 24-hour mode */
#define S35_RESET	0x80	/* B7 reset (write-only) */

/*
 * Reset-at-probe for the S-35390A RTC (0x30, behind PCA9546 mux ch0 = i2c bus 1,
 * the first mux child i2c@0 in the DT). Reached via the DM mux CHILD bus, NOT a
 * hand-poke of the 0x71 select register: the mux uclass selects ch0, runs the
 * transfer, and deselects (writes 0x00) after it — so nothing our code does can
 * leave ch0 latched and wedge the parent bus (Linux i2c/SATA-bay power live on
 * it). A manual select/deselect straddling get_chip could strand ch0 if the RTC
 * holds SDA between the two writes; the child bus makes deselect the uclass's job.
 * Mirrors stock 4.1.37: read STATUS1, and ONLY if POC/BLD (abnormal power state)
 * write RESET|24H — otherwise leave a running clock alone (an unconditional RESET
 * clobbers the time every boot). Clearing POC/BLD keeps the chip out of the
 * datasheet "indefinite status" (docs/rtc-s35390a-fault.md). Data is LSB-first
 * (bit-reversed). Runs at boot (board_late_init) and as the `rtcinit` command.
 */
static void rtc_s35390a_init(void)
{
	struct udevice *rtc;
	u8 v, st;
	int rc, rr, gr;

	gr = i2c_get_chip_for_busnum(1, 0x30, 0, &rtc);	/* bus 1 = mux ch0 child */
	if (gr) {
		printf("rtc-s35390a: get_chip (bus1/ch0) rtc=%d\n", gr);
		return;
	}

	rr = dm_i2c_read(rtc, 0, &st, 1);		/* STATUS1 first */
	if (rr) {
		printf("rtc-s35390a: STATUS1 read failed rc=%d\n", rr);
		return;
	}

	st = s35_rev(st);				/* to normal bit order */
	if (st & (S35_POC | S35_BLD)) {
		v = s35_rev(S35_RESET | S35_24H);
		rc = dm_i2c_write(rtc, 0, &v, 1);
		printf("rtc-s35390a: POC/BLD 0x%02x -> RESET|24H wr=%d\n", st, rc);
	} else {
		printf("rtc-s35390a: STATUS1=0x%02x clean, clock kept\n", st);
	}
}

/*
 * `rtcinit` command — the same reset-at-probe as the automatic board_late_init
 * call, for a deliberate re-run. Historically kept manual because a stuck ch0
 * wedged the pld bus; with i2c-sda-hold-time restored + ch0 reached via the mux
 * child bus (uclass deselects after each transfer), it is safe to run at boot.
 */
static int do_rtcinit(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	rtc_s35390a_init();
	return 0;
}
U_BOOT_CMD(rtcinit, 1, 0, do_rtcinit,
	   "initialize the s35390a RTC (write RESET via mux ch0)", "");

/*
 * Alpine internal-PCIe AXI-snoop fix (#74). The on-SoC units behind the internal
 * PCIe (eth/dma/crypto/AHCI, PCI vendor 0x1c36) need SMCC snoop enabled for
 * cache-coherent DMA; without it AHCI/DMA reads come back incoherent. Generic
 * ECAM doesn't do this — the stock vendor host driver did. Ref: linux
 * pcie-al-internal.c.
 *   SMCC sub-master 0 @ cfg 0x110, +0x20 for SM 1/2/3; SNOOP_OVR|SNOOP_EN = 0x3.
 *   APP_CONTROL @ 0x220: low 16 bits = 0x3ff, keep upper 16.
 *   slot <= 5: all 4 sub-masters; slot > 5: SM0 only. All devices: APP_CONTROL.
 * These live in extended config space (>0xff) — ECAM reaches them.
 */
#define AL_SMCC		0x110
#define AL_SMCC_BUNDLE	0x20
#define AL_SMCC_SNOOP	0x3	/* SNOOP_OVR|SNOOP_EN */
#define AL_APP_CONTROL	0x220
#define AL_APP_LO16	0x3ff
#define AL_SLOT_THRESH	5
#define AL_VENDOR	0x1c36

static void al_snoop_one(struct udevice *dev, uint slot)
{
	u32 v;
	int i;

	dm_pci_read_config32(dev, AL_SMCC, &v);
	v |= AL_SMCC_SNOOP;
	dm_pci_write_config32(dev, AL_SMCC, v);
	if (slot <= AL_SLOT_THRESH)
		for (i = 1; i < 4; i++)
			dm_pci_write_config32(dev, AL_SMCC + i * AL_SMCC_BUNDLE, v);

	dm_pci_read_config32(dev, AL_APP_CONTROL, &v);
	v = (v & 0xffff0000) | AL_APP_LO16;
	dm_pci_write_config32(dev, AL_APP_CONTROL, v);
}

static int al_pcie_snoop_fix(void)
{
	struct udevice *bus, *dev;
	int n = 0;

	for (uclass_first_device(UCLASS_PCI, &bus); bus;
	     uclass_next_device(&bus)) {
		for (device_find_first_child(bus, &dev); dev;
		     device_find_next_child(&dev)) {
			pci_dev_t bdf;
			u32 vendor;

			/* config access works on bound children — no probe */
			dm_pci_read_config32(dev, PCI_VENDOR_ID, &vendor);
			if ((vendor & 0xffff) != AL_VENDOR)
				continue;
			bdf = dm_pci_get_bdf(dev);
			al_snoop_one(dev, PCI_DEV(bdf));
			printf("al-snoop: %02x:%02x.%x slot %u done\n",
			       PCI_BUS(bdf), PCI_DEV(bdf), PCI_FUNC(bdf),
			       PCI_DEV(bdf));
			n++;
		}
	}
	printf("al-snoop: configured %d internal PCIe device(s)\n", n);
	return n;
}

static int do_snoopfix(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	return al_pcie_snoop_fix() > 0 ? 0 : 1;
}
U_BOOT_CMD(snoopfix, 1, 0, do_snoopfix,
	   "enable AXI SMCC snoop on the internal PCIe devices (coherent DMA)", "");

/*
 * Apply the snoop fix automatically at boot (after relocation, before bootcmd /
 * any AHCI or eth DMA). Enumerates PCI as a side effect; later `scsi scan`
 * reuses it. Verified: SMCC 0x110 -> 0x3 on all 6 internal devices (#74).
 */
int board_late_init(void)
{
	al_pcie_snoop_fix();
	rtc_s35390a_init();	/* reset-at-probe: clear POC/BLD before Linux (#86) */
	return 0;
}

/*
 * `fan <duty>` — set all 3 adt7475 fans (0x2e, behind mux ch3 = i2c bus 2) to a
 * manual PWM duty 0-255. Regs: 0x5c-0x5e PWM config (0xe8 = manual mode),
 * 0x30-0x32 PWM duty. Matches the stock `slowfan` env. `fan` with no arg reads
 * back the current duties.
 */
static int do_fan(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *adt;
	int rc, i;

	rc = i2c_get_chip_for_busnum(2, 0x2e, 1, &adt);
	if (rc) {
		printf("fan: adt7475 not found on bus 2 (rc=%d)\n", rc);
		return CMD_RET_FAILURE;
	}
	if (argc < 2) {
		for (i = 0; i < 3; i++)
			printf("fan%d PWM duty = %u\n", i + 1,
			       dm_i2c_reg_read(adt, 0x30 + i));
		return CMD_RET_SUCCESS;
	}
	int duty = simple_strtoul(argv[1], NULL, 0);

	if (duty > 255)
		duty = 255;
	for (i = 0; i < 3; i++) {
		dm_i2c_reg_write(adt, 0x5c + i, 0xe8);	/* manual mode */
		dm_i2c_reg_write(adt, 0x30 + i, duty);	/* duty */
	}
	printf("fan: set PWM1-3 = %d (manual)\n", duty);
	return CMD_RET_SUCCESS;
}
U_BOOT_CMD(fan, 2, 0, do_fan,
	   "set all 3 fans to a manual PWM duty (0-255), or read current", "[duty]");

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
