// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Annapurna Labs Alpine NAND controller driver.
 *
 * Copyright (C) 2013 Annapurna Labs Ltd.
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * Ported from the vendor 4.1.37 driver to the v7.3 nand subsystem (#208).
 * The command sequencing and the HW-ECC read/write page paths are the
 * vendor's; the glue around them is rewritten against the current API.
 *
 * Deliberately on chip->legacy: the HW-ECC read path calls chip->legacy.cmdfunc
 * (READOOB/RNDOUT) itself mid-page, which has no exec_op() equivalent. Porting
 * to exec_op() would mean redesigning that path, not porting it.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/rawnand.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/sizes.h>
#include <linux/slab.h>

#include "al_hal_nand.h"

#define AL_NAND_NAME "al-nand"

/* Highest ONFI SDR timing mode this driver will drive the device at. The DT
 * carries max-onfi-timing-mode=<1>; this is the compiled ceiling to match. */
#define AL_NAND_MAX_ONFI_TIMING_MODE 1

/* Below this many zero bits across the whole OOB, treat the page as erased.
 * BCH reports an uncorrectable error on an all-0xff page, so without this an
 * erased block reads as a hard ECC failure. Vendor value. */
#define AL_NAND_MAX_BIT_FLIPS 4

/* 4 KiB page / 224 B OOB on the MT29F8G08 fitted here; 1 KiB is slack. */
#define AL_NAND_MAX_OOB_SIZE SZ_1K

/* ONFI SET FEATURES address for the timing-mode feature. */
#define NAND_SET_FEATURES_ADDR 0xfa

/*
 * Command-FIFO drain poll.
 *   waits for:  the controller's command FIFO to go empty
 *   expected:   one FIFO entry is a few controller clocks at 375 MHz, so even
 *               a full FIFO drains in well under 1 us
 *   limit:      64 us (64 x 1 us polls) - ~64x the worst case, the floor at
 *               which udelay(1) granularity is still meaningful
 *   on expiry:  dev_err naming the elapsed limit; caller continues (the
 *               subsequent HW access reports the real failure)
 */
#define AL_NAND_CMD_FIFO_POLL_US 64

/*
 * Data-buffer interrupt wait.
 *   waits for:  BUF_RDRDY/BUF_WRRDY after a code-word read/write command
 *   expected:   tR (page read) on the MT29F8G08 is 75 us max per datasheet;
 *               tPROG 600 us max
 *   limit:      20 ms - ~33x tPROG, and the ONFI ceiling for any single
 *               page-level operation on an SLC device
 *   on expiry:  -ETIMEDOUT with the mask logged; the read/write aborts
 */
#define AL_NAND_IRQ_TIMEOUT_MS 20

struct al_nand_data {
	struct nand_chip chip;
	struct nand_controller controller;
	struct device *dev;

	struct al_nand_ctrl_obj nand_obj;
	struct al_nand_ecc_config ecc_config;
	struct al_nand_dev_properties dev_props;
	struct al_nand_extra_dev_properties ext_props;
	void __iomem *pbs_base;

	/* read_byte() emulation: the controller has no byte read, so a 4-byte
	 * code word is fetched and handed out one byte at a time. -1 = empty. */
	u8 word_cache[4];
	int cache_pos;

	u32 cw_size;

	/* HW ECC occupies oobsize-ecc_offset bytes starting at ecc_offset.
	 * v7.3 dropped struct nand_ecclayout, so the driver keeps them. */
	u32 ecc_offset;
	u32 ecc_bytes;

	struct completion complete;
	int irq;

	u8 oob[AL_NAND_MAX_OOB_SIZE];
};

static inline struct al_nand_data *to_al_nand(struct nand_chip *chip)
{
	return container_of(chip, struct al_nand_data, chip);
}

/*
 * Addressing RMN: 2903
 *
 * The controller's non-manual NAND timing parameters are wrong and cost
 * performance, so the driver programs them by hand from the ONFI mode.
 */
static unsigned long clk_freq_khz = 375000;
#define NAND_CLK_CYCLES(nsec) ((nsec) * clk_freq_khz / 1000000)

static int al_nand_timing_params_set(struct al_nand_device_timing *timing,
				     u8 mode)
{
	/* Per-mode ONFI SDR timings in ns, converted to controller clocks.
	 * Columns: tSETUP tHOLD tWRP tRR tWB tWH tINTCMD. readDelay is 3 ns
	 * in every mode. Vendor table, unchanged. */
	static const u16 ns[6][7] = {
		{ 14, 22, 54, 43, 206, 32, 86 },
		{ 14, 14, 27, 22, 104, 19, 43 },
		{ 14, 14, 19, 22, 104, 16, 43 },
		{ 14, 14, 16, 22, 104, 11, 27 },
		{ 14, 14, 14, 22, 104, 11, 27 },
		{ 14, 14, 14, 22, 104, 11, 27 },
	};

	if (mode >= ARRAY_SIZE(ns))
		return -EINVAL;

	timing->tSETUP = NAND_CLK_CYCLES(ns[mode][0]);
	timing->tHOLD = NAND_CLK_CYCLES(ns[mode][1]);
	timing->tWRP = NAND_CLK_CYCLES(ns[mode][2]);
	timing->tRR = NAND_CLK_CYCLES(ns[mode][3]);
	timing->tWB = NAND_CLK_CYCLES(ns[mode][4]);
	timing->tWH = NAND_CLK_CYCLES(ns[mode][5]);
	timing->tINTCMD = NAND_CLK_CYCLES(ns[mode][6]);
	timing->readDelay = NAND_CLK_CYCLES(3);

	return 0;
}

/******************************************************************************/
/******************************* interrupts ***********************************/
/******************************************************************************/

static irqreturn_t al_nand_isr(int irq, void *dev_id)
{
	struct al_nand_data *nand = dev_id;
	u32 irq_status;

	irq_status = al_nand_int_status_get(&nand->nand_obj);
	al_nand_int_disable(&nand->nand_obj, irq_status);
	complete(&nand->complete);

	return IRQ_HANDLED;
}

/*
 * Wait until one of irq_mask is actually asserted.
 *
 * The ISR is shared and completes on ANY NAND interrupt, so a bare
 * wait_for_completion() returns on the wrong one. Observed: a 4-byte READID
 * returned success with status 0x145 (CMD_BUF_EMPTY | DATA_BUF_EMPTY |
 * BUF_WRRDY | WRRD_DONE) - BUF_RDRDY clear and the data buffer EMPTY - so the
 * caller memcpy'd a stale buffer and read the same 4 bytes for every address.
 * The status register is the truth; the completion is only a wakeup.
 */
static int al_nand_wait_for_irq(struct al_nand_data *nand, u32 irq_mask)
{
	unsigned long deadline =
		jiffies + msecs_to_jiffies(AL_NAND_IRQ_TIMEOUT_MS);
	u32 status;

	al_nand_int_enable(&nand->nand_obj, irq_mask);

	for (;;) {
		status = al_nand_int_status_get(&nand->nand_obj);
		if (status & irq_mask)
			return 0;

		if (time_after(jiffies, deadline))
			break;

		wait_for_completion_timeout(&nand->complete,
					    max(1UL, deadline - jiffies));
	}

	dev_err(nand->dev,
		"irq timeout after %u ms: wanted 0x%x, status 0x%x\n",
		AL_NAND_IRQ_TIMEOUT_MS, irq_mask, status);
	al_nand_int_disable(&nand->nand_obj, irq_mask);

	return -ETIMEDOUT;
}

/******************************************************************************/
/**************************** command plumbing ********************************/
/******************************************************************************/

static void al_nand_cw_size_get(int num_bytes, u32 *cw_size, u32 *cw_count)
{
	num_bytes = AL_ALIGN_UP(num_bytes, 4);

	if (num_bytes < *cw_size)
		*cw_size = num_bytes;

	if (num_bytes % *cw_size)
		*cw_size = num_bytes / 4;

	*cw_count = num_bytes / *cw_size;
}

/* A byte-count command is split across two FIFO entries, low byte first. */
static void al_nand_send_byte_count_cmd(struct al_nand_ctrl_obj *obj,
					enum al_nand_command_type cmd_id,
					u16 len)
{
	al_nand_cmd_single_execute(obj,
				   AL_NAND_CMD_SEQ_ENTRY(cmd_id, len & 0xff));
	al_nand_cmd_single_execute(
		obj, AL_NAND_CMD_SEQ_ENTRY(cmd_id, (len >> 8) & 0xff));
}

/* Drain the command FIFO. See AL_NAND_CMD_FIFO_POLL_US for the bound. */
static void al_nand_wait_cmd_fifo_empty(struct al_nand_data *nand)
{
	unsigned int i;

	for (i = 0; i < AL_NAND_CMD_FIFO_POLL_US; i++) {
		if (al_nand_cmd_buff_is_empty(&nand->nand_obj))
			return;
		udelay(1);
	}

	dev_err(nand->dev, "command FIFO still busy after %u us\n",
		AL_NAND_CMD_FIFO_POLL_US);
}

static void al_nand_cmd_ctrl(struct nand_chip *chip, int dat,
			     unsigned int ctrl)
{
	struct al_nand_data *nand = to_al_nand(chip);
	enum al_nand_command_type type;
	u32 cmd;

	if (!(ctrl & (NAND_CLE | NAND_ALE)))
		return;

	nand->cache_pos = -1;

	type = ((ctrl & NAND_CTRL_CLE) == NAND_CTRL_CLE) ?
		       AL_NAND_COMMAND_TYPE_CMD :
		       AL_NAND_COMMAND_TYPE_ADDRESS;

	cmd = AL_NAND_CMD_SEQ_ENTRY(type, dat & 0xff);
	dev_dbg(nand->dev, "cmd_ctrl: %s 0x%02x (ctrl 0x%x)\n",
		type == AL_NAND_COMMAND_TYPE_CMD ? "CMD" : "ADDR",
		dat & 0xff, ctrl);
	al_nand_cmd_single_execute(&nand->nand_obj, cmd);
	al_nand_wait_cmd_fifo_empty(nand);

	if (dat == NAND_CMD_PAGEPROG && (ctrl & NAND_CLE)) {
		cmd = AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_WAIT_FOR_READY, 0);
		al_nand_cmd_single_execute(&nand->nand_obj, cmd);
		al_nand_wait_cmd_fifo_empty(nand);

		al_nand_wp_set_enable(&nand->nand_obj, 1);
		al_nand_tx_set_enable(&nand->nand_obj, 0);
	}
}

