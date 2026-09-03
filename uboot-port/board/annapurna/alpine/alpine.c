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

/*
 * External PCIe0 controller bring-up (#140). This controller (ASM1042A xHCI
 * card at 0001:01:00.0) is NOT flat-ECAM - pci-host-ecam-generic aliases the
 * same device across every devfn because the HAL never ran. Vendor's own
 * al_init_pcie() (decompiled: docs/nor-reference/uboot-old-decompiled.c:25535,
 * FUN_01125a58) does a fixed enable/disable/mode/lanes/enable/link-start
 * sequence before config-space (incl. the eventual CFG_TARGET_BUS write) is
 * safe to touch - an isolated CFG_TARGET_BUS write without this caused a full
 * board reset, see issue #140. Register offsets cross-referenced against
 * delroth-alpine_hal (services/pcie/al_init_pcie.c, drivers/pcie/
 * al_hal_pcie.c, al_hal_pcie_axi_reg.h, al_hal_pcie_w_reg.h) for our
 * confirmed AL_PCIE_REV_ID_3.
 *
 * controller_base = 0xfd800000 (real Linux DTS pcie_ext0 = AL_SB_PCIE_BASE(0)).
 * port_id = 0 (bit 0 of PBS pcie_conf_1 PCIE_EXIST). RC mode (root complex
 * facing the ASM1042A), x1 lane (live lspci: "LnkSta: ... Width x1"), 1 PF.
 * Does NOT include the CFG_TARGET_BUS write itself - deliberately out of
 * scope here, to be added/tested separately once this bring-up is verified
 * safe on hardware.
 */
#define AL_PCIE_EXT0_BASE		0xfd800000UL
/* axi.device_id.device_rev_id (al_hal_pcie_axi_reg.h:918, offset 0x16c) */
#define AL_PCIE_EXT0_DEV_REV_ID		(AL_PCIE_EXT0_BASE + 0x16c)
#define AL_PCIE_EXT0_REV_ID_3_VAL	0x00020000UL	/* live-read on hw, #140 */
/* axi.pcie_global.conf (al_hal_pcie_axi_reg.h:907, offset 0xc8) */
#define AL_PCIE_EXT0_GLOBAL_CONF	(AL_PCIE_EXT0_BASE + 0xc8)
/* axi.axi_attr_ovrd.pf_sel (al_hal_pcie_axi_reg.h:922+770, offset 0x1a0+0x10) */
#define AL_PCIE_EXT0_PF_SEL		(AL_PCIE_EXT0_BASE + 0x1b0)
/* app.global_ctrl.port_init (al_hal_pcie_w_reg.h:591+80, app @ +0x2000) */
#define AL_PCIE_EXT0_PORT_INIT		(AL_PCIE_EXT0_BASE + 0x2000)
/* pbs unit.pcie_conf_1 (al_hal_pbs_regs.h:137, unit @ PBS+0x0, reg @ +0xe4) */
#define AL_PBS_PCIE_CONF_1		0xfd8a80e4UL
#define AL_PCIE_EXT0_PORT_ID		0U	/* AL_SB_PCIE_BASE(0) = this port */
/*
 * ob_ctrl.cfg_target_bus (al_hal_pcie_axi_reg.h, ob_ctrl @ +0x30 for REV_ID_3
 * per pcie-al.c's al_pcie_reg_offsets_set()). #140: a live register dump
 * (tftpboot+go a standalone stub straight into STOCK's already-linked,
 * already-working environment - no md/pci commands exist in stock 2015.07
 * otherwise) read this as 0x000000ff under vendor - target_bus=0, NOT 1.
 * The DTS's bus-range=<0x01 0x01> and Linux's linux,pci-domain=<1> describe
 * a SEPARATE PCI *domain* number (this is the 2nd host controller U-Boot
 * registers, hence "01:00.0" in vendor's own boot log) - not a literal
 * target-bus value. Within its own domain the ASM1042A is genuinely on bus
 * 0. Every earlier round in this issue used 0x1ff (bus=1) and got a real
 * SError on every config access; 0xff (bus=0) removes the SError entirely,
 * confirmed live.
 */
#define AL_PCIE_EXT0_CFG_TARGET_BUS	(AL_PCIE_EXT0_BASE + 0x30)
#define AL_PCIE_CFG_TARGET_BUS_VAL	0x000000ffU	/* mask=0xff, target_bus=0 */
/*
 * axi.pre_configuration.pcie_core_setup / axi.init_fc.cfg
 * (al_hal_pcie_axi_reg.h:913-914, offsets 0xe8/0xec in al_pcie_rev3_axi_regs).
 * Written automatically by the REAL al_pcie_port_enable() (al_hal_pcie.c:
 * 1787-1790 calling al_pcie_ib_hcrd_os_ob_reads_config_default() at line 577)
 * whenever ib_hcrd_config.{nof_np_hdr,nof_p_hdr} are 0 - true on every
 * enable here since we never call the explicit config path. NOT the same
 * thing as the explicit al_pcie_port_ib_hcrd_os_ob_reads_config() the
 * previous pass correctly found vendor doesn't call for this port.
 */
#define AL_PCIE_EXT0_PRE_CONFIG		(AL_PCIE_EXT0_BASE + 0xe8)
#define AL_PCIE_EXT0_INIT_FC_CFG	(AL_PCIE_EXT0_BASE + 0xec)
/*
 * port_regs (al_hal_pcie_regs.h:86-117,168-197,332): core_space.func[0].
 * port_regs, at controller_base + CORE_CONF_BASE_OFFSET(0x10000, rev3) +
 * 0x700 (offset of port_regs within al_pcie_core_reg_space_rev_3_4).
 */
