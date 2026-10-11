// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
/* Host harness for modules/al_thermal (built by tests/test_al_thermal.py).
 *
 * The REAL al_thermal_main.c is #included below and linked against the REAL
 * al_hal_thermal_sensor.c / al_hal_pbs_utils.c. Fake: the kernel
 * (tests/c/kshim/kshim_thermal.h) and the sensor, modelled from the register
 * description in al_hal_thermal_sensor_regs.h:
 *   - T_PWR_OK / T_INIT_DONE rise 54 us after the sensor is enabled
 *   - in continuous mode a conversion completes every conv_period_us
 *   - T_VALID is CLEAR ON READ and set per completed conversion
 *   - RESULT holds the last completed conversion; before the first one it
 *     holds whatever the previous power cycle left (stale_result)
 * One virtual microsecond clock is advanced by every wait. Tests are selected
 * by argv[1]; each prints PASS/FAIL lines and exits non-zero on FAIL.
 */
#include "al_thermal_main.c"
#include "al_hal_pbs_regs.h"
#include "al_hal_thermal_sensor_regs.h"

/* ------------------------------------------------------------ kshim ---- */
int k_err_count, k_warn_count;
unsigned long long k_now_us, k_udelay_us, k_sleep_us;
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

void udelay(unsigned long us)
{
	k_now_us += us;
	k_udelay_us += us;
}

void k_sleep(unsigned long us)
{
	k_now_us += us;
	k_sleep_us += us;
}

/* ----------------------------------------------------- sensor model ---- */
#define RAW_59C 1955 /* (59*10 + 1090) * 4096 / 3520: V2 constants */
#define NEVER (1ULL << 50)

static struct {
	uint32_t regs[4]; /* static_config, threshold, dynamic, status */
	uint32_t pbs[0x4000]; /* al_pbs_regs is far larger than we touch */

	int powered, running;
	unsigned long long power_up_at, run_start;
	unsigned long long conv_period_us;
	unsigned long long conv_consumed;
	unsigned int stale_result, cur_raw, result;
} sens;

/* event log, for ordering assertions on the remove path */
enum ev { EV_ZONE_UNREG = 1, EV_SENSOR_OFF, EV_HWMON_REMOVE };
static int evlog[16], nev;
static void ev(int e) { if (nev < 16) evlog[nev++] = e; }

uint32_t host_thermal_rd32(volatile void *a)
{
	uintptr_t off = (uintptr_t)a - (uintptr_t)sens.regs;

	if (off < sizeof(sens.regs)) {
		if (off == offsetof(struct al_thermal_sensor_unit, status)) {
			unsigned long long conv = 0;
			uint32_t s = 0;

			if (sens.powered && k_now_us >= sens.power_up_at + 54)
				s |= THERMAL_SENSE_UNIT_STATUS_T_PWR_OK |
				     THERMAL_SENSE_UNIT_STATUS_T_INIT_DONE;
			if (sens.running && sens.conv_period_us < NEVER)
				conv = (k_now_us - sens.run_start) / sens.conv_period_us;
			if (conv)
				sens.result = sens.cur_raw;
			if (conv > sens.conv_consumed) {
				s |= THERMAL_SENSE_UNIT_STATUS_T_VALID; /* clear on read: */
				sens.conv_consumed = conv;
			}
			return s | (sens.result & THERMAL_SENSE_UNIT_STATUS_T_RESULT_MASK);
		}
		return sens.regs[off / 4];
	}
	return *(volatile uint32_t *)a; /* PBS memory */
}

void host_thermal_wr32(volatile void *a, uint32_t v)
{
	uintptr_t off = (uintptr_t)a - (uintptr_t)sens.regs;

	if (off >= sizeof(sens.regs)) {
		*(volatile uint32_t *)a = v;
		return;
	}
	sens.regs[off / 4] = v;
	if (off == offsetof(struct al_thermal_sensor_unit, static_config)) {
		if ((v & THERMAL_SENSE_UNIT_STATIC_CONFIG_ENABLE) && !sens.powered) {
			sens.powered = 1;
			sens.power_up_at = k_now_us;
			sens.result = sens.stale_result; /* RESULT after a power cycle */
			sens.conv_consumed = 0;
		}
		if (v & THERMAL_SENSE_UNIT_STATIC_CONFIG_POWER_DOWN) {
			if (sens.powered)
				ev(EV_SENSOR_OFF);
			sens.powered = 0;
			sens.running = 0;
		}
	}
	if (off == offsetof(struct al_thermal_sensor_unit, dynamic_config)) {
		sens.running = !!(v & THERMAL_SENSE_UNIT_DYNAMIC_CONFIG_KEEP_RUNNING);
		if (sens.running) {
			sens.run_start = k_now_us;
			sens.conv_consumed = 0;
		}
	}
}

/* ------------------------------------------------- platform fakes ---- */
static struct device_node pbs_np;
static int pbs_maps, node_gets;
static int zone_regs, zone_unregs, hwmon_adds, hwmon_removes, hwmon_adds_nondevm;
static int fail_zone_register;
static struct thermal_zone_device the_zone;

