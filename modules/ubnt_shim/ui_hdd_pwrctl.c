// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ui-hdd-pwrctl shim: the "ui-hdd-pwrctl" platform driver name (udev matches on
 * it) and the slot-N sysfs nodes the UniFi OS storage stack reads. Everything
 * is forwarded to modules/ubnt_hdd_pwrctl; no gpio or regulator access here.
 * Power is not gateable (always-on bay rails, docs/sata-bay-power-ordering.md),
 * so force_power is recorded only. Surface and TODOs: docs/unifi-shim-interface.md (#383).
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "ubnt_hdd_pwrctl.h"

#define MAX_SLOTS	4

static unsigned int pwr_default_on = 1;
module_param(pwr_default_on, uint, 0444);
MODULE_PARM_DESC(pwr_default_on, "Initial force_power of every slot");

struct slot {
	struct device	dev;
	u32		slot;
	bool		force_power;
};

struct shim {
	struct platform_device	*pdev;
	struct notifier_block	nb;
	unsigned int		nslots;
	struct slot		*slots[MAX_SLOTS];
};

static int slot_state(struct slot *s, bool *present, bool *fault)
{
	unsigned int i;
	u32 num;

	for (i = 0; i < MAX_SLOTS; i++) {
		if (ubnt_hdd_pwrctl_bay_info(i, &num, present, fault))
			break;
		if (num == s->slot)
			return 0;
	}
	return -ENODEV;
}

static ssize_t present_show(struct device *dev, struct device_attribute *a, char *buf)
{
	bool present, fault;
	int rc = slot_state(container_of(dev, struct slot, dev), &present, &fault);

	return rc ? rc : sysfs_emit(buf, "%d\n", present);
}
static DEVICE_ATTR_RO(present);

static ssize_t fault_show(struct device *dev, struct device_attribute *a, char *buf)
{
	bool present, fault;
	int rc = slot_state(container_of(dev, struct slot, dev), &present, &fault);

	return rc ? rc : sysfs_emit(buf, "%d\n", fault);
}

static ssize_t fault_store(struct device *dev, struct device_attribute *a,
			   const char *buf, size_t len)
{
	struct slot *s = container_of(dev, struct slot, dev);
	bool on;
	int rc = kstrtobool(buf, &on);

	if (!rc)
		rc = ubnt_hdd_pwrctl_set_fault(s->slot, on);
	return rc ? rc : len;
}
static DEVICE_ATTR_RW(fault);

static ssize_t led_state_show(struct device *dev, struct device_attribute *a, char *buf)
{
	bool present, fault;
	int rc = slot_state(container_of(dev, struct slot, dev), &present, &fault);

	return rc ? rc : sysfs_emit(buf, "%s\n", fault ? "on" : "off");
}

static ssize_t led_state_store(struct device *dev, struct device_attribute *a,
			       const char *buf, size_t len)
{
	struct slot *s = container_of(dev, struct slot, dev);
	bool on;
	int rc = kstrtobool(buf, &on);

	if (!rc)
		rc = ubnt_hdd_pwrctl_set_fault(s->slot, on);
	return rc ? rc : len;
}
static DEVICE_ATTR_RW(led_state);

static ssize_t force_power_show(struct device *dev, struct device_attribute *a, char *buf)
{
	return sysfs_emit(buf, "%d\n", container_of(dev, struct slot, dev)->force_power);
}

static ssize_t force_power_store(struct device *dev, struct device_attribute *a,
				 const char *buf, size_t len)
{
	struct slot *s = container_of(dev, struct slot, dev);
	bool on;
	int rc = kstrtobool(buf, &on);

	if (rc)
		return rc;
	s->force_power = on;
	return len;
}
static DEVICE_ATTR_RW(force_power);

static struct attribute *slot_attrs[] = {
	&dev_attr_present.attr,
	&dev_attr_fault.attr,
	&dev_attr_led_state.attr,
	&dev_attr_force_power.attr,
	NULL,
};
ATTRIBUTE_GROUPS(slot);

enum cnt { CNT_POWER, CNT_FAULT, CNT_PRESENT };

static unsigned int count(enum cnt what)
{
	unsigned int i, n = 0;
	bool present, fault;
	u32 num;

	for (i = 0; i < MAX_SLOTS; i++) {
		if (ubnt_hdd_pwrctl_bay_info(i, &num, &present, &fault))
			break;
		n += what == CNT_POWER || (what == CNT_FAULT ? fault : present);
	}
	return n;
}

#define CNT_ATTR(name, what)						\
	static ssize_t name##_show(struct device *dev,			\
				   struct device_attribute *a, char *buf) \
	{								\
		return sysfs_emit(buf, "%u\n", count(what));		\
	}								\
	static DEVICE_ATTR_RO(name)