static void al_nand_select_chip(struct nand_chip *chip, int cs)
{
	struct al_nand_data *nand = to_al_nand(chip);

	if (cs < 0)
		return;

	al_nand_dev_select(&nand->nand_obj, cs);
}

static int al_nand_dev_ready(struct nand_chip *chip)
{
	struct al_nand_data *nand = to_al_nand(chip);

	return al_nand_dev_is_ready(&nand->nand_obj);
}

static void al_nand_read_buf(struct nand_chip *chip, u8 *buf, int len)
{
	struct al_nand_data *nand = to_al_nand(chip);
	u32 cw_size = nand->cw_size;
	void __iomem *data_buff;
	u32 cw_count;

	/* The controller moves whole 4-byte words, and a pending read_byte()
	 * cache would make this read start mid-word. */
	if (WARN_ON(len & 3) || WARN_ON(nand->cache_pos != -1))
		return;

	al_nand_cw_size_get(len, &cw_size, &cw_count);
	al_nand_cw_config(&nand->nand_obj, cw_size, cw_count);

	while (cw_count--)
		al_nand_send_byte_count_cmd(
			&nand->nand_obj,
			AL_NAND_COMMAND_TYPE_DATA_READ_COUNT, cw_size);

	while (len > 0) {
		/* Returning without copying on failure is deliberate: the
		 * buffer is stale, and handing the core stale bytes reads as a
		 * wrong chip rather than as a failed read. */
		if (al_nand_wait_for_irq(nand, AL_NAND_INTR_STATUS_BUF_RDRDY))
			return;

		data_buff = al_nand_data_buff_base_get(&nand->nand_obj);
		memcpy(buf, data_buff, cw_size);
		dev_dbg(nand->dev, "read_buf: cw %u, first %*ph\n", cw_size,
			min(cw_size, 8U), buf);
		buf += cw_size;
		len -= cw_size;
	}
}

