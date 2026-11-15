// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * Userspace half of the kfpu stress (#364). Pinned to one CPU: load known
 * v0-v31/FPCR/FPSR, write(/dev/kfpu-test) (kernel runs kfpu sections in this
 * task's context), spin, read everything back, compare. Any mismatch = FAIL.
 * Built -mgeneral-regs-only so the compiler never touches v regs; between
 * load and store only raw syscalls and integer code run (no libc).
 *
 * usage: kfpu-check CPU SECONDS   (exit 0 pass, 1 mismatch, 2 setup error)
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static uint64_t mix(uint64_t x)
{
	x += 0x9e3779b97f4a7c15ULL;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

static const uint64_t fpcr_v[] = { 0x0, 0x400000, 0x800000, 0xc00000, 0x1000000 };
static const uint64_t fpsr_v[] = { 0x0, 0x08000011, 0x0800009f };

static inline long raw_write(int fd, const char *b, long n)
{
	register long x0 __asm__("x0") = fd;
	register long x1 __asm__("x1") = (long)b;
	register long x2 __asm__("x2") = n;
	register long x8 __asm__("x8") = 64;

	__asm__ __volatile__("svc 0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8)
	    : "memory", "cc");
	return x0;
}

#define LDP(a, b, o) "ldp q" #a ", q" #b ", [%[p], #" #o "]\n\t"
#define STP(a, b, o) "stp q" #a ", q" #b ", [%[p], #" #o "]\n\t"
#define ALL(M) \
	M(0, 1, 0) M(2, 3, 32) M(4, 5, 64) M(6, 7, 96) M(8, 9, 128) \
	M(10, 11, 160) M(12, 13, 192) M(14, 15, 224) M(16, 17, 256) \
	M(18, 19, 288) M(20, 21, 320) M(22, 23, 352) M(24, 25, 384) \
	M(26, 27, 416) M(28, 29, 448) M(30, 31, 480)

int main(int argc, char **argv)
{
	static uint64_t pat[64] __attribute__((aligned(16)));
	static uint64_t got[64] __attribute__((aligned(16)));
	uint64_t it = 0, bad = 0, exp_cr, exp_sr, got_cr, got_sr, seed, spin;
	int cpu, secs, fd, i, shown = 0;
	time_t end;
	cpu_set_t set;

	if (argc != 3)
		return 2;
	cpu = atoi(argv[1]);
	secs = atoi(argv[2]);
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	if (sched_setaffinity(0, sizeof(set), &set)) {
		perror("sched_setaffinity");
		return 2;
	}
	fd = open("/dev/kfpu-test", O_WRONLY);
	if (fd < 0) {
		perror("/dev/kfpu-test");
		return 2;
	}
	end = time(NULL) + secs;
	seed = (uint64_t)cpu << 56 ^ (uint64_t)getpid();
	while (time(NULL) < end) {
		for (int batch = 0; batch < 2000; batch++, it++) {
			for (i = 0; i < 64; i++)
				pat[i] = mix(seed + it * 64 + i);
			__asm__ __volatile__(
			    ALL(LDP)
			    "msr fpcr, %[ci]\n\t"
			    "msr fpsr, %[si]\n\t"
			    "mrs %[co], fpcr\n\t"
			    "mrs %[so], fpsr\n\t"
			    : [co] "=&r"(exp_cr), [so] "=&r"(exp_sr)
			    : [p] "r"(pat), [ci] "r"(fpcr_v[it % 5]),
			      [si] "r"(fpsr_v[it % 3])
			    : "memory");
			(void)raw_write(fd, "x", 1);
			for (spin = 0; spin < (it & 0x3ff) * 8; spin++)
				__asm__ __volatile__("" : "+r"(spin));
			__asm__ __volatile__(
			    ALL(STP)
			    "mrs %[co], fpcr\n\t"
			    "mrs %[so], fpsr\n\t"
			    : [co] "=&r"(got_cr), [so] "=&r"(got_sr)
			    : [p] "r"(got) : "memory");
			for (i = 0; i < 64; i++) {
				if (got[i] != pat[i]) {
					bad++;
					if (shown++ < 8)
						printf("cpu%d it %llu v%d.%d exp %016llx got %016llx\n",
						    cpu, (unsigned long long)it, i / 2, i & 1,
						    (unsigned long long)pat[i],
						    (unsigned long long)got[i]);
				}
			}
			if (got_cr != exp_cr || got_sr != exp_sr) {
				bad++;
				if (shown++ < 8)
					printf("cpu%d it %llu fpcr exp %llx got %llx fpsr exp %llx got %llx\n",
					    cpu, (unsigned long long)it,
					    (unsigned long long)exp_cr, (unsigned long long)got_cr,
					    (unsigned long long)exp_sr, (unsigned long long)got_sr);
			}
		}
	}
	printf("cpu%d iterations %llu mismatches %llu\n", cpu,
	    (unsigned long long)it, (unsigned long long)bad);
	return bad ? 1 : 0;
}