#define AL_PCIE_EXT0_PORT_REGS_BASE	(AL_PCIE_EXT0_BASE + 0x10700)
/* port_regs.port_link_ctrl (offset 0x10) / gen2_ctrl (offset 0x10C) */
#define AL_PCIE_EXT0_PORT_LINK_CTRL	(AL_PCIE_EXT0_PORT_REGS_BASE + 0x10)
#define AL_PCIE_EXT0_GEN2_CTRL		(AL_PCIE_EXT0_PORT_REGS_BASE + 0x10C)
/* port_regs.vc0_posted_rcv_q_ctrl (0x48) / vc0_non_posted_rcv_q_ctrl (0x4c) */
#define AL_PCIE_EXT0_VC0_POSTED_Q	(AL_PCIE_EXT0_PORT_REGS_BASE + 0x48)
#define AL_PCIE_EXT0_VC0_NON_POSTED_Q	(AL_PCIE_EXT0_PORT_REGS_BASE + 0x4c)
/* app.debug.info_0 (al_hal_pcie_w_reg.h:978-979), app @ +0x2000, offset +0x80 */
#define AL_PCIE_EXT0_DEBUG_INFO_0	(AL_PCIE_EXT0_BASE + 0x2080)

/*
 * al_pcie_port_config() (al_hal_pcie.c:1958-2189) pieces beyond the link_config/
 * ib_hcrd_config fragment already ported above. Confirmed on the real vendor
 * execution path, not just theoretical HAL scope: decompiled al_init_pcie()
 * (docs/nor-reference/uboot-old-decompiled.c FUN_01125a58, line ~25620) calls
 * FUN_0111d664 - matched 1:1 to al_pcie_port_config() by its own embedded
 * assertion strings ("al_pcie_port_config", al_hal_pcie.c:~1968-1969) -
 * unconditionally, right after the 2nd al_pcie_port_enable() and before
 * al_pcie_link_start(). So vendor's own bring-up (which trains this link
 * before we ever get control, #140) already applies all of this; porting it
 * here makes OUR bring-up match, and - critically - these writes must NOT be
 * gated behind the "link already up, skip retrain" branch the way the
 * disable/retrain sequence is: unlike LTSSM_EN, none of these touch link
 * training/PHY state, so re-asserting them over an already-good link is safe,
 * and skipping them in that (real-world, always-taken) branch would silently
 * never apply them - the same class of bug already hit once with al_eth's
 * SMCC snoop (which had to move from a pre-probe to a post-probe hook).
 * See al_pcie_ext0_port_config_fixup() below, called from BOTH branches.
 */
/* axi.ctrl.master_arctl / master_awctl (al_hal_pcie_axi_reg.h:88,90 -> +0x14/+0x18) */
#define AL_PCIE_EXT0_MASTER_ARCTL	(AL_PCIE_EXT0_BASE + 0x14)
#define AL_PCIE_EXT0_MASTER_AWCTL	(AL_PCIE_EXT0_BASE + 0x18)
/* OVR_SNOOP(bit26)|SNOOP(bit27), al_hal_pcie_axi_reg.h:1142,1145,1169,1172 */
#define AL_PCIE_AXI_SNOOP_MASK		(3U << 26)
/*
 * core_space.func[0].config_header (rev3/4 func0 @ AL_PCIE_EXT0_BASE+0x10000,
 * confirmed against reg_xref_alpine_v2.txt: pcie0->core_space.func[0].
 * pcie_pm_cap_base=0xfd810040, i.e. config_header base=0xfd810000) + 0x4 =
 * standard PCI COMMAND. al_pcie_port_config() RC-mode block (al_hal_pcie.c:
 * 2107-2110) sets Mem|MSE|IO(0x7) here - the RC's own routing-enable, most
 * likely candidate for "accepts/routes the transaction at all" rather than
 * just how an error is reported.
 */
#define AL_PCIE_EXT0_CONFIG_HEADER_CMD	(AL_PCIE_EXT0_BASE + 0x10004)
#define AL_PCIE_CMD_MEM_IO_BM_MASK	0x7U	/* IO Space | Mem Space | Bus Master */
/*
 * port_regs.axi_slave_err_resp (al_hal_pcie_regs.h:113, offset 0x1D0 within
 * port_regs). al_hal_pcie.h:598-602: TRUE = PCIe unit returns a real Slave
 * Error/Decoding Error to the AXI master on error; FALSE = silently returns
 * 0xFFFFFFFF with no error indication. Vendor's al_pcie_port_config() sets
 * this from params->enable_axi_slave_err_resp (al_hal_pcie.c:2074-2077); the
 * reference board's example params (al_init_pcie_params.h:101) default TRUE.
 */
#define AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP	(AL_PCIE_EXT0_PORT_REGS_BASE + 0x1D0)
#define AL_PCIE_AXI_SLAVE_ERR_RESP_ALL_MAPPING	(1U << 0)
/*
 * app.pm_state_per_func[1..3] (al_hal_pcie_w_reg.h:611, app+0x4b0, 4B stride;
 * confirmed pcie0->app.pm_state_per_func[1]=0xfd8024b4 etc). rev>=3 only
 * (al_hal_pcie.c:2140-2148): disable PM-state tracking for the 3 PFs we never
 * enable (max_num_of_pfs=1, PF0 only) - low-risk hygiene, vendor does it
 * unconditionally in the same sequence.
 */
#define AL_PCIE_EXT0_PM_STATE_PF(n)	(AL_PCIE_EXT0_BASE + 0x24b0 + (n) * 4)
/* ASPM_PF_ENABLE_MAX_FUNC_NUMBER(bit7) | DSATE_PF_ENABLE_MAX_FUNC_NUMBER(bit8) */
#define AL_PCIE_PM_STATE_DISABLE_MASK	0x180U

#define AL_PCIE_CONF1_EXIST		(1U << AL_PCIE_EXT0_PORT_ID)	/* PCIE_EXIST bit */
#define AL_PCIE_GLOBAL_MEM_SHUTDOWN	(1U << 20)	/* rev3/4 MEM_SHUTDOWN, bit 20 */
#define AL_PCIE_GLOBAL_DEV_TYPE_MASK	0xFU
#define AL_PCIE_GLOBAL_DEV_TYPE_RC	4U		/* PCIE_AXI_..._DEV_TYPE_RC */
#define AL_PCIE_GLOBAL_LANES_MASK	0xFFFF0U	/* rev3/4 NOF_ACT_LANES field */
#define AL_PCIE_GLOBAL_LANES_X1	(1U << 4)	/* AL_PCIE_PARSE_LANES(1) */
#define AL_PCIE_PF_SEL_MASK		0x7F7FU		/* PF_BIT0/1 ovrd+addr fields */
#define AL_PCIE_PF_SEL_FORCE_REG	0x202U		/* PF_BIT0/1_OVRD_FROM_REG only */
#define AL_PCIE_PORT_INIT_LTSSM_EN	(1U << 0)

