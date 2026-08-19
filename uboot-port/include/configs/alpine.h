/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Annapurna Labs Alpine V2 (AL-324) / Ubiquiti UNVR.
 * Most config now lives in Kconfig / defconfig; this header is minimal.
 */

#ifndef __ALPINE_H
#define __ALPINE_H

/*
 * Selectable console baud rates: `setenv baudrate <n>` then switch the host
 * terminal to match. Default stays 115200 (CONFIG_BAUDRATE). Fast rates added
 * for quicker console I/O; the CP2102 bridge on this board tops out ~1 Mbps, so
 * 921600 (8x) is the safe practical max — higher needs a CP2102N.
 */
#define CFG_SYS_BAUDRATE_TABLE	{ 115200, 230400, 460800, 921600 }

#endif /* __ALPINE_H */
