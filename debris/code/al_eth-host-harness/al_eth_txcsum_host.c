// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* #297 host harness (built by tests/test_al_eth.py).
 *
 * txcsum_funcs.inc holds al_eth_tx_l4_proto(), al_eth_features_check() and
 * al_eth_tx_csum() cut verbatim from modules/al_eth/al_eth_main.c. The skb is
 * a struct with the fields they read and real-layout IPv4/IPv6 headers, so
 * ip_hdr(skb)->version and ->protocol / ipv6_hdr(skb)->nexthdr read the same
 * bytes the kernel's would. Case = argv[1].
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef u64 netdev_features_t;

#define IPVERSION 4
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_GRE 47
#define IPPROTO_HOPOPTS 0
#define IPPROTO_DSTOPTS 60
#define CHECKSUM_NONE 0
#define CHECKSUM_PARTIAL 3
#define ETH_P_8021Q 0x8100
#define VLAN_HLEN 4
#define NETIF_F_IP_CSUM (1ULL << 1)
#define NETIF_F_IPV6_CSUM (1ULL << 2)
#define NETIF_F_CSUM_MASK (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM)
#define NETIF_F_TSO (1ULL << 10)
#define NETIF_F_TSO6 (1ULL << 11)
#define NETIF_F_GSO_MASK (NETIF_F_TSO | NETIF_F_TSO6)
#define NETIF_F_SG (1ULL << 20)
#define __constant_htons(x) ((u16)(((x) >> 8) | ((x) << 8)))
#define pr_warn(...) ((void)0)

#define AL_ETH_TX_FLAGS_INT 0x1
#define AL_ETH_TX_FLAGS_TSO 0x2
#define AL_ETH_TX_FLAGS_L4_CSUM 0x4
#define AL_ETH_TX_FLAGS_L4_PARTIAL_CSUM 0x8
#define AL_ETH_TX_FLAGS_IPV4_L3_CSUM 0x10
enum { AL_ETH_PROTO_ID_IPv4 = 1, AL_ETH_PROTO_ID_IPv6, AL_ETH_PROTO_ID_TCP, AL_ETH_PROTO_ID_UDP };

struct iphdr {
	u8 ihl : 4, version : 4;
	u8 tos;
	u16 tot_len, id, frag_off;
	u8 ttl, protocol;
	u16 check;
	u32 saddr, daddr;
};
struct ipv6hdr {
	u8 priority : 4, version : 4;
	u8 flow_lbl[3];
	u16 payload_len;
	u8 nexthdr, hop_limit;
	u8 addrs[32];
};
struct tcphdr {
	u16 source, dest;
	u32 seq, ack_seq;
	u16 res1 : 4, doff : 4, flags : 8;
	u16 window, check, urg_ptr;
};
struct skb_shared_info { u32 gso_size; };

struct sk_buff {
	int ip_summed;
	int encapsulation;
	u16 protocol;
	int len;
	unsigned int net_off, net_hlen, tp_off;
	struct skb_shared_info shinfo;
	union {
		struct iphdr ip4;
		struct ipv6hdr ip6;
	} net;
	struct tcphdr tcp;
};
struct net_device { int unused; };
struct al_eth_meta_data {
	int words_valid;
	unsigned int l3_header_len, l3_header_offset, l4_header_len, mss_idx_sel, mss_val;
};
struct al_eth_pkt {
	u32 flags;
	int l3_proto_idx, l4_proto_idx, source_vlan_count;
	struct al_eth_meta_data *meta;
};
struct al_eth_ring { struct al_eth_meta_data hal_meta; };
struct al_eth_tx_buffer { int unused; };

#define ip_hdr(skb) (&(skb)->net.ip4)
#define ipv6_hdr(skb) (&(skb)->net.ip6)
#define tcp_hdr(skb) (&(skb)->tcp)
#define skb_shinfo(skb) (&(skb)->shinfo)
#define skb_is_gso(skb) ((skb)->shinfo.gso_size != 0)
#define skb_network_header_len(skb) ((skb)->net_hlen)
#define skb_network_offset(skb) ((skb)->net_off)
#define skb_transport_offset(skb) ((skb)->tp_off)
#define tcp_hdrlen(skb) (tcp_hdr(skb)->doff * 4)

static int checksum_help_calls, checksum_help_rc, tso_fix_calls;

