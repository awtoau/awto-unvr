/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host stand-in for modules/al_dma/al_hal_plat_types.h (tests only). */
#ifndef __PLAT_TYPES_H__
#define __PLAT_TYPES_H__
#include "kshim.h"
typedef int al_bool;
#define AL_TRUE 1
#define AL_FALSE 0
typedef uint64_t al_phys_addr_t;
#define PLAT_ARCH_IS_LITTLE() AL_TRUE
#endif
