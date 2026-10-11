/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host stand-in for modules/al_nand/al_hal_plat_services.h (tests only).
 * Register accesses go to the harness' controller model, which decides per
 * address whether it is a plain register or has behaviour. */
#ifndef __PLAT_SERVICES_H__
#define __PLAT_SERVICES_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

uint32_t host_nand_rd32(volatile void *a);
void host_nand_wr32(volatile void *a, uint32_t v);

#define al_reg_read8(l) (*(volatile uint8_t *)(l))
#define al_reg_read16(l) (*(volatile uint16_t *)(l))
#define al_reg_read32(l) host_nand_rd32((volatile void *)(l))
#define al_reg_read64(l) (*(volatile uint64_t *)(l))
#define al_reg_write8(l, v) (*(volatile uint8_t *)(l) = (v))
#define al_reg_write16(l, v) (*(volatile uint16_t *)(l) = (v))
#define al_reg_write32(l, v) host_nand_wr32((volatile void *)(l), (v))
#define al_reg_write64(l, v) (*(volatile uint64_t *)(l) = (v))
#define al_reg_read32_relaxed(l) al_reg_read32(l)
#define al_reg_write32_relaxed(l, v) al_reg_write32(l, v)

#define al_print(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define al_err(...) fprintf(stderr, __VA_ARGS__)
#define al_warn(...) fprintf(stderr, __VA_ARGS__)
#define al_info(...) fprintf(stderr, __VA_ARGS__)
#define al_dbg(...) ((void)0)
#define al_assert(c) do { if (!(c)) abort(); } while (0)

static inline void al_data_memory_barrier(void) {}
static inline void al_smp_data_memory_barrier(void) {}
static inline void al_local_data_memory_barrier(void) {}
#define al_udelay(u) ((void)0)
#define al_msleep(m) ((void)0)
#define swap16_to_le(x) (x)
#define swap32_to_le(x) (x)
#define swap64_to_le(x) (x)
#define swap16_from_le(x) (x)
#define swap32_from_le(x) (x)
#define swap64_from_le(x) (x)
#define al_memset(p, val, cnt) memset(p, val, cnt)
#define al_memcmp(p1, p2, cnt) memcmp(p1, p2, cnt)
#define al_memcpy(d, s, cnt) memcpy(d, s, cnt)
#define al_popcount(x) __builtin_popcount(x)
#endif
