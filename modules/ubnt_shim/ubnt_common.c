// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ubnt_common shim: stands in for the vendor module of the same name.
 * Provides only the symbols the other shims use; no hardware work.
 * Surface and open items: docs/unifi-shim-interface.md (#383).
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */
#include <linux/err.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>

#include "ubnt_shim.h"

int ui_loglevel;
EXPORT_SYMBOL(ui_loglevel);

/* Read len bytes at *pos of the MTD partition `label`; 0 or -errno, *pos advances. */
int ubnt_mtd_partition_read(const char *label, void *buf, size_t len,
			    loff_t *pos)
{
	struct mtd_info *mtd = get_mtd_device_nm(label);
	size_t retlen = 0;
	int rc;

	if (IS_ERR(mtd))
		return PTR_ERR(mtd);
	rc = mtd_read(mtd, *pos, len, &retlen, buf);
	put_mtd_device(mtd);
	if (rc)
		return rc;
	if (retlen != len)
		return -EIO;
	*pos += retlen;
	return 0;
}
EXPORT_SYMBOL(ubnt_mtd_partition_read);

MODULE_DESCRIPTION("Shim for the Ubiquiti ubnt_common module");
MODULE_AUTHOR("Daniel Tyrrell <dan@awto.au>");
MODULE_LICENSE("GPL");