/* app.debug.info_0 LTSSM_STATE field (al_hal_pcie_w_reg.h:978-979) */
#define AL_PCIE_LTSSM_STATE_MASK		0x1F8U
#define AL_PCIE_LTSSM_STATE_SHIFT		3
/* al_hal_pcie.h:1176-1191 (subset relevant to the retrain workaround below) */
#define AL_PCIE_LTSSM_STATE_POLL_ACTIVE		0x2U
#define AL_PCIE_LTSSM_STATE_CFG_LINKWD_START	0x7U
#define AL_PCIE_LTSSM_STATE_CFG_LINKWD_ACEPT	0x8U
#define AL_PCIE_LTSSM_STATE_CFG_LANENUM_WAIT	0x9U
#define AL_PCIE_LTSSM_STATE_CFG_LANENUM_ACEPT	0xAU
#define AL_PCIE_LTSSM_STATE_L0			0x11U

/*
 * al_hal_pcie.c:2398-2487 _al_pcie_link_up_wait_retrain_if_not_full_width():
 * these 5 early-training LTSSM substates are known (vendor HAL, not
 * speculation) to sometimes need a LTSSM_EN toggle (stop+start = retrain) to
 * get unstuck - polled every 100us, retrain once a substate's own persistence
 * count is hit. POLL_ACTIVE gets the long budget (5ms); the 4 CFG_* substates
 * downstream of it get the short one (500us) since a real link moves through
 * those quickly once past Polling.
 */
#define AL_PCIE_RETRAIN_POLL_INTERVAL_US	100
#define AL_PCIE_RETRAIN_THRESH_POLL_ACTIVE_US	5000
#define AL_PCIE_RETRAIN_THRESH_CFG_US		500
/*
 * Total link-up budget. PCI Express Base Spec mandates L0 within 100ms of
 * LTSSM enable; x2 margin (not the usual 1.25x - this path can burn a few ms
 * per retrain cycle on top of the base spec number, and it's a one-shot boot
 * cost, not a hot loop) rounded to 200ms. Logged with elapsed/state on
 * expiry, never silent.
 */
#define AL_PCIE_LINK_WAIT_TOTAL_US		200000

/*
 * REV3 RC ib_hcrd_os_ob_reads_defaults (al_hal_pcie.c:196-201) - REV3 EP and
 * RC rows are identical, so which op-mode is read back on the first (pre
 * mode-config) port_enable() doesn't matter. nof_outstanding_ob_reads=32,
 * nof_cpl_hdr=161, nof_np_hdr=38, nof_p_hdr=60 (sum 259, matches the rev3
 * "must be 259" comment at al_hal_pcie_axi_reg.h:607).
 */
#define AL_PCIE_PRE_CONFIG_NOF_READS_MASK	0xFF00U
#define AL_PCIE_PRE_CONFIG_NOF_READS_VAL	(32U << 8)
#define AL_PCIE_INIT_FC_CFG_MASK		0x07FFFFFFU
/* p_hdr[8:0]=60 | np_hdr[17:9]=38 | cpl_hdr[26:18]=161 */
#define AL_PCIE_INIT_FC_CFG_VAL		(60U | (38U << 9) | (161U << 18))
/* al_pcie_port_ib_hcrd_config() (al_hal_pcie.c:1106-1121), RADM_*_HCRD_VC0
 * fields are (credit_count - 1) at bits[19:12]. */
#define AL_PCIE_VC0_HCRD_MASK			0xFF000U
#define AL_PCIE_VC0_POSTED_HCRD_VAL		((60U - 1U) << 12)
#define AL_PCIE_VC0_NON_POSTED_HCRD_VAL	((38U - 1U) << 12)
/* port_regs.gen2_ctrl NUM_OF_LANES[12:8] = raw lane count (not one-hot) */
#define AL_PCIE_GEN2_CTRL_LANES_MASK		0x1F00U
#define AL_PCIE_GEN2_CTRL_LANES_X1		(1U << 8)
/* port_regs.port_link_ctrl LINK_CAPABLE[21:16] = max_lanes+(max_lanes-1) */
#define AL_PCIE_LINK_CTRL_CAPABLE_MASK		0x3F0000U
#define AL_PCIE_LINK_CTRL_CAPABLE_X1		(1U << 16)

/* al_init_pcie.c PORT_DIS_EN_WAIT_US, Alpine V2 (dev_id <= ALPINE_V2) branch */
#define AL_PCIE_PORT_DIS_EN_WAIT_US	500

/*
 * #140 (verbose-logging pass): every write through here prints old->new so a
 * boot log alone shows exactly what bring-up did, without needing the source
 * open alongside it. `what` is a short tag naming the field/purpose (mirrors
 * the comment already above each call site) - kept in the log line itself
 * since the log is the thing actually read live over serial.
 */
static void al_pcie_reg_set(const char *what, unsigned long reg, u32 mask, u32 val)
{
	u32 v = readl((void __iomem *)reg);
	u32 before = v;

	v = (v & ~mask) | (val & mask);
	writel(v, (void __iomem *)reg);
	printf("al-pcie-ext0:   %-22s @ 0x%08lx  0x%08x -> 0x%08x (mask 0x%08x)\n",
	       what, reg, before, v, mask);
}

static u32 al_pcie_ext0_ltssm_state(void)
{
	u32 v = readl((void __iomem *)AL_PCIE_EXT0_DEBUG_INFO_0);

	return (v & AL_PCIE_LTSSM_STATE_MASK) >> AL_PCIE_LTSSM_STATE_SHIFT;
}

