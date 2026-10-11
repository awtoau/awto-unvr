// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* Host harness for modules/al_nand (built by tests/test_al_nand.py).
 *
 * The REAL al_nand_main.c is #included below and linked against the REAL HAL
 * (al_hal_nand.c, unmodified). Fake: the kernel (tests/c/kshim/kshim_nand.h),
 * the registers (a controller model reacting to the command-FIFO writes), and
 * the rawnand core, whose legacy paths are copied from the v7.3 tree with the
 * source line quoted. The device model is a busy timer: ERASE2 holds the
 * device busy for tBERS, PAGEPROG for tPROG, and a WAIT_FOR_READY entry (or,
 * per scenario, a CMD entry) retires only once it is ready.
 *
 * One virtual microsecond clock (k_now_us) is advanced by every wait, so a
 * bound that is too short or too long shows up as a number, not a hang.
 * Tests are selected by argv[1]; each prints PASS/FAIL lines.
 */
#include "al_nand_main.c"
#include "al_hal_nand_regs.h"

/* ------------------------------------------------------------ kshim ---- */
int k_warn_count, k_err_count;
unsigned long long k_now_us;
struct nand_ecc_props k_ecc_req = { .strength = 8, .step_size = 512 };
static int quiet;

void k_log(const char *lvl, const char *fmt, ...)
{
	va_list ap;

	if (quiet)
		return;
	fprintf(stderr, "[%s] ", lvl);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void k_spin_lock(spinlock_t *l)
{
	if (l->held) {
		k_log("BUG", "spin_lock: already held (self-deadlock)");
		abort();
	}
	l->held = 1;
}

void k_spin_unlock(spinlock_t *l)
{
	if (!l->held) {
		k_log("BUG", "spin_unlock: not held");
		abort();
	}
	l->held = 0;
}

int k_spin_held(spinlock_t *l) { return l->held; }

/* ------------------------------------------------- controller model ---- */
#define REGS_SIZE 0x202000
#define PBS_SIZE 0x10000
#define OFF_DATA 0x000000
#define OFF_CMD 0x100000
#define OFF_CTRL 0x201000
#define REG(name) (OFF_CTRL + offsetof(struct al_nand_regs, name))

#define T_BERS_US 10000ULL /* MT29F8G08 ONFI parameter page, docs/nand-study.md */
#define T_PROG_US 600ULL
#define T_ENTRY_US 1ULL /* one FIFO entry, well under the 64 us bound */
#define FOREVER_US (1ULL << 40)

enum stall { STALL_NONE, STALL_STATUS_CMD, STALL_ERASE2_CMD };

static struct {
	uint8_t regs[REGS_SIZE];
	uint8_t pbs[PBS_SIZE];

	/* scenario */
	enum stall stall;
	int stuck_addr; /* an ADDRESS entry never retires (idle device) */
	int fail_reads_from; /* page reads numbered >= this deliver no data */
	int latch_uncorr_on_read; /* page read N sets UNCORR_ERR; 0 = never */
	uint8_t page_data_byte;
	uint8_t status_fail;

	/* device / FIFO */
	unsigned long long busy_until_us;
	unsigned long long fifo_retire_us;
	unsigned int latched;
	unsigned int page_reads;
	int count_phase;
	unsigned int count_lo;
	unsigned int col;
	int addr_idx;
	int last_cmd;
	uint8_t features[256];
	uint8_t feat_addr;
	int feat_bytes;

	/* data FIFO */
	uint8_t rdq[8192];
	unsigned int rd_head, rd_tail;

	/* logs */
	uint32_t cmd_log[256];
	int ncmd;
	int data_requests;

	/* hooks */
	spinlock_t *int_en_lock; /* if set, writes to nfc_int_en must hold it */
} ctl;

static irqreturn_t (*k_irq_handler)(int, void *);
static void *k_irq_dev;
static struct device_node fake_np;
static struct clk fake_clk = { .rate = 500000000 };
static int k_clk_err;
static int k_irq_ret = 5;

static unsigned int stat_now(void)
{
	unsigned int s = ctl.latched;

	if (k_now_us >= ctl.fifo_retire_us)
		s |= AL_NAND_INTR_STATUS_CMD_BUF_EMPTY;
	if (ctl.rd_head != ctl.rd_tail)
		s |= AL_NAND_INTR_STATUS_BUF_RDRDY;
	s |= AL_NAND_INTR_STATUS_BUF_WRRDY;
	return s;
}

static void retire_after(unsigned long long t)
{
	if (ctl.fifo_retire_us < k_now_us)
		ctl.fifo_retire_us = k_now_us;
	ctl.fifo_retire_us += T_ENTRY_US;
	if (t > ctl.fifo_retire_us)
		ctl.fifo_retire_us = t;
}

static void rd_fill(unsigned int n)
{
	unsigned int i;

	ctl.data_requests++;
	if (ctl.fail_reads_from && ctl.page_reads >= (unsigned int)ctl.fail_reads_from)
		return; /* BUF_RDRDY never rises: the irq wait times out */

	for (i = 0; i < n; i++) {
		uint8_t b = 0xff;

		switch (ctl.last_cmd) {
		case NAND_CMD_STATUS: {
			int ready = k_now_us >= ctl.busy_until_us;

			b = 0x80 | (ready ? NAND_STATUS_READY : 0) |
			    (ctl.status_fail ? NAND_STATUS_FAIL : 0);
			break;
		}
		case NAND_CMD_READID: {
			static const uint8_t id[] = { 0x2c, 0xd3, 0xd1, 0x95 };

			b = i < sizeof(id) ? id[i] : 0;
			break;
		}
		case NAND_CMD_GET_FEATURES:
			b = i == 0 ? ctl.features[ctl.feat_addr] : 0;
			break;
		default: /* page / OOB data */
			b = (ctl.col + i) < 4096 ? ctl.page_data_byte : 0xff;
			break;
		}
		ctl.rdq[ctl.rd_tail++ % sizeof(ctl.rdq)] = b;
	}
	if (ctl.last_cmd != NAND_CMD_STATUS && ctl.last_cmd != NAND_CMD_READID &&
	    ctl.last_cmd != NAND_CMD_GET_FEATURES)
		ctl.col += n;
}

static void cmd_entry(uint32_t e)
{
	unsigned int type = e >> 8, arg = e & 0xff;

	if (ctl.ncmd < (int)ARRAY_SIZE(ctl.cmd_log))
		ctl.cmd_log[ctl.ncmd++] = e;

	switch (type) {
	case AL_NAND_COMMAND_TYPE_CMD:
		ctl.last_cmd = arg;
		ctl.addr_idx = 0;
		retire_after(0);
		if (arg == NAND_CMD_ERASE2)
			ctl.busy_until_us = k_now_us + T_BERS_US;
		else if (arg == NAND_CMD_PAGEPROG)
			ctl.busy_until_us = k_now_us + T_PROG_US;
		else if (arg == NAND_CMD_READSTART) {
			ctl.page_reads++;
			if (ctl.latch_uncorr_on_read &&
			    ctl.page_reads == (unsigned int)ctl.latch_uncorr_on_read)
				ctl.latched |= AL_NAND_INTR_STATUS_UNCORR_ERROR;
		}
		if (ctl.stall == STALL_STATUS_CMD && arg == NAND_CMD_STATUS)
			retire_after(ctl.busy_until_us);
		if (ctl.stall == STALL_ERASE2_CMD && arg == NAND_CMD_ERASE2)
			retire_after(ctl.busy_until_us);
		break;
	case AL_NAND_COMMAND_TYPE_ADDRESS:
		retire_after(ctl.stuck_addr ? FOREVER_US : 0);
		if (ctl.last_cmd == NAND_CMD_SET_FEATURES && ctl.addr_idx == 0)
			ctl.feat_addr = arg, ctl.feat_bytes = 0;
		if (ctl.last_cmd == NAND_CMD_GET_FEATURES && ctl.addr_idx == 0)
			ctl.feat_addr = arg;
		if ((ctl.last_cmd == NAND_CMD_READ0 || ctl.last_cmd == NAND_CMD_RNDOUT) &&
		    ctl.addr_idx < 2) {
			if (ctl.addr_idx == 0)
				ctl.col = arg;
			else
				ctl.col |= arg << 8;
		}
		ctl.addr_idx++;
		break;
	case AL_NAND_COMMAND_TYPE_WAIT_FOR_READY:
		retire_after(ctl.busy_until_us);
		break;
	case AL_NAND_COMMAND_TYPE_DATA_READ_COUNT:
		retire_after(0);
		if (!ctl.count_phase) {
			ctl.count_lo = arg;
			ctl.count_phase = 1;
		} else {
			ctl.count_phase = 0;
			rd_fill(ctl.count_lo | (arg << 8));
		}
		break;
	case AL_NAND_COMMAND_TYPE_STATUS_WRITE:
		retire_after(0);
		if (ctl.last_cmd == NAND_CMD_SET_FEATURES && ctl.feat_bytes < 4) {
			if (ctl.feat_bytes == 0)
				ctl.features[ctl.feat_addr] = arg;
			ctl.feat_bytes++;
		}
		break;
	default:
		retire_after(0);
		break;
	}
}

uint32_t host_nand_rd32(volatile void *a)
{
	unsigned long off = (uint8_t *)a - ctl.regs;
	uint32_t v;

	if (off >= REGS_SIZE)
		return *(volatile uint32_t *)a; /* PBS or caller memory */
	if (off == OFF_DATA) {
		v = 0;
		for (int i = 0; i < 4 && ctl.rd_head != ctl.rd_tail; i++)
			v |= (uint32_t)ctl.rdq[ctl.rd_head++ % sizeof(ctl.rdq)] << (8 * i);
		return v;
	}
	if (off == REG(nfc_int_stat))
		return stat_now();
	if (off == REG(rdy_busy_status_reg))
		return k_now_us >= ctl.busy_until_us ? 1 : 0;
	return *(volatile uint32_t *)a;
}

void host_nand_wr32(volatile void *a, uint32_t v)
{
	unsigned long off = (uint8_t *)a - ctl.regs;

	if (off >= REGS_SIZE) {
		*(volatile uint32_t *)a = v;
		return;
	}
	if (off == OFF_CMD || off == REG(command_buffer_reg)) {
		cmd_entry(v);
		return;
	}
	if (off == REG(nfc_int_stat)) {
		ctl.latched &= ~v; /* write-1-to-clear */
		return;
	}
	if (off == REG(nfc_int_en) && ctl.int_en_lock && !k_spin_held(ctl.int_en_lock)) {
		k_log("BUG", "nfc_int_en written without irq_lock");
		k_err_count += 1000;
	}
	*(volatile uint32_t *)a = v;
}

static void fire_irq_if_pending(void)
{
	uint32_t en = *(uint32_t *)(ctl.regs + REG(nfc_int_en));

	if (k_irq_handler && (stat_now() & en))
		k_irq_handler(k_irq_ret, k_irq_dev);
}

void udelay(unsigned long us) { k_now_us += us; }
void cond_resched(void) { k_now_us += 50; }
void complete(struct completion *c) { c->done++; }

unsigned long wait_for_completion_timeout(struct completion *c, unsigned long j)
{
	fire_irq_if_pending(); /* the interrupt the controller would raise */
	if (c->done) {
		c->done--;
		return j ? j : 1;
	}
	k_now_us += (unsigned long long)j * 1000;
	return 0;
}

/* ------------------------------------------------- platform fakes ---- */
void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp) { return calloc(1, size); }
void __iomem *devm_platform_ioremap_resource(struct platform_device *p, unsigned int i) { return ctl.regs; }
void iounmap(void __iomem *a) { }
struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compat) { return &fake_np; }
void __iomem *of_iomap(struct device_node *np, int index) { return ctl.pbs; }
void of_node_put(struct device_node *np) { }
struct clk *devm_clk_get(struct device *dev, const char *id) { return k_clk_err ? ERR_PTR(k_clk_err) : &fake_clk; }
unsigned long clk_get_rate(struct clk *clk) { return clk->rate; }
int platform_get_irq(struct platform_device *p, unsigned int i) { return k_irq_ret; }
int devm_request_irq(struct device *dev, unsigned int irq,
		     irqreturn_t (*handler)(int, void *), unsigned long flags,
		     const char *name, void *dev_id)
{
	k_irq_handler = handler;
	k_irq_dev = dev_id;
	return 0;
}
int mtd_device_register(struct mtd_info *m, const void *parts, int n) { return 0; }
int mtd_device_unregister(struct mtd_info *m) { return 0; }
void nand_controller_init(struct nand_controller *c) { c->ops = NULL; }
void nand_set_flash_node(struct nand_chip *chip, struct device_node *np) { }
void nand_cleanup(struct nand_chip *chip) { }

