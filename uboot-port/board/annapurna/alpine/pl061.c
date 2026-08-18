// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal ARM PL061 GPIO (DM) driver.
 *
 * Mainline U-Boot has NO PL061 driver (only DT bindings), yet the AL-324 SoC
 * GPIO banks are arm,pl061. This drives them so U-Boot can reach:
 *   - logo LEDs (ulogo_blue gpio3.7, ulogo_white gpio4.5)
 *   - RPS sense inputs (rps_prnt gpio4.1, 12v_lp gpio4.2)
 *   - reset button (gpio4.6) + SW1/SW2
 * Register model per the PL061 TRM: GPIODATA (0x000-0x3FC) is ADDRESS-MASKED —
 * bits [9:2] of the offset select which pins a read/write touches; GPIODIR
 * (0x400) 1=output. Reference: stock U-Boot drivers/gpio/al_gpio.c.
 */
#include <dm.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/errno.h>

#define PL061_DIR	0x400		/* 1 = output */

struct pl061_priv {
	void __iomem *base;
};

/* address-masked data access: only pin `off` is read/written here */
static inline void __iomem *pl061_data(struct pl061_priv *p, unsigned int off)
{
	return p->base + (1 << (off + 2));
}

static int pl061_get_value(struct udevice *dev, unsigned int off)
{
	struct pl061_priv *p = dev_get_priv(dev);

	return !!readl(pl061_data(p, off));
}

static int pl061_set_value(struct udevice *dev, unsigned int off, int val)
{
	struct pl061_priv *p = dev_get_priv(dev);

	writel(val ? (1 << off) : 0, pl061_data(p, off));
	return 0;
}

static int pl061_direction_input(struct udevice *dev, unsigned int off)
{
	struct pl061_priv *p = dev_get_priv(dev);

	writel(readl(p->base + PL061_DIR) & ~(1 << off), p->base + PL061_DIR);
	return 0;
}

static int pl061_direction_output(struct udevice *dev, unsigned int off, int val)
{
	struct pl061_priv *p = dev_get_priv(dev);

	pl061_set_value(dev, off, val);
	writel(readl(p->base + PL061_DIR) | (1 << off), p->base + PL061_DIR);
	return 0;
}

static int pl061_get_function(struct udevice *dev, unsigned int off)
{
	struct pl061_priv *p = dev_get_priv(dev);

	return (readl(p->base + PL061_DIR) & (1 << off)) ?
		GPIOF_OUTPUT : GPIOF_INPUT;
}

static const struct dm_gpio_ops pl061_ops = {
	.direction_input	= pl061_direction_input,
	.direction_output	= pl061_direction_output,
	.get_value		= pl061_get_value,
	.set_value		= pl061_set_value,
	.get_function		= pl061_get_function,
};

static int pl061_probe(struct udevice *dev)
{
	struct pl061_priv *p = dev_get_priv(dev);
	struct gpio_dev_priv *uc = dev_get_uclass_priv(dev);

	p->base = dev_read_addr_ptr(dev);
	if (!p->base)
		return -EINVAL;
	uc->gpio_count = 8;
	uc->bank_name = dev->name;
	return 0;
}

static const struct udevice_id pl061_ids[] = {
	{ .compatible = "arm,pl061" },
	{ }
};

U_BOOT_DRIVER(gpio_pl061) = {
	.name		= "gpio_pl061",
	.id		= UCLASS_GPIO,
	.of_match	= pl061_ids,
	.probe		= pl061_probe,
	.ops		= &pl061_ops,
	.priv_auto	= sizeof(struct pl061_priv),
};
