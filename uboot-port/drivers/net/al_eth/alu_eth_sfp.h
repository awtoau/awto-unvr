/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SFP+ module EEPROM (SFF-8472 base ID page, i2c 0x50). #108.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 */
#ifndef ALU_ETH_SFP_H
#define ALU_ETH_SFP_H

#include <linux/types.h>

enum alu_eth_sfp_media {
	ALU_SFP_NONE,		/* EEPROM unreadable: absent module or bus fault */
	ALU_SFP_UNKNOWN,	/* read ok, no code we recognise */
	ALU_SFP_10G_SR,
	ALU_SFP_10G_LR,
	ALU_SFP_10G_LRM,
	ALU_SFP_10G_ER,
	ALU_SFP_DAC_PASSIVE,	/* SFF-8431 passive copper */
	ALU_SFP_DAC_ACTIVE,	/* SFF-8431 active copper */
	ALU_SFP_1000BASE_X,	/* 1G-only optic/copper: cannot link at 10G */
};

struct alu_eth_sfp_info {
	enum alu_eth_sfp_media	media;
	char			vendor[17];	/* bytes 20-35, trimmed */
	char			part[17];	/* bytes 40-55, trimmed */
};

/* Pure decode of the base-ID bytes 0-11; no I/O. */
enum alu_eth_sfp_media alu_eth_sfp_decode(const u8 *id);
const char *alu_eth_sfp_media_name(enum alu_eth_sfp_media m);
bool alu_eth_sfp_is_dac(enum alu_eth_sfp_media m);

/* Reads the module over the DT-referenced SFP bus (never the RTC's).
 * Returns 0 and fills *info; on failure info->media = ALU_SFP_NONE and
 * returns the error. */
int alu_eth_sfp_probe(struct alu_eth_sfp_info *info);

#endif
