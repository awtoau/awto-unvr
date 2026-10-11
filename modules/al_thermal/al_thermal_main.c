/*
 * Annapurna Labs thermal driver.
 *
 * Copyright (C) 2013 Annapurna Labs
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/of_address.h>

#include "al_hal_thermal_sensor.h"

/*
 * Sensor power-up wait (probe, resume).
 *   waits for:  T_PWR_OK (about 50 us after power-up) and T_INIT_DONE (4 us
 *               after that) - al_hal_thermal_sensor_regs.h
 *   limit:      200 us (~3.7x), polled every 5 us; the poll counts its own
 *               delays, so preemption cannot expire it
 *   on expiry:  -ETIME, logged; probe/resume fails
 */
#define AL_THERMAL_READY_POLL_US	5
#define AL_THERMAL_READY_TIMEOUT_US	200

/*
 * get_temp wait for a fresh conversion (T_VALID, set per completed conversion).
 *   expected:   not documented for this sensor; armada_thermal.c uses 1 ms /
 *               100 ms (STATUS_POLL_PERIOD_US / _TIMEOUT_US) for its own
 *   on expiry:  -EIO, logged
 */
#define AL_THERMAL_VALID_POLL_US	1000
#define AL_THERMAL_VALID_TIMEOUT_US	100000

/* thermal_hwmon.h is private to drivers/thermal; the symbol is exported. */
int devm_thermal_add_hwmon_sysfs(struct device *dev,
				 struct thermal_zone_device *tz);

struct al_thermal_dev {
	struct al_thermal_sensor_handle handle;
};

static int thermal_enable(struct al_thermal_sensor_handle *handle)
{
	int ready, err;

	al_thermal_sensor_enable_set(handle, 1);

	err = read_poll_timeout_atomic(al_thermal_sensor_is_ready, ready, ready,
				       AL_THERMAL_READY_POLL_US,
				       AL_THERMAL_READY_TIMEOUT_US, false,
				       handle);
	if (err) {
		pr_err("%s: sensor not ready after %d us\n", __func__,
		       AL_THERMAL_READY_TIMEOUT_US);
		return -ETIME;
	}

	al_thermal_sensor_trigger_continuous(handle);

	return 0;
}

static int thermal_get_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct al_thermal_dev *al_dev = thermal_zone_device_priv(thermal);
	int valid, err;

	err = read_poll_timeout(al_thermal_sensor_readout_is_valid, valid, valid,
				AL_THERMAL_VALID_POLL_US,
				AL_THERMAL_VALID_TIMEOUT_US, false,
				&al_dev->handle);
	if (err) {
		pr_err_ratelimited("%s: no valid reading after %d us\n",
				   __func__, AL_THERMAL_VALID_TIMEOUT_US);
		return -EIO;
	}

	/* Sensor reads degrees C; the thermal core wants millidegrees. The
	 * vendor's >= 100 branch returned 100000 + temp1, capping the reported
	 * value near 100.25 C - which made the DT's 105 C critical trip
	 * unreachable (#279). */
	*temp = 1000 * al_thermal_sensor_readout_get(&al_dev->handle);

	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp = thermal_get_temp,
};

#ifdef CONFIG_PM
static int al_thermal_suspend(struct device *dev)
{
	struct al_thermal_dev *al_dev = dev_get_drvdata(dev);

	/* Disable Annapurna Labs Thermal Sensor */
	al_thermal_sensor_enable_set(&al_dev->handle, 0);

	pr_info("%s: Suspended.\n", __func__);

	return 0;
}

