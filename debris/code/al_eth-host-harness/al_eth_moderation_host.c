// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* #304: evaluates the REAL default-moderation paragraph cut from
 * al_eth_main.c (tests/test_al_eth.py). Prints the resulting variable. */
#include <stdio.h>

#define AL_TRUE 1
#define AL_FALSE 0
#define module_param(a, b, c)
#define MODULE_PARM_DESC(a, b)

#include "moderation.inc"

int main(void)
{
	printf("%d\n", adaptive_int_moderation);
	return 0;
}
