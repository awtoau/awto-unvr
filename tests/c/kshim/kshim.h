/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright (C) 2026 Awto / Daniel Tyrrell */
/* Host stand-ins for the kernel APIs modules/al_dma/*.c uses (tests only).
 * Every <linux/...> header the driver names forwards here (written by
 * tests/test_al_dma.py). Mirrors mainline include/linux/dmaengine.h layout
 * for the fields the driver touches; locks are error-checking mutexes so a
 * double lock or an unlock by a non-owner aborts the test. */
#ifndef KSHIM_H
#define KSHIM_H

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
typedef uint64_t dma_addr_t;
typedef unsigned int gfp_t;
typedef unsigned int irqreturn_t;

#define __iomem
#define __init
#define __exit
#define __percpu
#define __must_check
#define __always_inline inline
#define likely(x)	(__builtin_expect(!!(x), 1))
#define unlikely(x)	(__builtin_expect(!!(x), 0))
#define barrier()	__asm__ volatile("" ::: "memory")
#define mb()		__sync_synchronize()
#define smp_mb()	__sync_synchronize()
#define smp_rmb()	__sync_synchronize()
#define smp_wmb()	__sync_synchronize()
#define READ_ONCE(x)	(*(volatile __typeof__(x) *)&(x))
#define WRITE_ONCE(x, v) (*(volatile __typeof__(x) *)&(x) = (v))
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define GFP_KERNEL 0u
#define GFP_ATOMIC 1u
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL << (n)) - 1))

/* ---- logging: counted so tests can assert on faults/warnings ---- */
extern int k_warn_count, k_err_count;
void k_log(const char *lvl, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#define pr_err(...)	do { k_err_count++; k_log("ERR", __VA_ARGS__); } while (0)
#define pr_warn(...)	do { k_warn_count++; k_log("WARN", __VA_ARGS__); } while (0)
#define pr_info(...)	k_log("INFO", __VA_ARGS__)
#define pr_debug(...)	((void)0)
#define dev_err(d, ...)	do { (void)(d); k_err_count++; k_log("ERR", __VA_ARGS__); } while (0)
#define dev_warn(d, ...) do { (void)(d); k_warn_count++; k_log("WARN", __VA_ARGS__); } while (0)
#define dev_info(d, ...) do { (void)(d); k_log("INFO", __VA_ARGS__); } while (0)
#define dev_dbg(d, ...)	((void)(d))
#define BUG_ON(c) do { if (c) { k_log("BUG", "BUG_ON(%s) %s:%d", #c, __FILE__, __LINE__); abort(); } } while (0)
#define WARN_ON_ONCE(c) ({ int __w = !!(c); if (__w) { k_warn_count++; k_log("WARN", "WARN_ON_ONCE(%s) %s:%d", #c, __FILE__, __LINE__); } __w; })

/* ---- module glue (no-ops) ---- */
#define module_param(n, t, p)
#define MODULE_PARM_DESC(n, d)
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define MODULE_VERSION(x)
#define MODULE_DEVICE_TABLE(a, b)
#define THIS_MODULE ((void *)0)

/* ---- time ---- */
#define HZ 1000
extern unsigned long jiffies;
#define msecs_to_jiffies(m) ((unsigned long)(m))
#define jiffies_to_msecs(j) ((unsigned long)(j))
#define time_after(a, b) ((long)((b) - (a)) < 0)

/* ---- atomics ---- */
typedef struct { long long v; } atomic64_t;
static inline void atomic64_inc(atomic64_t *a) { __atomic_add_fetch(&a->v, 1, __ATOMIC_SEQ_CST); }
static inline long long atomic64_read(const atomic64_t *a) { return __atomic_load_n(&a->v, __ATOMIC_SEQ_CST); }

/* ---- lists ---- */
struct list_head { struct list_head *next, *prev; };
#define INIT_LIST_HEAD(h) do { (h)->next = (h); (h)->prev = (h); } while (0)
static inline void list_add_tail(struct list_head *n, struct list_head *h)
{
	n->prev = h->prev; n->next = h; h->prev->next = n; h->prev = n;
}

/* ---- locks ---- */
typedef struct {
	pthread_mutex_t m;
	pthread_t owner;
	volatile int held;
} spinlock_t;
extern int k_lock_yield;	/* >0: sched_yield() after each unlock (widens races) */
void k_spin_lock(spinlock_t *l);
void k_spin_unlock(spinlock_t *l);
int k_spin_held(spinlock_t *l);
#define spin_lock_init(l) do { pthread_mutexattr_t __a; pthread_mutexattr_init(&__a); \
	pthread_mutexattr_settype(&__a, PTHREAD_MUTEX_ERRORCHECK); \
	pthread_mutex_init(&(l)->m, &__a); (l)->held = 0; } while (0)
