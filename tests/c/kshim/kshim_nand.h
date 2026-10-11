/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright (C) 2026 Awto / Daniel Tyrrell */
/* Host stand-ins for the kernel APIs modules/al_nand/al_nand_main.c uses
 * (tests only). Every <linux/...> header the driver names forwards here
 * (written by tests/test_al_nand.py). Struct layouts mirror the v7.3
 * include/linux/mtd/rawnand.h fields the driver touches; the function bodies
 * (nand_erase_op() etc.) live in tests/c/al_nand_host.c and are copies of the
 * mainline LEGACY (non-exec_op) paths, with the source line quoted there. */
#ifndef KSHIM_NAND_H
#define KSHIM_NAND_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int gfp_t;
typedef unsigned int irqreturn_t;

#define __iomem
#define __init
#define __exit
#define likely(x)	(__builtin_expect(!!(x), 1))
#define unlikely(x)	(__builtin_expect(!!(x), 0))
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define SZ_1K 0x400
#define GFP_KERNEL 0u
#define EPROBE_DEFER 517
#define ENOTSUPP 524
#define EOPNOTSUPP_ EOPNOTSUPP

/* ---- ERR_PTR ---- */
#define MAX_ERRNO 4095
#define IS_ERR(p) ((unsigned long)(p) >= (unsigned long)-MAX_ERRNO)
#define PTR_ERR(p) ((long)(p))
#define ERR_PTR(e) ((void *)(long)(e))

/* ---- logging: counted so tests can assert on faults/warnings ---- */
extern int k_warn_count, k_err_count;
struct device;
void k_log(const char *lvl, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
#define dev_err(d, ...)	do { (void)(d); k_err_count++; k_log("ERR", __VA_ARGS__); } while (0)
#define dev_warn(d, ...) do { (void)(d); k_warn_count++; k_log("WARN", __VA_ARGS__); } while (0)
#define dev_info(d, ...) do { (void)(d); k_log("INFO", __VA_ARGS__); } while (0)
#define dev_dbg(d, ...)	((void)(d))
#define dev_err_probe(d, e, ...) \
	({ (void)(d); k_err_count++; k_log("ERR", __VA_ARGS__); (e); })
#define WARN_ON(c) ({ int __w = !!(c); if (__w) { k_warn_count++; k_log("WARN", "WARN_ON(%s) %s:%d", #c, __FILE__, __LINE__); } __w; })

/* ---- module glue (no-ops) ---- */
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define MODULE_DEVICE_TABLE(a, b)
#define module_platform_driver(d)

/* ---- time: one virtual microsecond clock, advanced by the waits ---- */
extern unsigned long long k_now_us;
#define HZ 1000
#define jiffies ((unsigned long)(k_now_us / 1000))
#define msecs_to_jiffies(m) ((unsigned long)(m))
#define jiffies_to_msecs(j) ((unsigned long)(j))
#define time_after(a, b) ((long)((b) - (a)) < 0)
#define time_before(a, b) time_after(b, a)
void udelay(unsigned long us);
void cond_resched(void);

/* ---- locks: a held flag is enough, the harness is single threaded ---- */
typedef struct { int held; } spinlock_t;
#define spin_lock_init(l) ((l)->held = 0)
void k_spin_lock(spinlock_t *l);
void k_spin_unlock(spinlock_t *l);
int k_spin_held(spinlock_t *l);
#define spin_lock_irqsave(l, f) do { (void)(f); k_spin_lock(l); (f) = 0; } while (0)
#define spin_unlock_irqrestore(l, f) do { (void)(f); k_spin_unlock(l); } while (0)
#define spin_lock(l) k_spin_lock(l)
#define spin_unlock(l) k_spin_unlock(l)

/* ---- completion ---- */
struct completion { int done; };
#define init_completion(c) ((c)->done = 0)
#define reinit_completion(c) ((c)->done = 0)
void complete(struct completion *c);
unsigned long wait_for_completion_timeout(struct completion *c, unsigned long j);

/* ---- irq ---- */
#define IRQ_NONE 0
#define IRQ_HANDLED 1
#define IRQF_SHARED 0x80
#define request_irq_t irqreturn_t (*)(int, void *)

/* ---- device / platform ---- */
struct device_node { int dummy; };
struct device { struct device *parent; struct device_node *of_node; };
struct platform_device { struct device dev; void *drvdata; };
struct clk { unsigned long rate; };
struct resource { int dummy; };
struct of_device_id { const char *compatible; };
struct platform_driver {
	struct { const char *name; const struct of_device_id *of_match_table; } driver;
	int (*probe)(struct platform_device *);
	void (*remove)(struct platform_device *);
};
static inline void platform_set_drvdata(struct platform_device *p, void *d) { p->drvdata = d; }
static inline void *platform_get_drvdata(struct platform_device *p) { return p->drvdata; }
void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp);
void __iomem *devm_platform_ioremap_resource(struct platform_device *p, unsigned int i);
void iounmap(void __iomem *a);
struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compat);
void __iomem *of_iomap(struct device_node *np, int index);
void of_node_put(struct device_node *np);
struct clk *devm_clk_get(struct device *dev, const char *id);
unsigned long clk_get_rate(struct clk *clk);
int platform_get_irq(struct platform_device *p, unsigned int i);
int devm_request_irq(struct device *dev, unsigned int irq,
		     irqreturn_t (*handler)(int, void *), unsigned long flags,
		     const char *name, void *dev_id);

