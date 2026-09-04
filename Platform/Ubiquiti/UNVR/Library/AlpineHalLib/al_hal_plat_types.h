/** @file
  UEFI platform types for the Annapurna Labs HAL.

  - Maps HAL basic types onto EDK2 Base.h types.
  - Supplies errno / inttypes / NULL that UEFI has no libc for.
  - Adapted from imbushuo/ccr2004-uefi's AlpineHalLib for the UNVR (AL-324).

  Copyright (c) 2024, MikroTik. All rights reserved.
  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PLAT_TYPES_H__
#define __PLAT_TYPES_H__

#include <Base.h>

typedef int al_bool;
#define AL_TRUE   1
#define AL_FALSE  0

/* HAL uses a 64-bit physical address (LPAE) regardless of pointer width. */
typedef UINT64 al_phys_addr_t;

/* AArch64 UEFI is always little-endian */
#define PLAT_ARCH_IS_LITTLE()  AL_TRUE

/* Standard C integer types required by HAL code */
typedef INT8    int8_t;
typedef INT16   int16_t;
typedef INT32   int32_t;
typedef INT64   int64_t;
typedef UINT8   uint8_t;
typedef UINT16  uint16_t;
typedef UINT32  uint32_t;
typedef UINT64  uint64_t;
typedef UINTN   uintptr_t;
typedef UINTN   size_t;

/* errno values used by HAL code (no <errno.h> in UEFI) */
#ifndef EPERM
#define EPERM   1
#endif
#ifndef ENOENT
#define ENOENT  2
#endif
#ifndef EIO
#define EIO     5
#endif
#ifndef EAGAIN
#define EAGAIN  11
#endif
#ifndef ENOMEM
#define ENOMEM  12
#endif
#ifndef EFAULT
#define EFAULT  14
#endif
#ifndef EBUSY
#define EBUSY   16
#endif
#ifndef EEXIST
#define EEXIST  17
#endif
#ifndef EINVAL
#define EINVAL  22
#endif
#ifndef ENOSPC
#define ENOSPC  28
#endif
#ifndef ERANGE
#define ERANGE  34
#endif
#ifndef ENOSYS
#define ENOSYS  38
#endif
#ifndef ETIME
#define ETIME   62
#endif
#ifndef ENOTSUP
#define ENOTSUP 95
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 110
#endif

/* <inttypes.h> format macros used in HAL log strings.
 * EDK2 PrintLib uses a single 'l' for 64-bit, not C's 'll'. */
#ifndef PRId64
#define PRId64  "ld"
#endif
#ifndef PRIu64
#define PRIu64  "lu"
#endif
#ifndef PRIx64
#define PRIx64  "lx"
#endif
#ifndef PRIX64
#define PRIX64  "lX"
#endif
#ifndef PRId32
#define PRId32  "d"
#endif
#ifndef PRIu32
#define PRIu32  "u"
#endif
#ifndef PRIx32
#define PRIx32  "x"
#endif
#ifndef PRIX32
#define PRIX32  "X"
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Linux __iomem annotation - no-op here */
#ifndef __iomem
#define __iomem
#endif

#endif /* __PLAT_TYPES_H__ */