/*
 * Wait for link up, retraining (LTSSM_EN off/on) if stuck in one of the 5
 * early substates past its vendor-HAL threshold - see the #defines above.
 * #140: observed live stuck stable at POLL_ACTIVE with no retrain attempted;
 * this is the missing piece, not another core/DWC register.
 */
static void al_pcie_ext0_link_retrain_wait(void)
{
	unsigned int elapsed_us = 0, state_us = 0, retrains = 0;
	u32 state, last_state = 0xFFFFFFFFU;

	while (elapsed_us < AL_PCIE_LINK_WAIT_TOTAL_US) {
		state = al_pcie_ext0_ltssm_state();

		if (state >= AL_PCIE_LTSSM_STATE_L0) {
			printf("al-pcie-ext0: link up, LTSSM state 0x%x (%u retrain%s)\n",
			       state, retrains, retrains == 1 ? "" : "s");
			return;
		}

		if (state != last_state) {
			printf("al-pcie-ext0: LTSSM 0x%x -> 0x%x (elapsed %u us)\n",
			       last_state, state, elapsed_us);
			last_state = state;
			state_us = 0;
		} else {
			state_us += AL_PCIE_RETRAIN_POLL_INTERVAL_US;
		}

		if ((state == AL_PCIE_LTSSM_STATE_POLL_ACTIVE &&
		     state_us >= AL_PCIE_RETRAIN_THRESH_POLL_ACTIVE_US) ||
		    ((state == AL_PCIE_LTSSM_STATE_CFG_LINKWD_START ||
		      state == AL_PCIE_LTSSM_STATE_CFG_LINKWD_ACEPT ||
		      state == AL_PCIE_LTSSM_STATE_CFG_LANENUM_WAIT ||
		      state == AL_PCIE_LTSSM_STATE_CFG_LANENUM_ACEPT) &&
		     state_us >= AL_PCIE_RETRAIN_THRESH_CFG_US)) {
			printf("al-pcie-ext0: retrain #%u: LTSSM stuck at 0x%x for %u us, toggling LTSSM_EN\n",
			       retrains + 1, state, state_us);
			/* al_pcie_link_stop() + al_pcie_link_start(), al_hal_pcie.c:2361-2378,2467-2472 */
			al_pcie_reg_set("LTSSM_EN(stop)", AL_PCIE_EXT0_PORT_INIT,
					AL_PCIE_PORT_INIT_LTSSM_EN, 0);
			udelay(1000);	/* al_hal_pcie.c:2469, between stop and start */
			al_pcie_reg_set("LTSSM_EN(start)", AL_PCIE_EXT0_PORT_INIT, AL_PCIE_PORT_INIT_LTSSM_EN,
					AL_PCIE_PORT_INIT_LTSSM_EN);
			retrains++;
			elapsed_us += 1000;
			last_state = 0xFFFFFFFFU;
			state_us = 0;
		}

		udelay(AL_PCIE_RETRAIN_POLL_INTERVAL_US);
		elapsed_us += AL_PCIE_RETRAIN_POLL_INTERVAL_US;
	}

	printf("al-pcie-ext0: link NOT up after %u ms, LTSSM state 0x%x (%u retrain%s)\n",
	       AL_PCIE_LINK_WAIT_TOTAL_US / 1000, last_state, retrains,
	       retrains == 1 ? "" : "s");
}

/*
 * al_pcie_port_config() pieces beyond link_config/ib_hcrd_config (see the big
 * comment above the register #defines for why these must run unconditionally,
 * independent of the "link already up" skip). Pure AXI-side/local-config-
 * header writes - none touch PCIE_EXIST/DEV_TYPE/PORT_INIT, so safe to apply
 * over an already-trained link.
 */
static void al_pcie_ext0_port_config_fixup(void)
{
	printf("al-pcie-ext0: port_config fixup starting\n");

	/*
	 * cfg_target_bus(0): NOT part of the real al_pcie_port_config() - this
	 * is the pci-host-ecam-generic-vs-this-controller gap #140 opened with
	 * (see AL_PCIE_EXT0_CFG_TARGET_BUS above). Applied here anyway since
	 * this function already runs on every path that needs config-space
	 * access to work, and it must be set before `pci enum`/`usb start`
	 * ever touches the ECAM window.
	 */
	al_pcie_reg_set("cfg_target_bus", AL_PCIE_EXT0_CFG_TARGET_BUS, 0xffffffffU,
			AL_PCIE_CFG_TARGET_BUS_VAL);

	/* al_pcie_port_snoop_config(TRUE) (al_hal_pcie.c:2024,2807-2833) */
	al_pcie_reg_set("master_arctl(snoop)", AL_PCIE_EXT0_MASTER_ARCTL, AL_PCIE_AXI_SNOOP_MASK,
			AL_PCIE_AXI_SNOOP_MASK);
	al_pcie_reg_set("master_awctl(snoop)", AL_PCIE_EXT0_MASTER_AWCTL, AL_PCIE_AXI_SNOOP_MASK,
			AL_PCIE_AXI_SNOOP_MASK);

	/* RC-mode config_header COMMAND: Mem|MSE|IO (al_hal_pcie.c:2107-2110) */
	al_pcie_reg_set("cfghdr_cmd(mem|io|bm)", AL_PCIE_EXT0_CONFIG_HEADER_CMD,
			AL_PCIE_CMD_MEM_IO_BM_MASK, AL_PCIE_CMD_MEM_IO_BM_MASK);

	/* enable_axi_slave_err_resp(TRUE) (al_hal_pcie.c:2074-2077) */
	al_pcie_reg_set("axi_slave_err_resp", AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP,
			AL_PCIE_AXI_SLAVE_ERR_RESP_ALL_MAPPING, AL_PCIE_AXI_SLAVE_ERR_RESP_ALL_MAPPING);

	/* disable PM-state tracking for unused PF1-3 (al_hal_pcie.c:2140-2148) */
	al_pcie_reg_set("pm_state_pf1(disable)", AL_PCIE_EXT0_PM_STATE_PF(1),
			AL_PCIE_PM_STATE_DISABLE_MASK, 0);
	al_pcie_reg_set("pm_state_pf2(disable)", AL_PCIE_EXT0_PM_STATE_PF(2),
			AL_PCIE_PM_STATE_DISABLE_MASK, 0);
	al_pcie_reg_set("pm_state_pf3(disable)", AL_PCIE_EXT0_PM_STATE_PF(3),
			AL_PCIE_PM_STATE_DISABLE_MASK, 0);

	printf("al-pcie-ext0: port_config fixup applied (snoop, cmd-reg, axi_slave_err_resp, pm-state)\n");
}

