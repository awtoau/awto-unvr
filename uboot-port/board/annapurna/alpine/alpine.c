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
#include <fdt_support.h>
#include <fdtdec.h>
#include <i2c.h>
#include <init.h>
#include <pci.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>

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
 *    PCIe ECAM/windows, GIC f0xxxxxx, MSI-X fbe00000), CCU (0xf0090000) split
 *    out as its own entry (#97, see ft_board_setup())
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
		.size = 0xf0090000UL - 0xC0000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN,
	}, {
		/*
		 * CCU (Cache Coherency Unit), carved out of the block above so it has
		 * its own table entry. #97: `md.l 0xf0090000` from our prompt data-
		 * aborted even though this whole range was nominally already device-
		 * mapped — see ft_board_setup() below, which needs to touch this
		 * block on every bootm.
		 */
		.virt = 0xf0090000UL,
		.phys = 0xf0090000UL,
		.size = 0x10000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN,
	}, {
		.virt = 0xf00a0000UL,
		.phys = 0xf00a0000UL,
		.size = 0x100000000UL - 0xf00a0000UL,
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
 * `rtcinit` command — reset-at-probe (clear POC/BLD) on DELIBERATE demand only.
 * NOT run at boot: touching ch0 still wedges the pld bus (docs/i2c-map.md), so
 * this will wedge until the SDA-hold is genuinely fixed (#78/#86). Recover a
 * wedge with an SP805 reset. Reached via the mux child bus (bus 1 = ch0) so the
 * uclass owns select/deselect; harmless once the underlying hold is resolved.
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
			/*
			 * SKIP the two al_eth devices (1c36:0001 RJ45, 0002 SFP+).
			 * eth DOES need SMCC snoop (M2S/TX reads descriptors from
			 * cacheable DRAM), but it must be applied AFTER the eth adapter
			 * FLR/init - those reset SMCC to default. Setting it here (pre-eth-
			 * probe) does not stick, and left set it poisons stock's eth across
			 * a warm reset. So the al_eth driver enables snoop on its own
			 * function post-init (al_eth_dm_snoop_enable), mirroring stock's
			 * kernel (bind-time, pcie-al-internal.c). Here we do AHCI/crypto/dma.
			 */
			if (((vendor >> 16) & 0xffff) == 0x0001 ||
			    ((vendor >> 16) & 0xffff) == 0x0002)
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
 * CCU (Cache Coherency Unit) coherency-enable fixup (#97). Vendor U-Boot does
 * this from its own ft_board_setup on every bootm; ours never did, and Linux's
 * ahci driver then hangs on IDENTIFY when OUR U-Boot did the pre-Linux bring-up
 * (stock's bring-up boots fine — it never touches AHCI or the CCU at all).
 * Registers recovered via delroth-alpine_hal + vendor binary decompile, see
 * docs/nor-reference/uboot-ccu-coherency.md. Vendor writes plain SNOOP_EN (1),
 * not the HAL's SNOOP_EN|DVMS (3) — matching the binary's real behavior.
 */
#define AL_CCU_COMPAT		"annapurna-labs,al-ccu"
#define AL_CCU_SPECULATION	0x4	/* speculation_ctrl_register_v1_v2 */
#define AL_CCU_SPECULATION_VAL	7
#define AL_CCU_SLAVE3_SNOOP	0x4000	/* slaves[3].snoop_control_register, cluster 0 */
#define AL_CCU_SLAVE4_SNOOP	0x5000	/* slaves[4].snoop_control_register, cluster 1 */
#define AL_CCU_SNOOP_EN		1

int ft_board_setup(void *blob, struct bd_info *bd)
{
	int off;
	u64 base;
	u32 iocc;

	off = fdt_node_offset_by_compatible(blob, -1, AL_CCU_COMPAT);
	if (off < 0)
		return 0;	/* no ccu node in this DTB - nothing to do */

	base = fdt_get_base_address(blob, off);
	if (base == OF_BAD_ADDR)
		return 0;

	iocc = fdtdec_get_uint(blob, off, "io_coherency", 0);
	if (iocc) {
		writel(AL_CCU_SNOOP_EN, (void __iomem *)(base + AL_CCU_SLAVE3_SNOOP));
		writel(AL_CCU_SNOOP_EN, (void __iomem *)(base + AL_CCU_SLAVE4_SNOOP));
	}
	writel(AL_CCU_SPECULATION_VAL, (void __iomem *)(base + AL_CCU_SPECULATION));

	printf("al-ccu: coherency fixup applied (base 0x%llx, io_coherency=%u)\n",
	       base, iocc);
	return 0;
}

/*
 * Apply the snoop fix automatically at boot (after relocation, before bootcmd /
 * any AHCI or eth DMA). Enumerates PCI as a side effect; later `scsi scan`
 * reuses it. Verified: SMCC 0x110 -> 0x3 on all 6 internal devices (#74).
 */
int board_late_init(void)
{
	al_pcie_snoop_fix();
	/*
	 * Do NOT touch ch0 (s35390a RTC) here. Selecting ch0 + reading ANY
	 * address holds SDA low and wedges the whole pld bus (docs/i2c-map.md) -
	 * even reading STATUS1 for reset-at-probe. An auto call here re-wedged the
	 * bus every boot (LEDs/eth/SATA-bay power all -121), and SP805 reset only
	 * un-wedged it until the next board_late_init. RTC is offline in Linux for
	 * now, so there is no consumer to serve. Reset it only via the deliberate
	 * `rtcinit` command, and only once the SDA-hold is genuinely fixed (#78/#86).
	 */
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

/*
 * `bootstrap` — decode PBS boot_strap reg (0xfd8a8000+0x110) for the running
 * clock tree, esp. ddr_pll_freq: the one live-open field in
 * docs/ddr-config-reverse.md §7 (EEPROM/SPD only bounds it to <=1866 MT/s).
 * Port of al_bootstrap_parse() / al_hal_bootstrap.c (delroth-alpine_hal),
 * Alpine V2 branches only — this board is fixed AL-324/Alpine V2.
 * Field layout: al_hal_bootstrap_map.h; value tables: al_hal_bootstrap.c.
 * Linux-side twin (same tables, cite the same lines): scripts/read-ddr-bootstrap.py.
 */
#define AL_BOOTSTRAP_REG	0xfd8a8110UL
#define BS_F(reg, shift, mask)	(((reg) >> (shift)) & (mask))

/* al_bootstrap_cpu_pll_freq_get(), dev_id<=ALPINE_V2 table. Index = field[3:0]. */
static const u32 bs_cpu_pll_tbl[16] = {
	0 /* bypass */, 1000000000, 1400000000, 1500000000, 1600000000,
	1700000000, 1800000000, 1900000000, 2100000000, 2200000000,
	2300000000, 2400000000, 2500000000, 2600000000, 2700000000, 2000000000,
};

/* al_bootstrap_ddr_pll_freq_get(), Alpine V2 arm. Index = field[6:4]. */
static const u32 bs_ddr_pll_tbl[8] = {
	0 /* bypass */, 1066666666, 666666666, 1300000000,
	933333333, 1050000000, 1200000000, 800000000,
};

static const u32 bs_sb_clk_tbl[4] = { 250000000, 375000000, 428000000, 500000000 };

static const char * const bs_boot_dev_names[8] = {
	"UART-CLI", "UART(2000000bps)", "NAND", "reserved",
	"UART(115200bps)", "SPI(M3)", "UART(1000000bps)", "SPI(M0)",
};

static int do_bootstrap(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	u32 reg = readl((void __iomem *)AL_BOOTSTRAP_REG);
	u32 ref_clk = BS_F(reg, 19, 0x1) ? 100000000 : 25000000;
	u32 cpu_pll = bs_cpu_pll_tbl[BS_F(reg, 0, 0xf)];
	u32 ddr_pll = bs_ddr_pll_tbl[BS_F(reg, 4, 0x7)];
	u32 sb_field = BS_F(reg, 7, 0x3);
	u32 sb_pll, sb_clk;

	if (cpu_pll == 0)
		cpu_pll = ref_clk;
	if (ddr_pll == 0)
		ddr_pll = ref_clk;

	if (sb_field == 0) {
		sb_pll = ref_clk;
		sb_clk = ref_clk;	/* bypass: clk follows pll */
	} else {
		sb_pll = (sb_field == 1) ? 3000000000U : 1500000000U;
		sb_clk = bs_sb_clk_tbl[BS_F(reg, 9, 0x3)];
	}

	printf("bootstrap: reg=0x%08x (PBS +0x110)\n", reg);
	printf("  pll_ref_clk_freq  %u MHz\n", ref_clk / 1000000);
	printf("  cpu_pll_freq      %u MHz\n", cpu_pll / 1000000);
	printf("  ddr_pll_freq      %u MHz  (%u MT/s DDR)\n",
	       ddr_pll / 1000000, (ddr_pll / 1000000) * 2);
	printf("  sb_pll_freq       %u MHz\n", sb_pll / 1000000);
	printf("  sb_clk_freq       %u MHz\n", sb_clk / 1000000);
	printf("  boot_device       %s\n", bs_boot_dev_names[BS_F(reg, 15, 0x7)]);
	printf("  debug_mode        %s\n", BS_F(reg, 18, 0x1) ? "disabled" : "enabled");
	printf("  cpu_exist field   %u  (0=1 core, 1=2, 3=4)\n", BS_F(reg, 20, 0x3));

	return CMD_RET_SUCCESS;
}
U_BOOT_CMD(bootstrap, 1, 0, do_bootstrap,
	   "decode the PBS bootstrap strap register (clocks incl. ddr_pll_freq)", "");

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

/*
 * FLR the al_eth devices before a reset so the NEXT boot (stock U-Boot) gets
 * CLEAN eth. Our eth bring-up can wedge the UDMA (adapter_init -110), and a warm
 * SoC reset does NOT clear that - it poisons stock's eth on the following boot,
 * so the auto-chainload tftp times out (proven: cold boot works, warm doesn't).
 * A PCIe Function-Level Reset returns the whole function to defaults via config
 * space. Best-effort - never blocks the reset; a no-op if FLR is unsupported.
 * UNTESTED on hardware (box was offline when added).
 */
static void al_eth_flr_all(void)
{
	struct udevice *bus, *dev;

	for (uclass_first_device(UCLASS_PCI, &bus); bus;
	     uclass_next_device(&bus)) {
		for (device_find_first_child(bus, &dev); dev;
		     device_find_next_child(&dev)) {
			u32 vendor;
			u16 ctl;
			int pos;

			dm_pci_read_config32(dev, PCI_VENDOR_ID, &vendor);
			if ((vendor & 0xffff) != AL_VENDOR)
				continue;
			if (((vendor >> 16) & 0xffff) != 0x0001 &&
			    ((vendor >> 16) & 0xffff) != 0x0002)
				continue;
			pos = dm_pci_find_capability(dev, PCI_CAP_ID_EXP);
			if (!pos)
				continue;
			dm_pci_read_config16(dev, pos + PCI_EXP_DEVCTL, &ctl);
			dm_pci_write_config16(dev, pos + PCI_EXP_DEVCTL,
					      ctl | PCI_EXP_DEVCTL_BCR_FLR);
		}
	}
	udelay(100000);	/* PCIe: an FLR completes within 100 ms */
}

/*
 * `reset` via the SP805 watchdog — the simple, working AL-324 reset.
 * PSCI SYSTEM_RESET does NOT work here (Linux `reboot` hangs, #51); stock
 * U-Boot and the al_reboot driver both reset through the SP805. Load = 0x100
 * (256 counts) then RESEN fires the SoC reset. FLR eth first (see above).
 * wdt0 @0xfd88c000: Load 0x000, Control 0x008 (INTEN|RESEN=0x3), Lock 0xC00.
 */
#define SP805_WDT_BASE	0xfd88c000UL
void reset_cpu(void)
{
	al_eth_flr_all();	/* hand stock CLEAN eth across the warm reset */
	writel(0x1ACCE551, (void __iomem *)(SP805_WDT_BASE + 0xC00)); /* unlock */
	writel(0x100,      (void __iomem *)(SP805_WDT_BASE + 0x000)); /* load */
	writel(0x3,        (void __iomem *)(SP805_WDT_BASE + 0x008)); /* INTEN|RESEN */
	while (1)
		;
}
