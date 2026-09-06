/* SPDX-License-Identifier: GPL-2.0-or-later */
/* al_ssm -> shared Linux HAL compatibility shim.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * al_ssm's vendor sources came from a NEWER Annapurna HAL generation than
 * modules/al_eth/ - the one that added V4 (25G) silicon support. #256 makes
 * modules/al_eth the single HAL, so the six symbols al_ssm needs from that
 * generation are expressed here in terms of the shared HAL rather than by
 * keeping a second copy of it.
 *
 * Each is a mechanical rename or a thin wrapper; none reimplements HAL logic.
 * Phase 3 of #256 revisits al_ssm properly - this keeps it building meanwhile.
 */

#ifndef __AL_SSM_HAL_COMPAT_H__
#define __AL_SSM_HAL_COMPAT_H__

#include <linux/stddef.h>	/* offsetof */

#include <al_hal_udma.h>
#include <al_hal_udma_config.h>	/* al_udma_get_revision */
#include <al_hal_udma_iofic.h>

/* The V4 generation raised the per-UDMA queue maximum to 16. This board is
 * rev 2 (V2 silicon, 4 queues), but al_hal_ssm.h sizes a q_types[] array by it,
 * so the larger bound is the safe one to keep. */
#ifndef DMA_MAX_Q_V4
#define DMA_MAX_Q_V4		16
#endif

/* Renamed in the newer generation. Same operation: read the DMA version off the
 * UDMA register window. */
#define al_udma_revision_get(regs_base) \
	al_udma_get_revision((struct unit_regs __iomem *)(regs_base))

/* The newer generation split al_udma_init() into handle-only and
 * handle+configure. The shared HAL has only the combined form, and every
 * al_ssm caller of the handle-only path follows it with its own register
 * programming, so the combined form is correct for both. */
#define al_udma_handle_init(udma, params)	al_udma_init((udma), (params))

/* The _adv iofic variants take the UDMA handle where the shared HAL's take the
 * unit_regs directly. The handle's gen_regs sits at the head of unit_regs, so
 * the conversion is a container cast. */
static INLINE struct unit_regs __iomem *
al_ssm_udma_unit_regs(struct al_udma *udma)
{
	return (struct unit_regs __iomem *)((char *)udma->gen_regs -
					    offsetof(struct unit_regs, gen));
}

#define al_udma_iofic_reg_base_get_adv(udma, level) \
	al_udma_iofic_reg_base_get(al_ssm_udma_unit_regs(udma), (level))

#define al_udma_iofic_unmask_adv(udma, level, group, mask) \
	al_udma_iofic_unmask(al_ssm_udma_unit_regs(udma), (level), (group), (mask))

#define al_udma_iofic_mask_adv(udma, level, group, mask) \
	al_udma_iofic_mask(al_ssm_udma_unit_regs(udma), (level), (group), (mask))

/* The newer generation made the max-descs-per-packet limits rev-dependent (60
 * on V4). The shared HAL keeps the flat 31, which IS the V1/V2/V3 value, so on
 * this rev-2 board the two agree - the argument is simply discarded. */
#undef AL_UDMA_M2S_MAX_ALLOWED_DESCS_PER_PACKET
#undef AL_UDMA_S2M_MAX_ALLOWED_DESCS_PER_PACKET
#define AL_UDMA_M2S_MAX_ALLOWED_DESCS_PER_PACKET(rev_id)	31
#define AL_UDMA_S2M_MAX_ALLOWED_DESCS_PER_PACKET(rev_id)	31

/* External-application interrupt bit in primary group D. Fixed for V1-V3
 * silicon; the newer generation made it a per-revision lookup for V4. */
#define al_udma_iofic_get_ext_app_bit(udma)	AL_INT_GROUP_D_APP_EXT_INT

#endif /* __AL_SSM_HAL_COMPAT_H__ */