static int al_pcie_ext0_bringup(void)
{
	u32 rev;

	printf("al-pcie-ext0: bring-up starting\n");

	rev = readl((void __iomem *)AL_PCIE_EXT0_DEV_REV_ID);
	if (rev != AL_PCIE_EXT0_REV_ID_3_VAL) {
		printf("al-pcie-ext0: DEVICE_REV_ID 0x%08x != expected 0x%08x (REV_ID_3), skipping\n",
		       rev, AL_PCIE_EXT0_REV_ID_3_VAL);
		return -1;
	}
	printf("al-pcie-ext0: DEVICE_REV_ID 0x%08x == REV_ID_3, proceeding\n", rev);

	/*
	 * #140: our chainload flow always runs after vendor U-Boot's own
	 * board_init already linked this port (confirmed live: LTSSM is L0
	 * at the very first stock prompt, before any usb command runs) - and
	 * that is the real deployment model too (vendor chainloads to us),
	 * not just a test artifact. Disabling+retraining an already-good link
	 * via local register writes alone (no electrical PERST# pulse) is
	 * what was producing the stable POLL_ACTIVE stall in prior rounds.
	 * If it's already up, leave it alone.
	 */
	if (al_pcie_ext0_ltssm_state() >= AL_PCIE_LTSSM_STATE_L0) {
		printf("al-pcie-ext0: link already up (LTSSM 0x%x), skipping bring-up\n",
		       al_pcie_ext0_ltssm_state());
		/*
		 * Still apply the port_config fixups (#140 SError round): vendor's
		 * own al_init_pcie() already did this before we got control, but
		 * re-asserting is safe (none touch link state) and this is the
		 * ONLY branch actually exercised in real deployment - skipping it
		 * here would mean it never runs at all, not "runs but stale".
		 */
		al_pcie_ext0_port_config_fixup();
		return 0;
	}
	printf("al-pcie-ext0: link NOT up (LTSSM 0x%x), running full cold bring-up\n",
	       al_pcie_ext0_ltssm_state());

	/* al_pcie_port_enable() #1: deassert core reset (PBS PCIE_EXIST bit) */
	al_pcie_reg_set("pcie_exist(enable#1)", AL_PBS_PCIE_CONF_1, AL_PCIE_CONF1_EXIST,
			AL_PCIE_CONF1_EXIST);
	udelay(AL_PCIE_PORT_DIS_EN_WAIT_US);
	printf("al-pcie-ext0: waited %u us (PORT_DIS_EN_WAIT) after enable#1\n",
	       AL_PCIE_PORT_DIS_EN_WAIT_US);

	/* al_pcie_port_memory_shutdown_set(FALSE): clear MEM_SHUTDOWN while enabled */
	al_pcie_reg_set("mem_shutdown(clear)", AL_PCIE_EXT0_GLOBAL_CONF,
			AL_PCIE_GLOBAL_MEM_SHUTDOWN, 0);

	/* al_pcie_port_disable(): mode/lanes/pfs below must be set while disabled */
	al_pcie_reg_set("pcie_exist(disable)", AL_PBS_PCIE_CONF_1, AL_PCIE_CONF1_EXIST, 0);

	/*
	 * al_pcie_port_operating_mode_config(RC): rev>=3 RC-only "config 1 PF"
	 * step first (al_hal_pcie.c:1473-1486), then set DEV_TYPE=RC.
	 */
	al_pcie_reg_set("pf_sel(force_reg)", AL_PCIE_EXT0_PF_SEL, AL_PCIE_PF_SEL_MASK,
			AL_PCIE_PF_SEL_FORCE_REG);
	al_pcie_reg_set("dev_type(RC)", AL_PCIE_EXT0_GLOBAL_CONF, AL_PCIE_GLOBAL_DEV_TYPE_MASK,
			AL_PCIE_GLOBAL_DEV_TYPE_RC);

	/* al_pcie_port_max_lanes_set(1): this port links at x1 (live lspci) */
	al_pcie_reg_set("max_lanes(x1)", AL_PCIE_EXT0_GLOBAL_CONF, AL_PCIE_GLOBAL_LANES_MASK,
			AL_PCIE_GLOBAL_LANES_X1);

	/*
	 * al_pcie_port_max_num_of_pfs_set(1): software-only here - the HAL only
	 * writes timer_ctrl_max_func_num once the port is already enabled
	 * (al_hal_pcie.c:1586), and vendor calls this before the final enable
	 * below. No register write in this ordering.
	 */
	printf("al-pcie-ext0: max_num_of_pfs(1) - no register write (see comment)\n");

	/*
	 * al_pcie_port_ib_hcrd_os_ob_reads_config() the EXPLICIT/external call:
	 * still correctly skipped, vendor's reads_config param is NULL for this
	 * port. BUT al_pcie_port_enable() itself unconditionally calls the
	 * DEFAULT version internally (al_hal_pcie.c:1787-1790) whenever
	 * ib_hcrd_config.{nof_np_hdr,nof_p_hdr} are 0 - true here, so this
	 * fires on the #2 enable below. Write it explicitly since we don't
	 * carry that state across calls like the real HAL does.
	 */
	al_pcie_reg_set("init_fc_cfg(default)", AL_PCIE_EXT0_INIT_FC_CFG,
			AL_PCIE_INIT_FC_CFG_MASK, AL_PCIE_INIT_FC_CFG_VAL);
	al_pcie_reg_set("pre_config(nof_reads)", AL_PCIE_EXT0_PRE_CONFIG,
			AL_PCIE_PRE_CONFIG_NOF_READS_MASK, AL_PCIE_PRE_CONFIG_NOF_READS_VAL);

	/* al_pcie_port_enable() #2 (final): deassert core reset again */
	al_pcie_reg_set("pcie_exist(enable#2)", AL_PBS_PCIE_CONF_1, AL_PCIE_CONF1_EXIST,
			AL_PCIE_CONF1_EXIST);
	udelay(AL_PCIE_PORT_DIS_EN_WAIT_US);
	printf("al-pcie-ext0: waited %u us (PORT_DIS_EN_WAIT) after enable#2\n",
	       AL_PCIE_PORT_DIS_EN_WAIT_US);

	/*
	 * al_pcie_port_config() -> al_pcie_port_link_config() (al_hal_pcie.c:
	 * 386-434): sets the DWC core's active-lane registers. Leading
	 * hypothesis for LTSSM stuck at DETECT_WAIT (#140) - without these the
	 * core doesn't know which/how many lanes to train on.
	 */
	al_pcie_reg_set("gen2_ctrl(lanes=x1)", AL_PCIE_EXT0_GEN2_CTRL, AL_PCIE_GEN2_CTRL_LANES_MASK,
			AL_PCIE_GEN2_CTRL_LANES_X1);
	al_pcie_reg_set("port_link_ctrl(capable=x1)", AL_PCIE_EXT0_PORT_LINK_CTRL,
			AL_PCIE_LINK_CTRL_CAPABLE_MASK, AL_PCIE_LINK_CTRL_CAPABLE_X1);

	/*
	 * al_pcie_port_config() -> al_pcie_port_ib_hcrd_config() (al_hal_pcie.c:
	 * 1106-1121, 2057-2059): REV3's ib_hcrd_use_hw_default is FALSE, so the
	 * default config above also sets crdt_update_required, and vendor
	 * writes these RADM credit registers before link_start.
	 */
	al_pcie_reg_set("vc0_posted_hcrd", AL_PCIE_EXT0_VC0_POSTED_Q, AL_PCIE_VC0_HCRD_MASK,
			AL_PCIE_VC0_POSTED_HCRD_VAL);
	al_pcie_reg_set("vc0_non_posted_hcrd", AL_PCIE_EXT0_VC0_NON_POSTED_Q,
			AL_PCIE_VC0_HCRD_MASK, AL_PCIE_VC0_NON_POSTED_HCRD_VAL);

	/*
	 * al_pcie_port_config()'s remaining pieces (snoop/cmd-reg/err-resp/pm-
	 * state, #140 SError round) - vendor calls the full port_config() here,
	 * before link_start (decompile-confirmed ordering, see comment above
	 * al_pcie_ext0_port_config_fixup()). Must run before the cold path's
	 * disable() would otherwise be too late to matter, and before link_start
	 * so a fresh link comes up with these already in place, matching vendor.
	 */
	al_pcie_ext0_port_config_fixup();

	/* al_pcie_link_start(): enable LTSSM so the link actually trains */
	al_pcie_reg_set("port_init(LTSSM_EN)", AL_PCIE_EXT0_PORT_INIT, AL_PCIE_PORT_INIT_LTSSM_EN,
			AL_PCIE_PORT_INIT_LTSSM_EN);

	printf("al-pcie-ext0: bring-up applied (RC mode, x1 lane, 1 PF, link start)\n");
	al_pcie_ext0_link_retrain_wait();
	return 0;
}