#define MAX_DEVRES 16
static struct { void (*fn)(void *); void *data; } devres[MAX_DEVRES];
static int ndevres;

static void devres_add_(void (*fn)(void *), void *data)
{
	devres[ndevres].fn = fn;
	devres[ndevres].data = data;
	ndevres++;
}

static void devres_release_all(void)
{
	while (ndevres > 0) {
		ndevres--;
		devres[ndevres].fn(devres[ndevres].data);
	}
}

struct resource *platform_get_resource(struct platform_device *p, unsigned int type, unsigned int n)
{
	p->res.start = 0xfd860a00;
	p->res.end = 0xfd860a0f;
	return &p->res;
}
void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp) { return calloc(1, size); }
void __iomem *devm_ioremap(struct device *dev, unsigned long start, unsigned long size) { return sens.regs; }
int devm_add_action_or_reset(struct device *dev, void (*action)(void *), void *data)
{
	devres_add_(action, data);
	return 0;
}
struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compat)
{
	pbs_np.refs++;
	node_gets++;
	return &pbs_np;
}
void of_node_put(struct device_node *np) { np->refs--; }
void __iomem *of_iomap(struct device_node *np, int index)
{
	pbs_maps++;
	return sens.pbs;
}
void iounmap(void __iomem *a) { pbs_maps--; }

static void zone_release(void *data)
{
	thermal_zone_device_unregister(data);
}

struct thermal_zone_device *devm_thermal_of_zone_register(struct device *dev, int id, void *data,
							  const struct thermal_zone_device_ops *ops)
{
	if (fail_zone_register)
		return ERR_PTR(-EINVAL);
	the_zone.priv = data;
	the_zone.ops = ops;
	zone_regs++;
	devres_add_(zone_release, &the_zone);
	return &the_zone;
}

void thermal_zone_device_unregister(struct thermal_zone_device *tz)
{
	zone_unregs++;
	ev(EV_ZONE_UNREG);
}

int thermal_add_hwmon_sysfs(struct thermal_zone_device *tz)
{
	hwmon_adds++;
	hwmon_adds_nondevm++;
	return 0;
}

void thermal_remove_hwmon_sysfs(struct thermal_zone_device *tz)
{
	hwmon_removes++;
	ev(EV_HWMON_REMOVE);
}

static void hwmon_release(void *data) { thermal_remove_hwmon_sysfs(data); }

int devm_thermal_add_hwmon_sysfs(struct device *dev, struct thermal_zone_device *tz)
{
	hwmon_adds++;
	devres_add_(hwmon_release, tz);
	return 0;
}

/* ----------------------------------------------------------- tests ---- */
static int fails;

#define CHECK(c, what)                                                     \
	do {                                                               \
		if (c) {                                                   \
			printf("PASS %s\n", what);                         \
		} else {                                                   \
			printf("FAIL %s\n", what);                         \
			fails++;                                           \
		}                                                          \
	} while (0)

static struct platform_device pdev;
static struct al_thermal_dev *al_dev;

static int do_probe(void)
{
	int ret;

	memset(&sens, 0, sizeof(sens));
	memset(&pdev, 0, sizeof(pdev));
	memset(&pbs_np, 0, sizeof(pbs_np));
	pbs_maps = node_gets = 0;
	zone_regs = zone_unregs = hwmon_adds = hwmon_removes = hwmon_adds_nondevm = 0;
	ndevres = nev = 0;
	k_now_us = 1000000;
	k_udelay_us = k_sleep_us = 0;
	k_err_count = k_warn_count = 0;
	sens.conv_period_us = 2000; /* assumed; see the issue comment */
	sens.cur_raw = RAW_59C;
	sens.stale_result = 0;
	/* PBS chip id: Alpine V2 (the conversion constants differ per revision) */
	((struct al_pbs_regs *)sens.pbs)->unit.chip_id =
		PBS_UNIT_CHIP_ID_DEV_ID_ALPINE_V2 << PBS_UNIT_CHIP_ID_DEV_ID_SHIFT;
	pdev.dev.of_node = &pbs_np; /* any non-NULL node */

	quiet = 1;
	ret = al_thermal_driver.probe(&pdev);
	quiet = 0;
	al_dev = platform_get_drvdata(&pdev);
	return ret;
}

static int read_temp(int *t)
{
	return the_zone.ops->get_temp(&the_zone, t);
}

static void do_remove(void)
{
	if (al_thermal_driver.remove)
		al_thermal_driver.remove(&pdev);
	devres_release_all();
}

static int t_steady_state(void)
{
	int t = 0, ret;

	CHECK(do_probe() == 0, "probe succeeds");
	ret = read_temp(&t);
	CHECK(ret == 0 && t == 59000, "first reading waits for a conversion: 59000 mC");
	k_now_us += 1000000; /* the thermal core polls ~1 s apart */
	t = 0;
	ret = read_temp(&t);
	CHECK(ret == 0 && t == 59000, "next poll (T_VALID was cleared by the last read): 59000 mC");
	return 0;
}

