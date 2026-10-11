// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* #305 host harness (built by tests/test_al_eth.py).
 *
 * rx_funcs.inc holds al_eth_rx_poll_finish(), al_eth_check_for_empty_rx_ring()
 * and al_eth_rx_watchdog() cut verbatim from modules/al_eth/al_eth_main.c;
 * consts.inc holds the #defines they use, cut from al_eth.h. Everything they
 * call is faked here with just the fields they touch. Case = argv[1].
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
#define unlikely(x) (x)
#define likely(x) (x)
#define __iomem

struct napi_struct {
	int completes;
	int schedules;
};
struct u64_stats_sync { int unused; };
struct net_device { int unused; };
struct al_udma_q { unsigned int avail; };
struct timer_list { int armed; unsigned long expires; };

struct al_eth_stats_rx {
	u64 refill_fail;
	u64 empty_ring;
};
struct al_eth_ring {
	struct al_udma_q *dma_q;
	struct napi_struct *napi;
	u8 empty_rx_queue;
	u32 *unmask_reg_offset;
	u32 unmask_val;
	struct u64_stats_sync syncp;
	struct al_eth_stats_rx rx_stats;
};
struct al_eth_adapter {
	struct net_device *netdev;
	int num_rx_queues;
	struct al_eth_ring rx_ring[4];
	int adaptive_intr_rate;
	bool up;
	struct timer_list rx_watchdog;
};
enum { AL_ETH_RX = 1 };
#define HZ 250

static unsigned long jiffies;
static int refill_shortfall;	/* buffers the fake allocator fails to provide */
static int refill_calls, moderation_calls, unmask_writes, err_logs;

#define u64_stats_update_begin(s) ((void)(s))
#define u64_stats_update_end(s) ((void)(s))
#define netdev_err(dev, ...) (err_logs++)
#define round_jiffies(x) (x)
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#define timer_container_of(var, t, member) \
	container_of(t, __typeof__(*(var)), member)

static unsigned int al_udma_available_get(struct al_udma_q *q) { return q->avail; }

static int al_eth_refill_rx_bufs(struct al_eth_adapter *a, unsigned int qid,
				 unsigned int num)
{
	int got = (int)num - refill_shortfall;

	(void)a; (void)qid;
	refill_calls++;
	return got < 0 ? 0 : got;
}

static void al_eth_update_intr_moderation(struct al_eth_adapter *a,
					  unsigned int qid, int dir)
{
	(void)a; (void)qid; (void)dir;
	moderation_calls++;
}

static void al_reg_write32_relaxed(u32 *reg, u32 val)
{
	*reg = val;
	unmask_writes++;
}

static void napi_complete(struct napi_struct *n) { n->completes++; }
static void napi_schedule(struct napi_struct *n) { n->schedules++; }
static void mod_timer(struct timer_list *t, unsigned long exp)
{
	t->armed++;
	t->expires = exp;
}

#include "consts.inc"
#include "rx_funcs.inc"

static struct al_eth_adapter ad;
static struct napi_struct napi[4];
static struct al_udma_q dq[4];
static u32 unmask_reg[4];

static void setup(int nq)
{
	memset(&ad, 0, sizeof(ad));
	memset(napi, 0, sizeof(napi));
	memset(unmask_reg, 0, sizeof(unmask_reg));
	refill_shortfall = refill_calls = moderation_calls = unmask_writes = err_logs = 0;
	ad.num_rx_queues = nq;
	for (int i = 0; i < nq; i++) {
		ad.rx_ring[i].dma_q = &dq[i];
		ad.rx_ring[i].napi = &napi[i];
		ad.rx_ring[i].unmask_reg_offset = &unmask_reg[i];
		ad.rx_ring[i].unmask_val = 0xA0 + i;
		dq[i].avail = 0;
	}
}

static int fails;
#define CHECK(cond, ...)                                              \
	do {                                                          \
		if (!(cond)) {                                        \
			printf("FAIL %s:%d: ", __func__, __LINE__);   \
			printf(__VA_ARGS__);                          \
			printf("\n");                                 \
			fails++;                                      \
		}                                                     \
	} while (0)

/* refill short, poll under budget: must still leave polling mode + unmask */
static void short_refill_completes(void)
{
	setup(1);
	dq[0].avail = 10;
	refill_shortfall = 4;
	al_eth_rx_poll_finish(&ad, &ad.rx_ring[0], &napi[0], 0, 5);
	CHECK(napi[0].completes == 1, "napi_complete x%d, want 1", napi[0].completes);
	CHECK(napi[0].schedules == 0, "napi_schedule from inside poll x%d", napi[0].schedules);
	CHECK(unmask_writes == 1 && unmask_reg[0] == 0xA0, "irq unmask not written");
	CHECK(ad.rx_ring[0].rx_stats.refill_fail == 1, "refill_fail=%llu",
	      (unsigned long long)ad.rx_ring[0].rx_stats.refill_fail);
}

/* nothing could be refilled at all: same, the watchdog owns recovery */
static void zero_refill_completes(void)
{
	setup(1);
	dq[0].avail = AL_ETH_DEFAULT_RX_DESCS - 1;
	refill_shortfall = 100000;
	al_eth_rx_poll_finish(&ad, &ad.rx_ring[0], &napi[0], 0, 64);
	CHECK(napi[0].completes == 1 && unmask_writes == 1, "completes=%d unmask=%d",
	      napi[0].completes, unmask_writes);
}

static void full_refill_completes(void)
{
	setup(1);
	dq[0].avail = 10;
	al_eth_rx_poll_finish(&ad, &ad.rx_ring[0], &napi[0], 0, 1);
	CHECK(napi[0].completes == 1 && unmask_writes == 1, "completes=%d unmask=%d",
	      napi[0].completes, unmask_writes);
	CHECK(ad.rx_ring[0].rx_stats.refill_fail == 0, "spurious refill_fail");
}

