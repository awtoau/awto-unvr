// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ubnthal shim: the /proc/ubnthal tree + a few exported getters, as the UniFi OS
 * userland expects them. Identity comes from the "EEPROM" MTD partition
 * (read through ubnt_common); no hardware work here.
 * Surface, field offsets and every TODO: docs/unifi-shim-interface.md (#383).
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */
#include <crypto/sha2.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <linux/unaligned.h>

#include "ubnt_shim.h"

static unsigned int flashsize = SZ_32M;
module_param(flashsize, uint, 0444);
MODULE_PARM_DESC(flashsize, "Reported flash size in bytes");
static unsigned int cpuid;
module_param(cpuid, uint, 0444);
MODULE_PARM_DESC(cpuid, "Reported cpuid (vendor reads SoC registers)");
static unsigned int boardrevision = 1;
module_param(boardrevision, uint, 0444);
MODULE_PARM_DESC(boardrevision, "Reported boardrevision");

#define EEPROM_LABEL	"EEPROM"
#define EE_MAC		0x00
#define EE_BOARDID	0x0c
#define EE_VENDORID	0x0e
#define EE_BOMREV	0x10
#define EE_HDR_LEN	0x14
#define NUM_ETH		2

struct ident {
	bool	valid;
	u8	mac[ETH_ALEN];
	u16	boardid;
	u16	vendorid;
	u32	bomrev;
};

static struct ident id;
static DEFINE_MUTEX(hal_lock);

static const struct {
	u16 boardid;
	const char *name, *shortname;
} boards[] = {
	{ 0xea16, "UniFi-NVR-4", "UNVR4" },
	{ 0xea1a, "UniFi-NVR-4", "UNVR4" },
	{ 0xea20, "Unifi-NVR-PRO", "UNVRPRO" },
};

static const char * const mesh_states[] = { "init", "wds", "eth", "down", "adopt" };

enum kind { K_BOOL, K_STR, K_INT, K_MESH };

struct status {
	const char *name;
	enum kind kind;
	bool writable;
	char val[64];
};

static struct status status_files[] = {
	{ .name = "IsDefault", .kind = K_BOOL, .writable = true, .val = "false" },
	{ .name = "IsLocated", .kind = K_BOOL, .writable = true, .val = "false" },
	{ .name = "IsIsolated", .kind = K_BOOL, .writable = true, .val = "false" },
	{ .name = "IsLte", .kind = K_BOOL, .val = "false" },
	{ .name = "ControllerHost", .kind = K_STR, .writable = true },
	{ .name = "ControllerPort", .kind = K_INT, .writable = true, .val = "0" },
	{ .name = "AnonNetAppId", .kind = K_STR, .writable = true },
	{ .name = "MeshStatus", .kind = K_MESH, .writable = true, .val = "init" },
};

static int scan_locked(void)
{
	u8 buf[EE_HDR_LEN];
	loff_t pos = 0;
	int rc;

	if (id.valid)
		return 0;
	rc = ubnt_mtd_partition_read(EEPROM_LABEL, buf, sizeof(buf), &pos);
	if (rc)
		return rc;
	memcpy(id.mac, buf + EE_MAC, ETH_ALEN);
	id.boardid = get_unaligned_be16(buf + EE_BOARDID);
	id.vendorid = get_unaligned_be16(buf + EE_VENDORID);
	id.bomrev = get_unaligned_be32(buf + EE_BOMREV);
	id.valid = true;
	return 0;
}

static int get_ident(struct ident *out)
{
	int rc;

	mutex_lock(&hal_lock);
	rc = scan_locked();
	if (!rc)
		*out = id;
	mutex_unlock(&hal_lock);
	return rc;
}

/* Our own derivation (vendor's hash is not reproduced): sha256(mac, tag). */
static void derive(const struct ident *i, const char *tag, u8 out[32])
{
	u8 in[ETH_ALEN + 16];
	size_t tl = strlen(tag);

	memcpy(in, i->mac, ETH_ALEN);
	memcpy(in + ETH_ALEN, tag, tl);
	sha256(in, ETH_ALEN + tl, out);
}

static unsigned long ram_bytes(void)
{
	return roundup((unsigned long)totalram_pages() << PAGE_SHIFT, SZ_1G);
}

static void emit_system(struct seq_file *m, const struct ident *i)
{
	const char *name = "unknown", *shortname = "unknown";
	u8 h[32], mac[ETH_ALEN];
	int n;

	for (n = 0; n < ARRAY_SIZE(boards); n++)
		if (boards[n].boardid == i->boardid) {
			name = boards[n].name;
			shortname = boards[n].shortname;
		}
	seq_printf(m, "cpuid=%08x\n", cpuid);
	seq_printf(m, "flashsize=%u\n", flashsize);
	seq_printf(m, "ramsize=%lu\n", ram_bytes());
	seq_printf(m, "vendorid=%04x\n", i->vendorid);
	seq_printf(m, "systemid=%04x\n", i->boardid);
	seq_printf(m, "shortname=%s\n", shortname);
	seq_printf(m, "name=%s\n", name);
	seq_printf(m, "boardrevision=%u\n", boardrevision);
	seq_printf(m, "serialno=%pM\n", i->mac);
	for (n = 0; n < NUM_ETH; n++) {
		ether_addr_copy(mac, i->mac);
		mac[ETH_ALEN - 1] += n;
		seq_printf(m, "eth%d.macaddr=%pM\n", n, mac);
	}
	derive(i, "hashid", h);
	seq_printf(m, "device.hashid=%8phN\n", h);
	derive(i, "anonid", h);
	h[6] = (h[6] & 0x0f) | 0x40;
	h[8] = (h[8] & 0x0f) | 0x80;
	seq_printf(m, "device.anonid=%pUb\n", h);
}

static int show_system(struct seq_file *m, void *v)
{
	struct ident i;
	int rc = get_ident(&i);

	if (rc)
		return rc;
	emit_system(m, &i);
	return 0;
}

static int show_board(struct seq_file *m, void *v)
{
	struct ident i;
	int rc = get_ident(&i);

	if (rc)
		return rc;
	emit_system(m, &i);
	seq_printf(m, "boardid=%04x\n", i.boardid);
	seq_printf(m, "bomrev=%08x\n", i.bomrev);
	seq_printf(m, "hwaddrbbase=%pM\n", i.mac);
	return 0;
}

static int show_factory(struct seq_file *m, void *v)
{
	seq_puts(m, "false\n");
	return 0;
}

static int show_status(struct seq_file *m, void *v)
{
	struct status *s = m->private;

	mutex_lock(&hal_lock);
	seq_printf(m, "%s\n", s->val);
	mutex_unlock(&hal_lock);
	return 0;
}

static int open_status(struct inode *inode, struct file *f)
{
	return single_open(f, show_status, pde_data(inode));
}

static bool parse_bool(const char *in, const char **out)
{
	if (!strcmp(in, "true") || !strcmp(in, "1")) {
		*out = "true";
		return true;
	}
	if (!strcmp(in, "false") || !strcmp(in, "0")) {
		*out = "false";
		return true;
	}
	return false;
}

static int store(struct status *s, char *in)
{
	const char *norm = in;
	int n, port;

	switch (s->kind) {
	case K_BOOL:
		if (!parse_bool(in, &norm))
			return -EINVAL;
		break;
	case K_INT:
		if (kstrtoint(in, 0, &port) || port < 0 || port > 65535)
			return -EINVAL;
		break;
	case K_MESH:
		for (n = 0; n < ARRAY_SIZE(mesh_states); n++)
			if (!strcmp(in, mesh_states[n]))
				break;
		if (n == ARRAY_SIZE(mesh_states))
			return -EINVAL;
		break;
	case K_STR:
		break;
	}
	mutex_lock(&hal_lock);
	strscpy(s->val, norm, sizeof(s->val));
	mutex_unlock(&hal_lock);
	return 0;
}

static ssize_t write_status(struct file *f, const char __user *ubuf,
			    size_t len, loff_t *off)
{
	struct status *s = pde_data(file_inode(f));
	char in[sizeof(s->val)];
	int rc;

	if (!len || len >= sizeof(in))
		return -EINVAL;
	if (copy_from_user(in, ubuf, len))
		return -EFAULT;
	in[len] = 0;
	strim(in);
	rc = store(s, in);
	return rc ? rc : len;
}

static const struct proc_ops status_ro_ops = {
	.proc_open = open_status,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops status_rw_ops = {
	.proc_open = open_status,
	.proc_read = seq_read,
	.proc_write = write_status,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static struct proc_dir_entry *root;

static int __init ubnthal_init(void)
{
	struct proc_dir_entry *sdir;
	int n;

	root = proc_mkdir("ubnthal", NULL);
	if (!root)
		return -ENOMEM;
	sdir = proc_mkdir("status", root);
	if (!sdir || !proc_create_single("system.info", 0444, root, show_system) ||
	    !proc_create_single("board", 0444, root, show_board) ||
	    !proc_create_single("factory", 0444, root, show_factory))
		goto err;
	for (n = 0; n < ARRAY_SIZE(status_files); n++) {
		struct status *s = &status_files[n];

		if (!proc_create_data(s->name, s->writable ? 0644 : 0444, sdir,
				      s->writable ? &status_rw_ops : &status_ro_ops, s))
			goto err;
	}
	return 0;
err:
	remove_proc_subtree("ubnthal", NULL);
	return -ENOMEM;
}

static void __exit ubnthal_exit(void)
{
	remove_proc_subtree("ubnthal", NULL);
}

#define GETTER(type, name, expr)				\
	type ubnthal_get_##name(void)				\
	{							\
		struct ident i;					\
		return get_ident(&i) ? 0 : (expr);		\
	}							\
	EXPORT_SYMBOL(ubnthal_get_##name)

GETTER(unsigned int, systemid, i.boardid);
GETTER(unsigned int, vendorid, i.vendorid);
GETTER(unsigned int, bomrev, i.bomrev);
unsigned int ubnthal_get_boardrevision(void) { return boardrevision; }
EXPORT_SYMBOL(ubnthal_get_boardrevision);
unsigned long ubnthal_get_flashsize(void) { return flashsize; }
EXPORT_SYMBOL(ubnthal_get_flashsize);
unsigned long ubnthal_get_ramsize(void) { return ram_bytes(); }
EXPORT_SYMBOL(ubnthal_get_ramsize);

module_init(ubnthal_init);
module_exit(ubnthal_exit);

MODULE_DESCRIPTION("Shim for the Ubiquiti ubnthal module");
MODULE_AUTHOR("Daniel Tyrrell <dan@awto.au>");
MODULE_LICENSE("GPL");