/* ------------------------------------------- rawnand core (legacy) ---- */
/* nand_command_lp(), drivers/mtd/nand/raw/nand_legacy.c:389-478. Only the
 * paths the driver reaches; the final ready wait is nand_wait_ready(). */
static void fake_wait_ready(struct nand_chip *chip)
{
	unsigned long long deadline = k_now_us + 400000;

	while (!chip->legacy.dev_ready(chip) && k_now_us < deadline)
		cond_resched();
}

static void fake_cmdfunc(struct nand_chip *chip, unsigned int command,
			 int column, int page_addr)
{
	struct mtd_info *mtd = nand_to_mtd(chip);

	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}
	if (command != (unsigned int)NAND_CMD_NONE)
		chip->legacy.cmd_ctrl(chip, command, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;
		int eight = command == NAND_CMD_READID || command == NAND_CMD_PARAM ||
			    command == NAND_CMD_GET_FEATURES || command == NAND_CMD_SET_FEATURES;

		if (column != -1) {
			chip->legacy.cmd_ctrl(chip, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;
			if (!eight)
				chip->legacy.cmd_ctrl(chip, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			chip->legacy.cmd_ctrl(chip, page_addr, ctrl);
			chip->legacy.cmd_ctrl(chip, page_addr >> 8, NAND_NCE | NAND_ALE);
			chip->legacy.cmd_ctrl(chip, page_addr >> 16, NAND_NCE | NAND_ALE);
		}
	}
	chip->legacy.cmd_ctrl(chip, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	switch (command) {
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_READID:
	case NAND_CMD_SET_FEATURES:
		return;
	case NAND_CMD_RNDIN:
		return;
	case NAND_CMD_RNDOUT:
		chip->legacy.cmd_ctrl(chip, NAND_CMD_RNDOUTSTART,
				      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->legacy.cmd_ctrl(chip, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		return;
	case NAND_CMD_READ0:
		if (column == -1 && page_addr == -1)
			return;
		chip->legacy.cmd_ctrl(chip, NAND_CMD_READSTART,
				      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->legacy.cmd_ctrl(chip, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	default:
		break;
	}
	k_now_us += 0; /* ndelay(100) */
	fake_wait_ready(chip);
}

/* nand_status_op() legacy branch, nand_base.c:1900-1903. */
static int fake_status_op(struct nand_chip *chip, u8 *status)
{
	chip->legacy.cmdfunc(chip, NAND_CMD_STATUS, -1, -1);
	if (status)
		*status = chip->legacy.read_byte(chip);
	return 0;
}

/* nand_read_data_op() legacy branch, nand_base.c:2230-2243. */
static int fake_read_data_op(struct nand_chip *chip, void *buf, unsigned int len,
			     bool force_8bit)
{
	if (force_8bit) {
		u8 *p = buf;

		for (unsigned int i = 0; i < len; i++)
			p[i] = chip->legacy.read_byte(chip);
	} else {
		chip->legacy.read_buf(chip, buf, len);
	}
	return 0;
}

/* nand_wait(), nand_legacy.c:533-577: the core's default waitfunc. */
static int fake_nand_wait(struct nand_chip *chip)
{
	unsigned long long deadline = k_now_us + 400000;
	u8 status;
	int ret;

	ret = fake_status_op(chip, NULL);
	if (ret)
		return ret;
	do {
		if (chip->legacy.dev_ready(chip))
			break;
		cond_resched();
	} while (k_now_us < deadline);

	ret = fake_read_data_op(chip, &status, sizeof(status), true);
	if (ret)
		return ret;
	return status;
}

static void fake_write_byte(struct nand_chip *chip, u8 byte)
{
	chip->legacy.write_buf(chip, &byte, 1); /* nand_write_byte(), nand_legacy.c:74 */
}

/* nand_erase_op() legacy branch, nand_base.c:1989-1999. */
int nand_erase_op(struct nand_chip *chip, unsigned int eraseblock)
{
	unsigned int page = eraseblock << (18 - chip->page_shift);
	int ret;
	u8 status;

	chip->legacy.cmdfunc(chip, NAND_CMD_ERASE1, -1, page);
	chip->legacy.cmdfunc(chip, NAND_CMD_ERASE2, -1, -1);
	ret = chip->legacy.waitfunc(chip);
	if (ret < 0)
		return ret;
	status = ret;
	if (status & NAND_STATUS_FAIL)
		return -EIO;
	return 0;
}

/* nand_read_page_op / nand_read_oob_op legacy branches, nand_base.c:1362, 1520. */
int nand_read_page_op(struct nand_chip *chip, unsigned int page, unsigned int off,
		      void *buf, unsigned int len)
{
	chip->legacy.cmdfunc(chip, NAND_CMD_READ0, off, page);
	if (len)
		chip->legacy.read_buf(chip, buf, len);
	return 0;
}

int nand_read_oob_op(struct nand_chip *chip, unsigned int page, unsigned int off,
		     void *buf, unsigned int len)
{
	chip->legacy.cmdfunc(chip, NAND_CMD_READOOB, off, page);
	if (len)
		chip->legacy.read_buf(chip, buf, len);
	return 0;
}

/* nand_prog_page_begin_op / _end_op legacy branches, nand_base.c:1626, 1659-1667. */
int nand_prog_page_begin_op(struct nand_chip *chip, unsigned int page,
			    unsigned int off, const void *buf, unsigned int len)
{
	chip->legacy.cmdfunc(chip, NAND_CMD_SEQIN, off, page);
	if (buf)
		chip->legacy.write_buf(chip, buf, len);
	return 0;
}

int nand_prog_page_end_op(struct nand_chip *chip)
{
	int ret;

	chip->legacy.cmdfunc(chip, NAND_CMD_PAGEPROG, -1, -1);
	ret = chip->legacy.waitfunc(chip);
	if (ret < 0)
		return ret;
	return (ret & NAND_STATUS_FAIL) ? -EIO : 0;
}

/* nand_write_data_op() legacy branch, nand_base.c:2282-2290. */
int nand_write_data_op(struct nand_chip *chip, const void *buf, unsigned int len,
		       bool force_8bit)
{
	if (force_8bit) {
		const u8 *p = buf;

		for (unsigned int i = 0; i < len; i++)
			chip->legacy.write_byte(chip, p[i]);
	} else {
		chip->legacy.write_buf(chip, buf, len);
	}
	return 0;
}

/* nand_read_page_raw(), nand_base.c:3548-3565; nand_read_oob_std(), :3629. */
int nand_read_page_raw(struct nand_chip *chip, u8 *buf, int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	int ret = nand_read_page_op(chip, page, 0, buf, mtd->writesize);

	if (ret)
		return ret;
	if (oob_required)
		ret = fake_read_data_op(chip, chip->oob_poi, mtd->oobsize, false);
	return ret;
}

int nand_read_oob_std(struct nand_chip *chip, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);

	return nand_read_oob_op(chip, page, 0, chip->oob_poi, mtd->oobsize);
}

/* nand_check_erased_buf / nand_check_erased_ecc_chunk, drivers/mtd/nand/core.c:
 * a plain bit-count rewrite of the same contract (returns the number of
 * zero bits if <= threshold, else -EBADMSG; fills with 0xff on success). */
static int check_erased_buf(const void *buf, int len, int thresh)
{
	const unsigned char *b = buf;
	int flips = 0;

	for (; len > 0; len--, b++) {
		flips += 8 - __builtin_popcount(*b);
		if (flips > thresh)
			return -EBADMSG;
	}
	return flips;
}

int nand_check_erased_ecc_chunk(void *data, int datalen, void *ecc, int ecclen,
				void *extraoob, int extraooblen, int thresh)
{
	int d = check_erased_buf(data, datalen, thresh), e, x;

	if (d < 0)
		return d;
	thresh -= d;
	e = check_erased_buf(ecc, ecclen, thresh);
	if (e < 0)
		return e;
	thresh -= e;
	x = check_erased_buf(extraoob, extraooblen, thresh);
	if (x < 0)
		return x;
	if (d)
		memset(data, 0xff, datalen);
	if (e)
		memset(ecc, 0xff, ecclen);
	return d + e + x;
}

static int fls_(unsigned int v) { return v ? 32 - __builtin_clz(v) : 0; }

/* ------------------------------------------------------- the scan ---- */
static struct onfi_params fake_onfi = { .tPROG = 600, .tBERS = 10000, .sdr_timing_modes = 0x3 };
static struct nand_interface_config iface_cfg;
static u8 oob_poi_buf[4096 + 224];

/* nand_scan() in outline, from the v7.3 sequence: nand_reset_interface()
 * (nand_base.c:798), attach_chip (:6509), nand_choose_best_sdr_timings()
 * (:973), nand_setup_interface() (:839-910). */
int nand_scan(struct nand_chip *chip, unsigned int max_chips)
{
	const struct nand_controller_ops *ops = chip->controller->ops;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_interface_config reset_cfg = { .type = NAND_SDR_IFACE };
	u8 param[ONFI_SUBFEATURE_PARAM_LEN] = { 0 };
	int mode, ret;

	if (!chip->legacy.cmdfunc)
		chip->legacy.cmdfunc = fake_cmdfunc;
	if (!chip->legacy.waitfunc)
		chip->legacy.waitfunc = fake_nand_wait;
	if (!chip->legacy.write_byte)
		chip->legacy.write_byte = fake_write_byte;

	mtd->writesize = 4096;
	mtd->oobsize = 224;
	mtd->erasesize = 256 * 1024;
	mtd->size = 1ULL << 30;
	chip->page_shift = 12;
	chip->chip_shift = 30;
	chip->oob_poi = oob_poi_buf;
	chip->parameters.onfi = &fake_onfi;
	chip->parameters.supports_set_get_features = true;

	if (ops->setup_interface) {
		ret = ops->setup_interface(chip, 0, &reset_cfg);
		if (ret)
			return ret;
	}
	chip->legacy.cmdfunc(chip, NAND_CMD_RESET, -1, -1);

	ret = ops->attach_chip(chip);
	if (ret)
		return ret;
	if (!ops->setup_interface)
		return 0;

	for (mode = fls_(fake_onfi.sdr_timing_modes) - 1; mode >= 0; mode--) {
		iface_cfg.type = NAND_SDR_IFACE;
		iface_cfg.timings.mode = mode;
		if (!ops->setup_interface(chip, NAND_DATA_IFACE_CHECK_ONLY, &iface_cfg)) {
			chip->best_interface_config = &iface_cfg;
			break;
		}
	}
	if (mode < 0)
		return -EOPNOTSUPP;

	param[0] = iface_cfg.timings.mode | ONFI_DATA_INTERFACE_SDR;
	chip->legacy.cmdfunc(chip, NAND_CMD_SET_FEATURES, ONFI_FEATURE_ADDR_TIMING_MODE, -1);
	for (int i = 0; i < ONFI_SUBFEATURE_PARAM_LEN; i++)
		chip->legacy.write_byte(chip, param[i]);
	ret = chip->legacy.waitfunc(chip);
	if (ret < 0 || (ret & NAND_STATUS_FAIL))
		return ret < 0 ? ret : -EIO;

	ret = ops->setup_interface(chip, 0, chip->best_interface_config);
	if (ret)
		return ret;

	memset(param, 0, sizeof(param));
	chip->legacy.cmdfunc(chip, NAND_CMD_GET_FEATURES, ONFI_FEATURE_ADDR_TIMING_MODE, -1);
	for (int i = 0; i < ONFI_SUBFEATURE_PARAM_LEN; i++)
		param[i] = chip->legacy.read_byte(chip);
	if (param[0] != (iface_cfg.timings.mode | ONFI_DATA_INTERFACE_SDR)) {
		k_log("WARN", "timing mode %u not acknowledged by the chip",
		      iface_cfg.timings.mode);
		return -EIO;
	}
	return 0;
}

/* ----------------------------------------------------------- tests ---- */
static int fails;

#define CHECK(c, what)                                                     \
	do {                                                               \
		if (c) {                                                   \
			printf("PASS %s\n", what);                         \
		} else {                                                   \
			printf("FAIL %s\n", what);                         \
			fails++;                                           \
		}                                                          \
	} while (0)

static struct platform_device pdev;
static struct al_nand_data *nand;
static struct nand_chip *chip;

/* Run the REAL al_nand_probe() against the model. */
static int do_probe(void)
{
	struct al_nand_dev_properties dp = { 0 };
	struct al_nand_ecc_config ec = { 0 };
	struct al_nand_extra_dev_properties xp = { 0 };
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	k_now_us = 1000000;
	k_err_count = k_warn_count = 0;
	ctl.page_data_byte = 0xff;
	ctl.int_en_lock = NULL;

	/* what the bootloader left in the PBS: 4 KiB page, BCH-8 / 512 B, ECC on */
	xp.pageSize = 4096;
	xp.blockSize = 4096 * 64;
	xp.eccIsEnabled = 1;
	xp.wordSize = 1;
	dp.pageSize = AL_NAND_DEVICE_PAGE_SIZE_4K;
	dp.num_col_cyc = 2;
	dp.num_row_cyc = 3;
	ec.algorithm = AL_NAND_ECC_ALGORITHM_BCH;
	ec.num_corr_bits = AL_NAND_ECC_BCH_NUM_CORR_BITS_8;
	ec.messageSize = 0;
	ec.spareAreaOffset = 4096 + 132;
	al_nand_properties_encode(ctl.pbs, &dp, &ec, &xp);

	memset(&pdev, 0, sizeof(pdev));
	quiet = 1; /* probe's dev_info chatter */
	ret = al_nand_probe(&pdev);
	quiet = 0;
	return ret;
}

static void bind(void)
{
	nand = platform_get_drvdata(&pdev);
	chip = &nand->chip;
}

static int t_probe_sanity(void)
{
	int ret = do_probe();

	CHECK(ret == 0, "real probe() succeeds against the model");
	if (ret)
		return 1;
	bind();
	CHECK(chip->legacy.waitfunc == al_nand_waitfunc, "waitfunc is wrapped");
	CHECK(nand->rdy_poll_ms == 13, "ready bound derived from ONFI: 13 ms");
	return 0;
}

/* #276 / #302: an erase must complete, quietly, with the device ready. */
static void erase_case(enum stall stall, const char *name)
{
	unsigned long long t0;
	char m[128];
	int ret;

	if (do_probe()) {
		CHECK(0, "probe");
		return;
	}
	bind();
	ctl.stall = stall;
	ctl.ncmd = 0;
	k_err_count = 0;
	t0 = k_now_us;
	ret = nand_erase_op(chip, 0);

	snprintf(m, sizeof(m), "%s: erase returns 0 (got %d)", name, ret);
	CHECK(ret == 0, m);
	snprintf(m, sizeof(m), "%s: no timeout logged (%d errors)", name, k_err_count);
	CHECK(k_err_count == 0, m);
	snprintf(m, sizeof(m), "%s: device ready when erase returns (tBERS not abandoned)", name);
	CHECK(k_now_us >= ctl.busy_until_us, m);
	snprintf(m, sizeof(m), "%s: latch clear afterwards", name);
	CHECK(nand->op_err == 0, m);
	snprintf(m, sizeof(m), "%s: took %llu us, bounded by the ready bound", name, k_now_us - t0);
	CHECK(k_now_us - t0 < 20000, m);
}

static int t_erase_stall_status(void) { erase_case(STALL_STATUS_CMD, "status stalls"); return 0; }
static int t_erase_stall_erase2(void) { erase_case(STALL_ERASE2_CMD, "erase2 stalls"); return 0; }
static int t_erase_no_stall(void) { erase_case(STALL_NONE, "no stall"); return 0; }

/* The 64 us bound must still catch a stuck CMD/ADDRESS FIFO on an idle device. */
static int t_stuck_addr_keeps_short_bound(void)
{
	unsigned long long t0;
	int ret;

	if (do_probe())
		return 1;
	bind();
	quiet = 1;
	ctl.stuck_addr = 1;
	k_err_count = 0;
	t0 = k_now_us;
	chip->legacy.cmdfunc(chip, NAND_CMD_READID, 0, -1);
	quiet = 0;
	ret = nand->op_err;

	CHECK(ret == -ETIMEDOUT, "stuck ADDRESS entry latches -ETIMEDOUT");
	CHECK(k_now_us - t0 < 200, "...within ~64 us, not the 13 ms ready bound");
	nand->op_err = 0;
	return 0;
}

/* #302: a latched error must still let core waitfunc wait the device out. */
static int t_waitfunc_runs_core_with_latched_error(void)
{
	int ret;

	if (do_probe())
		return 1;
	bind();
	chip->legacy.cmdfunc(chip, NAND_CMD_ERASE1, -1, 0);
	chip->legacy.cmdfunc(chip, NAND_CMD_ERASE2, -1, -1);
	al_nand_err_set(nand, -EIO);
	ret = chip->legacy.waitfunc(chip);

	CHECK(ret == -EIO, "the latched error is reported");
	CHECK(k_now_us >= ctl.busy_until_us, "...after the device finished (core waitfunc ran)");
	CHECK(nand->op_err == 0, "...and the latch is cleared");
	return 0;
}

/* #292: an uncorrectable page whose raw re-read FAILS is an error, not blank. */
static void reread_case(int fail_reread, int page_byte, int expect_ret_neg,
			unsigned int expect_failed, const char *name)
{
	u8 buf[4096];
	char m[160];
	int ret;

	if (do_probe()) {
		CHECK(0, "probe");
		return;
	}
	bind();
	ctl.page_data_byte = page_byte;
	ctl.latch_uncorr_on_read = 1; /* the ECC read comes back uncorrectable */
	ctl.fail_reads_from = fail_reread ? 2 : 0; /* ...the re-reads are 2 and 3 */
	quiet = 1;
	ret = al_nand_ecc_read_page(chip, buf, 0, 7);
	quiet = 0;

	snprintf(m, sizeof(m), "%s: ret %d, %s", name, ret, expect_ret_neg ? "negative" : "0");
	CHECK(expect_ret_neg ? ret < 0 : ret == 0, m);
	snprintf(m, sizeof(m), "%s: ecc_stats.failed == %u (got %u)", name, expect_failed,
		 chip->mtd.ecc_stats.failed);
	CHECK(chip->mtd.ecc_stats.failed == expect_failed, m);
	snprintf(m, sizeof(m), "%s: latch does not leak into the next page", name);
	CHECK(nand->op_err == 0, m);
}

static int t_reread_failure(void)
{
	reread_case(1, 0xff, 1, 0, "re-read times out");
	return 0;
}

static int t_reread_blank_page(void)
{
	reread_case(0, 0xff, 0, 0, "blank page, re-read ok");
	return 0;
}

static int t_reread_parityless_page(void)
{
	/* #274: data programmed, parity never written -> genuinely uncorrectable */
	reread_case(0, 0xa5, 0, 1, "data without parity");
	return 0;
}

static const struct {
	const char *name;
	int (*fn)(void);
} tests[] = {
	{ "probe_sanity", t_probe_sanity },
	{ "erase_stall_status", t_erase_stall_status },
	{ "erase_stall_erase2", t_erase_stall_erase2 },
	{ "erase_no_stall", t_erase_no_stall },
	{ "stuck_addr_keeps_short_bound", t_stuck_addr_keeps_short_bound },
	{ "waitfunc_runs_core_with_latched_error", t_waitfunc_runs_core_with_latched_error },
	{ "reread_failure", t_reread_failure },
	{ "reread_blank_page", t_reread_blank_page },
	{ "reread_parityless_page", t_reread_parityless_page },
};

int main(int argc, char **argv)
{
	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && strcmp(argv[1], tests[i].name))
			continue;
		tests[i].fn();
		if (argc > 1)
			return fails ? 1 : 0;
	}
	return argc > 1 ? 2 : (fails ? 1 : 0);
}
