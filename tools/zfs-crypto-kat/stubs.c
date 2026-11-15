// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * Single-threaded stand-ins for the libzpool locking/cmn_err symbols libicp
 * references, so the KAT links without building libzpool.
 */
#include <stdarg.h>
#include <stdio.h>
#include <sys/zfs_context.h>

void
mutex_init(kmutex_t *mp, char *name, int type, void *cookie)
{
	(void) mp; (void) name; (void) type; (void) cookie;
}

void mutex_destroy(kmutex_t *mp) { (void) mp; }
void mutex_enter(kmutex_t *mp) { (void) mp; }
void mutex_exit(kmutex_t *mp) { (void) mp; }

void
cv_init(kcondvar_t *cv, char *name, int type, void *arg)
{
	(void) cv; (void) name; (void) type; (void) arg;
}

void cv_destroy(kcondvar_t *cv) { (void) cv; }
void cv_broadcast(kcondvar_t *cv) { (void) cv; }

void
cv_wait(kcondvar_t *cv, kmutex_t *mp)
{
	(void) cv; (void) mp;
	fprintf(stderr, "FATAL: cv_wait in single-threaded KAT\n");
	abort();
}

void
cmn_err(int ce, const char *fmt, ...)
{
	va_list ap;

	(void) ce;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}