/* The controller has no byte read; fetch a 4-byte word and hand it out. */
static u8 al_nand_read_byte(struct nand_chip *chip)
{
	struct al_nand_data *nand = to_al_nand(chip);
	u8 ret;

	if (nand->cache_pos == -1) {
		al_nand_read_buf(chip, nand->word_cache,
				 sizeof(nand->word_cache));
		nand->cache_pos = 0;
	}

	ret = nand->word_cache[nand->cache_pos++];
	if (nand->cache_pos == sizeof(nand->word_cache))
		nand->cache_pos = -1;

	return ret;
}

static void al_nand_write_buf(struct nand_chip *chip, const u8 *buf, int len)
{
	struct al_nand_data *nand = to_al_nand(chip);
	u32 cw_size = chip->ecc.size;
	void __iomem *data_buff;
	u32 cw_count;

	al_nand_tx_set_enable(&nand->nand_obj, 1);
	al_nand_wp_set_enable(&nand->nand_obj, 0);

	al_nand_cw_size_get(len, &cw_size, &cw_count);
	al_nand_cw_config(&nand->nand_obj, cw_size, cw_count);

	while (cw_count--)
		al_nand_send_byte_count_cmd(
			&nand->nand_obj,
			AL_NAND_COMMAND_TYPE_DATA_WRITE_COUNT, cw_size);

	while (len > 0) {
		if (al_nand_wait_for_irq(nand,
					 AL_NAND_INTR_STATUS_BUF_WRRDY))
			return;

		data_buff = al_nand_data_buff_base_get(&nand->nand_obj);
		memcpy(data_buff, buf, cw_size);
		buf += cw_size;
		len -= cw_size;
	}

	/* WP re-enable and TX disable happen in al_nand_cmd_ctrl() after
	 * PAGEPROG + WAIT_FOR_READY, so all data is known written first. */
}

