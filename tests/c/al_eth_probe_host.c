// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* #299 host harness (built by tests/test_al_eth.py).
 *
 * probe.inc is the REAL al_eth_probe() cut verbatim from
 * modules/al_eth/al_eth_main.c. Every resource it can create (netdev,
 * registered netdev, phylink, sysfs files) is a counter here; every fallible
 * callee can be made to fail. argv: <fail-step> <use_phylink 0|1>. After
 * probe() returns, a failed probe must have released everything it created;
 * a successful one must have registered exactly one netdev.
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
typedef int al_bool;
#define AL_TRUE 1
#define AL_FALSE 0
#define __iomem
#define unlikely(x) (x)
#define likely(x) (x)
#define ____cacheline_aligned
#define MODULE_VARIANT_UNUSED

/* ------------------------------------------------------------ constants */
#define PCI_DEVICE_ID 0x02
#define PCI_REVISION_ID 0x08
#define PCI_COMMAND 0x04
#define PCI_COMMAND_MEMORY 2
#define PCI_COMMAND_MASTER 4
#define PCI_COMMAND_INTX_DISABLE 0x400
#define PCI_SRIOV_NUM_VF 0x10
#define PCI_SRIOV_CTRL 0x08
#define PCI_SRIOV_CTRL_VFE 1
#define PCI_SRIOV_CTRL_MSE 8
#define DMA_BIT_MASK(n) ((1ULL << (n)) - 1)
#define ETH_ALEN 6
#define NETIF_F_SG 1
#define NETIF_F_IP_CSUM 1
#define NETIF_F_IPV6_CSUM 1
#define NETIF_F_TSO 1
#define NETIF_F_TSO_ECN 1
#define NETIF_F_TSO6 1
#define NETIF_F_RXCSUM 1
#define NETIF_F_NTUPLE 1
#define NETIF_F_RXHASH 1
#define NETIF_F_HIGHDMA 1
#define IFF_UNICAST_FLT 1
#define DEFAULT_MSG_ENABLE 0
#define DRV_MODULE_NAME "al_eth"
#define AL_ETH_NUM_QUEUES 4
#define AL_ETH_RX_RSS_TABLE_SIZE 8
#define AL_ETH_MIN_MTU 68
#define AL_ETH_MAX_MTU 9216
#define AL_ETH_NAME_MAX_LEN 32
#define TX_TIMEOUT 5000
#define AL_ETH_UDMA_BAR 0
#define AL_ETH_EC_BAR 2
#define AL_ETH_MAC_BAR 4
#define AL_ETH_REV_ID_3 3
#define AL_ETH_DEFAULT_TX_SW_DESCS 1024
#define AL_ETH_DEFAULT_TX_HW_DESCS 1024
#define AL_ETH_DEFAULT_RX_DESCS 1024
#define AL_ETH_DEFAULT_SMALL_PACKET_LEN 126
#define AL_ETH_DEFAULT_LINK_POLL_INTERVAL 500
#define AL_ETH_DEFAULT_MAX_RX_BUFF_ALLOC_SIZE 1536
#define AL_ETH_DEFAULT_FORCE_1000_BASEX AL_FALSE
#define AL_ETH_LM_MAX_SPEED_10G 10
#define AL_ETH_MAC_MODE_10GbE_Serial 1
#define ALPINE_INTEGRATED 0
#define ALPINE_NIC_V2_25_DUAL 4
#define PCI_FUNC(devfn) ((devfn) & 7)

/* ----------------------------------------------------------- fake types */
struct device { int unused; };
struct pci_dev { struct device dev; unsigned int devfn; };
struct pci_device_id { unsigned long driver_data; };
struct work_struct { int unused; };
struct timer_list { int unused; };
struct spinlock { int unused; };
typedef struct spinlock spinlock_t;
struct u64_stats_sync { int unused; };
struct al_hal_eth_adapter { int mac_mode; };
struct al_eth_link_config { int force_1000_base_x; };

struct net_device {
	const void *netdev_ops;
	const void *ethtool_ops;
	unsigned int watchdog_timeo;
	u64 features, hw_features, vlan_features;
	unsigned int priv_flags;
	unsigned int min_mtu, max_mtu;
	u8 dev_addr[ETH_ALEN];
	u8 perm_addr[ETH_ALEN];
	unsigned char addr_len;
	int registered;
	void *priv;
};

