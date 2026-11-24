/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * In-kernel API of ubnt_hdd_pwrctl for modules/ubnt_shim/ui_hdd_pwrctl.
 * Also symlinked into modules/ubnt_shim/.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */
#ifndef UBNT_HDD_PWRCTL_H
#define UBNT_HDD_PWRCTL_H

#include <linux/notifier.h>
#include <linux/types.h>

/* Bay count, or -ENODEV when the driver is not bound. */
int ubnt_hdd_pwrctl_nbays(void);
/* Bay idx (0-based): vendor slot number, presence, fault LED state. */
int ubnt_hdd_pwrctl_bay_info(unsigned int idx, u32 *slot, bool *present,
			     bool *fault);
/* Set the amber fault LED of a slot; -ENOENT when unknown or LED-less. */
int ubnt_hdd_pwrctl_set_fault(u32 slot, bool on);
/* Notified (no payload) after any bay presence change. */
int ubnt_hdd_pwrctl_register_notifier(struct notifier_block *nb);
int ubnt_hdd_pwrctl_unregister_notifier(struct notifier_block *nb);

#endif