#define spin_lock_irqsave(l, f) do { (void)(f); k_spin_lock(l); (f) = 0; } while (0)
#define spin_unlock_irqrestore(l, f) do { (void)(f); k_spin_unlock(l); } while (0)
#define lockdep_assert_held(l) BUG_ON(!k_spin_held(l))

/* ---- tasklets: run by the test, never concurrently with themselves ---- */
struct tasklet_struct {
	void (*func)(struct tasklet_struct *);
	volatile int scheduled;
};
#define tasklet_setup(t, fn) do { (t)->func = (fn); (t)->scheduled = 0; } while (0)
#define from_tasklet(var, tl, field) container_of(tl, __typeof__(*(var)), field)
#define tasklet_schedule(t) __atomic_store_n(&(t)->scheduled, 1, __ATOMIC_SEQ_CST)
#define tasklet_kill(t) __atomic_store_n(&(t)->scheduled, 0, __ATOMIC_SEQ_CST)
struct work_struct { int dummy; };

/* ---- memory ---- */
#define kcalloc(n, sz, f) calloc((n), (sz))
#define kzalloc(sz, f) calloc(1, (sz))
#define kfree(p) free(p)
struct device { int dummy; };
void *k_dma_alloc(size_t size, dma_addr_t *dma);
#define dma_alloc_coherent(dev, size, dma, gfp) k_dma_alloc((size), (dma))
#define dma_free_coherent(dev, size, p, dma) free(p)

/* ---- dmaengine (field layout mirrors include/linux/dmaengine.h) ---- */
typedef s32 dma_cookie_t;
#define DMA_MIN_COOKIE 1
enum dma_status { DMA_COMPLETE, DMA_IN_PROGRESS, DMA_PAUSED, DMA_ERROR, DMA_OUT_OF_ORDER };
enum dma_transaction_type {
	DMA_MEMCPY, DMA_XOR, DMA_PQ, DMA_XOR_VAL, DMA_PQ_VAL, DMA_MEMSET,
	DMA_MEMSET_SG, DMA_INTERRUPT, DMA_PRIVATE, DMA_ASYNC_TX, DMA_SLAVE,
	DMA_CYCLIC, DMA_INTERLEAVE, DMA_COMPLETION_NO_ORDER, DMA_REPEAT,
	DMA_LOAD_EOT, DMA_TX_TYPE_END,
};
enum dma_ctrl_flags {
	DMA_PREP_INTERRUPT = (1 << 0), DMA_CTRL_ACK = (1 << 1),
	DMA_PREP_PQ_DISABLE_P = (1 << 2), DMA_PREP_PQ_DISABLE_Q = (1 << 3),
	DMA_PREP_CONTINUE = (1 << 4), DMA_PREP_FENCE = (1 << 5),
};
enum dmaengine_tx_result {
	DMA_TRANS_NOERROR, DMA_TRANS_READ_FAILED, DMA_TRANS_WRITE_FAILED,
	DMA_TRANS_ABORTED,
};
struct dmaengine_result { enum dmaengine_tx_result result; u32 residue; };
typedef void (*dma_async_tx_callback)(void *dma_async_param);
typedef void (*dma_async_tx_callback_result)(void *dma_async_param,
					     const struct dmaengine_result *result);
struct dma_tx_state { dma_cookie_t last, used; u32 residue; u32 in_flight_bytes; };
typedef struct { unsigned long bits; } dma_cap_mask_t;
#define dma_cap_set(bit, mask) ((mask).bits |= 1UL << (bit))
#define dma_has_cap(bit, mask) (!!((mask).bits & (1UL << (bit))))