/* ---- mtd ---- */
struct mtd_ecc_stats { unsigned int corrected, failed, badblocks, bbtblocks; };
struct mtd_oob_region { u32 offset; u32 length; };
struct mtd_info;
struct mtd_ooblayout_ops {
	int (*ecc)(struct mtd_info *mtd, int section, struct mtd_oob_region *r);
	int (*free)(struct mtd_info *mtd, int section, struct mtd_oob_region *r);
};
struct mtd_info {
	const char *name;
	struct device dev;
	u32 writesize, oobsize, erasesize;
	u64 size;
	struct mtd_ecc_stats ecc_stats;
	unsigned int bitflip_threshold;
	const struct mtd_ooblayout_ops *ooblayout;
};
static inline void mtd_set_ooblayout(struct mtd_info *m, const struct mtd_ooblayout_ops *o) { m->ooblayout = o; }
int mtd_device_register(struct mtd_info *m, const void *parts, int n);
int mtd_device_unregister(struct mtd_info *m);

/* ---- rawnand ---- */
#define NAND_CMD_READ0		0
#define NAND_CMD_RNDOUT		5
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_RNDIN		0x85
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_PARAM		0xec
#define NAND_CMD_GET_FEATURES	0xee
#define NAND_CMD_SET_FEATURES	0xef
#define NAND_CMD_RESET		0xff
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_RNDOUTSTART	0xE0
#define NAND_CMD_NONE		-1
#define NAND_NCE		0x01
#define NAND_CLE		0x02
#define NAND_ALE		0x04
#define NAND_CTRL_CLE		(NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE		(NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE	0x80
#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80
#define NAND_NO_SUBPAGE_WRITE	0x2000
#define NAND_DATA_IFACE_CHECK_ONLY -1
#define ONFI_FEATURE_ADDR_TIMING_MODE 0x01
#define ONFI_DATA_INTERFACE_SDR 0
#define ONFI_SUBFEATURE_PARAM_LEN 4

enum nand_ecc_engine_type { NAND_ECC_ENGINE_TYPE_INVALID, NAND_ECC_ENGINE_TYPE_NONE, NAND_ECC_ENGINE_TYPE_SOFT, NAND_ECC_ENGINE_TYPE_ON_HOST };
enum nand_ecc_algo { NAND_ECC_ALGO_UNKNOWN, NAND_ECC_ALGO_HAMMING, NAND_ECC_ALGO_BCH };
enum nand_interface_type { NAND_SDR_IFACE, NAND_NVDDR_IFACE };

struct nand_ecc_props { unsigned int strength; unsigned int step_size; };
struct nand_interface_config {
	enum nand_interface_type type;
	struct { unsigned int mode; } timings;
};
struct nand_sdr_timings { int dummy; };
static inline bool nand_interface_is_sdr(const struct nand_interface_config *c) { return c->type == NAND_SDR_IFACE; }
static inline const struct nand_sdr_timings *nand_get_sdr_timings(const struct nand_interface_config *c)
{
	static const struct nand_sdr_timings t;
	return c->type == NAND_SDR_IFACE ? &t : ERR_PTR(-EINVAL);
}

