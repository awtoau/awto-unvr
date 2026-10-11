/* SPDX-License-Identifier: GPL-2.0 */
/*
 * al_dma_logic.h - pure decisions of the al_dma driver.
 *
 * No kernel types, so tests/test_al_dma.py compiles this exact file on the
 * host.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#ifndef __AL_DMA_LOGIC_H__
#define __AL_DMA_LOGIC_H__

/* bool, size_t and errno come from the plat headers pulled in here. */
#include "al_hal_ssm_raid.h"

/* Per-op limits from the vendor driver (al_dma.h AL_DMA_MAX_SIZE_*,
 * al_hal_ssm_raid.c AL_RAID_OP_{P,Q}_MAX_BLOCK_SIZE). It splits larger
 * requests; we reject them (#290). Q and PQ are 8 KiB, not 16 (#169). */
#define AL_DMA_MAX_LEN_MEMCPY	(1u << 16)
#define AL_DMA_MAX_LEN_P	(1u << 14)
#define AL_DMA_MAX_LEN_Q	(1u << 13)

/* Sources the HAL can encode: one tx descriptor each (AL_SSM_MAX_SRC_DESCS). */
#define AL_DMA_MAX_SRCS		31

static inline size_t al_dma_max_len(enum al_raid_op op)
{
	switch (op) {
	case AL_RAID_OP_MEM_CPY:
		return AL_DMA_MAX_LEN_MEMCPY;
	case AL_RAID_OP_P_CALC:
		return AL_DMA_MAX_LEN_P;
	case AL_RAID_OP_Q_CALC:
	case AL_RAID_OP_PQ_CALC:
		return AL_DMA_MAX_LEN_Q;
	default:
		return 0;
	}
}

/* A request the descriptors can encode: len is non-zero and within the op's
 * limit, and 1..AL_DMA_MAX_SRCS sources (memcpy passes src_cnt = 1). */
static inline bool al_dma_request_ok(enum al_raid_op op, size_t len,
				     unsigned int src_cnt)
{
	return len != 0 && len <= al_dma_max_len(op) &&
	       src_cnt >= 1 && src_cnt <= AL_DMA_MAX_SRCS;
}

/* Which op a PQ request runs as, from DMA_PREP_PQ_DISABLE_{P,Q}. -1 when
 * both are disabled (nothing to compute). */
static inline int al_dma_pq_op(bool disable_p, bool disable_q)
{
	if (disable_p && disable_q)
		return -1;
	if (disable_p)
		return AL_RAID_OP_Q_CALC;
	if (disable_q)
		return AL_RAID_OP_P_CALC;
	return AL_RAID_OP_PQ_CALC;
}

/* What a completion status means for the client. */
enum al_dma_result {
	AL_DMA_RES_OK,
	AL_DMA_RES_READ_FAILED,
	AL_DMA_RES_ABORTED,
};

#define AL_DMA_STATUS_FAIL_MASK \
	(AL_RAID_BUS_PARITY_ERROR | AL_RAID_SOURCE_LEN_ERROR | \
	 AL_RAID_CMD_DECODE_ERROR | AL_RAID_INTERNAL_ERROR)

/* P/Q_VAL_ERROR are validate-op mismatches, not failures, and al_dma
 * advertises no validate op. A rejected command or an internal error leaves
 * the destination unusable: ABORTED. */
static inline enum al_dma_result al_dma_status_result(unsigned int status)
{
	if (status & AL_RAID_BUS_PARITY_ERROR)
		return AL_DMA_RES_READ_FAILED;
	if (status & AL_DMA_STATUS_FAIL_MASK)
		return AL_DMA_RES_ABORTED;
	return AL_DMA_RES_OK;
}

#endif /* __AL_DMA_LOGIC_H__ */