/******************************************************************************/
/********************************* OOB layout *********************************/
/******************************************************************************/

/* HW ECC covers one region: [ecc_offset, oobsize). Bytes 0..1 are the bad-block
 * marker, so free OOB is [2, ecc_offset). */
static int al_nand_ooblayout_ecc(struct mtd_info *mtd, int section,
				 struct mtd_oob_region *region)
{
	struct al_nand_data *nand = to_al_nand(mtd_to_nand(mtd));

	if (section)
		return -ERANGE;

	region->offset = nand->ecc_offset;
	region->length = nand->ecc_bytes;

	return 0;
}

static int al_nand_ooblayout_free(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	struct al_nand_data *nand = to_al_nand(mtd_to_nand(mtd));

	if (section || nand->ecc_offset < 2)
		return -ERANGE;

	region->offset = 2;
	region->length = nand->ecc_offset - 2;

	return 0;
}

static const struct mtd_ooblayout_ops al_nand_ooblayout_ops = {
	.ecc = al_nand_ooblayout_ecc,
	.free = al_nand_ooblayout_free,
};

/******************************************************************************/
/********************************* HW ECC *************************************/
/******************************************************************************/

/* An erased page is all 0xff. BCH flags it uncorrectable, so distinguish it by
 * counting zero bits in the OOB rather than trusting the ECC verdict. */
static bool al_nand_oob_is_empty(const u8 *oob, int len)
{
	int flips = 0;
	int i, j;

	for (i = 0; i < len; i++) {
		if (oob[i] == 0xff)
			continue;

		for (j = 0; j < 8; j++) {
			if (!(oob[i] & BIT(j))) {
				if (++flips >= AL_NAND_MAX_BIT_FLIPS)
					return false;
			}
		}
	}

	return true;
}

static int al_nand_ecc_read_page(struct nand_chip *chip, u8 *buf,
				 int oob_required, int page)
{
	struct al_nand_data *nand = to_al_nand(chip);
	struct mtd_info *mtd = nand_to_mtd(chip);
	int uncorr, corr;

	/* oob_required needs a raw OOB alongside the corrected data, which this
	 * controller cannot produce in one pass - the OOB is consumed by the
	 * ECC engine. nand->options sets NAND_NO_SUBPAGE_WRITE, and the core
	 * only asks for oob_required on a path we do not offer. */
	if (WARN_ON(oob_required))
		return -EINVAL;

	/* Toggle TX to clear the TX/RX ECC state machine. */
	al_nand_tx_set_enable(&nand->nand_obj, 1);
	al_nand_tx_set_enable(&nand->nand_obj, 0);

	al_nand_uncorr_err_clear(&nand->nand_obj);
	al_nand_corr_err_clear(&nand->nand_obj);
	al_nand_ecc_set_enabled(&nand->nand_obj, 1);

	/* The ECC engine needs the OOB before the data, so read the OOB into
	 * the controller first, then rewind the column to 0 for the data. */
	chip->legacy.cmdfunc(chip, NAND_CMD_READOOB, nand->ecc_offset, page);
	al_nand_send_byte_count_cmd(&nand->nand_obj,
				    AL_NAND_COMMAND_TYPE_SPARE_READ_COUNT,
				    nand->ecc_bytes);

	chip->legacy.cmdfunc(chip, NAND_CMD_RNDOUT, 0x00, -1);
	chip->legacy.read_buf(chip, buf, mtd->writesize);

	uncorr = al_nand_uncorr_err_get(&nand->nand_obj);
	corr = al_nand_corr_err_get(&nand->nand_obj);

	al_nand_ecc_set_enabled(&nand->nand_obj, 0);

	if (uncorr) {
		bool real = true;

		if (nand->ecc_config.algorithm == AL_NAND_ECC_ALGORITHM_BCH) {
			chip->legacy.read_buf(chip, nand->oob, mtd->oobsize);
			if (al_nand_oob_is_empty(nand->oob, mtd->oobsize)) {
				real = false;
				memset(buf, 0xff, mtd->writesize);
			}
		}

		if (real) {
			mtd->ecc_stats.failed++;
			dev_err(nand->dev,
				"uncorrectable ECC error in page %d (total %u)\n",
				page, mtd->ecc_stats.failed);
		}
	}

	if (corr) {
		mtd->ecc_stats.corrected++;
		dev_dbg(nand->dev, "corrected ECC error in page %d\n", page);
	}

	return 0;
}