static int do_pciebringup(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	return al_pcie_ext0_bringup() == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}
U_BOOT_CMD(pciebringup, 1, 0, do_pciebringup,
	   "run the external PCIe0 HAL bring-up sequence (#140, incl. CFG_TARGET_BUS)", "");

/*
 * Early diagnostic dump (#140) - every register this issue's rounds have
 * cared about, plus GPIO, printed unconditionally at the very start of
 * board_init() before any bring-up code runs. Exists because stock 2015.07
 * has no md/pci commands (no way to inspect its own state), and a live
 * before/after comparison across our own bring-up steps was previously only
 * possible by TFTP-loading a separate freestanding stub - this makes that
 * comparison a permanent, always-available part of every boot instead.
 * GPIO: 6x PL061 banks (docs/hardware.md), full 8-bit port read via the
 * PL061 address-mask trick (offset 0x3FC = GPIODATA with all 8 mask bits
 * set, i.e. mask=0xFF -> real value, not the address-decode itself).
 */
#define AL_GPIO_BANK_COUNT	6
static const unsigned long al_gpio_bank_base[AL_GPIO_BANK_COUNT] = {
	0xfd887000UL, 0xfd888000UL, 0xfd889000UL,
	0xfd88a000UL, 0xfd88b000UL, 0xfd897000UL,
};
#define AL_PL061_GPIODATA_ALL	0x3FCUL	/* offset with all 8 addr-mask bits set */

/*
 * CCU (Cache Coherency Unit) register offsets, needed here to dump live
 * state before al_ccu_early_coherency_enable() (defined below, CCU section)
 * runs - the rest of the CCU #defines (COMPAT string, SPECULATION_VAL,
 * SNOOP_EN) stay with that section since only the raw offsets are needed
 * this early.
 */
#define AL_CCU_BASE		0xf0090000UL	/* /soc/ccu reg, both DTS sources */
#define AL_CCU_SPECULATION	0x4	/* speculation_ctrl_register_v1_v2 */
#define AL_CCU_SLAVE3_SNOOP	0x4000	/* slaves[3].snoop_control_register, cluster 0 */
#define AL_CCU_SLAVE4_SNOOP	0x5000	/* slaves[4].snoop_control_register, cluster 1 */

