// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* #296 harness (built by tests/test_al_eth.py): TX drop accounting.
 *
 * xmit_funcs.inc holds al_eth_tx_drop(), al_eth_ring_drops() and the REAL
 * al_eth_start_xmit() cut from modules/al_eth/al_eth_main.c. DMA mapping,
 * eth_skb_pad(), the checksum setup and the HAL are fakes with injectable
 * failures; mappings are tracked so a drop path that unmaps too little or
 * twice shows up. Case = argv[1].
 */
#include <errno.h>
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
typedef uint64_t dma_addr_t;
typedef int netdev_tx_t;
#define NETDEV_TX_OK 0
#define NETDEV_TX_BUSY 16
#define unlikely(x) (x)
#define likely(x) (x)
#define DMA_TO_DEVICE 1
#define MAX_SKB_FRAGS 5
#define CONFIG_MAX_SKB_FRAGS 5
#define AL_ETH_PKT_MAX_BUFS 8
#define AL_ETH_TX_FLAGS_INT 1
#define AL_ETH_DEFAULT_TX_SW_DESCS 16
#define AL_ETH_NUM_QUEUES 1
#define AL_ETH_TX_RING_IDX_NEXT(r, i) (((i) + 1) & (AL_ETH_DEFAULT_TX_SW_DESCS - 1))
#define BUILD_BUG_ON_MSG(c, m) _Static_assert(!(c), m)
#define WARN_ONCE(c, ...) ((void)(c))
#define WARN_ON_ONCE(c) ((void)(c))
#define netdev_dbg(...) ((void)0)
#define dev_dbg(...) ((void)0)
#define net_warn_ratelimited(...) ((void)0)
#define netdev_name(n) "fake"
#define smp_wmb() ((void)0)
#define u64_stats_update_begin(s) ((void)(s))
#define u64_stats_update_end(s) ((void)(s))
#define u64_stats_fetch_begin(s) 0
#define u64_stats_fetch_retry(s, x) ((void)(x), 0)
#define dma_unmap_addr_set(p, n, v) ((p)->n = (v))
#define dma_unmap_len_set(p, n, v) ((p)->n = (v))
#define dma_unmap_addr(p, n) ((p)->n)
#define dma_unmap_len(p, n) ((p)->n)

struct u64_stats_sync { int unused; };
struct device { int unused; };
struct pci_dev { struct device dev; };
struct net_device { char name[8]; };
struct netdev_queue { int stopped; };
struct al_udma_q { int avail; };
struct al_buf { dma_addr_t addr; u32 len; };
typedef struct { u32 size; } skb_frag_t;
struct skb_shared_info { unsigned int nr_frags; skb_frag_t frags[16]; };
struct sk_buff { u32 len; u32 headlen; void *data; struct skb_shared_info shinfo; int freed; };
struct al_eth_pkt {
	u32 flags;
	int num_of_bufs;
	struct al_buf bufs[AL_ETH_PKT_MAX_BUFS];
};
struct al_eth_tx_buffer {
	struct sk_buff *skb;
	struct al_eth_pkt hal_pkt;
	unsigned int tx_descs;
};
struct al_eth_stats_tx {
	u64 packets, bytes, dropped, pad_err, dma_mapping_err, csum_err, frag_err;
};
struct al_eth_stats_rx { u64 packets, bytes, skb_alloc_fail, dropped; };
struct al_eth_ring {
	struct device *dev;
	struct al_udma_q *dma_q;
	u16 next_to_use;
	struct al_eth_tx_buffer *tx_buffer_info;
	struct u64_stats_sync syncp;
	union {
		struct al_eth_stats_tx tx_stats;
		struct al_eth_stats_rx rx_stats;
	};
};
struct al_eth_adapter {
	struct net_device *netdev;
	struct pci_dev *pdev;
	int num_tx_queues, num_rx_queues;
	struct al_eth_ring tx_ring[2], rx_ring[2];
};

#define skb_shinfo(skb) (&(skb)->shinfo)
#define skb_headlen(skb) ((skb)->headlen)
#define skb_frag_size(f) ((f)->size)
#define netdev_priv(dev) (&g_adapter)
#define skb_get_queue_mapping(skb) 0
#define netdev_get_tx_queue(dev, q) (&g_txq)
#define netif_tx_stop_queue(q) ((q)->stopped = 1)
#define netdev_tx_sent_queue(q, n) ((void)(n))

static struct al_eth_adapter g_adapter;
static struct netdev_queue g_txq;

