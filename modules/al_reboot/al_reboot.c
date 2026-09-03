// SPDX-License-Identifier: GPL-2.0
/*
 * Annapurna Labs Alpine V2 (AL-324) reboot driver.
 *
 * Registers a restart handler that resets the SoC via the SP805 watchdog
 * (wdt0), the stock reboot mechanism. Fixes the Linux reboot hang (#51):
 * mainline 7.1 has no restart handler for the AL-324, so machine_restart()
 * finds nothing that resets, prints "Reboot failed" and halts (looks like a
 * hang at "Restarting system"); a power-cycle is the only recovery.
 *
 * Ported from the Ubiquiti GPL 4.1.37 drivers/power/reset/alpine-reboot.c: the stock
 * driver set the (since-removed) arm_pm_restart hook; we use the modern sys-off handler
 * framework instead. The SP805 register sequence is unchanged, and matches the
 * manual reset validated in scripts/reboot-to-uboot.tcl.
 *
 * wdt0 is SHARED with sp805_wdt (/dev/watchdog0), so this must map it WITHOUT
 * requesting the region. devm_of_iomap() does request it ("Requests a resource
 * and maps the memory mapped IO", lib/devres.c) and starved sp805_wdt with
 * -EBUSY, leaving the box with no watchdog device at all. We only write on
 * restart (LOAD=1 -> immediate reset), so a non-exclusive mapping is correct.
 */
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>

/* SP805 (ARM PrimeCell watchdog) register block */
#define WDTLOAD			0x000
#define WDTCONTROL		0x008
#define WDTLOCK			0xc00
#define WDT_LOAD_MIN		0x1		/* 1 tick -> reset almost immediately */
#define WDT_INT_ENABLE		BIT(0)
#define WDT_RESET_ENABLE	BIT(1)
#define WDT_UNLOCK		0x1acce551	/* magic to unlock WDTLOCK for writes */

struct al_reboot {
	void __iomem *wdt_base;
};

static int al_reboot_handler(struct sys_off_data *data)
{
	struct al_reboot *ar = data->cb_data;

	/* Arm the SP805 for an immediate reset: unlock, load 1 tick, enable the
	 * reset output. At the APB clock the reset fires in well under 1 us; the
	 * mdelay is only a guard so we don't return into machine_restart()'s
	 * fall-through before the SoC is gone. If it ever failed to reset we'd
	 * fall through to the legacy behaviour (no worse than today). */
	writel(WDT_UNLOCK, ar->wdt_base + WDTLOCK);
	writel(WDT_LOAD_MIN, ar->wdt_base + WDTLOAD);
	writel(WDT_INT_ENABLE | WDT_RESET_ENABLE, ar->wdt_base + WDTCONTROL);

	mdelay(50);
	return NOTIFY_DONE;
}

static int al_reboot_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *wdt_node;
	struct al_reboot *ar;
	struct resource wdt_res;
	int ret;

	ar = devm_kzalloc(dev, sizeof(*ar), GFP_KERNEL);
	if (!ar)
		return -ENOMEM;

	wdt_node = of_parse_phandle(dev->of_node, "wdt-parent", 0);
	if (!wdt_node)
		return dev_err_probe(dev, -ENODEV, "missing wdt-parent phandle\n");

	ret = of_address_to_resource(wdt_node, 0, &wdt_res);
	of_node_put(wdt_node);
	if (ret)
		return dev_err_probe(dev, ret, "no reg for SP805 watchdog\n");

	/* devm_ioremap(), NOT devm_ioremap_resource()/devm_of_iomap() - those
	 * request the region exclusively and lock sp805_wdt out (-EBUSY). */
	ar->wdt_base = devm_ioremap(dev, wdt_res.start, resource_size(&wdt_res));
	if (!ar->wdt_base)
		return dev_err_probe(dev, -ENOMEM, "cannot map SP805 watchdog\n");

	/* Must outrank PSCI. Our DT declares arm,psci-0.2, so the kernel
	 * registers psci_sys_reset_nb at notifier priority 129, but AL-324's
	 * firmware does not implement SYSTEM_RESET - it returns and the box
	 * hangs at "Restarting system" (#51). devm_register_restart_handler()
	 * would use SYS_OFF_PRIO_DEFAULT (0) and never run. */
	ret = devm_register_sys_off_handler(dev, SYS_OFF_MODE_RESTART,
					    SYS_OFF_PRIO_HIGH,
					    al_reboot_handler, ar);
	if (ret)
		return dev_err_probe(dev, ret, "cannot register restart handler\n");

	dev_info(dev, "AL-324 SP805 restart handler registered\n");
	return 0;
}

static const struct of_device_id al_reboot_of_match[] = {
	{ .compatible = "annapurna-labs,alpine-reboot" },
	{ }
};
MODULE_DEVICE_TABLE(of, al_reboot_of_match);

static struct platform_driver al_reboot_driver = {
	.probe = al_reboot_probe,
	.driver = {
		.name = "al-reboot",
		.of_match_table = al_reboot_of_match,
	},
};
module_platform_driver(al_reboot_driver);

MODULE_DESCRIPTION("Annapurna Labs Alpine (AL-324) SP805 restart handler");
MODULE_AUTHOR("awto");
MODULE_LICENSE("GPL");