static void al_diag_dump(void)
{
	int i;

	printf("\n=== al-diag: PCIe ext0 + GPIO (#140) ===\n");
	printf("DEV_REV_ID      @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_DEV_REV_ID,
	       readl((void __iomem *)AL_PCIE_EXT0_DEV_REV_ID));
	printf("GLOBAL_CONF     @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_GLOBAL_CONF,
	       readl((void __iomem *)AL_PCIE_EXT0_GLOBAL_CONF));
	printf("PF_SEL          @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_PF_SEL,
	       readl((void __iomem *)AL_PCIE_EXT0_PF_SEL));
	printf("MASTER_ARCTL    @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_MASTER_ARCTL,
	       readl((void __iomem *)AL_PCIE_EXT0_MASTER_ARCTL));
	printf("MASTER_AWCTL    @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_MASTER_AWCTL,
	       readl((void __iomem *)AL_PCIE_EXT0_MASTER_AWCTL));
	printf("PBS_PCIE_CONF_1 @ 0x%08lx = 0x%08x\n", AL_PBS_PCIE_CONF_1,
	       readl((void __iomem *)AL_PBS_PCIE_CONF_1));
	printf("CFG_TARGET_BUS  @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_CFG_TARGET_BUS,
	       readl((void __iomem *)AL_PCIE_EXT0_CFG_TARGET_BUS));
	printf("PORT_INIT       @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_PORT_INIT,
	       readl((void __iomem *)AL_PCIE_EXT0_PORT_INIT));
	printf("DEBUG_INFO_0    @ 0x%08lx = 0x%08x (LTSSM 0x%x)\n",
	       AL_PCIE_EXT0_DEBUG_INFO_0, readl((void __iomem *)AL_PCIE_EXT0_DEBUG_INFO_0),
	       al_pcie_ext0_ltssm_state());
	printf("CFGHDR_VID_DID  @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_BASE + 0x10000,
	       readl((void __iomem *)(AL_PCIE_EXT0_BASE + 0x10000)));
	printf("CFGHDR_CMD_STAT @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_CONFIG_HEADER_CMD,
	       readl((void __iomem *)AL_PCIE_EXT0_CONFIG_HEADER_CMD));
	printf("PORT_LINK_CTRL  @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_PORT_LINK_CTRL,
	       readl((void __iomem *)AL_PCIE_EXT0_PORT_LINK_CTRL));
	printf("GEN2_CTRL       @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_GEN2_CTRL,
	       readl((void __iomem *)AL_PCIE_EXT0_GEN2_CTRL));
	printf("VC0_POSTED_Q    @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_VC0_POSTED_Q,
	       readl((void __iomem *)AL_PCIE_EXT0_VC0_POSTED_Q));
	printf("VC0_NONPOST_Q   @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_VC0_NON_POSTED_Q,
	       readl((void __iomem *)AL_PCIE_EXT0_VC0_NON_POSTED_Q));
	printf("AXI_SLAVE_ERR   @ 0x%08lx = 0x%08x\n", AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP,
	       readl((void __iomem *)AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP));
	/*
	 * #140: CCU cluster-snoop state as genuinely inherited (this dump runs
	 * before al_ccu_early_coherency_enable(), see board_init()) - the live
	 * data point needed to tell whether a given test round's `usb start`
	 * ran with coherency already on (sticky from a prior Fedora boot's
	 * ft_board_setup, surviving warm `reset`) or genuinely off.
	 */
	printf("CCU_SPECULATION @ 0x%08lx = 0x%08x\n", AL_CCU_BASE + AL_CCU_SPECULATION,
	       readl((void __iomem *)(AL_CCU_BASE + AL_CCU_SPECULATION)));
	printf("CCU_SLAVE3_SNOOP@ 0x%08lx = 0x%08x\n", AL_CCU_BASE + AL_CCU_SLAVE3_SNOOP,
	       readl((void __iomem *)(AL_CCU_BASE + AL_CCU_SLAVE3_SNOOP)));
	printf("CCU_SLAVE4_SNOOP@ 0x%08lx = 0x%08x\n", AL_CCU_BASE + AL_CCU_SLAVE4_SNOOP,
	       readl((void __iomem *)(AL_CCU_BASE + AL_CCU_SLAVE4_SNOOP)));

	for (i = 0; i < AL_GPIO_BANK_COUNT; i++)
		printf("GPIO bank %d @ 0x%08lx = 0x%02x\n", i, al_gpio_bank_base[i],
		       readl((void __iomem *)(al_gpio_bank_base[i] + AL_PL061_GPIODATA_ALL)) & 0xff);

	printf("=== al-diag done ===\n\n");
}

static int do_aldiag(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	al_diag_dump();
	return CMD_RET_SUCCESS;
}
U_BOOT_CMD(aldiag, 1, 0, do_aldiag,
	   "dump PCIe ext0 + GPIO diagnostic registers (#140)", "");

/* Defined below (CCU section) - forward-declared so board_init() can apply
 * it before any manual usb/pci command, not just at bootm time (#140). */
static void al_ccu_early_coherency_enable(void);

int board_init(void)
{
	/* #140: dump before anything else runs, so it shows vendor's state as
	 * inherited (link already trained, target-bus/snoop/etc as vendor left
	 * them) - not state our own bring-up already altered. */
	al_diag_dump();

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
	printf("board_init: AL_CPUS_SECURE @ 0x%08lx = 0x%x (DBGEN|NIDEN|SPIDEN|SPNIDEN)\n",
	       AL_CPUS_SECURE, AL_DBG_ALL);

	/* External PCIe0 HAL bring-up, before any PCI/USB enumeration (#140). */
	al_pcie_ext0_bringup();

	/*
	 * CCU cluster-snoop coherency (#97's fixup, applied here too - #140):
	 * previously only ran at bootm time via ft_board_setup(), i.e. AFTER
	 * every manual `usb start` this issue has tested with. See the big
	 * comment above al_ccu_early_coherency_enable() for why this matters
	 * for xHCI's device-initiated Event Ring DMA specifically.
	 */
	al_ccu_early_coherency_enable();
	printf("board_init: done\n");
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
 *
 * Register defs come from pcie-al-alpine-regs.h - shared verbatim with
 * this SoC's Linux driver (drivers/pci/controller/pcie-al-internal.c
 * in our fork) and with the EDK2/UEFI port's AlPcieSnoopFixDxe.c, not
 * just cross-checked. See docs/audits/audit-edk2-pcie-glue.md.
 */
#include "pcie-al-alpine-regs.h"