static int al_thermal_resume(struct device *dev)
{
	struct al_thermal_dev *al_dev = dev_get_drvdata(dev);
	int err;

	/* Enable Annapurna Labs Thermal Sensor */
	err = thermal_enable(&al_dev->handle);
	if (err) {
		pr_err("%s: thermal_enable failed!\n", __func__);
		return err;
	}

	pr_info("%s: Resumed.\n", __func__);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(al_thermal_pm_ops, al_thermal_suspend,
		al_thermal_resume);

/*
 * The PBS block is read once, for the chip revision that picks the conversion
 * constants; it is not kept mapped.
 */
static int al_thermal_handle_init(struct al_thermal_dev *al_dev,
				  void __iomem *thermal_base)
{
	struct device_node *np;
	void __iomem *pbs_reg_base;
	int err;

	np = of_find_compatible_node(NULL, NULL, "al,alpine-pbs");
	if (!np) {
		pr_err("%s: Failed: PBS DT data not available\n", __func__);
		return -EINVAL;
	}

	pbs_reg_base = of_iomap(np, 0);
	of_node_put(np);
	if (!pbs_reg_base) {
		pr_err("%s: failed to map PBS registers\n", __func__);
		return -ENOMEM;
	}

	err = al_thermal_sensor_handle_init(&al_dev->handle, thermal_base,
					    pbs_reg_base);
	iounmap(pbs_reg_base);

	return err;
}

/* Runs after the zone and hwmon are gone: devres releases in reverse order. */
static void al_thermal_disable(void *data)
{
	struct al_thermal_dev *al_dev = data;

	al_thermal_sensor_enable_set(&al_dev->handle, 0);
}

static int al_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *al_thermal;
	struct al_thermal_dev *al_dev;
	struct resource *al_res =
		platform_get_resource(pdev, IORESOURCE_MEM, 0);
	void __iomem *thermal_base;
	int err;

	if (!pdev->dev.of_node) {
		pr_err("%s: Failed: DT data not available\n", __func__);
		return -EINVAL;
	}

	if (!al_res) {
		pr_err("%s: memory resource missing\n", __func__);
		return -ENODEV;
	}

	al_dev = devm_kzalloc(&pdev->dev, sizeof(*al_dev), GFP_KERNEL);
	if (!al_dev) {
		pr_err("%s: kzalloc fail\n", __func__);
		return -ENOMEM;
	}

	thermal_base = devm_ioremap(&pdev->dev, al_res->start,
			resource_size(al_res));
	if (!thermal_base) {
		pr_err("%s: ioremap failed\n", __func__);
		return -ENOMEM;
	}

	err = al_thermal_handle_init(al_dev, thermal_base);
	if (err) {
		pr_err("%s: al_thermal_sensor_init failed!\n", __func__);
		return err;
	}

	err = thermal_enable(&al_dev->handle);
	if (err) {
		pr_err("%s: thermal_enable failed!\n", __func__);
		return err;
	}

	err = devm_add_action_or_reset(&pdev->dev, al_thermal_disable, al_dev);
	if (err)
		return err;

	al_thermal = devm_thermal_of_zone_register(&pdev->dev, 0, al_dev, &ops);
	if (IS_ERR(al_thermal)) {
		pr_err("%s: thermal zone device is NULL\n", __func__);
		return PTR_ERR(al_thermal);
	}

	if (devm_thermal_add_hwmon_sysfs(&pdev->dev, al_thermal) < 0)
		pr_warn("%s: could not add hwmon sysfs\n", __func__);

	platform_set_drvdata(pdev, al_dev);

	pr_info("%s: Thermal Sensor Loaded at: 0x%p.\n",
			__func__, thermal_base);

	return 0;
}

static const struct of_device_id al_thermal_id_table[] = {
	{ .compatible = "al,alpine-thermal" },
	{}
};
MODULE_DEVICE_TABLE(of, al_thermal_id_table);

static struct platform_driver al_thermal_driver = {
	.probe = al_thermal_probe,
	.driver = {
		.name = "al_thermal",
		.owner = THIS_MODULE,
		.pm = &al_thermal_pm_ops,
		.of_match_table = of_match_ptr(al_thermal_id_table),
	},
};

module_platform_driver(al_thermal_driver);

MODULE_DESCRIPTION("Annapurna Labs thermal driver");
MODULE_LICENSE("GPL");
