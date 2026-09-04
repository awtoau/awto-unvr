// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * PSCI CPU_ON test command for AL-324 (#140).
 *
 * Tests whether xHCI Enable-Slot needs cluster1 powered (CCU slave4 snoop
 * routing hypothesis). Secondary writes a signature + MPIDR to DRAM then
 * parks in WFI; it never re-enters U-Boot.
 */

#include <config.h>
#include <command.h>
#include <asm/io.h>
#include <linux/arm-smccc.h>
#include <linux/delay.h>
#include <linux/kernel.h>	/* ARRAY_SIZE */
#include <vsprintf.h>		/* hextoul */

/* PSCI 0.2 SMC64 function IDs (ARM DEN 0022; live.dts psci method="smc"). */
#define PSCI_FN_PSCI_VERSION	0x84000000UL
#define PSCI_FN_CPU_ON		0xC4000003UL
#define PSCI_FN_AFFINITY_INFO	0xC4000004UL

/*
 * Signature mailbox: DRAM bank0, clear of U-Boot TEXT (0x01100000), its stack
 * (0x05000000) and the boot canary word (0x10000000).
 */
#define AL_CPUON_MBOX		0x10100000UL
#define AL_CPUON_MAGIC		0xC0DEDEADUL

/*
 * Poll for the secondary's signature. PSCI CPU_ON returns as soon as the
 * request is accepted, so the core's actual arrival is asynchronous.
 * Expected: an A57 power-up + reset-vector fetch is microseconds (the SoC's
 * own PSCI implementation in al_boot does a CPU-power-domain sequence, worst
 * case tens of us). 5000 us is ~100x that - deliberately loose here only
 * because no vendor figure exists for this SoC's power-up latency, so the
 * multiplier is guessed rather than derived. On expiry: report NOT arrived
 * with the elapsed time; do not retry, do not hang.
 */
#define AL_CPUON_ARRIVE_US	5000
#define AL_CPUON_POLL_STEP_US	50

static const char *psci_err(long r)
{
	switch (r) {
	case 0:		return "SUCCESS";
	case -1:	return "NOT_SUPPORTED";
	case -2:	return "INVALID_PARAMS";
	case -3:	return "DENIED";
	case -4:	return "ALREADY_ON";
	case -5:	return "ON_PENDING";
	case -6:	return "INTERNAL_FAILURE";
	case -7:	return "NOT_PRESENT";
	case -8:	return "DISABLED";
	case -9:	return "INVALID_ADDRESS";
	default:	return "?";
	}
}

/*
 * Secondary entry stub. Position-independent, MMU/caches OFF (PSCI hands the
 * core over that way), touches nothing U-Boot owns. x0 = context_id = mailbox.
 *   [x0]    magic   [x0+8] MPIDR_EL1   [x0+16] CurrentEL
 */
void al_cpuon_secondary_entry(void);
asm(
"	.pushsection .text.al_cpuon_secondary, \"ax\"\n"
"	.global al_cpuon_secondary_entry\n"
"	.type al_cpuon_secondary_entry, %function\n"
"al_cpuon_secondary_entry:\n"
"	mov	x9, #0xDEAD\n"
"	movk	x9, #0xC0DE, lsl #16\n"
"	str	x9, [x0]\n"
"	mrs	x10, mpidr_el1\n"
"	str	x10, [x0, #8]\n"
"	mrs	x11, CurrentEL\n"
"	str	x11, [x0, #16]\n"
"	dsb	sy\n"
"1:	wfi\n"
"	b	1b\n"
"	.size al_cpuon_secondary_entry, . - al_cpuon_secondary_entry\n"
"	.popsection\n"
);

static long psci_call(unsigned long fn, unsigned long a1, unsigned long a2,
		      unsigned long a3)
{
	struct arm_smccc_res res;

	arm_smccc_smc(fn, a1, a2, a3, 0, 0, 0, 0, &res);
	return (long)res.a0;
}

/*
 * Both plausible MPIDR encodings. The DTs (ours and vendor's live.dts) give
 * cpu reg 0..3 (flat aff0), but the CCU exposes slaves[3]=cluster0 and
 * slaves[4]=cluster1, i.e. a 2x2 part whose real MPIDRs would be
 * 0x0/0x1/0x100/0x101. Which one PSCI accepts is the thing to find out, so
 * probe both rather than assume.
 */
static const unsigned long al_cpuon_mpidrs[] = {
	0x0, 0x1, 0x2, 0x3, 0x100, 0x101,
};

