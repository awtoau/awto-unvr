/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Annapurna Labs 25G-SerDes + 10GBASE-R PCS bring-up for the UNVR SFP+ port.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL (Copyright (C) Annapurna Labs Ltd, GPLv2 OR
 * BSD-3-Clause); reimplemented on U-Boot primitives.
 */

#ifndef __AL_SERDES_10G_H__
#define __AL_SERDES_10G_H__

/* Bring the SFP+ port's HSSP group-D lane up for a FIXED 10.3125 Gbps
 * (10GBASE-R), KR AN + link-training off. Reads PMA signal-detect, then applies
 * the 10G optic TX/RX EQ via the HSSP HAL vtable (#111). PCS belongs to the MAC
 * driver. Returns 0, or a negative errno. Tap values are unretuned (#207). */
int al_serdes_10g_init(void);

/* Print group-D lane status (version, PMA signal-detect). No HW side effects.
 * PCS block-lock is the MAC driver's - see `eth diag 2`. */
void al_serdes_10g_status(void);

/* Readback for `eth diag`: the TX taps and lane state ACTUALLY in force, not
 * the static table above. Return 0, or a negative errno. */
int al_serdes_10g_tx_params_get(unsigned int lane, unsigned int *override,
				unsigned int *c_minus_1, unsigned int *c_plus_1,
				unsigned int *c_plus_2, unsigned int *tdu,
				unsigned int *amp);
int al_serdes_10g_lane_status_get(unsigned int lane, unsigned int *sig_det,
				  unsigned int *version);

#endif /* __AL_SERDES_10G_H__ */
