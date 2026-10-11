// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* #298 host harness (built by tests/test_al_eth.py).
 *
 * udma_funcs.inc holds al_eth_udma_queues_reset_all(), al_eth_hal_adapter_init()
 * and al_eth_init_rings() cut verbatim from modules/al_eth/al_eth_main.c. The
 * UDMA HAL entry points they call are fakes whose failures are injected, so
 * each case shows the error reaching the caller (and not a NULL dereference
 * on the queue handle the failed call left unset). Case = argv[1].
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
#define __iomem
#define unlikely(x) (x)
#define __must_check
#define AL_ETH_UDMA_TX_QUEUES 4
#define AL_ETH_UDMA_RX_QUEUES 4
#define AL_ETH_REF_CLK_FREQ_TO_HZ(x) (x)
#define AL_ETH_INTR_LOWEST_VALUE(x) (x)
#define AL_ETH_INTR_MODERATION_LOWEST 0
#define AL_UDMA_IOFIC_LEVEL_PRIMARY 0
#define AL_INT_GROUP_B 1
#define AL_INT_GROUP_C 2
#define AL_ETH_RXQ_NAPI_IDX(a, i) (i)
#define pr_err(...) ((void)0)
#define u64_stats_init(s) ((void)(s))
#define netdev_err(n, ...) (err_logs++)

enum al_udma_type { UDMA_TX, UDMA_RX };
union udma_q_regs { int unused; };
struct udma_regs { struct { union udma_q_regs m2s_q[4]; } m2s; struct { union udma_q_regs s2m_q[4]; } s2m; };
struct al_udma { struct udma_regs *udma_regs; };
struct al_udma_q { union udma_q_regs *q_regs; u32 qid; struct al_udma *udma; };
struct al_udma_params { void *udma_regs_base; int type; int num_of_queues; const char *name; };
struct unit_regs { int unused; };
struct napi_struct { int unused; };
struct al_eth_napi { struct napi_struct napi; };
struct u64_stats_sync { int unused; };
struct device { int unused; };
struct pci_dev { struct device dev; };
struct net_device { int unused; };

struct al_hal_eth_adapter {
	const char *name;
	int rev_id, udma_id, enable_rx_parser, serdes_lane;
	void *udma_regs_base, *ec_regs_base, *mac_regs_base, *ec_ints_base, *mac_ints_base;
	struct unit_regs *unit_regs;
	struct al_udma tx_udma, rx_udma;
};
struct al_eth_ring {
	struct device *dev;
	struct net_device *netdev;
	struct napi_struct *napi;
	struct al_udma_q *dma_q;
	int sw_count, hw_count, moderation_table_indx;
	unsigned int smoothed_interval, packets, bytes;
	u32 *unmask_reg_offset;
	u32 unmask_val;
	struct u64_stats_sync syncp;
};
struct al_eth_adapter {
	struct net_device *netdev;
	struct pci_dev *pdev;
	char *name;
	int rev_id, udma_num, serdes_lane, ref_clk_freq;
	void *udma_base, *ec_base, *mac_base;
	int num_tx_queues, num_rx_queues;
	int tx_ring_count, tx_descs_count, rx_ring_count, rx_descs_count;
	struct al_eth_ring tx_ring[4], rx_ring[4];
	struct al_eth_napi al_napi[8];
	struct al_hal_eth_adapter hal_adapter;
};
typedef unsigned int uint;

static int err_logs;
/* ------------------------------------------------------------ HAL fakes */
static int fail_reset_call;	/* 1-based al_udma_q_reset call that fails (0 = none) */
static int reset_calls;
static int fail_handle_call;	/* 1-based al_udma_q_handle_get call that fails */
static int handle_calls;
static int fail_init_call;	/* 1-based al_udma_init call that fails */
static int init_calls;
static struct al_udma_q queues[16];

static int al_udma_q_reset(struct al_udma_q *q)
{
	(void)q;
	return ++reset_calls == fail_reset_call ? -ETIME : 0;
}

static int al_udma_q_handle_get(struct al_udma *udma, uint32_t qid, struct al_udma_q **h)
{
	(void)udma;
	if (++handle_calls == fail_handle_call) {
		*h = NULL;	/* what the real HAL leaves behind on -EINVAL */
		return -EINVAL;
	}
	*h = &queues[handle_calls];
	(void)qid;
	return 0;
}

static int al_udma_init(struct al_udma *udma, struct al_udma_params *p)
{
	(void)p;
	udma->udma_regs = calloc(1, sizeof(struct udma_regs));
	return ++init_calls == fail_init_call ? -EINVAL : 0;
}

static u32 *al_udma_iofic_unmask_offset_get(struct unit_regs *r, int level, int group)
{
	(void)r; (void)level; (void)group;
	return NULL;
}

#include "udma_funcs.inc"