static int skb_checksum_help(struct sk_buff *skb)
{
	checksum_help_calls++;
	if (skb_is_gso(skb))
		return -EINVAL;	/* the kernel refuses GSO skbs */
	if (checksum_help_rc)
		return checksum_help_rc;
	skb->ip_summed = CHECKSUM_NONE;
	return 0;
}
static int al_eth_tx_tso_mss_fix(struct sk_buff *skb, u32 mss)
{
	(void)skb; (void)mss;
	tso_fix_calls++;
	return 0;
}

#include "txcsum_funcs.inc"

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

static struct sk_buff skb;
static struct al_eth_ring ring;
static struct al_eth_pkt pkt;
static struct al_eth_tx_buffer txb;

static void mk4(u8 proto, int mss)
{
	memset(&skb, 0, sizeof(skb));
	memset(&ring, 0, sizeof(ring));
	memset(&pkt, 0, sizeof(pkt));
	checksum_help_calls = checksum_help_rc = tso_fix_calls = 0;
	skb.ip_summed = CHECKSUM_PARTIAL;
	skb.net.ip4.version = 4;
	skb.net.ip4.protocol = proto;
	skb.net_off = 14;
	skb.net_hlen = 20;
	skb.tp_off = 34;
	skb.tcp.doff = 5;
	skb.shinfo.gso_size = mss;
	pkt.flags = AL_ETH_TX_FLAGS_INT;
	pkt.l4_proto_idx = 99;	/* stale value from the slot's previous packet */
}

static void mk6(u8 nexthdr, int mss)
{
	mk4(0, mss);
	memset(&skb.net, 0, sizeof(skb.net));
	skb.net.ip6.version = 6;
	skb.net.ip6.nexthdr = nexthdr;
	skb.net_hlen = 40;
}

static int tx(void) { return al_eth_tx_csum(&ring, &txb, &pkt, &skb); }

/* the plain cases keep their offload exactly as before */
static void v4_tcp_offloaded(void)
{
	mk4(IPPROTO_TCP, 0);
	CHECK(tx() == 0 && checksum_help_calls == 0, "fell back for plain v4 tcp");
	CHECK(pkt.flags == (AL_ETH_TX_FLAGS_INT | AL_ETH_TX_FLAGS_L4_CSUM | AL_ETH_TX_FLAGS_L4_PARTIAL_CSUM),
	      "flags %x", pkt.flags);
	CHECK(pkt.l3_proto_idx == AL_ETH_PROTO_ID_IPv4 && pkt.l4_proto_idx == AL_ETH_PROTO_ID_TCP, "proto idx");
	CHECK(pkt.meta == &ring.hal_meta && ring.hal_meta.l4_header_len == 5, "meta");
}

static void v4_udp_offloaded(void)
{
	mk4(IPPROTO_UDP, 0);
	CHECK(tx() == 0 && checksum_help_calls == 0, "fell back for v4 udp");
	CHECK(pkt.l4_proto_idx == AL_ETH_PROTO_ID_UDP && ring.hal_meta.l4_header_len == 0, "udp meta");
}

static void v6_tcp_offloaded(void)
{
	mk6(IPPROTO_TCP, 0);
	CHECK(tx() == 0 && checksum_help_calls == 0, "fell back for plain v6 tcp");
	CHECK(pkt.l3_proto_idx == AL_ETH_PROTO_ID_IPv6 && pkt.l4_proto_idx == AL_ETH_PROTO_ID_TCP, "proto idx");
}

static void v4_tso_offloaded(void)
{
	mk4(IPPROTO_TCP, 1448);
	CHECK(tx() == 0 && tso_fix_calls == 1, "tso path");
	CHECK((pkt.flags & AL_ETH_TX_FLAGS_TSO) && (pkt.flags & AL_ETH_TX_FLAGS_IPV4_L3_CSUM), "tso flags %x", pkt.flags);
	CHECK(ring.hal_meta.mss_val == 1448, "mss not in meta");
}

/* IPv6 + extension header: nexthdr is the ext type, not TCP. No HW offload. */
static void v6_exthdr_falls_back_to_software(void)
{
	u8 exts[] = { IPPROTO_HOPOPTS, 43, IPPROTO_DSTOPTS, 44, 60 };

	for (unsigned int i = 0; i < sizeof(exts); i++) {
		mk6(exts[i], 0);
		CHECK(tx() == 0, "ext %u: rc", exts[i]);
		CHECK(checksum_help_calls == 1, "ext %u: skb_checksum_help x%d", exts[i], checksum_help_calls);
		CHECK(skb.ip_summed == CHECKSUM_NONE, "ext %u: csum still pending", exts[i]);
		CHECK(pkt.flags == AL_ETH_TX_FLAGS_INT, "ext %u: HW csum flags set (%x)", exts[i], pkt.flags);
		CHECK(pkt.meta == NULL, "ext %u: meta descriptor built", exts[i]);
	}
}