/* #294: after a power cycle RESULT is stale; it must not become a reading. */
static int t_stale_result_after_resume(void)
{
	const struct dev_pm_ops *pm = al_thermal_driver.driver.pm;
	int t = 0, ret;

	if (do_probe())
		return 1;
	read_temp(&t);
	pm->suspend(&pdev.dev);

	sens.stale_result = 0x7ff; /* what a power cycle leaves: reads as ~66 C */
	pm->resume(&pdev.dev);
	ret = read_temp(&t);

	CHECK(ret == 0, "reading after resume succeeds once a conversion completes");
	CHECK(t == 59000, "...and is the NEW conversion, not the stale RESULT (66000 mC)");
	return 0;
}

static int t_no_conversion_is_eio(void)
{
	int t = 1234, ret;

	if (do_probe())
		return 1;
	read_temp(&t);
	sens.conv_period_us = NEVER; /* sensor stops converting */
	sens.running = 0;
	k_now_us += 1000000;
	t = 1234;
	quiet = 1;
	ret = read_temp(&t);
	quiet = 0;

	CHECK(ret == -EIO, "no valid conversion -> -EIO, not the last RESULT");
	CHECK(t == 1234, "...and *temp is untouched");
	return 0;
}

/* #280.4: the wait must sleep, and be bounded well under the vendor's 1 s. */
static int t_wait_sleeps_and_is_bounded(void)
{
	unsigned long long t0;
	int t = 0, ret;

	if (do_probe())
		return 1;
	sens.conv_period_us = NEVER;
	sens.running = 0;
	k_udelay_us = k_sleep_us = 0;
	t0 = k_now_us;
	quiet = 1;
	ret = read_temp(&t);
	quiet = 0;

	CHECK(ret < 0, "a sensor that never converts fails get_temp");
	CHECK(k_udelay_us == 0, "get_temp never busy-waits (udelay)");
	CHECK(k_now_us - t0 <= 150000, "...and gives up within ~100 ms, not 1 s");
	return 0;
}

/* #280.1: devres already unregisters the zone; the driver must not as well. */
static int t_remove_unregisters_once(void)
{
	if (do_probe())
		return 1;
	do_remove();

	CHECK(zone_unregs == 1, "the thermal zone is unregistered exactly once");
	CHECK(hwmon_adds == hwmon_removes, "hwmon sysfs add/remove balance");
	return 0;
}

static int t_sensor_off_after_zone(void)
{
	int zi = -1, si = -1, i;

	if (do_probe())
		return 1;
	CHECK(sens.powered, "sensor is powered after probe");
	do_remove();
	for (i = 0; i < nev; i++) {
		if (evlog[i] == EV_ZONE_UNREG)
			zi = i;
		if (evlog[i] == EV_SENSOR_OFF)
			si = i;
	}
	CHECK(!sens.powered, "sensor is powered down after remove");
	CHECK(zi >= 0 && si > zi, "...after the zone stopped polling it");
	return 0;
}

/* #280.2 */
static int t_pbs_mapping_not_leaked(void)
{
	if (do_probe())
		return 1;
	CHECK(pbs_maps == 0, "the PBS mapping is released after probe");
	CHECK(pbs_np.refs == 0, "the PBS device_node reference is dropped");
	return 0;
}

/* #280.3 */
static int t_hwmon_is_devm(void)
{
	if (do_probe())
		return 1;
	CHECK(hwmon_adds == 1, "hwmon attributes added");
	CHECK(hwmon_adds_nondevm == 0, "...through the devm interface");
	return 0;
}

/* a probe that fails after the sensor is up must not leave it running */
static int t_probe_failure_powers_down(void)
{
	int ret;

	fail_zone_register = 1;
	ret = do_probe();
	devres_release_all(); /* what the driver core does when probe fails */
	fail_zone_register = 0;

	CHECK(ret < 0, "zone registration failure fails probe");
	CHECK(!sens.powered, "...and the sensor is powered back down");
	return 0;
}

static const struct {
	const char *name;
	int (*fn)(void);
} tests[] = {
	{ "steady_state", t_steady_state },
	{ "stale_result_after_resume", t_stale_result_after_resume },
	{ "no_conversion_is_eio", t_no_conversion_is_eio },
	{ "wait_sleeps_and_is_bounded", t_wait_sleeps_and_is_bounded },
	{ "remove_unregisters_once", t_remove_unregisters_once },
	{ "sensor_off_after_zone", t_sensor_off_after_zone },
	{ "pbs_mapping_not_leaked", t_pbs_mapping_not_leaked },
	{ "hwmon_is_devm", t_hwmon_is_devm },
	{ "probe_failure_powers_down", t_probe_failure_powers_down },
};

int main(int argc, char **argv)
{
	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && strcmp(argv[1], tests[i].name))
			continue;
		tests[i].fn();
		if (argc > 1)
			return fails ? 1 : 0;
	}
	return argc > 1 ? 2 : (fails ? 1 : 0);
}
