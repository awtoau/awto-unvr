/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Symbols ubnt_common exports to the other shims. Copyright (C) 2026 Awto / Daniel Tyrrell */
#ifndef UBNT_SHIM_H
#define UBNT_SHIM_H

#include <linux/types.h>

extern int ui_loglevel;
int ubnt_mtd_partition_read(const char *label, void *buf, size_t len,
			    loff_t *pos);

/* ubnthal exports (prototypes unverified vs vendor, see the inventory doc) */
unsigned int ubnthal_get_systemid(void);
unsigned int ubnthal_get_vendorid(void);
unsigned int ubnthal_get_bomrev(void);
unsigned int ubnthal_get_boardrevision(void);
unsigned long ubnthal_get_flashsize(void);
unsigned long ubnthal_get_ramsize(void);

#endif