struct al_eth_adapter {
	struct net_device *netdev;
	struct pci_dev *pdev;
	int board_type;
	u16 dev_id;
	u8 rev_id;
	int id_number;
	int msg_enable;
	struct al_hal_eth_adapter hal_adapter;
	int udma_num;
	void *udma_base, *ec_base, *mac_base, *internal_pcie_base, *serdes_base;
	int tx_ring_count, tx_descs_count, rx_ring_count, rx_descs_count;
	int num_tx_queues, num_rx_queues;
	unsigned int small_copy_len, link_poll_interval, max_rx_buff_alloc_size;
	struct al_eth_link_config link_config;
	spinlock_t serdes_config_lock;
	char name[AL_ETH_NAME_MAX_LEN];
	int use_lm, sfp_detection_needed, max_speed, mac_mode;
	u8 mac_addr[ETH_ALEN];
	u8 rss_ind_tbl[AL_ETH_RX_RSS_TABLE_SIZE];
	struct u64_stats_sync syncp;
	struct work_struct reset_task;
	struct timer_list rx_watchdog;
	int adaptive_intr_rate;
	void *plink;
};

struct board_info_s { const char *name; int bar; };
static const struct board_info_s board_info[] = { { "fake", 0 } };

static const char version[] = "fake";
static int debug, port_num, udma_num, adaptive_int_moderation;

/* ------------------------------------------------------- resource ledger */
static int fail_step;	/* index into steps[]; 0 = none */
static int netdevs_allocated, netdevs_registered, phylinks_live, sysfs_live;
static struct net_device g_netdev;
static struct al_eth_adapter g_adapter;
static void *fake_bar[6];
static char bar_mem[6][0x2000000];

enum { F_NONE, F_BOARD_PARAMS, F_SERDES, F_PHYLINK, F_HW_INIT, F_SYSFS, F_REGISTER };
static const char *const step_names[] = {
	"none", "board_params", "serdes", "phylink", "hw_init", "sysfs", "register",
};

static int inject(int step) { return fail_step == step ? -ENOMEM : 0; }

/* -------------------------------------------------------------- the kernel */
#define dev_dbg(d, ...) ((void)(d))
#define dev_info(d, ...) ((void)(d))
#define dev_err(d, ...) ((void)(d))
#define dev_warn(d, ...) ((void)(d))
#define netdev_info(n, ...) ((void)(n))
#define netdev_err(n, ...) ((void)(n))
#define SET_NETDEV_DEV(n, d) ((void)(n))
#define spin_lock_init(l) ((void)(l))
#define u64_stats_init(s) ((void)(s))
#define INIT_WORK(w, f) ((void)(w))
#define timer_setup(t, f, fl) ((void)(t))
#define netif_msg_init(a, b) (b)
#define pci_resource_start(p, b) 0UL
#define ethtool_rxfh_indir_default(i, n) ((i) % (n))
#define pci_read_config_word(p, o, v) (*(v) = 0x1)
#define pci_read_config_byte(p, o, v) (*(v) = 0x2)
#define snprintf_name(...) 0

static int pcim_enable_device(struct pci_dev *p) { (void)p; return 0; }
static int pcim_iomap_regions(struct pci_dev *p, int mask, const char *n)
{
	(void)p; (void)mask; (void)n;
	return 0;
}
static void *const *pcim_iomap_table(struct pci_dev *p)
{
	(void)p;
	return fake_bar;
}
static int dma_set_mask_and_coherent(struct device *d, u64 m) { (void)d; (void)m; return 0; }
static void pci_set_master(struct pci_dev *p) { (void)p; }
static void pci_save_state(struct pci_dev *p) { (void)p; }
static void pci_set_drvdata(struct pci_dev *p, void *d) { (void)p; (void)d; }

static struct net_device *alloc_etherdev_mq(int priv, int q)
{
	(void)priv; (void)q;
	netdevs_allocated++;
	memset(&g_netdev, 0, sizeof(g_netdev));
	memset(&g_adapter, 0, sizeof(g_adapter));
	g_netdev.priv = &g_adapter;
	g_netdev.addr_len = ETH_ALEN;
	return &g_netdev;
}
static void *netdev_priv(struct net_device *n) { return n->priv; }
static void free_netdev(struct net_device *n)
{
	if (n->registered) {
		printf("BUG free_netdev() on a registered netdev\n");
		exit(1);
	}
	netdevs_allocated--;
}
static int register_netdev(struct net_device *n)
{
	if (inject(F_REGISTER))
		return -ENOMEM;
	n->registered = 1;
	netdevs_registered++;
	return 0;
}
static int is_valid_ether_addr(const u8 *a) { return a[0] || a[1]; }
static void eth_hw_addr_random(struct net_device *n) { n->dev_addr[0] = 2; }
static void eth_hw_addr_set(struct net_device *n, const u8 *a) { memcpy(n->dev_addr, a, ETH_ALEN); }

