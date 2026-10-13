// SPDX-License-Identifier: GPL-2.0-or-later
/* SFP+ module EEPROM read + media decode (#108). SFF-8472 base ID, i2c 0x50.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Bus = port2's "sfp-i2c-bus" phandle -> PCA9546 mux ch1 (never ch0, the RTC).
 * Phandle, not a bus number: numbers shift when a bus is added (#98).
 */

#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <linux/string.h>

#include "alu_eth_sfp.h"

#define SFP_EEPROM_ADDR		0x50
#define SFP_ID_LEN		12	/* bytes 0-11: id, connector, codes, encoding */
#define SFP_ATTEMPTS		2	/* one retry, for a mux-select glitch */
#define SFP_PORT_NODE		"/soc/board-cfg/ethernet/port2"

#define SFP_ID_SFP		0x03
#define SFP_CONN_COPPER_PIGTAIL	0x21

/* Waits for: one 12-byte random read on the mux child bus.
 * Expected: 400 kHz, 9 clocks per byte. Mux select + deselect 2 x 2 bytes,
 * read = addr+W, offset, addr+R, 12 data = 15 bytes: 19 x 9 = 171 clocks
 * = 428 us. Bound is the DW driver's own per-byte limit (CONFIG_SYS_HZ/500
 * = 2 ms, 4.7x), so no extra timer here; SFP_ATTEMPTS caps the total.
 * On expiry: driver returns an error, retry once, then media = NONE and the
 * caller keeps the fixed 10GBASE-R path. */
static int sfp_xfer(u8 addr, int off, u8 *buf, int len, bool write)
{
	struct udevice *bus, *chip;
	ofnode port, bus_node;
	u32 ph;
	int rc = -ENODEV, i;

	port = ofnode_path(SFP_PORT_NODE);
	if (!ofnode_valid(port) || ofnode_read_u32(port, "sfp-i2c-bus", &ph))
		return -ENODEV;
	bus_node = ofnode_get_by_phandle(ph);
	if (!ofnode_valid(bus_node))
		return -ENODEV;
	rc = uclass_get_device_by_ofnode(UCLASS_I2C, bus_node, &bus);
	if (rc)
		return rc;

	for (i = 0; i < SFP_ATTEMPTS; i++) {
		rc = i2c_get_chip(bus, addr, 1, &chip);
		if (!rc)
			rc = write ? dm_i2c_write(chip, off, buf, len)
				   : dm_i2c_read(chip, off, buf, len);
		if (!rc)
			return 0;
	}
	return rc;
}

static int sfp_read_id(u8 *buf, int len, int off)
{
	return sfp_xfer(SFP_EEPROM_ADDR, off, buf, len, false);
}

/* SFF-8472 A2h (0x51): byte 110 status/control, 102-103 measured TX power. */
#define SFP_DDM_ADDR		0x51
#define SFP_A2_STATUS		110
#define SFP_A2_TX_POWER		102
#define SFP_A2_SOFT_TX_DISABLE	BIT(6)

int alu_eth_sfp_tx_enable(struct alu_eth_sfp_tx *st)
{
	u8 v, pw[2];
	int rc;

	memset(st, 0, sizeof(*st));
	rc = sfp_xfer(SFP_DDM_ADDR, SFP_A2_STATUS, &v, 1, false);
	if (rc)
		return rc;
	st->status_before = v;
	if (!sfp_xfer(SFP_DDM_ADDR, SFP_A2_TX_POWER, pw, 2, false))
		st->tx_power_before = (pw[0] << 8) | pw[1];

	/* Linux's sfp.c sets soft TX_DISABLE on driver remove (every shutdown)
	 * and the module keeps it across a warm reboot: laser off, partner sees
	 * no light, our RX-side indicators all stay green. Clear it here, every
	 * start, so no previous stage can leave the transmitter dark. #345 */
	if (v & SFP_A2_SOFT_TX_DISABLE) {
		v &= ~SFP_A2_SOFT_TX_DISABLE;
		rc = sfp_xfer(SFP_DDM_ADDR, SFP_A2_STATUS, &v, 1, true);
		if (rc)
			return rc;
		st->cleared = true;
	}
	if (!sfp_xfer(SFP_DDM_ADDR, SFP_A2_STATUS, &v, 1, false))
		st->status_after = v;
	return 0;
}

enum alu_eth_sfp_media alu_eth_sfp_decode(const u8 *id)
{
	if (id[0] != SFP_ID_SFP)
		return ALU_SFP_UNKNOWN;
	/* Byte 3 = 10G Ethernet compliance, bits 4-7 */
	if (id[3] & 0x10)
		return ALU_SFP_10G_SR;
	if (id[3] & 0x20)
		return ALU_SFP_10G_LR;
	if (id[3] & 0x40)
		return ALU_SFP_10G_LRM;
	if (id[3] & 0x80)
		return ALU_SFP_10G_ER;
	/* Byte 8 = SFF-8431 cable technology: bit 2 passive, bit 3 active */
	if (id[8] & 0x04)
		return ALU_SFP_DAC_PASSIVE;
	if (id[8] & 0x08)
		return ALU_SFP_DAC_ACTIVE;
	if (id[2] == SFP_CONN_COPPER_PIGTAIL)
		return ALU_SFP_DAC_PASSIVE;
	/* Byte 6 = 1G compliance: SX, LX, CX, T */
	if (id[6] & 0x0f)
		return ALU_SFP_1000BASE_X;
	return ALU_SFP_UNKNOWN;
}

const char *alu_eth_sfp_media_name(enum alu_eth_sfp_media m)
{
	static const char * const names[] = {
		[ALU_SFP_NONE] = "none/unreadable",
		[ALU_SFP_UNKNOWN] = "unrecognised",
		[ALU_SFP_10G_SR] = "10GBASE-SR",
		[ALU_SFP_10G_LR] = "10GBASE-LR",
		[ALU_SFP_10G_LRM] = "10GBASE-LRM",
		[ALU_SFP_10G_ER] = "10GBASE-ER",
		[ALU_SFP_DAC_PASSIVE] = "passive DAC",
		[ALU_SFP_DAC_ACTIVE] = "active DAC",
		[ALU_SFP_1000BASE_X] = "1000BASE-X",
	};

	return m < ARRAY_SIZE(names) ? names[m] : "?";
}

bool alu_eth_sfp_is_dac(enum alu_eth_sfp_media m)
{
	return m == ALU_SFP_DAC_PASSIVE || m == ALU_SFP_DAC_ACTIVE;
}

static void sfp_str(char *dst, const u8 *src, int n)
{
	memcpy(dst, src, n);
	dst[n] = 0;
	while (n > 0 && (dst[n - 1] == ' ' || dst[n - 1] == 0))
		dst[--n] = 0;
}

int alu_eth_sfp_probe(struct alu_eth_sfp_info *info)
{
	u8 id[SFP_ID_LEN], name[16], part[16];
	int rc;

	memset(info, 0, sizeof(*info));
	info->media = ALU_SFP_NONE;

	rc = sfp_read_id(id, sizeof(id), 0);
	if (rc)
		return rc;
	info->media = alu_eth_sfp_decode(id);

	/* Vendor/part are cosmetic: a failed read here keeps the media. */
	if (!sfp_read_id(name, sizeof(name), 20))
		sfp_str(info->vendor, name, sizeof(name));
	if (!sfp_read_id(part, sizeof(part), 40))
		sfp_str(info->part, part, sizeof(part));
	return 0;
}