/* The controller reads whole code words only; a sub-page read would have to
 * discard a partial word, which the ECC engine cannot be asked to do. */
static int al_nand_ecc_read_subpage(struct nand_chip *chip, u32 offs, u32 len,
				    u8 *buf, int page)
{
	return -ENOTSUPP;
}

/* page is unused: the core has already issued SEQIN with the page address via
 * cmdfunc by the time write_page() is called. */
static int al_nand_ecc_write_page(struct nand_chip *chip, const u8 *buf,
				  int oob_required, int page)
{
	struct al_nand_data *nand = to_al_nand(chip);
	struct mtd_info *mtd = nand_to_mtd(chip);
	u32 cmd;

	if (WARN_ON(oob_required))
		return -EINVAL;

	al_nand_ecc_set_enabled(&nand->nand_obj, 1);

	al_nand_write_buf(chip, buf, mtd->writesize);

	chip->legacy.cmdfunc(chip, NAND_CMD_RNDIN,
			     mtd->writesize + nand->ecc_offset, -1);

	cmd = AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_WAIT_CYCLE_COUNT, 0);

	al_nand_tx_set_enable(&nand->nand_obj, 1);
	al_nand_wp_set_enable(&nand->nand_obj, 0);
	al_nand_cmd_single_execute(&nand->nand_obj, cmd);

	al_nand_send_byte_count_cmd(&nand->nand_obj,
				    AL_NAND_COMMAND_TYPE_SPARE_WRITE_COUNT,
				    nand->ecc_bytes);

	al_nand_wait_cmd_fifo_empty(nand);

	al_nand_wp_set_enable(&nand->nand_obj, 1);
	al_nand_tx_set_enable(&nand->nand_obj, 0);
	al_nand_ecc_set_enabled(&nand->nand_obj, 0);

	return 0;
}

/******************************************************************************/
/***************************** configuration **********************************/
/******************************************************************************/

static int al_nand_bch_bits_convert(unsigned int bits,
				    enum al_nand_ecc_bch_num_corr_bits *out)
{
	/* The HAL enum is ordered 4,8,12,...,40 in steps of 4. */
	if (bits < 4 || bits > 40 || bits % 4)
		return -EINVAL;

	*out = AL_NAND_ECC_BCH_NUM_CORR_BITS_4 + (bits - 4) / 4;

	return 0;
}

static int al_nand_page_size_convert(unsigned int bytes,
				     enum al_nand_device_page_size *out)
{
	switch (bytes) {
	case 2048:
		*out = AL_NAND_DEVICE_PAGE_SIZE_2K;
		break;
	case 4096:
		*out = AL_NAND_DEVICE_PAGE_SIZE_4K;
		break;
	case 8192:
		*out = AL_NAND_DEVICE_PAGE_SIZE_8K;
		break;
	case 16384:
		*out = AL_NAND_DEVICE_PAGE_SIZE_16K;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* Tell the device which SDR timing mode to use, via ONFI SET FEATURES. */
static void al_nand_set_timing_mode(struct al_nand_data *nand,
				    enum al_nand_device_timing_mode timing)
{
	const u32 cmds[] = {
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_CMD,
				      NAND_CMD_SET_FEATURES),
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_ADDRESS,
				      NAND_SET_FEATURES_ADDR),
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_STATUS_WRITE,
				      timing),
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_STATUS_WRITE, 0x00),
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_STATUS_WRITE, 0x00),
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_STATUS_WRITE, 0x00),
	};

	al_nand_cmd_seq_execute(&nand->nand_obj, cmds, ARRAY_SIZE(cmds));
	al_nand_wait_cmd_fifo_empty(nand);
}

/* Pick the highest SDR mode the device and AL_NAND_MAX_ONFI_TIMING_MODE both
 * allow, and program the controller and the device for it. */
