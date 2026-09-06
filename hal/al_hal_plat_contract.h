/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * al_hal_plat_contract.h - the primitives every host must provide to the
 * Annapurna Labs HAL, and what each one MEANS.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * A host (Linux module, U-Boot, EDK2) supplies al_hal_plat_services.h defining
 * every primitive below, then includes THIS header last. A missing primitive is
 * a compile error naming the primitive - not a silent behavioural gap.
 *
 * Why this exists: the vendor shipped one plat_services.h per host with no
 * contract between them, so the copies drifted to 31 vs 41 defines and - worse -
 * gave the SAME NAME OPPOSITE SEMANTICS. al_assert was BUG_ON in Linux and
 * log-and-continue in U-Boot, so a HAL author writing al_assert(group < ngroups)
 * got a warning on one host and a dead box on the other (#245).
 *
 * The primitive list below is not aspirational: it is every al_* primitive
 * actually referenced by the HAL sources in this repo, counted by use.
 */

#ifndef __AL_HAL_PLAT_CONTRACT_H__
#define __AL_HAL_PLAT_CONTRACT_H__

/*
 * Include order is fixed: the host's al_hal_plat_services.h defines the
 * primitives, this header then verifies them. Including this first is a bug.
 */
#ifndef __PLAT_SERVICES_H__
#error "include the host's al_hal_plat_services.h before al_hal_plat_contract.h"
#endif

/*
 * Each section below #errors on a missing macro, naming it. Primitives a host
 * may supply as inline functions (the barriers) cannot be seen by #if defined,
 * so they are covered by an explicit acknowledgement - AL_PLAT_BARRIERS_PROVIDED.
 */

/*
 * ---------------------------------------------------------------------------
 * 1. MMIO register access.  al_reg_readN(addr) -> value;
 *    al_reg_writeN(addr, val) - NOTE the argument order is (addr, val), which
 *    is the reverse of Linux/U-Boot writeN(val, addr).  Every host shim gets
 *    this backwards exactly once.
 *
 *    These MUST be macros, not typed inlines: the HAL passes both
 *    `void __iomem *` (DDR, serdes) and typed `uintN_t *` (eth) addresses to
 *    the same primitive.  A typed inline rejects one of them under
 *    -Wincompatible-pointer-types.
 *
 *    Ordering: the plain forms carry the host's normal MMIO ordering
 *    guarantees.  The _relaxed forms may drop them; they exist for
 *    performance-critical paths and must never be used to order a DMA handoff.
 * ---------------------------------------------------------------------------
 */
#if !defined(al_reg_read8)
#error "plat_services must define al_reg_read8(addr)"
#endif
#if !defined(al_reg_read16)
#error "plat_services must define al_reg_read16(addr)"
#endif
#if !defined(al_reg_read32)
#error "plat_services must define al_reg_read32(addr)"
#endif
#if !defined(al_reg_read64)
#error "plat_services must define al_reg_read64(addr)"
#endif

#if !defined(al_reg_write8)
#error "plat_services must define al_reg_write8(addr, val)"
#endif
#if !defined(al_reg_write16)
#error "plat_services must define al_reg_write16(addr, val)"
#endif
#if !defined(al_reg_write32)
#error "plat_services must define al_reg_write32(addr, val)"
#endif
#if !defined(al_reg_write64)
#error "plat_services must define al_reg_write64(addr, val)"
#endif

#if !defined(al_reg_read32_relaxed)
#error "plat_services must define al_reg_read32_relaxed(addr)"
#endif
#if !defined(al_reg_write32_relaxed)
#error "plat_services must define al_reg_write32_relaxed(addr, val)"
#endif

/*
 * ---------------------------------------------------------------------------
 * 2. Logging.  printf-style, newline supplied by the caller.
 *
 *    al_err / al_warn / al_info / al_print always reach the console.
 *    al_dbg is compiled out unless the host defines DEBUG - it is a
 *    per-register bring-up trace, thousands of lines per boot.
 * ---------------------------------------------------------------------------
 */
#if !defined(al_print)
#error "plat_services must define al_print(fmt, ...)"
#endif
#if !defined(al_err)
#error "plat_services must define al_err(fmt, ...)"
#endif
#if !defined(al_warn)
#error "plat_services must define al_warn(fmt, ...)"
#endif
#if !defined(al_info)
#error "plat_services must define al_info(fmt, ...)"
#endif
#if !defined(al_dbg)
#error "plat_services must define al_dbg(fmt, ...)"
#endif

#if !defined(al_sprintf)
#error "plat_services must define al_sprintf(buf, fmt, ...)"
#endif

/*
 * ---------------------------------------------------------------------------
 * 3. Assertions - THE primitive that must mean one thing on every host.
 *
 *    al_assert(cond) / al_assert_msg(cond, fmt, ...):
 *      - cond true  -> nothing.
 *      - cond false -> report loudly on the console AND CONTINUE.
 *
 *    NON-FATAL, everywhere, no exceptions.  Rationale (#245): the HAL asserts
 *    on bounds it computes itself, and one such bound was wrong -
 *    AL_IOFIC_MAX_GROUPS is the PRIMARY group count (4) used to bound a
 *    SECONDARY loop that has 2 or 3.  Under Linux's BUG_ON that was an
 *    instant panic on a box whose only recovery is a physical power cycle;
 *    under U-Boot's log-and-continue it was a visible warning.  The warning is
 *    the behaviour worth keeping: HAL asserts run against live, already-trained
 *    hardware during diagnostics, where wedging the console loses the evidence.
 *
 *    A host that genuinely wants to die on a failed invariant uses
 *    al_assert_fatal() - a SEPARATE, EXPLICITLY NAMED macro (below), so the
 *    choice is visible at the call site instead of hidden in a per-host header.
 *    Never redefine al_assert to be fatal.
 * ---------------------------------------------------------------------------
 */
#if !defined(al_assert)
#error "plat_services must define al_assert(cond) - non-fatal, see the contract"
#endif
#if !defined(al_assert_msg)
#error "plat_services must define al_assert_msg(cond, fmt, ...) - non-fatal"
#endif

/*
 * al_assert_fatal(cond) - halt/panic the host if cond is false.
 *
 * Not used by shared HAL code.  Provided so a host-specific caller that truly
 * cannot continue can say so explicitly.  A host must define it; the obvious
 * definition is the host's panic/BUG_ON/hang.
 */
#if !defined(al_assert_fatal)
#error "plat_services must define al_assert_fatal(cond) - see the contract"
#endif

/*
 * ---------------------------------------------------------------------------
 * 4. Memory barriers.
 *
 *    al_data_memory_barrier()            - full system barrier; data written by
 *                                          the CPU is visible to DMA masters.
 *                                          Required before ringing a doorbell.
 *    al_local_data_memory_barrier()      - inner-shareable full barrier.
 *    al_smp_data_memory_barrier()        - inner-shareable, CPU-to-CPU ordering.
 *    al_smp_write_data_memory_barrier()  - inner-shareable, stores only.
 *
 *    May be inline functions rather than macros, so the #if-defined test above
 *    cannot see them.  A host acknowledges it has provided all four by defining
 *    AL_PLAT_BARRIERS_PROVIDED; a host that forgets gets this error, and a host
 *    that defines the ack without the functions gets a link error naming the
 *    missing symbol.
 * ---------------------------------------------------------------------------
 */
#if !defined(AL_PLAT_BARRIERS_PROVIDED)
#error "plat_services must provide the four al_*memory_barrier() and #define AL_PLAT_BARRIERS_PROVIDED"
#endif

/*
 * ---------------------------------------------------------------------------
 * 5. Delays.  al_udelay(us) busy-waits.  al_msleep(ms) may sleep on a host that
 *    has a scheduler and must busy-wait on one that does not; HAL callers must
 *    not assume either, and must not call it from an atomic context.
 * ---------------------------------------------------------------------------
 */
#if !defined(al_udelay)
#error "plat_services must define al_udelay(us)"
#endif
#if !defined(al_msleep)
#error "plat_services must define al_msleep(ms)"
#endif

/*
 * ---------------------------------------------------------------------------
 * 6. Endian conversion, host order <-> explicit order.  Identity on a
 *    little-endian CPU for the _le forms.  All twelve are required even though
 *    the Alpine V2 HAL only exercises some: a host that defines a subset makes
 *    the next HAL file that uses one fail at a random later date.
 * ---------------------------------------------------------------------------
 */
#if !defined(swap16_to_le)
#error "plat_services must define swap16_to_le(x)"
#endif
#if !defined(swap32_to_le)
#error "plat_services must define swap32_to_le(x)"
#endif
#if !defined(swap64_to_le)
#error "plat_services must define swap64_to_le(x)"
#endif
#if !defined(swap16_from_le)
#error "plat_services must define swap16_from_le(x)"
#endif
#if !defined(swap32_from_le)
#error "plat_services must define swap32_from_le(x)"
#endif
#if !defined(swap64_from_le)
#error "plat_services must define swap64_from_le(x)"
#endif

#if !defined(swap16_to_be)
#error "plat_services must define swap16_to_be(x)"
#endif
#if !defined(swap32_to_be)
#error "plat_services must define swap32_to_be(x)"
#endif
#if !defined(swap64_to_be)
#error "plat_services must define swap64_to_be(x)"
#endif
#if !defined(swap16_from_be)
#error "plat_services must define swap16_from_be(x)"
#endif
#if !defined(swap32_from_be)
#error "plat_services must define swap32_from_be(x)"
#endif
#if !defined(swap64_from_be)
#error "plat_services must define swap64_from_be(x)"
#endif

/*
 * ---------------------------------------------------------------------------
 * 7. Memory and string operations - C library semantics.
 * ---------------------------------------------------------------------------
 */
#if !defined(al_memset)
#error "plat_services must define al_memset(p, val, cnt)"
#endif
#if !defined(al_memcpy)
#error "plat_services must define al_memcpy(dst, src, cnt)"
#endif
#if !defined(al_memcmp)
#error "plat_services must define al_memcmp(p1, p2, cnt)"
#endif
#if !defined(al_strcmp)
#error "plat_services must define al_strcmp(s1, s2)"
#endif

/* al_popcount(x) - number of set bits in a 32-bit value. */
#if !defined(al_popcount)
#error "plat_services must define al_popcount(x)"
#endif

/*
 * ---------------------------------------------------------------------------
 * 8. CPU identity.  A host with no SMP concept returns 0 from both.
 * ---------------------------------------------------------------------------
 */
#if !defined(al_get_cpu_id)
#error "plat_services must define al_get_cpu_id()"
#endif
#if !defined(al_get_cluster_id)
#error "plat_services must define al_get_cluster_id()"
#endif

#endif /* __AL_HAL_PLAT_CONTRACT_H__ */