static struct al_eth_adapter ad;
static struct pci_dev pdev;
static struct net_device nd;
static int fails;

static void setup(void)
{
	memset(&ad, 0, sizeof(ad));
	memset(queues, 0, sizeof(queues));
	fail_reset_call = fail_handle_call = fail_init_call = 0;
	reset_calls = handle_calls = init_calls = err_logs = 0;
	ad.num_tx_queues = ad.num_rx_queues = 4;
	ad.pdev = &pdev;
	ad.netdev = &nd;
	for (int i = 0; i < 4; i++) {
		ad.tx_ring[i].dma_q = &queues[8 + i];
		ad.tx_ring[i].dma_q->q_regs = (union udma_q_regs *)&ad;
		ad.rx_ring[i].dma_q = &queues[12 + i];
		ad.rx_ring[i].dma_q->q_regs = (union udma_q_regs *)&ad;
	}
}

#define CHECK(cond, ...)                                              \
	do {                                                          \
		if (!(cond)) {                                        \
			printf("FAIL %s:%d: ", __func__, __LINE__);   \
			printf(__VA_ARGS__);                          \
			printf("\n");                                 \
			fails++;                                      \
		}                                                     \
	} while (0)

static void reset_all_clean(void)
{
	setup();
	CHECK(al_eth_udma_queues_reset_all(&ad) == 0, "clean reset reported an error");
	CHECK(reset_calls == 8, "reset %d queues, want 8", reset_calls);
}

/* the timed-out reset must reach the caller, and the other queues still get reset */
static void reset_all_reports_timeout_and_keeps_going(void)
{
	setup();
	fail_reset_call = 2;
	CHECK(al_eth_udma_queues_reset_all(&ad) == -ETIME, "ETIME swallowed");
	CHECK(reset_calls == 8, "stopped after the failure: %d of 8 reset", reset_calls);
	CHECK(err_logs == 1, "failure logged %d times, want 1", err_logs);
}

static void reset_all_rx_failure(void)
{
	setup();
	fail_reset_call = 6;	/* second rx queue */
	CHECK(al_eth_udma_queues_reset_all(&ad) == -ETIME, "rx ETIME swallowed");
}

static void reset_all_skips_unmapped_queue(void)
{
	setup();
	ad.tx_ring[0].dma_q->q_regs = NULL;
	ad.rx_ring[3].dma_q = NULL;
	CHECK(al_eth_udma_queues_reset_all(&ad) == 0, "unmapped queue is not an error");
	CHECK(reset_calls == 6, "reset %d queues, want 6", reset_calls);
}

/* fail the n-th handle_get: must return the error, not dereference the NULL handle */
static void hal_init_handle_get_failure(void)
{
	for (int n = 1; n <= 8; n++) {
		setup();
		fail_handle_call = n;
		CHECK(al_eth_hal_adapter_init(&ad) == -EINVAL, "handle_get #%d failure lost", n);
	}
}

static void hal_init_udma_init_failure(void)
{
	setup();
	fail_init_call = 2;
	CHECK(al_eth_hal_adapter_init(&ad) == -EINVAL, "rx udma init failure lost");
	CHECK(handle_calls == 0, "went on to fetch queues after udma_init failed");
}

static void hal_init_clean(void)
{
	setup();
	CHECK(al_eth_hal_adapter_init(&ad) == 0, "clean init failed");
	CHECK(handle_calls == 8 && ad.tx_ring[3].dma_q && ad.rx_ring[3].dma_q->udma, "queues not wired");
}

static void init_rings_handle_get_failure(void)
{
	for (int n = 1; n <= 8; n++) {
		setup();
		fail_handle_call = n;
		CHECK(al_eth_init_rings(&ad) == -EINVAL, "handle_get #%d failure lost", n);
		CHECK(handle_calls == n, "kept going after failure #%d (%d calls)", n, handle_calls);
	}
}

static void init_rings_clean(void)
{
	setup();
	CHECK(al_eth_init_rings(&ad) == 0, "clean init_rings failed");
	CHECK(handle_calls == 8, "%d handles fetched", handle_calls);
}

static const struct { const char *name; void (*fn)(void); } cases[] = {
	{ "reset_all_clean", reset_all_clean },
	{ "reset_all_reports_timeout_and_keeps_going", reset_all_reports_timeout_and_keeps_going },
	{ "reset_all_rx_failure", reset_all_rx_failure },
	{ "reset_all_skips_unmapped_queue", reset_all_skips_unmapped_queue },
	{ "hal_init_handle_get_failure", hal_init_handle_get_failure },
	{ "hal_init_udma_init_failure", hal_init_udma_init_failure },
	{ "hal_init_clean", hal_init_clean },
	{ "init_rings_handle_get_failure", init_rings_handle_get_failure },
	{ "init_rings_clean", init_rings_clean },
};

int main(int argc, char **argv)
{
	if (argc < 2)
		return 2;
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
