// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* Host harness for modules/al_dma (built by tests/test_al_dma.py).
 *
 * The REAL al_dma_drv.c is #included below and linked against the REAL HAL
 * (.c files copied unmodified); only the kernel (tests/c/kshim) and the
 * registers are fake. The fake UDMA engine reacts to the doorbell write and
 * to the queue-reset write, and produces S2M completion descriptors in the
 * ring memory the driver programmed. Tests are selected by argv[1]; each
 * prints PASS/FAIL lines and exits non-zero on FAIL.
 */
#include <sched.h>
#include <unistd.h>

#include "al_dma_drv.c"

/* ---------------------------------------------------------------- kshim */
int k_warn_count, k_err_count;
unsigned long jiffies;
int k_lock_yield;
int k_unmap_calls, k_dependency_calls;
static int quiet;

void k_log(const char *lvl, const char *fmt, ...)
{
	va_list ap;

	if (quiet)
		return;
	fprintf(stderr, "[%s] ", lvl);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void k_spin_lock(spinlock_t *l)
{
	int rc = pthread_mutex_lock(&l->m);

	if (rc) {
		k_log("BUG", "spin_lock: rc=%d (relock by owner = deadlock)", rc);
		abort();
	}
	l->owner = pthread_self();
	l->held = 1;
}

void k_spin_unlock(spinlock_t *l)
{
	int rc;

	l->held = 0;
	rc = pthread_mutex_unlock(&l->m);
	if (rc) {
		k_log("BUG", "spin_unlock: rc=%d (not the owner)", rc);
		abort();
	}
	if (k_lock_yield)
		sched_yield();
}

int k_spin_held(spinlock_t *l)
{
	return l->held && pthread_equal(l->owner, pthread_self());
}

void *k_dma_alloc(size_t size, dma_addr_t *dma)
{
	void *p = aligned_alloc(4096, ALIGN(size, 4096));

	if (!p)
		return NULL;
	memset(p, 0, size);
	*dma = (dma_addr_t)(uintptr_t)p;
	return p;
}

static int g_register_calls, g_unregister_calls;
int dma_async_device_register(struct dma_device *d) { (void)d; g_register_calls++; return 0; }
void dma_async_device_unregister(struct dma_device *d) { (void)d; g_unregister_calls++; }

static void *g_bar0, *g_bar4;
int pci_enable_device(struct pci_dev *p) { (void)p; return 0; }
void pci_disable_device(struct pci_dev *p) { (void)p; }
int pci_request_regions(struct pci_dev *p, const char *n) { (void)p; (void)n; return 0; }
void pci_release_regions(struct pci_dev *p) { (void)p; }
void pci_set_master(struct pci_dev *p) { (void)p; }
int dma_set_mask_and_coherent(struct device *d, u64 m) { (void)d; (void)m; return 0; }
void *pci_iomap(struct pci_dev *p, int bar, unsigned long max)
{
	(void)p; (void)max;
	return bar == 0 ? g_bar0 : g_bar4;
}
void pci_iounmap(struct pci_dev *p, void *a) { (void)p; (void)a; }
int pci_read_config_byte(struct pci_dev *p, int where, u8 *val)
{
	(void)p; (void)where;
	*val = 2;
	return 0;
}

/* devlink / sysfs live in their own files in the module; counted here. */
static int g_fault[AL_DMA_FAULT_COUNT];
void al_dma_fault(struct al_dma_device *dev, enum al_dma_fault f)
{
	(void)dev;
	__atomic_add_fetch(&g_fault[f], 1, __ATOMIC_SEQ_CST);
}
int al_dma_devlink_init(struct al_dma_device *dev) { (void)dev; return 0; }
void al_dma_devlink_fini(struct al_dma_device *dev) { (void)dev; }
int al_dma_sysfs_init(struct al_dma_device *dev) { (void)dev; return 0; }
void al_dma_sysfs_remove(struct al_dma_device *dev) { (void)dev; }

/* ----------------------------------------------------- fake UDMA engine */
#define NQ DMA_MAX_Q
#define MAXPEND 2048

struct emu_pend { uint32_t n_tx; uint32_t status; };
struct emu_q {
	uint32_t tx_crhp;	/* M2S completion head: descriptors consumed */
	uint32_t rx_slot;	/* next S2M completion slot */
	uint32_t rx_ring_id;	/* current phase */
	int hold;		/* 1: doorbells queue up, nothing completes */
	struct emu_pend pend[MAXPEND];
	int npend;
	uint32_t script[MAXPEND];	/* status for upcoming ops, FIFO */
	int script_head, script_tail;
	int rst_tx, rst_rx;
	int doorbells, completions, dropped;
};
static struct emu_q emu[NQ];
static pthread_mutex_t emu_mu = PTHREAD_MUTEX_INITIALIZER;
static struct unit_regs *R;

static union udma_q_regs *txq(int q) { return (union udma_q_regs *)&R->m2s.m2s_q[q]; }
static union udma_q_regs *rxq(int q) { return (union udma_q_regs *)&R->s2m.s2m_q[q]; }

static void emu_reset_state(int q)
{
	emu[q].tx_crhp = 0;
	emu[q].rx_slot = 0;
	emu[q].rx_ring_id = 1;	/* AL_UDMA_INITIAL_RING_ID */
}

static void emu_complete_locked(int q, uint32_t n_tx, uint32_t status)
{
	struct emu_q *e = &emu[q];
	volatile uint32_t *cd;
	uint64_t base;
	uint32_t size;

	e->tx_crhp = (e->tx_crhp + n_tx) & 0xFFFF;
	txq(q)->rings.crhp = e->tx_crhp;
	txq(q)->rings.dcp = e->tx_crhp;

	base = ((uint64_t)rxq(q)->rings.crbp_high << 32) | rxq(q)->rings.crbp_low;
	size = rxq(q)->rings.drl;
	cd = (volatile uint32_t *)(uintptr_t)(base + (uint64_t)e->rx_slot * AL_DMA_RAID_RX_CDESC_SIZE);
	__sync_synchronize();
	*cd = (e->rx_ring_id << 24) | AL_UDMA_CDESC_LAST | AL_UDMA_CDESC_FIRST | status;
	e->rx_slot++;
	if (e->rx_slot == size) {
		e->rx_slot = 0;
		e->rx_ring_id = (e->rx_ring_id + 1) & 3;
	}
	rxq(q)->rings.crhp = e->rx_slot;
	rxq(q)->rings.dcp = e->rx_slot;
	e->completions++;
}

static uint32_t emu_next_status_locked(int q)
{
	struct emu_q *e = &emu[q];

	if (e->script_head == e->script_tail)
		return 0;
	return e->script[e->script_head++ % MAXPEND];
}

static void emu_doorbell(int q, uint32_t n_tx)
{
	struct emu_q *e = &emu[q];

	pthread_mutex_lock(&emu_mu);
	e->doorbells++;
	if (!(txq(q)->rings.cfg & UDMA_M2S_Q_CFG_EN_SCHEDULING)) {
		e->dropped++;	/* a stopped queue ignores the doorbell */
	} else if (e->hold) {
		BUG_ON(e->npend >= MAXPEND);
		e->pend[e->npend].n_tx = n_tx;
		e->pend[e->npend].status = emu_next_status_locked(q);
		e->npend++;
	} else {
		emu_complete_locked(q, n_tx, emu_next_status_locked(q));
	}
	pthread_mutex_unlock(&emu_mu);
}

static void emu_release(int q)
{
	struct emu_q *e = &emu[q];
	int i;

	pthread_mutex_lock(&emu_mu);
	for (i = 0; i < e->npend; i++)
		emu_complete_locked(q, e->pend[i].n_tx, e->pend[i].status);
	e->npend = 0;
	e->hold = 0;
	pthread_mutex_unlock(&emu_mu);
}

static void emu_script(int q, uint32_t status)
{
	pthread_mutex_lock(&emu_mu);
	emu[q].script[emu[q].script_tail++ % MAXPEND] = status;
	pthread_mutex_unlock(&emu_mu);
}

static void emu_queue_reset(int q, int is_tx)
{
	struct emu_q *e = &emu[q];

	pthread_mutex_lock(&emu_mu);
	if (is_tx) {
		e->rst_tx++;
		e->npend = 0;
		e->tx_crhp = 0;
		txq(q)->rings.crhp = 0;
		txq(q)->rings.dcp = 0;
	} else {
		e->rst_rx++;
		e->rx_slot = 0;
		e->rx_ring_id = 1;
		rxq(q)->rings.crhp = 0;
		rxq(q)->rings.dcp = 0;
	}
	pthread_mutex_unlock(&emu_mu);
}

uint32_t host_dma_rd32(const volatile void *a)
{
	return *(const volatile uint32_t *)a;
}

void host_dma_wr32(volatile void *a, uint32_t v)
{
	int q;

	*(volatile uint32_t *)a = v;
	if (!R)
		return;
	for (q = 0; q < NQ; q++) {
		if (a == &txq(q)->rings.drtp_inc) {
			emu_doorbell(q, v);
			return;
		}
		if (a == &txq(q)->m2s_q.q_sw_ctrl && (v & UDMA_M2S_Q_SW_CTRL_RST_Q)) {
			emu_queue_reset(q, 1);
			return;
		}
		if (a == &rxq(q)->s2m_q.q_sw_ctrl && (v & UDMA_M2S_Q_SW_CTRL_RST_Q)) {
			emu_queue_reset(q, 0);
			return;
		}
	}
}

/* ------------------------------------------------------------ test env */
static int fails;

#define CHECK(c, what)                                                     \
	do {                                                               \
		if (c) {                                                   \
			printf("PASS %s\n", what);                         \
		} else {                                                   \
			printf("FAIL %s (%s:%d)\n", what, __FILE__, __LINE__); \
			fails++;                                           \
		}                                                          \
	} while (0)

static struct pci_dev g_pdev;
static struct al_dma_device *D;

static void env_init(void)
{
	int q, rc;

	memset(emu, 0, sizeof(emu));
	for (q = 0; q < NQ; q++)
		emu_reset_state(q);
	memset(g_fault, 0, sizeof(g_fault));
	k_warn_count = k_err_count = k_unmap_calls = k_dependency_calls = 0;
	jiffies = 100000;

	g_bar0 = calloc(1, 0x20000);
	g_bar4 = calloc(1, 0x10000);
	R = g_bar0;
	BUG_ON(sizeof(struct unit_regs) > 0x20000);

	rc = al_dma_pci_probe(&g_pdev, NULL);
	BUG_ON(rc);
	D = pci_get_drvdata(&g_pdev);
}

static struct al_dma_chan *chan(int i) { return &D->channels[i]; }

static int alloc_chan(int i)
{
	return D->dma_dev.device_alloc_chan_resources(&chan(i)->chan);
}

static void free_chan(int i)
{
	D->dma_dev.device_free_chan_resources(&chan(i)->chan);
}

/* Run the cleanup tasklet while it is scheduled, at most `max` times (a held
 * queue makes it re-arm itself until the stall bound). */
static void run_tasklet(int i, int max)
{
	struct al_dma_chan *ch = chan(i);

	while (max-- > 0 && __atomic_exchange_n(&ch->cleanup_task.scheduled, 0, __ATOMIC_SEQ_CST))
		ch->cleanup_task.func(&ch->cleanup_task);
}

struct client {
	int cb, cb_result;
	enum dmaengine_tx_result last_result;
	u32 last_residue;
	dma_cookie_t last_completed_cookie;
	int order_violations;
	void (*in_cb)(struct client *);
	struct client *self;
};

static void client_cb(void *p)
{
	struct client *c = p;

	c->cb++;
	if (c->in_cb)
		c->in_cb(c);
}

static void client_cb_result(void *p, const struct dmaengine_result *r)
{
	struct client *c = p;

	c->cb_result++;
	c->last_result = r->result;
	c->last_residue = r->residue;
	if (c->in_cb)
		c->in_cb(c);
}

/* prep + submit one memcpy; returns the cookie or -1 if prep refused */
static dma_cookie_t submit_copy(int i, struct client *c, size_t len, bool want_result)
{
	struct dma_async_tx_descriptor *tx;

	tx = D->dma_dev.device_prep_dma_memcpy(&chan(i)->chan, 0x1000, 0x2000, len,
					       DMA_PREP_INTERRUPT);
	if (!tx)
		return -1;
	if (want_result)
		tx->callback_result = client_cb_result;
	else
		tx->callback = client_cb;
	tx->callback_param = c;
	return tx->tx_submit(tx);
}

static dma_cookie_t submit_xor(int i, struct client *c, unsigned int srcs, size_t len)
{
	struct dma_async_tx_descriptor *tx;
	dma_addr_t src[AL_DMA_MAX_XOR + 1];
	unsigned int k;

	for (k = 0; k < srcs; k++)
		src[k] = 0x10000 + k * 0x1000;
	tx = D->dma_dev.device_prep_dma_xor(&chan(i)->chan, 0x8000, src, srcs, len,
					    DMA_PREP_INTERRUPT);
	if (!tx)
		return -1;
	tx->callback = client_cb;
	tx->callback_param = c;
	return tx->tx_submit(tx);
}

static enum dma_status status_of(int i, dma_cookie_t ck)
{
	return D->dma_dev.device_tx_status(&chan(i)->chan, ck, NULL);
}

/* ---------------------------------------------------------------- tests */

static int t_smoke(void)
{
	struct client c = { 0 };
	dma_cookie_t ck;

	env_init();
	CHECK(alloc_chan(0) > 0, "alloc_chan_resources");
	ck = submit_copy(0, &c, 4096, false);
	CHECK(ck == 1, "first cookie is 1");
	CHECK(status_of(0, ck) == DMA_IN_PROGRESS, "in progress before issue");
	D->dma_dev.device_issue_pending(&chan(0)->chan);
	run_tasklet(0, 4);
	CHECK(c.cb == 1, "callback ran once");
	CHECK(status_of(0, ck) == DMA_COMPLETE, "complete after tasklet");
	CHECK(chan(0)->chan.completed_cookie == ck, "completed_cookie == cookie");
	CHECK(k_unmap_calls == 1, "descriptor unmapped (#278)");
	return fails;
}

/* #286: N clients prep+submit+issue while one thread plays the tasklet.
 * Cookie order must equal ring order, so completed_cookie never moves
 * backwards and every callback fires. */
#define STRESS_THREADS 4
#define STRESS_OPS 4000
static volatile int stress_stop;
static atomic64_t stress_cb;
static dma_cookie_t stress_last;
static int stress_backwards;

static void stress_client_cb(void *p)
{
	dma_cookie_t now = chan(0)->chan.completed_cookie;

	(void)p;
	if (now <= stress_last)
		stress_backwards++;
	stress_last = now;
	atomic64_inc(&stress_cb);
}

static void *stress_worker(void *arg)
{
	int n;

	(void)arg;
	for (n = 0; n < STRESS_OPS; n++) {
		struct dma_async_tx_descriptor *tx;

		for (;;) {
			tx = D->dma_dev.device_prep_dma_memcpy(&chan(0)->chan, 0x1000, 0x2000,
							       512, DMA_PREP_INTERRUPT);
			if (tx)
				break;
			D->dma_dev.device_issue_pending(&chan(0)->chan);
			sched_yield();
		}
		tx->callback = stress_client_cb;
		tx->callback_param = NULL;
		(void)tx->tx_submit(tx);
		D->dma_dev.device_issue_pending(&chan(0)->chan);
	}
	return NULL;
}

static void *stress_softirq(void *arg)
{
	struct al_dma_chan *ch = chan(0);

	(void)arg;
	while (!stress_stop) {
		if (__atomic_exchange_n(&ch->cleanup_task.scheduled, 0, __ATOMIC_SEQ_CST))
			ch->cleanup_task.func(&ch->cleanup_task);
		else
			sched_yield();
	}
	return NULL;
}

static int t_cookie_order_stress(void)
{
	pthread_t w[STRESS_THREADS], s;
	int i;
	long long total = (long long)STRESS_THREADS * STRESS_OPS;

	quiet = 1;
	env_init();
	alloc_chan(0);
	k_lock_yield = 1;
	pthread_create(&s, NULL, stress_softirq, NULL);
	for (i = 0; i < STRESS_THREADS; i++)
		pthread_create(&w[i], NULL, stress_worker, NULL);
	for (i = 0; i < STRESS_THREADS; i++)
		pthread_join(w[i], NULL);
	/* drain: every op is issued; keep the softirq thread going until done */
	for (i = 0; i < 2000000 && atomic64_read(&stress_cb) < total; i++)
		sched_yield();
	stress_stop = 1;
	pthread_join(s, NULL);
	k_lock_yield = 0;
	quiet = 0;
	CHECK(atomic64_read(&stress_cb) == total, "every op completed");
	CHECK(stress_backwards == 0, "completed_cookie never moved backwards (#286)");
	CHECK(chan(0)->chan.completed_cookie == chan(0)->chan.cookie,
	      "completed_cookie caught up to cookie");
	return fails;
}

/* A slot prepped and then dropped without tx_submit() carries cookie 0.
 * Completing it must not set completed_cookie to 0 (mainline BUG_ONs). */
static int t_cookie_zero_skipped(void)
{
	struct client c1 = { 0 }, c2 = { 0 };
	struct dma_async_tx_descriptor *tx;
	struct al_dma_chan *ch;
	dma_cookie_t ck1, ck2;
	int unmaps;

	env_init();
	alloc_chan(0);
	ch = chan(0);
	ck1 = submit_copy(0, &c1, 4096, false);
	D->dma_dev.device_issue_pending(&ch->chan);
	run_tasklet(0, 4);
	CHECK(c1.cb == 1 && ch->chan.completed_cookie == ck1, "first op completed");
	unmaps = k_unmap_calls;

	tx = D->dma_dev.device_prep_dma_memcpy(&ch->chan, 0x1000, 0x2000, 4096, 0);
	CHECK(tx != NULL, "prep ok");
	/* the client drops the descriptor: release the lock as tx_submit would,
	 * but assign no cookie */
	spin_unlock_irqrestore(&ch->lock, ch->prep_flags);
	CHECK(tx->cookie == 0, "dropped slot has cookie 0");
	D->dma_dev.device_issue_pending(&ch->chan);
	run_tasklet(0, 4);
	CHECK(ch->chan.completed_cookie == ck1, "completed_cookie did not move to 0");
	CHECK(status_of(0, ck1) == DMA_COMPLETE, "cookie 1 still reads COMPLETE");
	CHECK(k_unmap_calls == unmaps, "no unmap for a slot nobody submitted");
	CHECK(ch->completed == ch->tail, "slot was consumed");

	ck2 = submit_copy(0, &c2, 4096, false);
	D->dma_dev.device_issue_pending(&ch->chan);
	run_tasklet(0, 4);
	CHECK(ck2 == ck1 + 1 && c2.cb == 1, "next real op completes");
	CHECK(ch->chan.completed_cookie == ck2, "completed_cookie == next cookie");
	return fails;
}

/* Tx owned by the test: submit it so the lock taken by prep is released. */
static void finish(struct dma_async_tx_descriptor *tx) { (void)tx->tx_submit(tx); }

/* #290/#169: a prep the engine cannot encode returns NULL, takes no ring slot,
 * leaves ->lock released and is no fault; the limits are inclusive. */
static int prep_rejected(struct dma_async_tx_descriptor *tx, int head0)
{
	int ok = tx == NULL && chan(0)->head == head0 && !k_spin_held(&chan(0)->lock);

	if (tx)
		finish(tx);	/* an accepted prep holds the lock until submit */
	return ok;
}

static int t_len_limits(void)
{
	struct dma_chan *c;
	struct dma_async_tx_descriptor *tx;
	dma_addr_t src[AL_DMA_MAX_XOR + 1], dst[2] = { 0x8000, 0x9000 };
	unsigned char scf[AL_DMA_MAX_XOR + 1];
	struct dma_device *dd;
	int h;

	quiet = 1;
	env_init();
	alloc_chan(0);
	c = &chan(0)->chan;
	dd = &D->dma_dev;
	memset(scf, 1, sizeof(scf));
	for (h = 0; h < AL_DMA_MAX_XOR + 1; h++)
		src[h] = 0x10000 + h * 0x1000;

	h = chan(0)->head;
	CHECK(prep_rejected(dd->device_prep_dma_memcpy(c, 0x1000, 0x2000, 0, 0), h),
	      "memcpy len 0 rejected");
	CHECK(prep_rejected(dd->device_prep_dma_memcpy(c, 0x1000, 0x2000, 65537, 0), h),
	      "memcpy 64 KiB + 1 rejected");
	tx = dd->device_prep_dma_memcpy(c, 0x1000, 0x2000, 65536, 0);
	CHECK(tx != NULL, "memcpy 64 KiB accepted");
	if (tx)
		finish(tx);
	tx = dd->device_prep_dma_memcpy(c, 0x1000, 0x2000, 1, 0);
	CHECK(tx != NULL, "memcpy 1 byte accepted");
	if (tx)
		finish(tx);

	h = chan(0)->head;
	CHECK(prep_rejected(dd->device_prep_dma_xor(c, 0x8000, src, 2, 0, 0), h),
	      "xor len 0 rejected");
	CHECK(prep_rejected(dd->device_prep_dma_xor(c, 0x8000, src, 2, 16385, 0), h),
	      "xor 16 KiB + 1 rejected");
	CHECK(prep_rejected(dd->device_prep_dma_xor(c, 0x8000, src, 0, 4096, 0), h),
	      "xor 0 sources rejected");
	CHECK(prep_rejected(dd->device_prep_dma_xor(c, 0x8000, src, AL_DMA_MAX_XOR + 1, 4096, 0), h),
	      "xor 32 sources rejected");
	tx = dd->device_prep_dma_xor(c, 0x8000, src, AL_DMA_MAX_XOR, 16384, 0);
	CHECK(tx != NULL, "xor 31 sources x 16 KiB accepted");
	if (tx)
		finish(tx);

	h = chan(0)->head;
	CHECK(prep_rejected(dd->device_prep_dma_pq(c, dst, src, 3, scf, 8193, 0), h),
	      "pq 8 KiB + 1 rejected (#169)");
	CHECK(prep_rejected(dd->device_prep_dma_pq(c, dst, src, 3, scf, 16384, 0), h),
	      "pq 16 KiB rejected (#169)");
	CHECK(prep_rejected(dd->device_prep_dma_pq(c, dst, src, 3, scf, 8193,
						   DMA_PREP_PQ_DISABLE_P), h),
	      "q-only 8 KiB + 1 rejected");
	CHECK(prep_rejected(dd->device_prep_dma_pq(c, dst, src, 3, scf, 4096,
						   DMA_PREP_PQ_DISABLE_P | DMA_PREP_PQ_DISABLE_Q), h),
	      "pq with P and Q both disabled rejected");
	CHECK(prep_rejected(dd->device_prep_dma_pq(c, dst, src, 3, scf, 16385,
						   DMA_PREP_PQ_DISABLE_Q), h),
	      "p-only 16 KiB + 1 rejected");
	tx = dd->device_prep_dma_pq(c, dst, src, 3, scf, 8192, 0);
	CHECK(tx != NULL, "pq 8 KiB accepted");
	if (tx)
		finish(tx);
	tx = dd->device_prep_dma_pq(c, dst, src, 3, scf, 8192, DMA_PREP_PQ_DISABLE_P);
	CHECK(tx != NULL, "q-only 8 KiB accepted");
	if (tx)
		finish(tx);
	tx = dd->device_prep_dma_pq(c, dst, src, 3, scf, 16384, DMA_PREP_PQ_DISABLE_Q);
	CHECK(tx != NULL, "p-only 16 KiB accepted");
	if (tx)
		finish(tx);

	quiet = 0;
	CHECK(atomic64_read(&D->stat_errors) == 0, "rejects are not counted as errors");
	CHECK(g_fault[AL_DMA_FAULT_SUBMIT] == 0, "rejects raise no devlink fault");
	return fails;
}

/* #169: DMA_PQ stays withdrawn unless enable_pq=1 */
static int t_pq_cap_gate(void)
{
	env_init();
	CHECK(!dma_has_cap(DMA_PQ, D->dma_dev.cap_mask), "DMA_PQ not advertised by default");
	CHECK(dma_has_cap(DMA_MEMCPY, D->dma_dev.cap_mask) &&
	      dma_has_cap(DMA_XOR, D->dma_dev.cap_mask), "memcpy and xor advertised");
	enable_pq = true;
	env_init();
	CHECK(dma_has_cap(DMA_PQ, D->dma_dev.cap_mask), "enable_pq=1 advertises DMA_PQ");
	return fails;
}

struct test { const char *name; int (*fn)(void); };
static const struct test tests[] = {
	{ "smoke", t_smoke },
	{ "cookie_order_stress", t_cookie_order_stress },
	{ "cookie_zero_skipped", t_cookie_zero_skipped },
	{ "len_limits", t_len_limits },
	{ "pq_cap_gate", t_pq_cap_gate },
};

int main(int argc, char **argv)
{
	size_t i;

	setvbuf(stdout, NULL, _IOLBF, 0);	/* keep PASS lines if a BUG_ON aborts */
	if (argc < 2) {
		fprintf(stderr, "usage: %s <test>\n", argv[0]);
		return 2;
	}
	for (i = 0; i < ARRAY_SIZE(tests); i++) {
		if (!strcmp(argv[1], tests[i].name)) {
			int rc = tests[i].fn();

			printf("%s %s\n", rc ? "FAILED" : "OK", tests[i].name);
			return rc ? 1 : 0;
		}
	}
	fprintf(stderr, "no such test: %s\n", argv[1]);
	return 2;
}
