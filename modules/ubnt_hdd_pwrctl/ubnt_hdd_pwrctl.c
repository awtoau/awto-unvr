// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Ubiquiti UNVR drive-bay presence and fault-LED driver ("ui,hdd-pwrctl").
 *
 * The DT node described four bays and nothing bound it (#208), so bay
 * presence and the amber fault LEDs were invisible and unusable.
 *
 * Deliberately does NOT own the bay power-enable lines. Those are
 * reg_hdd_bay1-4 (regulator-fixed on the same PCA9575 pins 0-3) so AHCI has a
 * supplier to defer on - docs/sata-bay-power-ordering.md, #104. The vendor
 * 4.19 driver polled presence and toggled pwren itself; taking that here
 * would fight the regulators for the same gpios and break the probe order.
 *
 * Surfaces per bay:
 *   /sys/class/leds/hdd<slot>:amber:fault  fault LED (LED class)
 *   presence via gpio-keys-style poll -> sysfs "present" + a uevent on change
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>

#define UBNT_HDD_MAX_BAYS	4

/*
 * Presence poll interval. The PCA9575 has no interrupt wired on this board,
 * so presence can only be polled. A drive insertion is a human action on the
 * order of seconds and nothing downstream is latency-sensitive - AHCI does
 * its own hotplug - so 2 s is ample and keeps i2c traffic negligible
 * (4 reads / 2 s on a 400 kHz bus). On expiry it simply reads again.
 */
#define UBNT_HDD_POLL_MS	2000

struct ubnt_hdd_bay {
	struct ubnt_hdd_pwrctl	*pwrctl;
	struct gpio_desc	*present;
	struct gpio_desc	*fault_led;
	struct led_classdev	fault_cdev;
	char			fault_name[32];
	u32			slot;
	bool			present_state;
	bool			fault_registered;
};

struct ubnt_hdd_pwrctl {
	struct device		*dev;
	struct delayed_work	poll;
	unsigned int		nbays;
	struct ubnt_hdd_bay	bay[UBNT_HDD_MAX_BAYS];
};

/* _blocking: the LEDs hang off a PCA9575 over i2c, so the setter sleeps. */
static int ubnt_hdd_fault_set(struct led_classdev *cdev,
			      enum led_brightness value)
{
	struct ubnt_hdd_bay *bay = container_of(cdev, struct ubnt_hdd_bay,
						fault_cdev);

	gpiod_set_value_cansleep(bay->fault_led, value ? 1 : 0);
	return 0;
}

static ssize_t present_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct ubnt_hdd_pwrctl *pwrctl = dev_get_drvdata(dev);
	unsigned int i;
	int len = 0;

	for (i = 0; i < pwrctl->nbays; i++)
		len += sysfs_emit_at(buf, len, "%u %u\n", pwrctl->bay[i].slot,
				     pwrctl->bay[i].present_state);

	return len;
}
static DEVICE_ATTR_RO(present);

static struct attribute *ubnt_hdd_attrs[] = {
	&dev_attr_present.attr,
	NULL,
};
ATTRIBUTE_GROUPS(ubnt_hdd);

/* Poll presence; emit a uevent only on a change so udev can act on it. */
static void ubnt_hdd_poll_work(struct work_struct *work)
{
	struct ubnt_hdd_pwrctl *pwrctl = container_of(work,
					struct ubnt_hdd_pwrctl, poll.work);
	bool changed = false;
	unsigned int i;

	for (i = 0; i < pwrctl->nbays; i++) {
		struct ubnt_hdd_bay *bay = &pwrctl->bay[i];
		bool now = gpiod_get_value_cansleep(bay->present) > 0;

		if (now != bay->present_state) {
			bay->present_state = now;
			changed = true;
			dev_info(pwrctl->dev, "bay %u %s\n", bay->slot,
				 now ? "occupied" : "empty");
		}
	}

	if (changed)
		kobject_uevent(&pwrctl->dev->kobj, KOBJ_CHANGE);

	schedule_delayed_work(&pwrctl->poll,
			      msecs_to_jiffies(UBNT_HDD_POLL_MS));
}

static int ubnt_hdd_probe_bay(struct ubnt_hdd_pwrctl *pwrctl,
			      struct device_node *np, unsigned int idx)
{
	struct ubnt_hdd_bay *bay = &pwrctl->bay[idx];
	struct gpio_desc *led;
	int rc;