CNT_ATTR(power_cnt, CNT_POWER);
CNT_ATTR(fault_cnt, CNT_FAULT);
CNT_ATTR(present_cnt, CNT_PRESENT);

static struct attribute *platform_attrs[] = {
	&dev_attr_power_cnt.attr,
	&dev_attr_fault_cnt.attr,
	&dev_attr_present_cnt.attr,
	NULL,
};
ATTRIBUTE_GROUPS(platform);

static void slot_release(struct device *dev)
{
	kfree(container_of(dev, struct slot, dev));
}

/* Our driver reports a presence change: re-emit it from the ui-hdd-pwrctl device. */
static int presence_changed(struct notifier_block *nb, unsigned long ev, void *p)
{
	struct shim *shim = container_of(nb, struct shim, nb);

	kobject_uevent(&shim->pdev->dev.kobj, KOBJ_CHANGE);
	return NOTIFY_OK;
}

static void remove_slots(struct shim *shim)
{
	while (shim->nslots)
		device_unregister(&shim->slots[--shim->nslots]->dev);
}

static int shim_probe(struct platform_device *pdev)
{
	struct shim *shim;
	int nbays, i, rc;

	nbays = ubnt_hdd_pwrctl_nbays();
	if (nbays < 0)
		return -EPROBE_DEFER;

	shim = devm_kzalloc(&pdev->dev, sizeof(*shim), GFP_KERNEL);
	if (!shim)
		return -ENOMEM;
	shim->pdev = pdev;
	platform_set_drvdata(pdev, shim);

	for (i = 0; i < nbays && i < MAX_SLOTS; i++) {
		struct slot *s = kzalloc(sizeof(*s), GFP_KERNEL);
		bool present, fault;

		if (!s) {
			rc = -ENOMEM;
			goto err;
		}
		rc = ubnt_hdd_pwrctl_bay_info(i, &s->slot, &present, &fault);
		if (rc) {
			kfree(s);
			goto err;
		}
		s->force_power = pwr_default_on;
		s->dev.parent = &pdev->dev;
		s->dev.groups = slot_groups;
		s->dev.release = slot_release;
		dev_set_name(&s->dev, "slot-%u", s->slot);
		rc = device_register(&s->dev);
		if (rc) {
			put_device(&s->dev);
			goto err;
		}
		shim->slots[shim->nslots++] = s;
	}

	shim->nb.notifier_call = presence_changed;
	rc = ubnt_hdd_pwrctl_register_notifier(&shim->nb);
	if (rc)
		goto err;
	dev_info(&pdev->dev, "%u slots forwarded to ubnt_hdd_pwrctl\n", shim->nslots);
	return 0;
err:
	remove_slots(shim);
	return rc;
}

static void shim_remove(struct platform_device *pdev)
{
	struct shim *shim = platform_get_drvdata(pdev);

	ubnt_hdd_pwrctl_unregister_notifier(&shim->nb);
	remove_slots(shim);
}

static struct platform_driver shim_driver = {
	.probe	= shim_probe,
	.remove	= shim_remove,
	.driver	= {
		.name		= "ui-hdd-pwrctl",
		.dev_groups	= platform_groups,
	},
};

static struct platform_device *shim_pdev;

static int __init shim_init(void)
{
	int rc = platform_driver_register(&shim_driver);

	if (rc)
		return rc;
	shim_pdev = platform_device_register_simple("ui-hdd-pwrctl", PLATFORM_DEVID_NONE,
						    NULL, 0);
	if (IS_ERR(shim_pdev)) {
		platform_driver_unregister(&shim_driver);
		return PTR_ERR(shim_pdev);
	}
	return 0;
}

static void __exit shim_exit(void)
{
	platform_device_unregister(shim_pdev);
	platform_driver_unregister(&shim_driver);
}

module_init(shim_init);
module_exit(shim_exit);

MODULE_DESCRIPTION("Shim for the Ubiquiti ui-hdd-pwrctl module");
MODULE_AUTHOR("Daniel Tyrrell <dan@awto.au>");
MODULE_LICENSE("GPL");
