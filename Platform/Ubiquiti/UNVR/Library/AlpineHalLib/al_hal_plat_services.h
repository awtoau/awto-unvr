/** @file
  UEFI platform services for the Annapurna Labs HAL.

  - Maps HAL MMIO / barriers / delays / mem ops / logging onto MdePkg calls.
  - Adapted from imbushuo/ccr2004-uefi's AlpineHalLib for the UNVR (AL-324).
  - Reg accessors are MACROS, not typed inlines (unlike the ccr2004 original):
    our HAL vintage passes both `void __iomem *` and typed `uintN_t *`, which a
    typed inline rejects under -Wincompatible-pointer-types. Same choice as our
    U-Boot shim (uboot-port/drivers/net/al_hal_shim/al_hal_plat_services.h).
  - al_assert is NON-fatal (print and continue), matching the U-Boot shim: a
    fired assert must be loud but must not wedge a live console.

  Copyright (c) 2024, MikroTik. All rights reserved.
  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PLAT_SERVICES_H__
#define __PLAT_SERVICES_H__

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>

/*
 * Register I/O - MmioReadN/MmioWriteN. Macro form: accepts any pointer type.
 */
#define al_reg_read8(l)               MmioRead8 ((UINTN)(l))
#define al_reg_read16(l)              MmioRead16 ((UINTN)(l))
#define al_reg_read32(l)              MmioRead32 ((UINTN)(l))
#define al_reg_read64(l)              MmioRead64 ((UINTN)(l))
#define al_reg_read32_relaxed(l)      MmioRead32 ((UINTN)(l))

#define al_reg_write8(l, v)           MmioWrite8 ((UINTN)(l), (UINT8)(v))
#define al_reg_write16(l, v)          MmioWrite16 ((UINTN)(l), (UINT16)(v))
#define al_reg_write32(l, v)          MmioWrite32 ((UINTN)(l), (UINT32)(v))
#define al_reg_write64(l, v)          MmioWrite64 ((UINTN)(l), (UINT64)(v))
#define al_reg_write32_relaxed(l, v)  MmioWrite32 ((UINTN)(l), (UINT32)(v))

/*
 * Logging. HAL format strings are C ("%s" = ASCII); EDK2 PrintLib wants "%a".
 * AlHalPrint rewrites %s -> %a, then prints the formatted result.
 */
static inline void
AlHalPrint (
  UINTN       Level,
  const char  *Fmt,
  ...
  )
{
  VA_LIST  Args;
  CHAR8    ConvFmt[256];
  CHAR8    Buf[512];
  UINTN    i;
  UINTN    j;

  if (!DebugPrintLevelEnabled (Level)) {
    return;
  }

  for (i = 0, j = 0; Fmt[i] != '\0' && j < sizeof (ConvFmt) - 2; ) {
    if ((Fmt[i] == '%') && (Fmt[i + 1] == 's')) {
      ConvFmt[j++] = '%';
      ConvFmt[j++] = 'a';
      i += 2;
    } else {
      ConvFmt[j++] = Fmt[i++];
    }
  }

  ConvFmt[j] = '\0';

  VA_START (Args, Fmt);
  AsciiVSPrint (Buf, sizeof (Buf), ConvFmt, Args);
  VA_END (Args);

  DebugPrint (Level, "%a", Buf);
}

#define al_print(...)  AlHalPrint (DEBUG_INFO, __VA_ARGS__)
#define al_err(...)    AlHalPrint (DEBUG_ERROR, __VA_ARGS__)
#define al_warn(...)   AlHalPrint (DEBUG_WARN, __VA_ARGS__)
#define al_info(...)   AlHalPrint (DEBUG_INFO, __VA_ARGS__)
#define al_dbg(...)    AlHalPrint (DEBUG_VERBOSE, __VA_ARGS__)

/* al_sprintf: unused on the eth path; stub rather than pull in a libc. */
#define al_sprintf(buf, ...)  (0)

/*
 * Assertions - print and continue (see file header).
 */
#define al_assert(COND)                                          \
  do {                                                           \
    if (!(COND)) {                                               \
      DEBUG ((DEBUG_ERROR, "%a:%d:%a: assert failed: (%a)\n",    \
              __FILE__, __LINE__, __func__, #COND));             \
    }                                                            \
  } while (AL_FALSE)

#define al_assert_msg(COND, ...)                                 \
  do {                                                           \
    if (!(COND)) {                                               \
      DEBUG ((DEBUG_ERROR, "%a:%d:%a: assert failed: (%a)\n",    \
              __FILE__, __LINE__, __func__, #COND));             \
      AlHalPrint (DEBUG_ERROR, __VA_ARGS__);                     \
    }                                                            \
  } while (AL_FALSE)

/*
 * Memory barriers. AArch64 asm rather than MemoryFence(): MemoryFence() on
 * GCC/AARCH64 is a compiler barrier only, which is not enough between a
 * descriptor write and the doorbell MMIO write.
 */
static inline void
al_data_memory_barrier (
  void
  )
{
  __asm__ __volatile__ ("dsb sy" : : : "memory");
}

static inline void
al_local_data_memory_barrier (
  void
  )
{
  __asm__ __volatile__ ("dmb ish" : : : "memory");
}

static inline void
al_smp_data_memory_barrier (
  void
  )
{
  __asm__ __volatile__ ("dmb ish" : : : "memory");
}

static inline void
al_smp_write_data_memory_barrier (
  void
  )
{
  __asm__ __volatile__ ("dmb ishst" : : : "memory");
}

/*
 * Delays
 */
#define al_udelay(u)  MicroSecondDelay ((UINTN)(u))
#define al_msleep(m)  MicroSecondDelay ((UINTN)(m) * 1000)

/*
 * Endianness - AArch64 UEFI is always LE, so LE conversions are identity.
 */
#define swap16_to_le(x)    (x)
#define swap32_to_le(x)    (x)
#define swap64_to_le(x)    (x)
#define swap16_from_le(x)  (x)
#define swap32_from_le(x)  (x)
#define swap64_from_le(x)  (x)
#define swap16_to_be(x)    SwapBytes16 (x)
#define swap32_to_be(x)    SwapBytes32 (x)
#define swap64_to_be(x)    SwapBytes64 (x)
#define swap16_from_be(x)  SwapBytes16 (x)
#define swap32_from_be(x)  SwapBytes32 (x)
#define swap64_from_be(x)  SwapBytes64 (x)

/*
 * Memory ops. al_memset is (p, val, cnt); SetMem is (p, cnt, val) - swapped.
 */
#define al_memset(p, val, cnt)  SetMem ((p), (cnt), (UINT8)(val))
#define al_memcpy(p1, p2, cnt)  CopyMem ((p1), (p2), (cnt))
#define al_memcmp(p1, p2, cnt)  ((int)CompareMem ((p1), (p2), (cnt)))

#define al_strcmp(s1, s2)  ((int)AsciiStrCmp ((s1), (s2)))

/* Single-CPU context from a UEFI boot-services perspective. */
#define al_get_cpu_id()      0
#define al_get_cluster_id()  0

#endif /* __PLAT_SERVICES_H__ */
