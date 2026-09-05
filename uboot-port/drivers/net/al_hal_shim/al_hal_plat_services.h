/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Unified U-Boot platform services shim for the Annapurna Labs HAL.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL (Copyright (C) 2011/2015 Annapurna Labs Ltd,
 * GPLv2 OR BSD-3-Clause); reimplemented on U-Boot primitives. Modelled on the
 * kernel-tree variant (drivers/soc/alpine/al_hal_plat_services.h) shipped in
 * the Ubiquiti GPL kernel.
 *
 * ONE shim for all HAL ports here (DDR, al_eth, al_serdes, al_ssm). Implements
 * hal/al_hal_plat_contract.h, which is included at the bottom and fails the
 * build naming any primitive this file forgets.
 *  - reg accessors: MACRO form (untyped) — the HAL passes both void __iomem*
 *    (DDR/serdes) and typed uintN_t* (eth) addresses; a macro accepts either,
 *    a typed inline would reject the void* under -Wincompatible-pointer-types.
 *  - barriers: explicit AArch64 dsb/dmb inline asm.
 *  - al_dbg -> debug() (compiled out unless DEBUG).
 *  - endianness: compiler builtins (no byteorder-header dependency).
 *  - al_assert: NON-fatal on every host, per the contract (#245). Fatal is the
 *    separately-named al_assert_fatal(), which no shared HAL code calls.
 */

#ifndef __PLAT_SERVICES_H__
#define __PLAT_SERVICES_H__

#include <stdio.h>		/* printf */
#include <linux/types.h>	/* uintN_t */
#include <linux/errno.h>	/* EIO, EINVAL, ETIME (HAL returns -Exxx) */
#include <linux/delay.h>	/* udelay, mdelay */
#include <linux/string.h>	/* memset, memcpy, memcmp, strcmp */
#include <asm/io.h>		/* readX/writeX */
#include <vsprintf.h>		/* sprintf */
#include <hang.h>		/* hang() — al_assert_fatal */
#include <linux/bitops.h>	/* hweight32 — al_popcount */

#ifdef __cplusplus
extern "C" {
#endif

/* --- MMIO register access (U-Boot readX take addr; writeX take (val, addr)).
 * Macros (untyped) so both void __iomem* and typed uintN_t* callers compile. */
#define al_reg_read8(l)			readb(l)
#define al_reg_read16(l)		readw(l)
#define al_reg_read32(l)		readl(l)
#define al_reg_read64(l)		readq(l)
#define al_reg_read32_relaxed(l)	readl_relaxed(l)

#define al_reg_write8(l, v)		writeb(v, l)
#define al_reg_write16(l, v)		writew(v, l)
#define al_reg_write32(l, v)		writel(v, l)
#define al_reg_write64(l, v)		writeq(v, l)
#define al_reg_write32_relaxed(l, v)	writel_relaxed(v, l)

/* --- Logging (routed to U-Boot printf; al_dbg -> debug, off unless DEBUG) */
#define al_print(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#define al_err(...)		printf(__VA_ARGS__)
#define al_warn(...)		printf(__VA_ARGS__)
#define al_info(...)		printf(__VA_ARGS__)
#define al_dbg(...)		debug(__VA_ARGS__)
#define al_sprintf(...)		sprintf(__VA_ARGS__)

/* --- Assertions: NON-fatal — print once and continue. */
#define al_assert(COND)							\
	do {								\
		if (!(COND))						\
			printf("%s:%d:%s: assert failed: (%s)\n",	\
			       __FILE__, __LINE__, __func__, #COND);	\
	} while (0)

#define al_assert_msg(COND, ...)					\
	do {								\
		if (!(COND)) {						\
			printf("%s:%d:%s: assert failed: (%s)\n",	\
			       __FILE__, __LINE__, __func__, #COND);	\
			printf(__VA_ARGS__);				\
		}							\
	} while (0)

/* Fatal variant: explicitly named, so a caller choosing to die says so.
 * No shared HAL code calls it. U-Boot has no panic() — hang() is the halt. */
#define al_assert_fatal(COND)						\
	do {								\
		if (!(COND)) {						\
			printf("%s:%d:%s: FATAL assert: (%s)\n",	\
			       __FILE__, __LINE__, __func__, #COND);	\
			hang();						\
		}							\
	} while (0)

/* --- Memory barriers (AArch64). Contract requires all four; ack below. */
#define AL_PLAT_BARRIERS_PROVIDED
static inline void al_data_memory_barrier(void)
{
	asm volatile("dsb sy" : : : "memory");
}

static inline void al_local_data_memory_barrier(void)
{
	asm volatile("dmb ish" : : : "memory");
}

static inline void al_smp_data_memory_barrier(void)
{
	asm volatile("dmb ish" : : : "memory");
}

static inline void al_smp_write_data_memory_barrier(void)
{
	asm volatile("dmb ishst" : : : "memory");
}

/* --- Delays */
#define al_udelay(u)		udelay(u)
#define al_msleep(m)		mdelay(m)

/* --- Endianness (little-endian CPU; compiler builtins, no header dep) */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "al HAL plat shim assumes a little-endian CPU"
#endif

#define swap16_to_le(x)		(x)
#define swap32_to_le(x)		(x)
#define swap64_to_le(x)		(x)
#define swap16_from_le(x)	(x)
#define swap32_from_le(x)	(x)
#define swap64_from_le(x)	(x)
#define swap16_to_be(x)		__builtin_bswap16(x)
#define swap32_to_be(x)		__builtin_bswap32(x)
#define swap64_to_be(x)		__builtin_bswap64(x)
#define swap16_from_be(x)	__builtin_bswap16(x)
#define swap32_from_be(x)	__builtin_bswap32(x)
#define swap64_from_be(x)	__builtin_bswap64(x)

/* --- Mem/str ops */
#define al_memset(p, val, cnt)	memset(p, val, cnt)
#define al_memcpy(d, s, cnt)	memcpy(d, s, cnt)
#define al_memcmp(p1, p2, cnt)	memcmp(p1, p2, cnt)
#define al_strcmp(s1, s2)	strcmp(s1, s2)
#define al_popcount(x)		hweight32(x)

/* Single-CPU context for HAL bring-up code. */
#define al_get_cpu_id()		0
#define al_get_cluster_id()	0

#ifdef __cplusplus
}
#endif

/* Last: verifies this file defined every primitive the contract requires. */
#include "al_hal_plat_contract.h"

#endif /* __PLAT_SERVICES_H__ */
