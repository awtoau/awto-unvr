// SPDX-License-Identifier: GPL-2.0-or-later
/* Annapurna al_ssm (crypto/RAID) UDMA mem-to-mem test - `iodma` U-Boot command.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Glue-reference: vendor U-Boot board/annapurna-labs/common/iodma.c (GPLv2).
 *
 * Purpose: a controlled on-box UDMA test that does NOT touch al_eth. The al_ssm
 * engine (PCI 1c36:0022) drives a SEPARATE al_udma instance from al_eth's, and
 * stock U-Boot's iodma proved mem-to-mem copy works there. If THIS UDMA's M2S
 * descriptor read completes (drhp advances, copy verifies) where al_eth's hangs
 * (#90), the fault is specific to the eth function, not the al_udma fabric path.
 *
 * Reuses the al_eth-built HAL objects (udma/iofic) - depends on CONFIG_AL_ETH
 * (al_unit_adapter_init stub + udma/iofic HAL live in the al_eth module).
 *
 * Ring + buffers MUST be low DRAM: the al_udma master can't reach U-Boot's ~3GB
 * relocated heap (mirror of al_eth_dm.c al_eth_dma_low_alloc - see that file's
 * long comment). Separate window from al_eth's 0x02000000 so the two never clash.
 */

#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <memalign.h>
#include <pci.h>
#include <vsprintf.h>
#include <cpu_func.h>
#include <linux/delay.h>
#include <asm/io.h>

#include <al_hal_ssm.h>
#include <al_hal_ssm_raid.h>
#include <al_hal_udma.h>

#define AL_SSM_PCI_VENDOR	0x1c36
#define AL_SSM_PCI_DEV_SSMAE	0x0022	/* Alpine V2 SSM crypto/RAID */
#define AL_SSM_UDMA_BAR		PCI_BASE_ADDRESS_0
#define AL_SSM_APP_BAR		PCI_BASE_ADDRESS_4
#define AL_SSM_Q_ID		0

/* Ring geometry, matching stock iodma.c. union al_udma_desc = 16 bytes. */
#define AL_SSM_DESCS_PER_Q	256
#define AL_SSM_CDESC_SIZE	16
#define AL_SSM_DESC_ALIGN	16

/* Completion poll bound. A few-hundred-byte mem-to-mem copy on the RAID engine
 * completes in << 1us of transfer; each spin is udelay(1). 100000 (~100ms) is
 * ~5 orders over worst case - it names itself in the log rather than hanging.
 * On expiry: dump the M2S state + print how long we waited. */
#define AL_SSM_COMP_POLL_MAX	100000

/* Default copy size when the command is run with no length arg. */
#define AL_SSM_DEFAULT_LEN	256

/*
 * Low-DRAM bump allocator (see file-head note). Distinct window from al_eth's
 * (0x02000000..0x02100000): use 0x02200000. 1:1 mapped, so virt == phys.
 */
#define AL_SSM_DMA_LOW_BASE	0x02200000UL
#define AL_SSM_DMA_LOW_SIZE	0x00040000UL	/* 256KB: 3 rings (~12KB) + buffers */
static uintptr_t al_ssm_dma_low_next;

static void *al_ssm_dma_low_alloc(size_t size)
{
	uintptr_t p = ALIGN(al_ssm_dma_low_next, ARCH_DMA_MINALIGN);
	uintptr_t end = p + ALIGN(size, ARCH_DMA_MINALIGN);

	if (end > AL_SSM_DMA_LOW_BASE + AL_SSM_DMA_LOW_SIZE)
		return NULL;	/* window exhausted */
	al_ssm_dma_low_next = end;
	return (void *)p;
}

/* TX-state dump - same registers/style as al_eth_dm_dump_tx so the two engines
 * are directly comparable on-box. NEVER read 0x1038 (drtp_inc write-only, data-
 * aborts). m2s.state nibbles: 1 = NORMAL/active, 2 = ABORT; 0 = healthy idle. */
static void al_ssm_dump_tx(void __iomem *u, const char *tag)
{
	printf("iodma[%s]: m2s.state=%08x tx0 drhp=%08x drtp=%08x dcp=%08x crhp=%08x qpkt=%08x\n",
	       tag, readl(u + 0x200), readl(u + 0x1034), readl(u + 0x103c),
	       readl(u + 0x1040), readl(u + 0x104c), readl(u + 0x10c0));
}

static void al_ssm_cache_flush(void *p, size_t len)
{
	uintptr_t s = rounddown((uintptr_t)p, ARCH_DMA_MINALIGN);
	uintptr_t e = roundup((uintptr_t)p + len, ARCH_DMA_MINALIGN);

	flush_dcache_range(s, e);
}

static void al_ssm_cache_inval(void *p, size_t len)
{
	uintptr_t s = rounddown((uintptr_t)p, ARCH_DMA_MINALIGN);
	uintptr_t e = roundup((uintptr_t)p + len, ARCH_DMA_MINALIGN);

	invalidate_dcache_range(s, e);
}