static u16 readw(const void *p) { (void)p; return 0; }
static u8 readb(const void *p) { (void)p; return 0; }
static void writew(u16 v, void *p) { (void)v; (void)p; }
static void writel(u32 v, void *p) { (void)v; (void)p; }

/* ------------------------------------------------------- driver callees */
static int al_eth_board_params_init(struct al_eth_adapter *a)
{
	a->use_lm = 1;
	a->sfp_detection_needed = getenv("USE_PHYLINK") != NULL;
	a->max_speed = AL_ETH_LM_MAX_SPEED_10G;
	a->mac_addr[0] = 0x02;
	return inject(F_BOARD_PARAMS);
}
static void al_eth_hal_adapter_init(struct al_eth_adapter *a) { (void)a; }
static int al_eth_hw_stop(struct al_eth_adapter *a) { (void)a; return 0; }
static int al_eth_serdes_init(struct al_eth_adapter *a) { (void)a; return inject(F_SERDES); }
static void al_eth_lm_static_init(struct al_eth_adapter *a) { (void)a; }
static int al_eth_phylink_setup(struct al_eth_adapter *a)
{
	if (inject(F_PHYLINK))
		return -ENODEV;
	a->plink = &phylinks_live;
	phylinks_live++;
	return 0;
}
static void al_eth_phylink_teardown(struct al_eth_adapter *a)
{
	if (!a->plink)
		return;
	a->plink = NULL;
	phylinks_live--;
}
static void al_eth_function_reset(struct al_eth_adapter *a) { (void)a; }
static int al_eth_hw_init_adapter(struct al_eth_adapter *a) { (void)a; return inject(F_HW_INIT); }
static void al_eth_init_rings(struct al_eth_adapter *a) { (void)a; }
static void al_eth_rx_watchdog(struct timer_list *t) { (void)t; }
static void al_eth_reset_task(struct work_struct *w) { (void)w; }
static const int al_eth_netdev_ops, al_eth_ethtool_ops;
static int al_eth_sysfs_init(struct device *d)
{
	(void)d;
	sysfs_live = 1;	/* files are created one by one: a failure leaves some */
	return inject(F_SYSFS);
}
static void al_eth_sysfs_terminate(struct device *d) { (void)d; sysfs_live = 0; }
static int al_eth_devlink_init(struct al_eth_adapter *a) { (void)a; return -ENOMEM; }
static void al_eth_bp_debugfs_init(struct al_eth_adapter *a) { (void)a; }
static void al_eth_init_intr_default_moderation_table_intervals(struct al_eth_adapter *a) { (void)a; }
static void al_eth_initial_moderation_table_restore_default(void) { }

#include "probe.inc"

int main(int argc, char **argv)
{
	struct pci_dev pdev = { 0 };
	struct pci_device_id ent = { .driver_data = ALPINE_INTEGRATED };
	int step = 0, rc;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <step>\n", argv[0]);
		return 2;
	}
	for (int i = 0; i < 7; i++)
		if (!strcmp(argv[1], step_names[i]))
			step = i;
	fail_step = step;
	for (int i = 0; i < 6; i++)
		fake_bar[i] = bar_mem[i];

	rc = al_eth_probe(&pdev, &ent);

	if (!fail_step) {
		if (rc || netdevs_registered != 1 || !sysfs_live) {
			printf("FAIL clean probe rc=%d registered=%d sysfs=%d\n", rc,
			       netdevs_registered, sysfs_live);
			return 1;
		}
		printf("PASS none\n");
		return 0;
	}
	if (!rc) {
		printf("FAIL probe ignored the %s failure\n", step_names[step]);
		return 1;
	}
	if (g_netdev.registered || netdevs_allocated || phylinks_live || sysfs_live) {
		printf("FAIL after %s failure, probe returned %d and left: registered=%d "
		       "allocated=%d phylink=%d sysfs=%d\n",
		       step_names[step], rc, g_netdev.registered, netdevs_allocated,
		       phylinks_live, sysfs_live);
		return 1;
	}
	printf("PASS %s\n", step_names[step]);
	return 0;
}