static void al_snoop_one(struct udevice *dev, uint slot)
{
	u32 v;
	int i;

	dm_pci_read_config32(dev, AL_ADAPTER_SMCC, &v);
	v |= AL_ADAPTER_SMCC_SNOOP_BITS;
	dm_pci_write_config32(dev, AL_ADAPTER_SMCC, v);
	if (slot <= AL_INTERNAL_SLOT_THRESHOLD)
		for (i = 1; i < AL_ADAPTER_SMCC_NUM_SUBMASTERS; i++)
			dm_pci_write_config32(dev, AL_ADAPTER_SMCC + i * AL_ADAPTER_SMCC_BUNDLE_SIZE, v);

	dm_pci_read_config32(dev, AL_ADAPTER_APP_CONTROL, &v);
	v = (v & 0xffff0000) | AL_ADAPTER_APP_CONTROL_LO16;
	dm_pci_write_config32(dev, AL_ADAPTER_APP_CONTROL, v);
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
			if ((vendor & 0xffff) != PCI_VENDOR_ID_ANNAPURNA_LABS)
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
/* AL_CCU_BASE/SPECULATION/SLAVE3_SNOOP/SLAVE4_SNOOP: see al_diag_dump() above. */
#define AL_CCU_COMPAT		"annapurna-labs,al-ccu"
#define AL_CCU_SPECULATION_VAL	7
#define AL_CCU_SNOOP_EN		1

static void al_ccu_write_coherency(u64 base, u32 iocc)
{
	printf("al-ccu: coherency fixup starting (base 0x%llx, io_coherency=%u)\n", base, iocc);

	if (iocc) {
		writel(AL_CCU_SNOOP_EN, (void __iomem *)(base + AL_CCU_SLAVE3_SNOOP));
		printf("al-ccu:   slave3_snoop(cluster0) @ 0x%llx = 0x%x\n",
		       base + AL_CCU_SLAVE3_SNOOP, AL_CCU_SNOOP_EN);
		writel(AL_CCU_SNOOP_EN, (void __iomem *)(base + AL_CCU_SLAVE4_SNOOP));
		printf("al-ccu:   slave4_snoop(cluster1) @ 0x%llx = 0x%x\n",
		       base + AL_CCU_SLAVE4_SNOOP, AL_CCU_SNOOP_EN);
	} else {
		printf("al-ccu:   io_coherency=0 - slave3/4 snoop NOT touched\n");
	}
	writel(AL_CCU_SPECULATION_VAL, (void __iomem *)(base + AL_CCU_SPECULATION));
	printf("al-ccu:   speculation_ctrl @ 0x%llx = 0x%x\n",
	       base + AL_CCU_SPECULATION, AL_CCU_SPECULATION_VAL);

	printf("al-ccu: coherency fixup applied (base 0x%llx, io_coherency=%u)\n",
	       base, iocc);
}

/*
 * #140: al_ccu_write_coherency() was, until now, ONLY reachable via
 * ft_board_setup() - a CONFIG_OF_BOARD_SETUP hook that only runs from
 * bootm's FDT-fixup path, i.e. right before jumping to Linux. Every manual
 * `usb start`/`pci enum` run at our own prompt (the entire test loop this
 * issue has used all night) happens BEFORE that - so CCU cluster-snoop
 * (slaves[3]/[4], the actual interconnect-level coherency gate the CCU
 * exposes to the CPU clusters, distinct from the PCIe-unit-local
 * MASTER_ARCTL/AWCTL snoop-request bits already confirmed matching vendor)
 * has never actually been enabled by OUR code at the point xHCI's Enable
 * Slot Command relies on the device's Event Ring write becoming visible.
 * Vendor's own decompiled al_ccu_init() writes these same registers
 * (docs/nor-reference/uboot-ccu-coherency.md) - also only from its own
 * ft_board_setup, at bootm time - but a genuinely fresh vendor boot (no
 * prior Linux session in this power cycle) would hit its own `usb start`
 * with CCU snoop equally unset; this project's own working boot chain runs
 * ft_board_setup on every successful Fedora boot, so whether a given test
 * round's `usb start` inherited an already-enabled CCU (sticky across a
 * warm `reset`, not cleared until a real power-cycle) or not depends on
 * boot history not yet tracked - see al_diag_dump() above, extended to show
 * the live value so this is directly observable instead of assumed.
 *
 * base/io_coherency are hardcoded (matching AL_CCU_BASE and both DTS
 * sources' `io_coherency = <1>`) rather than read from a DTB: board_init()
 * runs before any DTB is loaded under this port (the Fedora DTB is only
 * ext4load'd by bootcmd, right before bootm - see ft_board_setup() below,
 * which genuinely does get a real blob, just too late for this purpose).
 */
static void al_ccu_early_coherency_enable(void)
{
	printf("al-ccu: early (pre-bootm) coherency enable - hardcoded base 0x%lx, io_coherency=1 (no DTB loaded yet, see comment above)\n",
	       AL_CCU_BASE);
	al_ccu_write_coherency(AL_CCU_BASE, 1);
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	int off;
	u64 base;
	u32 iocc;

	off = fdt_node_offset_by_compatible(blob, -1, AL_CCU_COMPAT);
	if (off < 0) {
		printf("al-ccu: no '%s' node in this DTB, skipping bootm-time fixup\n",
		       AL_CCU_COMPAT);
		return 0;
	}

	base = fdt_get_base_address(blob, off);
	if (base == OF_BAD_ADDR) {
		printf("al-ccu: '%s' node has no valid reg, skipping bootm-time fixup\n",
		       AL_CCU_COMPAT);
		return 0;
	}

	iocc = fdtdec_get_uint(blob, off, "io_coherency", 0);
	printf("al-ccu: bootm-time (re-)apply from real DTB (already applied early in board_init(), #140)\n");
	al_ccu_write_coherency(base, iocc);
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
 * Confirmed on hardware (#95): deliberately wedged the 10G UDMA (setenv
 * ethact eth1; ping - reproduces #90's TX-completion-timeout hang), then
 * `reset`. Stock's next boot had a fully clean eth both times - ping and a
 * real bulk tftpboot transfer both succeeded, no T-timeout poisoning.
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
			if ((vendor & 0xffff) != PCI_VENDOR_ID_ANNAPURNA_LABS)
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
