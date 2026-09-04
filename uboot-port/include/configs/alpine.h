/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Annapurna Labs Alpine V2 (AL-324) / Ubiquiti UNVR.
 * Most config now lives in Kconfig / defconfig; this header is minimal.
 */

#ifndef __ALPINE_H
#define __ALPINE_H

/*
 * Selectable console baud rates: `setenv baudrate <n>` then switch the host
 * terminal to match. Default is 1500000 (CONFIG_BAUDRATE); 115200 stays in the
 * table so a stock-U-Boot chainload can be followed at its own rate.
 *
 * The limit is the SoC's UART divisor, not the CP2102 (which accepts 3 Mbps).
 * sbclk is 500 MHz and the divisor is integer, so error = |500e6/(16*d) - baud|:
 *   921600 -> d=34, -0.27%    1000000 -> d=31, +0.81%
 *   1500000 -> d=21, -0.79%   2000000 -> d=16, -2.34%  TOO COARSE
 * 2M exceeds the ~2% framing budget and corrupts under sustained load even
 * though short bursts look fine. 1500000 is the fastest usable rate. #220.
 */
#define CFG_SYS_BAUDRATE_TABLE	{ 115200, 230400, 460800, 921600, 1000000, 1500000 }

#endif /* __ALPINE_H */