static int al_nand_onfi_config_set(struct nand_chip *chip)
{
	struct al_nand_data *nand = to_al_nand(chip);
	struct al_nand_dev_properties *props = &nand->dev_props;
	const struct nand_ecc_props *req;
	enum al_nand_device_page_size page_size;
	struct mtd_info *mtd = nand_to_mtd(chip);
	u16 modes = 0;
	int i, ret;

	/* Addressing RMN: 2903 - see al_nand_timing_params_set(). */
	props->timingMode = AL_NAND_DEVICE_TIMING_MODE_MANUAL;

	if (chip->parameters.onfi)
		modes = chip->parameters.onfi->sdr_timing_modes;

	/* A non-ONFI chip reports no modes; fall through to the ceiling. */
	for (i = AL_NAND_MAX_ONFI_TIMING_MODE;
	     modes && i && !((1 << i) & modes); i--)
		;

	ret = al_nand_timing_params_set(&props->timing, i);
	if (ret) {
		dev_err(nand->dev, "invalid ONFI timing mode %d\n", i);
		return ret;
	}

	al_nand_set_timing_mode(nand, props->timingMode);

	props->num_col_cyc = chip->page_shift;
	props->num_row_cyc = chip->chip_shift;

	ret = al_nand_page_size_convert(mtd->writesize, &page_size);
	if (ret) {
		dev_err(nand->dev, "unsupported page size %u\n",
			mtd->writesize);
		return ret;
	}
	props->pageSize = page_size;

	req = nanddev_get_ecc_requirements(&chip->base);
	if (req->strength == 1) {
		nand->ecc_config.algorithm = AL_NAND_ECC_ALGORITHM_HAMMING;
	} else if (req->strength > 1) {
		nand->ecc_config.algorithm = AL_NAND_ECC_ALGORITHM_BCH;
		ret = al_nand_bch_bits_convert(
			req->strength, &nand->ecc_config.num_corr_bits);
		if (ret) {
			dev_err(nand->dev,
				"no BCH setting for strength %u\n",
				req->strength);
			return ret;
		}
	}

	return 0;
}

/*
 * Runs between nand_scan_ident() and nand_scan_tail() - exactly the window the
 * vendor driver did this work in, now that nand_scan() is one call.
 */
static int al_nand_attach_chip(struct nand_chip *chip)
{
	struct al_nand_extra_dev_properties *ext_props;
	struct al_nand_data *nand = to_al_nand(chip);
	struct mtd_info *mtd = nand_to_mtd(chip);
	const struct nand_ecc_props *req;
	int ret;

	ext_props = &nand->ext_props;

	if (mtd->oobsize > AL_NAND_MAX_OOB_SIZE) {
		dev_err(nand->dev, "oobsize %u exceeds the %u byte buffer\n",
			mtd->oobsize, (unsigned int)AL_NAND_MAX_OOB_SIZE);
		return -EINVAL;
	}

	ret = al_nand_onfi_config_set(chip);
	if (ret)
		return ret;

	/* ECC region is whatever the boot-time config left between the end of
	 * the page and the spare-area offset the controller was programmed with
	 * - the bootloader's layout, which must be matched exactly to read what
	 * it wrote. */
	if (nand->ecc_config.spareAreaOffset < ext_props->pageSize) {
		dev_err(nand->dev,
			"spare offset %u below page size %u\n",
			nand->ecc_config.spareAreaOffset, ext_props->pageSize);
		return -EINVAL;
	}
	nand->ecc_offset =
		nand->ecc_config.spareAreaOffset - ext_props->pageSize;
	if (nand->ecc_offset >= mtd->oobsize) {
		dev_err(nand->dev, "ecc offset %u outside %u byte OOB\n",
			nand->ecc_offset, mtd->oobsize);
		return -EINVAL;
	}
	nand->ecc_bytes = mtd->oobsize - nand->ecc_offset;

	req = nanddev_get_ecc_requirements(&chip->base);

	if (ext_props->eccIsEnabled) {
		chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
		chip->ecc.algo =
			nand->ecc_config.algorithm == AL_NAND_ECC_ALGORITHM_BCH ?
				NAND_ECC_ALGO_BCH : NAND_ECC_ALGO_HAMMING;
		chip->ecc.read_page = al_nand_ecc_read_page;
		chip->ecc.read_subpage = al_nand_ecc_read_subpage;
		chip->ecc.write_page = al_nand_ecc_write_page;
		chip->ecc.strength = req->strength;
		chip->ecc.size = req->step_size;
		chip->ecc.bytes = nand->ecc_bytes;
		mtd_set_ooblayout(mtd, &al_nand_ooblayout_ops);
	} else {
		chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_NONE;
	}

	ret = al_nand_dev_config(&nand->nand_obj, &nand->dev_props,
				 &nand->ecc_config);
	if (ret) {
		dev_err(nand->dev, "dev_config failed\n");
		return -EIO;
	}

	return 0;
}

static const struct nand_controller_ops al_nand_controller_ops = {
	.attach_chip = al_nand_attach_chip,
};

/******************************************************************************/
/********************************** probe *************************************/
/******************************************************************************/

/* The HAL decodes the controller's NAND config out of the PBS syscon, which is
 * a separate DT node reached by compatible rather than by phandle. */
