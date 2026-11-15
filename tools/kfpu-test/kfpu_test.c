// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * Stress for the OpenZFS arm64 kfpu_begin/kfpu_end save/restore (#364).
 * - one kthread per online CPU: begin, clobber v0-v31 + FPCR + FPSR, end
 * - /dev/kfpu-test write(): the same section in the CALLER's context, so a
 *   user task's live NEON state is what gets saved and restored
 * Section code is generated from the openzfs patch (kfpu-test.py), not copied.
 */
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/timex.h>
#include <asm/sysreg.h>

typedef bool boolean_t;
#define	ID_AA64PFR0_EL1		sys_reg(3, 0, 0, 1, 0)
#define	get_ftr(id) {				\
	unsigned long __val;			\
	asm("mrs %0, "#id : "=r" (__val));	\
	__val;					\
}
uint8_t **zfs_kfpu_fpregs;
#include "kfpu_arm64_gen.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("kfpu_begin/end save-restore stress");

static unsigned int max_udelay = 3;
module_param(max_udelay, uint, 0644);
MODULE_PARM_DESC(max_udelay, "random in-section delay upper bound, us");

static bool sabotage;
module_param(sabotage, bool, 0644);
MODULE_PARM_DESC(sabotage, "negative control: clobber the write() caller with no save/restore");

/* Self-stop: a broken section breaks ssh, so rmmod may be unreachable (#365).
 * 300 s = 1.25 x the 240 s kfpu-test.py run; after it, sections become no-ops. */
static unsigned int seconds = 300;
module_param(seconds, uint, 0444);
MODULE_PARM_DESC(seconds, "stop clobbering this many seconds after load");
static unsigned long deadline;

static u64 sections_user;
static struct task_struct **threads;
static unsigned long *counts;

static inline u64 mix(u64 x)
{
	x += 0x9e3779b97f4a7c15ULL;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

/* clobber all of v0-v31; FPCR RMode/FZ and FPSR flags changed too */
static void clobber(u64 seed)
{
	static const u64 fpcr_v[] = { 0x0, 0x400000, 0x800000, 0xc00000, 0x1000000 };
	u64 pat[64] __aligned(16);
	u64 fpcr, fpsr;
	int i;

	for (i = 0; i < 64; i++)
		pat[i] = mix(seed + i);
	fpcr = fpcr_v[seed % ARRAY_SIZE(fpcr_v)];
	fpsr = 0x0800009f;
	__asm__ __volatile__(
	    "ldp q0,  q1,  [%0, #0]\n\t"
	    "ldp q2,  q3,  [%0, #32]\n\t"
	    "ldp q4,  q5,  [%0, #64]\n\t"
	    "ldp q6,  q7,  [%0, #96]\n\t"
	    "ldp q8,  q9,  [%0, #128]\n\t"
	    "ldp q10, q11, [%0, #160]\n\t"
	    "ldp q12, q13, [%0, #192]\n\t"
	    "ldp q14, q15, [%0, #224]\n\t"
	    "ldp q16, q17, [%0, #256]\n\t"
	    "ldp q18, q19, [%0, #288]\n\t"
	    "ldp q20, q21, [%0, #320]\n\t"
	    "ldp q22, q23, [%0, #352]\n\t"
	    "ldp q24, q25, [%0, #384]\n\t"
	    "ldp q26, q27, [%0, #416]\n\t"
	    "ldp q28, q29, [%0, #448]\n\t"
	    "ldp q30, q31, [%0, #480]\n\t"
	    "msr fpcr, %1\n\t"
	    "msr fpsr, %2\n\t"
	    : : "r" (pat), "r" (fpcr), "r" (fpsr) : "memory");
}

/* sabotage hits only the write() caller (the checker): kthread sabotage broke every
 * userspace NEON user on the box, ssh included (#365) */
static void one_section(u64 seed, bool user)
{
	if (!kfpu_allowed() || time_after(jiffies, deadline))
		return;
	if (sabotage && !user)
		return;
	if (sabotage) {
		preempt_disable();
		clobber(seed);
		preempt_enable();
		return;
	}
	kfpu_begin();
	clobber(seed);
	if (max_udelay)
		udelay(seed % (max_udelay + 1));
	if ((seed & 3) == 0) {
		/* nested section (#368): inner end must not restore or unmask */
		kfpu_begin();
		clobber(seed + 1);
		kfpu_end();
		clobber(seed + 2);
	}
	kfpu_end();
}

static int worker(void *arg)
{
	unsigned long cpu = (unsigned long)arg;
	u64 n = (u64)cpu << 48;

	while (!kthread_should_stop()) {
		int i;

		if (time_after(jiffies, deadline)) {
			schedule_timeout_interruptible(HZ / 10);
			continue;
		}
		for (i = 0; i < 64; i++)
			one_section(n++, false);
		counts[cpu] += 64;
		cond_resched();
	}
	return 0;
}

static ssize_t kt_write(struct file *f, const char __user *b, size_t len,
    loff_t *off)
{
	u64 n = get_cycles();
	int i;

	for (i = 0; i < 16; i++)
		one_section(n + i, true);
	WRITE_ONCE(sections_user, READ_ONCE(sections_user) + 16);
	return len;
}

static const struct file_operations kt_fops = {
	.owner = THIS_MODULE,
	.write = kt_write,
};

static struct miscdevice kt_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "kfpu-test",
	.fops = &kt_fops,
	.mode = 0600,
};

static int __init kt_init(void)
{
	unsigned long cpu;
	int err;

	if (!kfpu_allowed()) {
		pr_err("kfpu-test: kfpu_allowed() == 0 (SVE/SME CPU)\n");
		return -ENODEV;
	}
	deadline = jiffies + (unsigned long)seconds * HZ;
	err = kfpu_init();
	if (err)
		return err;
	threads = kcalloc(nr_cpu_ids, sizeof(*threads), GFP_KERNEL);
	counts = kcalloc(nr_cpu_ids, sizeof(*counts), GFP_KERNEL);
	if (!threads || !counts) {
		err = -ENOMEM;
		goto fail;
	}
	err = misc_register(&kt_misc);
	if (err)
		goto fail;
	for_each_online_cpu(cpu) {
		struct task_struct *t = kthread_create(worker, (void *)cpu,
		    "kfpu-test/%lu", cpu);

		if (IS_ERR(t))
			continue;
		kthread_bind(t, cpu);
		threads[cpu] = t;
		wake_up_process(t);
	}
	pr_info("kfpu-test: running on %u cpus for %u s\n", num_online_cpus(),
	    seconds);
	return 0;
fail:
	kfree(threads);
	kfree(counts);
	kfpu_fini();
	return err;
}

static void __exit kt_exit(void)
{
	unsigned long cpu, total = 0;

	misc_deregister(&kt_misc);
	for_each_online_cpu(cpu) {
		if (threads[cpu])
			kthread_stop(threads[cpu]);
		total += counts[cpu];
	}
	pr_info("kfpu-test: kthread sections %lu, user-context sections %llu\n",
	    total, sections_user);
	kfree(threads);
	kfree(counts);
	kfpu_fini();
}

module_init(kt_init);
module_exit(kt_exit);