/* ------------------------------------------------------- injectable fakes */
static int fail_pad, fail_csum, fail_map_at, maps, live_maps, unmaps, frees, double_unmap;
static int map_calls, dma_tx_actions;

static int eth_skb_pad(struct sk_buff *skb)
{
	if (fail_pad) {
		skb->freed = 1;
		frees++;
		return -ENOMEM;
	}
	return 0;
}
static int al_eth_tx_csum(struct al_eth_ring *r, struct al_eth_tx_buffer *t,
			  struct al_eth_pkt *p, struct sk_buff *skb)
{
	(void)r; (void)t; (void)p; (void)skb;
	return fail_csum ? -EOPNOTSUPP : 0;
}
static unsigned int al_udma_available_get(struct al_udma_q *q) { return q->avail; }
static int map_one(void)
{
	map_calls++;
	if (map_calls == fail_map_at)
		return -1;
	maps++;
	live_maps++;
	return 0;
}
static dma_addr_t dma_map_single(struct device *d, void *p, u32 len, int dir)
{
	(void)d; (void)p; (void)len; (void)dir;
	return map_one() ? 0 : 0x1000 + 0x10 * map_calls;
}
static dma_addr_t skb_frag_dma_map(struct device *d, const skb_frag_t *f, int off, u32 len, int dir)
{
	(void)d; (void)f; (void)off; (void)len; (void)dir;
	return map_one() ? 0 : 0x1000 + 0x10 * map_calls;
}
static int dma_mapping_error(struct device *d, dma_addr_t a) { (void)d; return a == 0; }
static void dma_unmap_single(struct device *d, dma_addr_t a, u32 len, int dir)
{
	(void)d; (void)len; (void)dir;
	if (!a || --live_maps < 0)
		double_unmap++;
	unmaps++;
}
static void dma_unmap_page(struct device *d, dma_addr_t a, u32 len, int dir)
{
	dma_unmap_single(d, a, len, dir);
}
static void dev_kfree_skb(struct sk_buff *skb)
{
	if (skb->freed)
		double_unmap += 100;	/* double free */
	skb->freed = 1;
	frees++;
}
static unsigned int al_eth_tx_pkt_prepare(struct al_udma_q *q, struct al_eth_pkt *p)
{
	(void)q;
	return p->num_of_bufs + 1;
}
static void al_eth_tx_dma_action(struct al_udma_q *q, unsigned int n) { (void)q; (void)n; dma_tx_actions++; }

#include "xmit_funcs.inc"

/* ------------------------------------------------------------------ cases */
static struct pci_dev pdev;
static struct net_device nd;
static struct al_udma_q dq;
static struct al_eth_tx_buffer tb[AL_ETH_DEFAULT_TX_SW_DESCS];
static struct sk_buff skb;
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

static struct al_eth_ring *ring0(void) { return &g_adapter.tx_ring[0]; }

static void setup(unsigned int nr_frags)
{
	memset(&g_adapter, 0, sizeof(g_adapter));
	memset(tb, 0, sizeof(tb));
	memset(&skb, 0, sizeof(skb));
	fail_pad = fail_csum = fail_map_at = 0;
	maps = live_maps = unmaps = frees = double_unmap = map_calls = dma_tx_actions = 0;
	g_txq.stopped = 0;
	dq.avail = 100;
	g_adapter.netdev = &nd;
	g_adapter.pdev = &pdev;
	g_adapter.num_tx_queues = g_adapter.num_rx_queues = 1;
	ring0()->dev = &pdev.dev;
	ring0()->dma_q = &dq;
	ring0()->tx_buffer_info = tb;
	skb.len = 200;
	skb.headlen = 100;
	skb.shinfo.nr_frags = nr_frags;
	for (unsigned int i = 0; i < nr_frags; i++)
		skb.shinfo.frags[i].size = 50;
}

static void check_dropped(const char *what, u64 *reason)
{
	struct al_eth_stats_tx *t = &ring0()->tx_stats;

	CHECK(t->dropped == 1, "%s: dropped=%llu", what, (unsigned long long)t->dropped);
	CHECK(*reason == 1, "%s: reason counter=%llu", what, (unsigned long long)*reason);
	CHECK(t->packets == 0, "%s: counted as sent", what);
	CHECK(skb.freed && frees == 1, "%s: skb frees=%d", what, frees);
	CHECK(live_maps == 0 && double_unmap == 0, "%s: live_maps=%d double_unmap=%d", what, live_maps, double_unmap);
	CHECK(dma_tx_actions == 0, "%s: reached the doorbell", what);
}