static void __iomem *al_nand_pbs_map(struct device *dev)
{
	struct device_node *np;
	void __iomem *base;

	np = of_find_compatible_node(NULL, NULL, "annapurna-labs,al-pbs");
	if (!np) {
		dev_err(dev, "no annapurna-labs,al-pbs node in the DT\n");
		return NULL;
	}

	base = of_iomap(np, 0);
	of_node_put(np);
	if (!base)
		dev_err(dev, "failed to map the pbs registers\n");

	return base;
}

static int al_nand_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct al_nand_data *nand;
	void __iomem *nand_base;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	struct clk *clk;
	int ret;

	nand = devm_kzalloc(dev, sizeof(*nand), GFP_KERNEL);
	if (!nand)
		return -ENOMEM;

	nand->dev = dev;
	nand->cache_pos = -1;
	platform_set_drvdata(pdev, nand);

	nand_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(nand_base))
		return PTR_ERR(nand_base);

	nand->pbs_base = al_nand_pbs_map(dev);
	if (!nand->pbs_base)
		return -ENODEV;

	ret = al_nand_init(&nand->nand_obj, nand_base, NULL, 0);
	if (ret) {
		dev_err(dev, "al_nand_init failed\n");
		ret = -EIO;
		goto err_unmap_pbs;
	}

	/* al_hal_nand.h's documented init flow is select -> config_basic ->
	 * RESET -> READID. The vendor driver skipped select and RESET and got
	 * away with it on a controller the 4.1.37 bootloader had already left
	 * configured; we must not assume that. */
	al_nand_dev_select(&nand->nand_obj, 0);

	ret = al_nand_dev_config_basic(&nand->nand_obj);
	if (ret) {
		dev_err(dev, "dev_config_basic failed\n");
		ret = -EIO;
		goto err_terminate;
	}

	/* The bootloader leaves HW ECC ON. Leaving it on through READID feeds
	 * the ID bytes to the BCH engine, which returns them mangled - the
	 * 0xd0/0xad seen instead of Micron's 0x2c/0xd3. ECC is turned back on
	 * per-page by the read/write_page hooks. */
	al_nand_ecc_set_enabled(&nand->nand_obj, 0);

	/* NAND_CLK_CYCLES() converts the ONFI timings to controller clocks, so
	 * a wrong rate here silently mistimes every transfer. */
	clk = devm_clk_get(dev, NULL);
	if (IS_ERR(clk))
		dev_warn(dev, "no clock in the DT, assuming %lu kHz\n",
			 clk_freq_khz);
	else
		clk_freq_khz = clk_get_rate(clk) / 1000;

	nand->irq = platform_get_irq(pdev, 0);
	if (nand->irq < 0) {
		ret = nand->irq;
		goto err_terminate;
	}

	init_completion(&nand->complete);
	al_nand_int_disable(&nand->nand_obj, 0xffff);

	ret = devm_request_irq(dev, nand->irq, al_nand_isr, IRQF_SHARED,
			       AL_NAND_NAME, nand);
	if (ret) {
		dev_err(dev, "failed to request irq %d: %d\n", nand->irq, ret);
		goto err_terminate;
	}

	chip = &nand->chip;
	mtd = nand_to_mtd(chip);
	mtd->dev.parent = dev;
	mtd->name = AL_NAND_NAME;

	nand_controller_init(&nand->controller);
	nand->controller.ops = &al_nand_controller_ops;
	chip->controller = &nand->controller;

	nand_set_flash_node(chip, dev->of_node);

	chip->options = NAND_NO_SUBPAGE_WRITE;
	chip->legacy.cmd_ctrl = al_nand_cmd_ctrl;
	chip->legacy.read_byte = al_nand_read_byte;
	chip->legacy.read_buf = al_nand_read_buf;
	chip->legacy.write_buf = al_nand_write_buf;
	chip->legacy.dev_ready = al_nand_dev_ready;
	chip->legacy.select_chip = al_nand_select_chip;

	/* Must precede nand_scan(), not be deferred to attach_chip(): ident
	 * reads the chip ID through read_buf(), which sizes its transfers from
	 * cw_size, and cw_size comes from ecc_config.messageSize. */
	ret = al_nand_properties_decode(nand->pbs_base, &nand->dev_props,
					&nand->ecc_config, &nand->ext_props);
	if (ret) {
		dev_err(dev, "nand_properties_decode failed\n");
		ret = -EIO;
		goto err_terminate;
	}

	nand->cw_size = 512 << nand->ecc_config.messageSize;
	chip->ecc.size = nand->cw_size;

	/* The bootloader's layout, which reads must match exactly. Logged
	 * because a silently-wrong decode shows up as a garbled chip ID. */
	dev_info(dev,
		 "pbs config: cw %u, ecc %s, spare off %d, page %u, algo %d\n",
		 nand->cw_size,
		 nand->ext_props.eccIsEnabled ? "on" : "off",
		 nand->ecc_config.spareAreaOffset, nand->ext_props.pageSize,
		 nand->ecc_config.algorithm);

	/*
	 * Apply the decoded device config BEFORE nand_scan(), not only in
	 * attach_chip(). al_nand_dev_config_basic() configures from a ZEROED
	 * dev_properties, which leaves sdr_timing_params_0/1 at 0 - no setup,
	 * hold or pulse width on the NAND bus at all. Verified on the box:
	 * both registers read 0x0 and every READID returned the same
	 * `d0 ad d0 ba` regardless of address, ~800 ms per 4-byte read.
	 *
	 * The vendor driver has the same gap and gets away with it because the
	 * 4.1.37 bootloader left working timings in the registers. awto-uboot
	 * does not touch them, so we must program them ourselves - "our U-Boot
	 * does all init itself; chainload pre-state is a crutch".
	 *
	 * Timing mode comes from the device's own ONFI data, which needs the
	 * chip talking first, so attach_chip() still re-runs dev_config with
	 * the ONFI-chosen mode. This pass only has to be good enough to read
	 * the ID, and the PBS-decoded timing set is the bootloader's own.
	 */
	/* The PBS carries timing SET 0 with the manual fields zero, so the
	 * mode-0 ONFI values have to be filled in from the table. Mode 0 is
	 * the slowest and every ONFI device supports it, which is what makes
	 * it the right choice before the device has been identified. */
	nand->dev_props.timingMode = AL_NAND_DEVICE_TIMING_MODE_MANUAL;
	ret = al_nand_timing_params_set(&nand->dev_props.timing, 0);
	if (ret)
		goto err_terminate;

	ret = al_nand_dev_config(&nand->nand_obj, &nand->dev_props,
				 &nand->ecc_config);
	if (ret) {
		dev_err(dev, "pre-scan dev_config failed\n");
		ret = -EIO;
		goto err_terminate;
	}

	/* dev_config re-enables HW ECC from ecc_config; READID must not go
	 * through the BCH engine. Re-enabled per page by the ecc hooks. */
	al_nand_ecc_set_enabled(&nand->nand_obj, 0);

	/* RESET before the first READID, per al_hal_nand.h's init flow: the
	 * device may be mid-operation from the bootloader's last access. */
	al_nand_cmd_single_execute(
		&nand->nand_obj,
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_CMD,
				      NAND_CMD_RESET));
	al_nand_cmd_single_execute(
		&nand->nand_obj,
		AL_NAND_CMD_SEQ_ENTRY(AL_NAND_COMMAND_TYPE_WAIT_FOR_READY, 0));
	al_nand_wait_cmd_fifo_empty(nand);

	/* One CS wired on this board. */
	ret = nand_scan(chip, 1);
	if (ret) {
		dev_err(dev, "nand_scan failed: %d\n", ret);
		goto err_terminate;
	}

	ret = mtd_device_register(mtd, NULL, 0);
	if (ret) {
		dev_err(dev, "mtd_device_register failed: %d\n", ret);
		goto err_cleanup;
	}

	dev_info(dev, "Annapurna Labs NAND: %llu MiB, page %u, oob %u, erase %u\n",
		 mtd->size >> 20, mtd->writesize, mtd->oobsize, mtd->erasesize);

	return 0;

err_cleanup:
	nand_cleanup(chip);
err_terminate:
	al_nand_terminate(&nand->nand_obj);
err_unmap_pbs:
	iounmap(nand->pbs_base);

	return ret;
}

static void al_nand_remove(struct platform_device *pdev)
{
	struct al_nand_data *nand = platform_get_drvdata(pdev);

	WARN_ON(mtd_device_unregister(nand_to_mtd(&nand->chip)));
	nand_cleanup(&nand->chip);
	al_nand_terminate(&nand->nand_obj);
	iounmap(nand->pbs_base);
}

static const struct of_device_id al_nand_match[] = {
	{ .compatible = "annapurna-labs,al-nand" },
	{}
};
MODULE_DEVICE_TABLE(of, al_nand_match);

static struct platform_driver al_nand_driver = {
	.driver = {
		.name = AL_NAND_NAME,
		.of_match_table = al_nand_match,
	},
	.probe = al_nand_probe,
	.remove = al_nand_remove,
};
module_platform_driver(al_nand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Annapurna Labs");
MODULE_AUTHOR("Daniel Tyrrell <dan@awto.au>");
MODULE_DESCRIPTION("Annapurna Labs Alpine NAND controller driver");