struct onfi_params {
	u16 tPROG, tBERS, sdr_timing_modes;
};
struct nand_parameters {
	struct onfi_params *onfi;
	bool supports_set_get_features;
};

struct nand_chip;
struct nand_controller_ops {
	int (*attach_chip)(struct nand_chip *chip);
	int (*setup_interface)(struct nand_chip *chip, int chipnr,
			       const struct nand_interface_config *conf);
};
struct nand_controller { const struct nand_controller_ops *ops; };

struct nand_ecc_ctrl {
	enum nand_ecc_engine_type engine_type;
	enum nand_ecc_algo algo;
	int (*read_page)(struct nand_chip *chip, u8 *buf, int oob_required, int page);
	int (*read_subpage)(struct nand_chip *chip, u32 offs, u32 len, u8 *buf, int page);
	int (*write_page)(struct nand_chip *chip, const u8 *buf, int oob_required, int page);
	int (*read_page_raw)(struct nand_chip *chip, u8 *buf, int oob_required, int page);
	int (*write_page_raw)(struct nand_chip *chip, const u8 *buf, int oob_required, int page);
	int (*read_oob)(struct nand_chip *chip, int page);
	int (*write_oob)(struct nand_chip *chip, int page);
	unsigned int strength, size, steps, bytes, total;
};

struct nand_chip {
	struct mtd_info mtd;
	int base;
	struct nand_controller *controller;
	const struct nand_interface_config *best_interface_config;
	struct nand_parameters parameters;
	struct nand_ecc_ctrl ecc;
	unsigned int options;
	int page_shift, chip_shift;
	u8 *oob_poi;
	struct {
		void (*cmd_ctrl)(struct nand_chip *chip, int dat, unsigned int ctrl);
		void (*cmdfunc)(struct nand_chip *chip, unsigned int command, int column, int page_addr);
		int (*waitfunc)(struct nand_chip *chip);
		u8 (*read_byte)(struct nand_chip *chip);
		void (*write_byte)(struct nand_chip *chip, u8 byte);
		void (*read_buf)(struct nand_chip *chip, u8 *buf, int len);
		void (*write_buf)(struct nand_chip *chip, const u8 *buf, int len);
		int (*dev_ready)(struct nand_chip *chip);
		void (*select_chip)(struct nand_chip *chip, int cs);
		int (*set_features)(struct nand_chip *chip, int addr, u8 *p);
		int (*get_features)(struct nand_chip *chip, int addr, u8 *p);
	} legacy;
};
static inline struct mtd_info *nand_to_mtd(struct nand_chip *c) { return &c->mtd; }
static inline struct nand_chip *mtd_to_nand(struct mtd_info *m) { return container_of(m, struct nand_chip, mtd); }
extern struct nand_ecc_props k_ecc_req;
static inline const struct nand_ecc_props *nanddev_get_ecc_requirements(int *base_unused) { (void)base_unused; return &k_ecc_req; }

void nand_controller_init(struct nand_controller *c);
void nand_set_flash_node(struct nand_chip *chip, struct device_node *np);
int nand_scan(struct nand_chip *chip, unsigned int max_chips);
void nand_cleanup(struct nand_chip *chip);
int nand_erase_op(struct nand_chip *chip, unsigned int eraseblock);
int nand_read_page_op(struct nand_chip *chip, unsigned int page, unsigned int off, void *buf, unsigned int len);
int nand_read_oob_op(struct nand_chip *chip, unsigned int page, unsigned int off, void *buf, unsigned int len);
int nand_prog_page_begin_op(struct nand_chip *chip, unsigned int page, unsigned int off, const void *buf, unsigned int len);
int nand_prog_page_end_op(struct nand_chip *chip);
int nand_write_data_op(struct nand_chip *chip, const void *buf, unsigned int len, bool force_8bit);
int nand_read_page_raw(struct nand_chip *chip, u8 *buf, int oob_required, int page);
int nand_read_oob_std(struct nand_chip *chip, int page);
int nand_check_erased_ecc_chunk(void *data, int datalen, void *ecc, int ecclen,
				void *extraoob, int extraooblen, int bitflips_threshold);

#endif /* KSHIM_NAND_H */
