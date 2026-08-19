/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Unified U-Boot platform types shim for the Annapurna Labs HAL.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL (Copyright (C) 2015 Annapurna Labs Ltd,
 * GPLv2 OR BSD-3-Clause); reimplemented on U-Boot primitives.
 *
 * ONE shim for all three HAL ports (DDR, al_eth, al_serdes). <inttypes.h> is
 * pulled in for the PRIx64-family macros the eth HAL uses in log strings.
 */

#ifndef __PLAT_TYPES_H__
#define __PLAT_TYPES_H__

#include <linux/types.h>	/* uintN_t, phys_addr_t, size_t */
#include <inttypes.h>		/* PRIx64 etc — used by the eth HAL log strings */

#ifdef __cplusplus
extern "C" {
#endif

typedef int al_bool;
#define AL_TRUE		1
#define AL_FALSE	0

/* HAL uses a 64-bit physical address (LPAE) regardless of pointer width. */
typedef uint64_t al_phys_addr_t;

#define PLAT_ARCH_IS_LITTLE()	AL_TRUE

#ifdef __cplusplus
}
#endif

#endif /* __PLAT_TYPES_H__ */
