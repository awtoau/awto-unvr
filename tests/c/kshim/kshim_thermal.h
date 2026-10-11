/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright (C) 2026 Awto / Daniel Tyrrell */
/* Host stand-ins for the kernel APIs modules/al_thermal/al_thermal_main.c
 * uses (tests only). Every <linux/...> header the driver names forwards here
 * (written by tests/test_al_thermal.py). The device-resource list, the
 * thermal-zone and hwmon calls are counted so a test can assert on the
 * remove path. */
#ifndef KSHIM_THERMAL_H
#define KSHIM_THERMAL_H

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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define __iomem
#define __init
#define __exit
#define ETIME 62
#define GFP_KERNEL 0u
#define IORESOURCE_MEM 0x200

#define MAX_ERRNO 4095
#define IS_ERR(p) ((unsigned long)(p) >= (unsigned long)-MAX_ERRNO)
#define PTR_ERR(p) ((long)(p))
#define ERR_PTR(e) ((void *)(long)(e))

/* ---- logging ---- */
extern int k_err_count, k_warn_count;
void k_log(const char *lvl, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#define pr_err(...)	do { k_err_count++; k_log("ERR", __VA_ARGS__); } while (0)
#define pr_err_ratelimited(...) pr_err(__VA_ARGS__)
#define pr_warn(...)	do { k_warn_count++; k_log("WARN", __VA_ARGS__); } while (0)
#define pr_info(...)	k_log("INFO", __VA_ARGS__)
#define pr_debug(...)	((void)0)
#define BUG_ON(c)	do { if (c) { k_log("BUG", "BUG_ON(%s)", #c); abort(); } } while (0)

/* ---- module glue ---- */
#define MODULE_DEVICE_TABLE(a, b)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define THIS_MODULE ((void *)0)
#define module_platform_driver(d)
#define of_match_ptr(p) (p)

/* ---- time: one virtual microsecond clock ---- */
extern unsigned long long k_now_us, k_udelay_us, k_sleep_us;
void udelay(unsigned long us);
void k_sleep(unsigned long us);
#define read_poll_timeout(op, val, cond, sleep_us, timeout_us, sleep_before_read, args...) \
({ \
	unsigned long long __t = 0; \
	int __r = 0; \
	if ((sleep_before_read) && (sleep_us)) { k_sleep(sleep_us); __t += (sleep_us); } \
	for (;;) { \
		(val) = op(args); \
		if (cond) break; \
		if ((timeout_us) && __t >= (unsigned long long)(timeout_us)) { \
			(val) = op(args); \
			__r = (cond) ? 0 : -ETIMEDOUT; \
			break; \
		} \
		if (sleep_us) { k_sleep(sleep_us); __t += (sleep_us); } \
	} \
	__r; \
})
#define read_poll_timeout_atomic(op, val, cond, delay_us, timeout_us, delay_before_read, args...) \
({ \
	unsigned long long __t = 0; \
	int __r = 0; \
	if ((delay_before_read) && (delay_us)) { udelay(delay_us); __t += (delay_us); } \
	for (;;) { \
		(val) = op(args); \
		if (cond) break; \
		if ((timeout_us) && __t >= (unsigned long long)(timeout_us)) { \
			(val) = op(args); \
			__r = (cond) ? 0 : -ETIMEDOUT; \
			break; \
		} \
		if (delay_us) { udelay(delay_us); __t += (delay_us); } \
	} \
	__r; \
})

/* ---- device / platform / OF ---- */
struct device_node { int refs; };
struct device { struct device_node *of_node; void *drvdata; };
struct resource { unsigned long start, end; };
#define resource_size(r) ((r)->end - (r)->start + 1)
struct platform_device { struct device dev; struct resource res; };
struct dev_pm_ops { int (*suspend)(struct device *); int (*resume)(struct device *); };
#define SIMPLE_DEV_PM_OPS(name, s, r) const struct dev_pm_ops name = { .suspend = s, .resume = r }
struct of_device_id { const char *compatible; };
struct platform_driver {
	int (*probe)(struct platform_device *);
	void (*remove)(struct platform_device *);
	struct {
		const char *name;
		void *owner;
		const struct dev_pm_ops *pm;
		const struct of_device_id *of_match_table;
	} driver;
};
#define to_platform_device(d) container_of(d, struct platform_device, dev)
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
static inline void *dev_get_drvdata(const struct device *d) { return d->drvdata; }
static inline void platform_set_drvdata(struct platform_device *p, void *d) { p->dev.drvdata = d; }
static inline void *platform_get_drvdata(struct platform_device *p) { return p->dev.drvdata; }
struct resource *platform_get_resource(struct platform_device *p, unsigned int type, unsigned int n);
void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp);
void __iomem *devm_ioremap(struct device *dev, unsigned long start, unsigned long size);
int devm_add_action_or_reset(struct device *dev, void (*action)(void *), void *data);
struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compat);
void __iomem *of_iomap(struct device_node *np, int index);
void of_node_put(struct device_node *np);
void iounmap(void __iomem *a);

/* ---- thermal ---- */
struct thermal_zone_device;
struct thermal_zone_device_ops { int (*get_temp)(struct thermal_zone_device *, int *); };
struct thermal_zone_device { void *priv; const struct thermal_zone_device_ops *ops; };
static inline void *thermal_zone_device_priv(struct thermal_zone_device *tz) { return tz->priv; }
struct thermal_zone_device *devm_thermal_of_zone_register(struct device *dev, int id, void *data,
							  const struct thermal_zone_device_ops *ops);
void thermal_zone_device_unregister(struct thermal_zone_device *tz);
int thermal_add_hwmon_sysfs(struct thermal_zone_device *tz);
void thermal_remove_hwmon_sysfs(struct thermal_zone_device *tz);
int devm_thermal_add_hwmon_sysfs(struct device *dev, struct thermal_zone_device *tz);

#endif /* KSHIM_THERMAL_H */