static void sends_ok(void)
{
	setup(2);
	CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_OK, "rc");
	CHECK(ring0()->tx_stats.packets == 1 && ring0()->tx_stats.dropped == 0, "sent counters");
	CHECK(ring0()->next_to_use == 1 && tb[0].skb == &skb, "ring not advanced");
	CHECK(dma_tx_actions == 1 && maps == 3, "maps=%d", maps);
}

static void pad_failure_counted(void)
{
	setup(0);
	fail_pad = 1;
	CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_OK, "rc");
	CHECK(ring0()->tx_stats.dropped == 1 && ring0()->tx_stats.pad_err == 1, "dropped=%llu pad_err=%llu",
	      (unsigned long long)ring0()->tx_stats.dropped, (unsigned long long)ring0()->tx_stats.pad_err);
	CHECK(maps == 0, "mapped after pad failure");
}

static void csum_failure_counted(void)
{
	setup(1);
	fail_csum = 1;
	CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_OK, "rc");
	check_dropped("csum", &ring0()->tx_stats.csum_err);
	CHECK(tb[0].skb == NULL, "slot keeps the freed skb");
	CHECK(maps == 0, "mapped after csum failure");
}

/* head mapping fails: counted, and the ring slot must not keep the freed skb
 * (al_eth_free_tx_bufs() would free + unmap it again at ifdown) */
static void head_map_failure_counted_and_slot_cleared(void)
{
	setup(2);
	fail_map_at = 1;
	CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_OK, "rc");
	check_dropped("head map", &ring0()->tx_stats.dma_mapping_err);
	CHECK(tb[0].skb == NULL, "slot keeps the freed skb (double free at ifdown)");
}

static void frag_map_failure_unwinds_and_counts(void)
{
	for (int at = 2; at <= 4; at++) {
		setup(3);
		fail_map_at = at;
		CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_OK, "rc");
		check_dropped("frag map", &ring0()->tx_stats.dma_mapping_err);
		CHECK(tb[0].skb == NULL, "slot keeps the skb");
		CHECK(unmaps == at - 1, "mapping #%d failed: unmapped %d, want %d", at, unmaps, at - 1);
	}
}

static void frag_overflow_counted_as_frag_err(void)
{
	setup(AL_ETH_PKT_MAX_BUFS - 1);	/* > AL_ETH_PKT_MAX_BUFS - 2 */
	CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_OK, "rc");
	check_dropped("frag overflow", &ring0()->tx_stats.frag_err);
	CHECK(ring0()->tx_stats.dma_mapping_err == 0, "counted as a mapping error too");
	CHECK(maps == AL_ETH_PKT_MAX_BUFS && unmaps == maps, "maps=%d unmaps=%d", maps, unmaps);
}

static void ring_full_is_busy_not_a_drop(void)
{
	setup(1);
	dq.avail = 1;
	CHECK(al_eth_start_xmit(&skb, &nd) == NETDEV_TX_BUSY, "rc");
	CHECK(ring0()->tx_stats.dropped == 0 && !skb.freed, "BUSY skb was dropped or freed");
}

static void ring_drops_sums_every_queue(void)
{
	u64 rx, tx;

	setup(0);
	g_adapter.num_rx_queues = g_adapter.num_tx_queues = 2;
	g_adapter.rx_ring[0].rx_stats.dropped = 3;
	g_adapter.rx_ring[0].rx_stats.skb_alloc_fail = 4;
	g_adapter.rx_ring[1].rx_stats.dropped = 10;
	g_adapter.tx_ring[0].tx_stats.dropped = 5;
	g_adapter.tx_ring[1].tx_stats.dropped = 6;
	al_eth_ring_drops(&g_adapter, &rx, &tx);
	CHECK(rx == 17 && tx == 11, "rx=%llu tx=%llu", (unsigned long long)rx, (unsigned long long)tx);
}

static const struct { const char *name; void (*fn)(void); } cases[] = {
	{ "sends_ok", sends_ok },
	{ "pad_failure_counted", pad_failure_counted },
	{ "csum_failure_counted", csum_failure_counted },
	{ "head_map_failure_counted_and_slot_cleared", head_map_failure_counted_and_slot_cleared },
	{ "frag_map_failure_unwinds_and_counts", frag_map_failure_unwinds_and_counts },
	{ "frag_overflow_counted_as_frag_err", frag_overflow_counted_as_frag_err },
	{ "ring_full_is_busy_not_a_drop", ring_full_is_busy_not_a_drop },
	{ "ring_drops_sums_every_queue", ring_drops_sums_every_queue },
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
