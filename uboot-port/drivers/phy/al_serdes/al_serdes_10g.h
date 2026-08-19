/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Annapurna Labs 25G-SerDes + 10GBASE-R PCS bring-up for the UNVR SFP+ port.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL (Copyright (C) Annapurna Labs Ltd, GPLv2 OR
 * BSD-3-Clause); reimplemented on U-Boot primitives.
 */

#ifndef __AL_SERDES_10G_H__
#define __AL_SERDES_10G_H__

/*
 * Bring the AL 25G SerDes group up for the SFP+ port at a FIXED 10.3125 Gbps
 * (10GBASE-R), with KR auto-neg + link-training DISABLED (mode 10G_OPTIC / DAC,
 * link_training=false). Configures the SerDes PMA lane, applies the 10G optic
 * TX/RX equaliser params, then (if the DT exposes the "pcs" reg) the 10GBASE-R
 * PCS. Returns 0 on success, negative errno otherwise.
 *
 * Compile-verified only. The lane index, optic EQ params and PCS reset ordering
 * are hardware-iteration points — see al_serdes_10g.c and README.md.
 */
int al_serdes_10g_init(void);

/* Read + print lane status (PLL lock, signal-detect, CDR lock, rx-valid) and,
 * if the "pcs" reg is present, the 10GBASE-R PCS block-lock. No HW side effects.
 */
void al_serdes_10g_status(void);

#endif /* __AL_SERDES_10G_H__ */
