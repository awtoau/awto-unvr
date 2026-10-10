// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* Host harness for al_nand_data_buff_{read,write}() CW accounting (#208).
 * Built by tests/test_al_nand_hal.py against the real al_hal_nand.c with
 * host stand-ins for the plat headers. Prints PASS/FAIL lines; exit 1 on FAIL. */
#include <stdio.h>
#include <string.h>
#include "al_hal_nand.h"
#include "al_hal_nand_regs.h"

static int fails;

#define CHECK(c, what)                                                     \
	do {                                                               \
		if (c) {                                                   \
			printf("PASS %s\n", what);                         \
		} else {                                                   \
			printf("FAIL %s\n", what);                         \
			fails++;                                           \
		}                                                          \
	} while (0)

/* fake_fifo_writes counts al_reg_write32() hits on the data buffer address */
extern unsigned int host_fifo_writes;
extern unsigned int host_fifo_reads;
extern void *host_fifo_addr;

int main(void)
{
	static struct al_nand_regs regs;
	static struct al_nand_wrap_regs wrap;
	static uint32_t fifo;
	struct al_nand_ctrl_obj obj;
	uint8_t page[1024];
	int i, ret;

	memset(&obj, 0, sizeof(obj));
	obj.regs_base = &regs;
	obj.wrap_regs_base = &wrap;
	obj.data_buff_base = &fifo;
	host_fifo_addr = &fifo;
	for (i = 0; i < (int)sizeof(page); i++)
		page[i] = (uint8_t)i;

	/* 2 codewords of 512: two in-budget writes succeed, a third is refused. */
	al_nand_cw_config(&obj, 512, 2);
	ret = al_nand_data_buff_write(&obj, 512, page);
	CHECK(ret == 0, "write 1st cw ok");
	CHECK(obj.cw_count_remaining == 1 && obj.cw_size_remaining == 0,
	      "write stores accounting back (1 cw left)");
	CHECK(host_fifo_writes == 128, "1st cw = 128 words to the FIFO");
	ret = al_nand_data_buff_write(&obj, 512, page + 512);
	CHECK(ret == 0, "write 2nd cw ok");
	CHECK(obj.cw_count_remaining == 0, "write stores accounting back (0 left)");

	host_fifo_writes = 0;
	ret = al_nand_data_buff_write(&obj, 512, page);
	CHECK(ret != 0, "write past the configured cw_count is refused");
	CHECK(host_fifo_writes == 0, "refused write touches no register");

	/* Same property on read, which already stored the accounting. */
	al_nand_cw_config(&obj, 512, 1);
	ret = al_nand_data_buff_read(&obj, 512, 0, 0, page);
	CHECK(ret == 0, "read 1 cw ok");
	ret = al_nand_data_buff_read(&obj, 512, 0, 0, page);
	CHECK(ret != 0, "read past the configured cw_count is refused");

	/* cw_config() re-arms the accounting. */
	al_nand_cw_config(&obj, 512, 1);
	ret = al_nand_data_buff_write(&obj, 512, page);
	CHECK(ret == 0, "cw_config re-arms the write budget");

	return fails ? 1 : 0;
}