static int al_ssm_probe(struct udevice **devp, void __iomem **udma_base,
			void __iomem **app_base, uint8_t *rev_id)
{
	struct udevice *dev;
	u16 cmd;
	int ret;

	ret = dm_pci_find_device(AL_SSM_PCI_VENDOR, AL_SSM_PCI_DEV_SSMAE, 0, &dev);
	if (ret) {
		printf("iodma: SSM device %04x:%04x not found: %d\n",
		       AL_SSM_PCI_VENDOR, AL_SSM_PCI_DEV_SSMAE, ret);
		return ret;
	}

	dm_pci_read_config8(dev, PCI_REVISION_ID, rev_id);

	/* Enable memory space + bus mastering (DMA) on the endpoint. */
	dm_pci_clrset_config16(dev, PCI_COMMAND, 0,
			       PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	dm_pci_read_config16(dev, PCI_COMMAND, &cmd);
	if (!(cmd & PCI_COMMAND_MEMORY) || !(cmd & PCI_COMMAND_MASTER)) {
		printf("iodma: failed to enable MEM+MASTER (PCI_COMMAND=%04x)\n", cmd);
		return -EIO;
	}

	*udma_base = dm_pci_map_bar(dev, AL_SSM_UDMA_BAR, 0, 0,
				    PCI_REGION_TYPE, PCI_REGION_MEM);
	*app_base = dm_pci_map_bar(dev, AL_SSM_APP_BAR, 0, 0,
				   PCI_REGION_TYPE, PCI_REGION_MEM);
	if (!*udma_base || !*app_base) {
		printf("iodma: failed to map BARs (udma=%p app=%p)\n",
		       *udma_base, *app_base);
		return -EINVAL;
	}

	*devp = dev;
	return 0;
}

static int al_ssm_memcpy(struct al_ssm_dma *raid, void __iomem *udma_base,
			 al_phys_addr_t dst, al_phys_addr_t src, uint32_t size)
{
	struct al_buf src_buf = { .addr = src, .len = size };
	struct al_buf dst_buf = { .addr = dst, .len = size };
	struct al_block src_block = { .bufs = &src_buf, .num = 1 };
	struct al_block dst_block = { .bufs = &dst_buf, .num = 1 };
	struct al_raid_transaction xaction = { 0 };
	uint32_t status = 0;
	int poll = 0;
	int err;

	xaction.op = AL_RAID_OP_MEM_CPY;
	/* NO_SNOOP on both sides: the engine reads src + writes dst as PLAIN DRAM
	 * accesses (serviced by our flush/invalidate) instead of snoop cycles the
	 * fabric never answers in this chainloaded U-Boot - the al_eth TX fix. */
	xaction.flags = AL_SSM_SRC_NO_SNOOP | AL_SSM_DEST_NO_SNOOP;
	xaction.srcs_blocks = &src_block;
	xaction.num_of_srcs = 1;
	xaction.total_src_bufs = 1;
	xaction.dsts_blocks = &dst_block;
	xaction.num_of_dsts = 1;
	xaction.total_dst_bufs = 1;

	err = al_raid_dma_prepare(raid, AL_SSM_Q_ID, &xaction);
	if (err) {
		printf("iodma: al_raid_dma_prepare failed: %d\n", err);
		return err;
	}

	/* Publish src data + the just-written TX/RX submission descriptors to the
	 * engine before ringing the doorbell. */
	al_ssm_cache_flush((void *)(uintptr_t)src, size);
	al_ssm_cache_flush((void *)(uintptr_t)AL_SSM_DMA_LOW_BASE, AL_SSM_DMA_LOW_SIZE);

	err = al_raid_dma_action(raid, AL_SSM_Q_ID, xaction.tx_descs_count);
	if (err) {
		printf("iodma: al_raid_dma_action failed: %d\n", err);
		return err;
	}
	al_ssm_dump_tx(udma_base, "doorbell");

	while (1) {
		/* pull the RX completion + destination the engine wrote. */
		al_ssm_cache_inval((void *)(uintptr_t)AL_SSM_DMA_LOW_BASE,
				   AL_SSM_DMA_LOW_SIZE);
		if (al_raid_dma_completion(raid, AL_SSM_Q_ID, &status))
			break;
		udelay(1);
		if (++poll >= AL_SSM_COMP_POLL_MAX) {
			printf("iodma: completion timeout after %d us\n", poll);
			al_ssm_dump_tx(udma_base, "timeout");
			return -ETIMEDOUT;
		}
	}

	if (status) {
		printf("iodma: transaction completed with status %#x\n", status);
		return -EIO;
	}
	al_ssm_dump_tx(udma_base, "complete");
	return 0;
}

static int do_iodma(struct cmd_tbl *cmdtp, int flag, int argc,
		    char *const argv[])
{
	struct al_ssm_dma raid = { 0 };
	struct al_ssm_dma_params dma_params = { 0 };
	struct al_udma_q_params txp = { 0 }, rxp = { 0 };
	union al_udma_desc *tx_sdesc, *rx_sdesc;
	uint8_t *rx_cdesc, *src, *dst;
	struct udevice *dev;
	void __iomem *udma_base, *app_base;
	uint8_t rev_id = 0;
	uint32_t size = AL_SSM_DEFAULT_LEN;
	unsigned int i, bad = 0;
	int ret;

	if (argc > 1)
		size = simple_strtoul(argv[1], NULL, 0);
	if (!size || size > 0x8000) {
		printf("iodma: size must be 1..32768 bytes\n");
		return CMD_RET_USAGE;
	}

	ret = al_ssm_probe(&dev, &udma_base, &app_base, &rev_id);
	if (ret)
		return CMD_RET_FAILURE;
	printf("iodma: SSM %04x:%04x rev %u  udma=%p app=%p\n",
	       AL_SSM_PCI_VENDOR, AL_SSM_PCI_DEV_SSMAE, rev_id, udma_base, app_base);

	/* All al_udma-visible memory in the low window. */
	al_ssm_dma_low_next = AL_SSM_DMA_LOW_BASE;
	tx_sdesc = al_ssm_dma_low_alloc(AL_SSM_DESCS_PER_Q * sizeof(union al_udma_desc));
	rx_sdesc = al_ssm_dma_low_alloc(AL_SSM_DESCS_PER_Q * sizeof(union al_udma_desc));
	rx_cdesc = al_ssm_dma_low_alloc(AL_SSM_DESCS_PER_Q * AL_SSM_CDESC_SIZE);
	src = al_ssm_dma_low_alloc(size);
	dst = al_ssm_dma_low_alloc(size);
	if (!tx_sdesc || !rx_sdesc || !rx_cdesc || !src || !dst) {
		printf("iodma: low-DRAM window exhausted\n");
		return CMD_RET_FAILURE;
	}
	memset(rx_cdesc, 0, AL_SSM_DESCS_PER_Q * AL_SSM_CDESC_SIZE);

	/* seed src with a walking pattern, clear dst */
	for (i = 0; i < size; i++)
		src[i] = (uint8_t)(i ^ 0xa5);
	memset(dst, 0, size);

	dma_params.rev_id = rev_id;
	dma_params.udma_regs_base = udma_base;
	dma_params.name = "ssm_iodma";
	dma_params.num_of_queues = 4;
	dma_params.unit_adapter = NULL;	/* board_late_init already did PCI/snoop init */

	txp.size = AL_SSM_DESCS_PER_Q;
	txp.desc_base = tx_sdesc;
	txp.desc_phy_base = (al_phys_addr_t)(uintptr_t)tx_sdesc;
	txp.cdesc_base = NULL;		/* TX completions unused (iodma uses RX only) */
	txp.cdesc_phy_base = 0;
	txp.cdesc_size = AL_SSM_CDESC_SIZE;

	rxp.size = AL_SSM_DESCS_PER_Q;
	rxp.desc_base = rx_sdesc;
	rxp.desc_phy_base = (al_phys_addr_t)(uintptr_t)rx_sdesc;
	rxp.cdesc_base = rx_cdesc;
	rxp.cdesc_phy_base = (al_phys_addr_t)(uintptr_t)rx_cdesc;
	rxp.cdesc_size = AL_SSM_CDESC_SIZE;

	if (al_ssm_dma_init(&raid, &dma_params)) {
		printf("iodma: al_ssm_dma_init failed\n");
		return CMD_RET_FAILURE;
	}
	if (al_ssm_dma_q_init(&raid, AL_SSM_Q_ID, &txp, &rxp, AL_RAID_Q)) {
		printf("iodma: al_ssm_dma_q_init failed\n");
		return CMD_RET_FAILURE;
	}
	if (al_ssm_dma_state_set(&raid, UDMA_NORMAL)) {
		printf("iodma: al_ssm_dma_state_set failed\n");
		return CMD_RET_FAILURE;
	}
	al_ssm_dump_tx(udma_base, "post-init");

	ret = al_ssm_memcpy(&raid, udma_base,
			    (al_phys_addr_t)(uintptr_t)dst,
			    (al_phys_addr_t)(uintptr_t)src, size);
	if (ret)
		return CMD_RET_FAILURE;

	/* verify the copy */
	al_ssm_cache_inval(dst, size);
	for (i = 0; i < size; i++) {
		if (dst[i] != src[i]) {
			if (bad < 8)
				printf("iodma:  mismatch @%u: dst=%02x src=%02x\n",
				       i, dst[i], src[i]);
			bad++;
		}
	}

	printf("iodma: copy %u bytes src=%p dst=%p -> %s (%u mismatches)\n",
	       size, src, dst, bad ? "FAIL" : "PASS", bad);
	return bad ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

U_BOOT_CMD(iodma, 2, 0, do_iodma,
	   "al_ssm UDMA mem-to-mem DMA self-test (PCI 1c36:0022)",
	   "[size]\n"
	   "    - DMA-copy <size> bytes (default 256, max 32768) via the al_ssm\n"
	   "      RAID engine's UDMA and verify. Probes the crypto/RAID endpoint,\n"
	   "      runs the copy from low DRAM, prints M2S state + pass/fail.");