static void unknown_l4_falls_back(void)
{
	mk4(IPPROTO_GRE, 0);
	CHECK(tx() == 0 && checksum_help_calls == 1 && pkt.flags == AL_ETH_TX_FLAGS_INT && !pkt.meta,
	      "GRE: helps=%d flags=%x", checksum_help_calls, pkt.flags);
}

static void encapsulated_falls_back(void)
{
	mk4(IPPROTO_UDP, 0);
	skb.encapsulation = 1;
	CHECK(tx() == 0 && checksum_help_calls == 1 && !pkt.meta, "tunnel skb used HW csum");
}

static void fallback_failure_is_reported(void)
{
	mk6(IPPROTO_HOPOPTS, 0);
	checksum_help_rc = -ENOMEM;
	CHECK(tx() == -ENOMEM, "skb_checksum_help failure swallowed");
}

/* GSO the MAC cannot segment: skb_checksum_help() would refuse it, so drop */
static void gso_unsupported_is_dropped(void)
{
	mk6(IPPROTO_HOPOPTS, 1440);
	CHECK(tx() == -EOPNOTSUPP && pkt.flags == AL_ETH_TX_FLAGS_INT, "v6 ext TSO not refused");
	mk4(IPPROTO_UDP, 1400);
	CHECK(tx() == -EOPNOTSUPP && !(pkt.flags & AL_ETH_TX_FLAGS_TSO), "udp 'TSO' not refused");
}

static void no_offload_wanted_is_untouched(void)
{
	mk4(IPPROTO_TCP, 0);
	skb.ip_summed = CHECKSUM_NONE;
	CHECK(tx() == 0 && pkt.meta == NULL && checksum_help_calls == 0, "touched a no-offload skb");
}

static const netdev_features_t ALLF = NETIF_F_CSUM_MASK | NETIF_F_GSO_MASK | NETIF_F_SG;

static void fc_keeps_features_for_supported(void)
{
	struct net_device nd;

	mk4(IPPROTO_TCP, 0);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == ALLF, "stripped v4 tcp");
	mk4(IPPROTO_UDP, 0);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == ALLF, "stripped v4 udp");
	mk6(IPPROTO_TCP, 0);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == ALLF, "stripped v6 tcp");
	mk4(IPPROTO_TCP, 1448);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == ALLF, "stripped v4 tso");
	mk4(IPPROTO_TCP, 0);
	skb.ip_summed = CHECKSUM_NONE;
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == ALLF, "stripped a plain skb");
}

static void fc_strips_csum_and_gso_for_unsupported(void)
{
	struct net_device nd;
	netdev_features_t want = NETIF_F_SG;

	mk6(IPPROTO_HOPOPTS, 0);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == want, "v6 ext hdr keeps offload");
	mk6(IPPROTO_HOPOPTS, 1440);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == want, "v6 ext hdr TSO keeps offload");
	mk4(IPPROTO_GRE, 0);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == want, "GRE keeps offload");
	mk4(IPPROTO_UDP, 0);
	skb.encapsulation = 1;
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == want, "tunnel keeps offload");
	mk4(IPPROTO_UDP, 1400);
	CHECK(al_eth_features_check(&skb, &nd, ALLF) == want, "udp gso keeps offload");
}

static const struct { const char *name; void (*fn)(void); } cases[] = {
	{ "v4_tcp_offloaded", v4_tcp_offloaded },
	{ "v4_udp_offloaded", v4_udp_offloaded },
	{ "v6_tcp_offloaded", v6_tcp_offloaded },
	{ "v4_tso_offloaded", v4_tso_offloaded },
	{ "v6_exthdr_falls_back_to_software", v6_exthdr_falls_back_to_software },
	{ "unknown_l4_falls_back", unknown_l4_falls_back },
	{ "encapsulated_falls_back", encapsulated_falls_back },
	{ "fallback_failure_is_reported", fallback_failure_is_reported },
	{ "gso_unsupported_is_dropped", gso_unsupported_is_dropped },
	{ "no_offload_wanted_is_untouched", no_offload_wanted_is_untouched },
	{ "fc_keeps_features_for_supported", fc_keeps_features_for_supported },
	{ "fc_strips_csum_and_gso_for_unsupported", fc_strips_csum_and_gso_for_unsupported },
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