/* budget used up: NAPI keeps polling, so no complete and no unmask */
static void budget_exhausted_stays_scheduled(void)
{
	setup(1);
	dq[0].avail = 10;
	al_eth_rx_poll_finish(&ad, &ad.rx_ring[0], &napi[0], 0, 0);
	CHECK(napi[0].completes == 0 && unmask_writes == 0, "completes=%d unmask=%d",
	      napi[0].completes, unmask_writes);
	CHECK(refill_calls == 1, "refill still runs when budget is exhausted");
}

static void moderation_follows_the_flag(void)
{
	setup(1);
	al_eth_rx_poll_finish(&ad, &ad.rx_ring[0], &napi[0], 0, 3);
	CHECK(moderation_calls == 0, "moderation ran while disabled");
	ad.adaptive_intr_rate = 1;
	al_eth_rx_poll_finish(&ad, &ad.rx_ring[0], &napi[0], 0, 3);
	CHECK(moderation_calls == 1, "moderation calls=%d", moderation_calls);
}

static void watchdog_kicks_after_threshold(void)
{
	setup(1);
	dq[0].avail = AL_ETH_DEFAULT_RX_DESCS - 1;
	for (int i = 0; i < AL_ETH_EMPTY_RX_REFILL - 1; i++)
		al_eth_check_for_empty_rx_ring(&ad);
	CHECK(napi[0].schedules == 0, "kicked after only %d looks", AL_ETH_EMPTY_RX_REFILL - 1);
	al_eth_check_for_empty_rx_ring(&ad);
	CHECK(napi[0].schedules == 1, "schedules=%d, want 1", napi[0].schedules);
	CHECK(ad.rx_ring[0].rx_stats.empty_ring == 1, "empty_ring stat not counted");
	CHECK(ad.rx_ring[0].empty_rx_queue == 0, "counter not reset after kick");
	CHECK(err_logs == 1, "kick not logged");
}

static void watchdog_nonempty_resets_count(void)
{
	setup(1);
	dq[0].avail = AL_ETH_DEFAULT_RX_DESCS - 1;
	al_eth_check_for_empty_rx_ring(&ad);
	al_eth_check_for_empty_rx_ring(&ad);
	dq[0].avail = AL_ETH_DEFAULT_RX_DESCS - 2;	/* one buffer posted */
	al_eth_check_for_empty_rx_ring(&ad);
	dq[0].avail = AL_ETH_DEFAULT_RX_DESCS - 1;
	al_eth_check_for_empty_rx_ring(&ad);
	al_eth_check_for_empty_rx_ring(&ad);
	CHECK(napi[0].schedules == 0, "kicked on non-consecutive empties");
}

static void watchdog_full_ring_never_kicks(void)
{
	setup(1);
	dq[0].avail = 0;	/* every descriptor posted to the MAC */
	for (int i = 0; i < 20; i++)
		al_eth_check_for_empty_rx_ring(&ad);
	CHECK(napi[0].schedules == 0, "kicked a healthy ring");
}

static void watchdog_queues_independent(void)
{
	setup(2);
	dq[1].avail = AL_ETH_DEFAULT_RX_DESCS - 1;
	for (int i = 0; i < AL_ETH_EMPTY_RX_REFILL; i++)
		al_eth_check_for_empty_rx_ring(&ad);
	CHECK(napi[0].schedules == 0 && napi[1].schedules == 1, "q0=%d q1=%d",
	      napi[0].schedules, napi[1].schedules);
}

static void watchdog_rearms_only_while_up(void)
{
	setup(1);
	jiffies = 1000;
	ad.up = true;
	al_eth_rx_watchdog(&ad.rx_watchdog);
	CHECK(ad.rx_watchdog.armed == 1, "not re-armed while up");
	CHECK(ad.rx_watchdog.expires == 1000 + AL_ETH_RX_WATCHDOG_PERIOD, "expires=%lu",
	      ad.rx_watchdog.expires);
	ad.up = false;
	dq[0].avail = AL_ETH_DEFAULT_RX_DESCS - 1;
	ad.rx_ring[0].empty_rx_queue = AL_ETH_EMPTY_RX_REFILL - 1;
	al_eth_rx_watchdog(&ad.rx_watchdog);
	CHECK(ad.rx_watchdog.armed == 1, "re-armed after down");
	CHECK(napi[0].schedules == 0, "kicked NAPI after down");
}

static const struct { const char *name; void (*fn)(void); } cases[] = {
	{ "short_refill_completes", short_refill_completes },
	{ "zero_refill_completes", zero_refill_completes },
	{ "full_refill_completes", full_refill_completes },
	{ "budget_exhausted_stays_scheduled", budget_exhausted_stays_scheduled },
	{ "moderation_follows_the_flag", moderation_follows_the_flag },
	{ "watchdog_kicks_after_threshold", watchdog_kicks_after_threshold },
	{ "watchdog_nonempty_resets_count", watchdog_nonempty_resets_count },
	{ "watchdog_full_ring_never_kicks", watchdog_full_ring_never_kicks },
	{ "watchdog_queues_independent", watchdog_queues_independent },
	{ "watchdog_rearms_only_while_up", watchdog_rearms_only_while_up },
};

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <case>\n", argv[0]);
		return 2;
	}
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		if (!strcmp(argv[1], cases[i].name)) {
			cases[i].fn();
			if (!fails)
				printf("PASS %s\n", cases[i].name);
			return fails ? 1 : 0;
		}
	}
	fprintf(stderr, "unknown case %s\n", argv[1]);
	return 2;
}