	bay->pwrctl = pwrctl;
	if (of_property_read_u32(np, "slot-no", &bay->slot))
		bay->slot = idx + 1;

	bay->present = devm_fwnode_gpiod_get_index(pwrctl->dev,
						   of_fwnode_handle(np),
						   "present", 0, GPIOD_IN,
						   "hdd-present");
	if (IS_ERR(bay->present))
		return dev_err_probe(pwrctl->dev, PTR_ERR(bay->present),
				     "bay %u: no present gpio\n", bay->slot);

	bay->present_state = gpiod_get_value_cansleep(bay->present) > 0;

	/* Fault LED is optional: a bay may legitimately have none. */
	led = devm_fwnode_gpiod_get_index(pwrctl->dev, of_fwnode_handle(np),
					  "fault-led", 0, GPIOD_OUT_LOW,
					  "hdd-fault-led");
	if (IS_ERR(led)) {
		if (PTR_ERR(led) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		return 0;
	}
	bay->fault_led = led;

	snprintf(bay->fault_name, sizeof(bay->fault_name),
		 "hdd%u:amber:fault", bay->slot);
	bay->fault_cdev.name = bay->fault_name;
	bay->fault_cdev.max_brightness = 1;
	bay->fault_cdev.brightness_set_blocking = ubnt_hdd_fault_set;
	bay->fault_cdev.default_trigger = "none";

	rc = devm_led_classdev_register(pwrctl->dev, &bay->fault_cdev);
	if (rc)
		return dev_err_probe(pwrctl->dev, rc,
				     "bay %u: LED register failed\n", bay->slot);
	bay->fault_registered = true;
	return 0;
}

static int ubnt_hdd_pwrctl_probe(struct platform_device *pdev)
{
	struct ubnt_hdd_pwrctl *pwrctl;
	unsigned int idx = 0;
	int rc;

	pwrctl = devm_kzalloc(&pdev->dev, sizeof(*pwrctl), GFP_KERNEL);
	if (!pwrctl)
		return -ENOMEM;

	pwrctl->dev = &pdev->dev;
	platform_set_drvdata(pdev, pwrctl);

	for_each_available_child_of_node_scoped(pdev->dev.of_node, np) {
		if (idx >= UBNT_HDD_MAX_BAYS) {
			dev_warn(&pdev->dev, "more than %d bays, ignoring rest\n",
				 UBNT_HDD_MAX_BAYS);
			break;
		}
		rc = ubnt_hdd_probe_bay(pwrctl, np, idx);
		if (rc)
			return rc;
		idx++;
	}

	if (!idx)
		return dev_err_probe(&pdev->dev, -ENODEV, "no bay nodes\n");

	pwrctl->nbays = idx;

	INIT_DELAYED_WORK(&pwrctl->poll, ubnt_hdd_poll_work);
	schedule_delayed_work(&pwrctl->poll,
			      msecs_to_jiffies(UBNT_HDD_POLL_MS));

	dev_info(&pdev->dev, "%u bays, presence polled every %u ms\n",
		 pwrctl->nbays, UBNT_HDD_POLL_MS);
	return 0;
}

static void ubnt_hdd_pwrctl_remove(struct platform_device *pdev)
{
	struct ubnt_hdd_pwrctl *pwrctl = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&pwrctl->poll);
}

static const struct of_device_id ubnt_hdd_pwrctl_match[] = {
	{ .compatible = "ui,hdd-pwrctl" },
	{ }
};
MODULE_DEVICE_TABLE(of, ubnt_hdd_pwrctl_match);

static struct platform_driver ubnt_hdd_pwrctl_driver = {
	.probe	= ubnt_hdd_pwrctl_probe,
	.remove	= ubnt_hdd_pwrctl_remove,
	.driver	= {
		.name		= "ubnt-hdd-pwrctl",
		.of_match_table	= ubnt_hdd_pwrctl_match,
		.dev_groups	= ubnt_hdd_groups,
	},
};
module_platform_driver(ubnt_hdd_pwrctl_driver);

MODULE_DESCRIPTION("Ubiquiti UNVR drive-bay presence and fault LEDs");
MODULE_AUTHOR("Daniel Tyrrell <dan@awto.au>");
MODULE_LICENSE("GPL");