struct dma_device;
struct dma_chan {
	struct dma_device *device;
	dma_cookie_t cookie;
	dma_cookie_t completed_cookie;
	struct list_head device_node;
};
struct dma_async_tx_descriptor {
	dma_cookie_t cookie;
	enum dma_ctrl_flags flags;
	dma_addr_t phys;
	struct dma_chan *chan;
	dma_cookie_t (*tx_submit)(struct dma_async_tx_descriptor *tx);
	dma_async_tx_callback callback;
	dma_async_tx_callback_result callback_result;
	void *callback_param;
	void *unmap;
};
static inline void dma_async_tx_descriptor_init(struct dma_async_tx_descriptor *tx,
						struct dma_chan *chan)
{
	tx->chan = chan;
}
extern int k_unmap_calls, k_dependency_calls;
static inline void dma_descriptor_unmap(struct dma_async_tx_descriptor *tx) { (void)tx; k_unmap_calls++; }
static inline void dma_run_dependencies(struct dma_async_tx_descriptor *tx) { (void)tx; k_dependency_calls++; }

struct dma_device {
	struct device *dev;
	dma_cap_mask_t cap_mask;
	unsigned short max_xor, max_pq;
	unsigned char copy_align, xor_align, pq_align;
	struct list_head channels;
	int (*device_alloc_chan_resources)(struct dma_chan *chan);
	void (*device_free_chan_resources)(struct dma_chan *chan);
	struct dma_async_tx_descriptor *(*device_prep_dma_memcpy)(
		struct dma_chan *chan, dma_addr_t dst, dma_addr_t src,
		size_t len, unsigned long flags);
	struct dma_async_tx_descriptor *(*device_prep_dma_xor)(
		struct dma_chan *chan, dma_addr_t dst, dma_addr_t *src,
		unsigned int src_cnt, size_t len, unsigned long flags);
	struct dma_async_tx_descriptor *(*device_prep_dma_pq)(
		struct dma_chan *chan, dma_addr_t *dst, dma_addr_t *src,
		unsigned int src_cnt, const unsigned char *scf, size_t len,
		unsigned long flags);
	enum dma_status (*device_tx_status)(struct dma_chan *chan,
					    dma_cookie_t cookie,
					    struct dma_tx_state *state);
	void (*device_issue_pending)(struct dma_chan *chan);
};
int dma_async_device_register(struct dma_device *d);
void dma_async_device_unregister(struct dma_device *d);

/* ---- PCI: just enough for probe/remove to compile and run ---- */
struct pci_dev { struct device dev; void *drvdata; };
struct pci_device_id { unsigned int class, class_mask; };
struct pci_driver {
	const char *name;
	const struct pci_device_id *id_table;
	int (*probe)(struct pci_dev *, const struct pci_device_id *);
	void (*remove)(struct pci_dev *);
};
#define PCI_DEVICE_CLASS(c, m) .class = (c), .class_mask = (m)
#define PCI_REVISION_ID 0x08
#define module_pci_driver(d)
#define to_pci_dev(x) container_of(x, struct pci_dev, dev)
static inline void pci_set_drvdata(struct pci_dev *p, void *d) { p->drvdata = d; }
static inline void *pci_get_drvdata(struct pci_dev *p) { return p->drvdata; }
int pci_enable_device(struct pci_dev *p);
void pci_disable_device(struct pci_dev *p);
int pci_request_regions(struct pci_dev *p, const char *n);
void pci_release_regions(struct pci_dev *p);
void pci_set_master(struct pci_dev *p);
int dma_set_mask_and_coherent(struct device *d, u64 mask);
void *pci_iomap(struct pci_dev *p, int bar, unsigned long max);
void pci_iounmap(struct pci_dev *p, void *a);
int pci_read_config_byte(struct pci_dev *p, int where, u8 *val);
#define PCI_VENDOR_ID_AMAZON_ANNAPURNA_LABS 0x1c36

#endif /* KSHIM_H */