static void al_cpuon_show_affinity(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(al_cpuon_mpidrs); i++) {
		unsigned long m = al_cpuon_mpidrs[i];
		long r = psci_call(PSCI_FN_AFFINITY_INFO, m, 0, 0);

		printf("cpuon: AFFINITY_INFO(mpidr=0x%lx) = %ld (%s)\n", m, r,
		       r == 0 ? "ON" : r == 1 ? "OFF" : r == 2 ? "ON_PENDING" :
		       psci_err(r));
	}
}

static int do_cpuon(struct cmd_tbl *cmdtp, int flag, int argc,
		    char *const argv[])
{
	unsigned long mpidr, entry, elapsed, self;
	volatile u64 *mbox = (volatile u64 *)AL_CPUON_MBOX;
	long ver, r;

	__asm__ volatile("mrs %0, mpidr_el1" : "=r"(self));
	printf("cpuon: boot core MPIDR_EL1 = 0x%lx (aff1/cluster %lu, aff0/core %lu)\n",
	       self, (self >> 8) & 0xff, self & 0xff);

	ver = psci_call(PSCI_FN_PSCI_VERSION, 0, 0, 0);
	printf("cpuon: PSCI_VERSION = 0x%lx (major %ld, minor %ld)\n",
	       ver, (ver >> 16) & 0x7fff, ver & 0xffff);

	al_cpuon_show_affinity();

	if (argc < 2) {
		printf("cpuon: no target given - affinity dump only\n");
		return CMD_RET_SUCCESS;
	}

	mpidr = hextoul(argv[1], NULL);
	/*
	 * Optional entry override: U-Boot relocates itself to the top of DRAM,
	 * so the built-in stub lands ~0xbff2xxxx. If the SoC's PSCI cannot
	 * dispatch a core to an arbitrary high address (e.g. it only honours a
	 * low reset vector, or a secure-world address filter), pointing it at a
	 * hand-placed low-DRAM copy distinguishes that from "the core never
	 * ran at all".
	 */
	entry = (argc > 2) ? hextoul(argv[2], NULL)
			   : (unsigned long)&al_cpuon_secondary_entry;

	/* Clear the mailbox so a stale signature can't read as success. */
	mbox[0] = 0;
	mbox[1] = 0;
	mbox[2] = 0;
	__asm__ volatile("dsb sy" ::: "memory");

	printf("cpuon: CPU_ON(mpidr=0x%lx, entry=0x%lx, ctx=0x%lx)\n",
	       mpidr, entry, (unsigned long)AL_CPUON_MBOX);

	r = psci_call(PSCI_FN_CPU_ON, mpidr, entry, AL_CPUON_MBOX);
	printf("cpuon: CPU_ON returned %ld (%s)\n", r, psci_err(r));

	/* See AL_CPUON_ARRIVE_US above for what this waits for and why. */
	for (elapsed = 0; elapsed < AL_CPUON_ARRIVE_US;
	     elapsed += AL_CPUON_POLL_STEP_US) {
		__asm__ volatile("dsb sy" ::: "memory");
		if (mbox[0] == AL_CPUON_MAGIC)
			break;
		udelay(AL_CPUON_POLL_STEP_US);
	}

	if (mbox[0] == AL_CPUON_MAGIC) {
		printf("cpuon: SECONDARY RAN - magic 0x%llx after %lu us\n",
		       (unsigned long long)mbox[0], elapsed);
		printf("cpuon:   MPIDR_EL1 = 0x%llx (aff1/cluster %llu, aff0/core %llu)\n",
		       (unsigned long long)mbox[1],
		       (unsigned long long)((mbox[1] >> 8) & 0xff),
		       (unsigned long long)(mbox[1] & 0xff));
		printf("cpuon:   CurrentEL = 0x%llx (EL%llu)\n",
		       (unsigned long long)mbox[2],
		       (unsigned long long)((mbox[2] >> 2) & 3));
	} else {
		printf("cpuon: secondary did NOT arrive within %u us (mbox[0]=0x%llx)\n",
		       AL_CPUON_ARRIVE_US, (unsigned long long)mbox[0]);
	}

	al_cpuon_show_affinity();
	return r == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

U_BOOT_CMD(cpuon, 3, 0, do_cpuon,
	   "PSCI CPU_ON a secondary core, prove it ran (#140 cluster test)",
	   "[<mpidr> [<entry>]]\n"
	   "    - no arg: PSCI version + AFFINITY_INFO for cpu 0-3, 0x100/0x101.\n"
	   "    - <mpidr>: CPU_ON that core onto a stub that writes a signature\n"
	   "      to 0x10100000 then parks in WFI. Reports the PSCI return code\n"
	   "      and whether the core actually executed.\n"
	   "    - <entry>: override the entry address (the built-in stub sits in\n"
	   "      relocated U-Boot near the top of DRAM; use this to aim PSCI at\n"
	   "      a hand-placed low-DRAM copy).");
