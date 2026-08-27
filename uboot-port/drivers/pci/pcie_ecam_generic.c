// SPDX-License-Identifier: GPL-2.0
/*
 * Generic PCIE host provided by e.g. QEMU
 *
 * Heavily based on drivers/pci/pcie_xilinx.c
 *
 * Copyright (C) 2016 Imagination Technologies
 */

#include <dm.h>
#include <pci.h>
#include <linux/ioport.h>
#include <linux/printk.h>

#include <asm/io.h>
#include <asm/ptrace.h>
#include <vsprintf.h>

#define TYPE_PCI 0x1

/*
 * #140 candidate fix: CRS (Configuration Request Retry Status) handling.
 *
 * A PCIe endpoint that hasn't finished its own internal boot yet (e.g. our
 * ASM1042A xHCI card, moments after PCIe link-up) can answer a Configuration
 * Read Request with CRS instead of real data - architecturally a normal,
 * expected "not ready yet, retry" signal. On this SoC (AL-324), per vendor's
 * own GPL source for this exact board family
 * (UBNT-source-code/UNVR-1.3.35-GPL/u-boot/board/annapurna-labs/common/
 * pci.c's al_pci_read_config32/readl_might_abort, cmd_cpu_misc.c's
 * abort_is_pending/soak_all_aborts/do_error), that CRS response surfaces as
 * a pending SError (asynchronous abort), not a transparent retry - and
 * nothing in this generic (QEMU-derived) ECAM driver knows to expect or
 * handle that. An unhandled SError here would otherwise panic via the
 * mainline do_error() default (arch/arm/lib/interrupts_64.c) or, if masked,
 * sit pending and potentially surface unpredictably later.
 *
 * Ported near-verbatim from vendor's own AArch64 branch of the same file
 * (their tree has both an ARMv7 CPSR-bit version and this ISR_EL1/HCR_EL2
 * AArch64 version side by side) - same registers, same mask/unmask/soak
 * sequence, adapted to mainline's do_error(struct pt_regs *) signature
 * (esr is a pt_regs field here, not a separate vendor-style 2nd argument).
 */
static volatile unsigned int crs_data_abort_is_silent;
static volatile unsigned int crs_num_data_aborts;

/* do_error() is __weak in arch/arm/lib/interrupts_64.c - this overrides the
 * default panic-on-any-SError behaviour, but ONLY while a CRS retry is
 * actively in progress (crs_data_abort_is_silent); any other SError still
 * falls through to a real panic via the normal weak-symbol mechanism not
 * applying here - if this override is linked in, ALL SErrors route here,
 * so unexpected ones must still panic rather than be silently dropped. */
void do_error(struct pt_regs *pt_regs)
{
	if (crs_data_abort_is_silent) {
		crs_num_data_aborts++;
		return;
	}

	printf("\"Error\" handler, esr 0x%08lx (unexpected - not a CRS retry)\n",
	       pt_regs->esr);
	show_regs(pt_regs);
	panic("Resetting CPU ...\n");
}

static int crs_abort_is_pending(void)
{
	unsigned long reg;

	asm volatile ("mrs %0, ISR_EL1\n" : "=r" (reg));
	return !!(reg & (1 << 8));	/* ISR_EL1.A - pending SError */
}

static void crs_aborts_enable(void)
{
	unsigned long reg;

	asm volatile("msr daifclr, #4" : : : "memory");	/* PSTATE.A = 0 */
	asm volatile("mrs %0, HCR_EL2\n" : "=r" (reg));
	asm volatile("orr %0, %0, #(1<<5)\n" : "=r" (reg));	/* HCR_EL2.AMO */
	asm volatile("msr HCR_EL2, %0\n" : "=r" (reg));
}

static void crs_aborts_disable(void)
{
	unsigned long reg;

	asm volatile("mrs %0, HCR_EL2\n" : "=r" (reg));
	asm volatile("bic %0, %0, #(1<<5)\n" : "=r" (reg));
	asm volatile("msr HCR_EL2, %0\n" : "=r" (reg));
}

/* Unmask SError long enough for a pending one to be taken (caught silently
 * by do_error() above) and cleared, then re-mask. Blocking - returns once
 * ISR_EL1.A confirms nothing is pending any more. */
static void crs_soak_all_aborts(void)
{
	crs_data_abort_is_silent = 1;
	mb();
	crs_aborts_enable();
	mb();
	while (crs_abort_is_pending())
		;
	mb();
	crs_aborts_disable();
	mb();
	crs_data_abort_is_silent = 0;
	crs_num_data_aborts = 0;
}

/* Read that treats a resulting SError as "device answered CRS, try again"
 * rather than a fatal fault. Caller-visible via the retry loop in
 * pci_generic_ecam_read_config() below - not exported further, this driver
 * is the only place that currently probes a device this early after its
 * own PCIe link-up. */
static u32 crs_readl_might_abort(void *addr, int *aborted)
{
	u32 val = readl(addr);

	mb();
	*aborted = crs_abort_is_pending();
	if (*aborted)
		crs_soak_all_aborts();

	return val;
}

/**
 * struct generic_ecam_pcie - generic_ecam PCIe controller state
 * @cfg_base: The base address of memory mapped configuration space
 */
struct generic_ecam_pcie {
	void *cfg_base;
	pci_size_t size;
	int first_busno;
};

/**
 * pci_generic_ecam_conf_address() - Calculate the address of a config access
 * @bus: Pointer to the PCI bus
 * @bdf: Identifies the PCIe device to access
 * @offset: The offset into the device's configuration space
 * @paddress: Pointer to the pointer to write the calculates address to
 *
 * Calculates the address that should be accessed to perform a PCIe
 * configuration space access for a given device identified by the PCIe
 * controller device @pcie and the bus, device & function numbers in @bdf. If
 * access to the device is not valid then the function will return an error
 * code. Otherwise the address to access will be written to the pointer pointed
 * to by @paddress.
 */
static int pci_generic_ecam_conf_address(const struct udevice *bus,
					 pci_dev_t bdf, uint offset,
					 void **paddress)
{
	struct generic_ecam_pcie *pcie = dev_get_priv(bus);
	void *addr;

	addr = pcie->cfg_base;

	if (dev_get_driver_data(bus) == TYPE_PCI) {
		addr += ((PCI_BUS(bdf) - pcie->first_busno) << 16) |
			 (PCI_DEV(bdf) << 11) | (PCI_FUNC(bdf) << 8) | offset;
	} else {
		addr += PCIE_ECAM_OFFSET(PCI_BUS(bdf) - pcie->first_busno,
					 PCI_DEV(bdf), PCI_FUNC(bdf), offset);
	}
	*paddress = addr;

	return 0;
}

static bool pci_generic_ecam_addr_valid(const struct udevice *bus,
					pci_dev_t bdf)
{
	struct generic_ecam_pcie *pcie = dev_get_priv(bus);
	int num_buses = DIV_ROUND_UP(pcie->size, 1 << 16);

	return (PCI_BUS(bdf) >= pcie->first_busno &&
		PCI_BUS(bdf) < pcie->first_busno + num_buses);
}

/**
 * pci_generic_ecam_read_config() - Read from configuration space
 * @bus: Pointer to the PCI bus
 * @bdf: Identifies the PCIe device to access
 * @offset: The offset into the device's configuration space
 * @valuep: A pointer at which to store the read value
 * @size: Indicates the size of access to perform
 *
 * Read a value of size @size from offset @offset within the configuration
 * space of the device identified by the bus, device & function numbers in @bdf
 * on the PCI bus @bus.
 */
static int pci_generic_ecam_read_config(const struct udevice *bus,
					pci_dev_t bdf, uint offset,
					ulong *valuep, enum pci_size_t size)
{
	if (!pci_generic_ecam_addr_valid(bus, bdf)) {
		*valuep = pci_get_ff(size);
		return 0;
	}

	/*
	 * #140 CRS handling, scoped exactly like vendor's own
	 * al_pci_read_config32 (see the big comment above): a secondary-bus
	 * (PCI_BUS(bdf) != 0), primary-function (PCI_FUNC(bdf) == 0) read is
	 * the "is a device here yet, and is it ready" probe for something
	 * downstream of the root port - the one case where the device might
	 * still answer CRS while finishing its own boot. Root-port-local
	 * (bus 0) and non-primary-function reads never hit this on vendor's
	 * hardware either, so they're left on the untouched fast path below.
	 *
	 * "Prime" with one abort-aware dword read at this exact address
	 * first, soaking/retrying through any CRS-triggered SError until the
	 * device responds cleanly - then fall through to the normal,
	 * unmodified read path, which will now succeed without aborting.
	 * Deliberately not reimplementing pci_generic_mmap_read_config()'s
	 * own byte/word/dword extraction logic - this only needs to clear
	 * the CRS condition once, not replace the real read.
	 */
	if (PCI_BUS(bdf) != 0 && PCI_FUNC(bdf) == 0) {
		void *addr;
		int aborted;
		unsigned int spins = 0;

		if (pci_generic_ecam_conf_address(bus, bdf, offset & ~3, &addr) == 0) {
			do {
				crs_readl_might_abort(addr, &aborted);
				if (aborted)
					spins++;
			} while (aborted);
			if (spins)
				printf("pcie-ecam-generic: bus %d dev %d func %d was CRS "
				       "(soaked %u SError%s before it answered)\n",
				       PCI_BUS(bdf), PCI_DEV(bdf), PCI_FUNC(bdf), spins,
				       spins == 1 ? "" : "s");
		}
	}

	return pci_generic_mmap_read_config(bus, pci_generic_ecam_conf_address,
					    bdf, offset, valuep, size);
}

/**
 * pci_generic_ecam_write_config() - Write to configuration space
 * @bus: Pointer to the PCI bus
 * @bdf: Identifies the PCIe device to access
 * @offset: The offset into the device's configuration space
 * @value: The value to write
 * @size: Indicates the size of access to perform
 *
 * Write the value @value of size @size from offset @offset within the
 * configuration space of the device identified by the bus, device & function
 * numbers in @bdf on the PCI bus @bus.
 */
static int pci_generic_ecam_write_config(struct udevice *bus, pci_dev_t bdf,
				    uint offset, ulong value,
				    enum pci_size_t size)
{
	if (!pci_generic_ecam_addr_valid(bus, bdf))
		return 0;

	return pci_generic_mmap_write_config(bus, pci_generic_ecam_conf_address,
					     bdf, offset, value, size);
}

/**
 * pci_generic_ecam_of_to_plat() - Translate from DT to device state
 * @dev: A pointer to the device being operated on
 *
 * Translate relevant data from the device tree pertaining to device @dev into
 * state that the driver will later make use of. This state is stored in the
 * device's private data structure.
 *
 * Return: 0 on success, else -EINVAL
 */
static int pci_generic_ecam_of_to_plat(struct udevice *dev)
{
	struct generic_ecam_pcie *pcie = dev_get_priv(dev);
	ofnode node = dev_ofnode(dev);
	struct resource reg_res;
	int err;

	err = ofnode_read_resource(node, 0, &reg_res);
	if (err < 0) {
		pr_err("\"reg\" resource not found\n");
		return err;
	}

	pcie->size = resource_size(&reg_res);
	pcie->cfg_base = map_physmem(reg_res.start, pcie->size, MAP_NOCACHE);

	return 0;
}

static int pci_generic_ecam_probe(struct udevice *dev)
{
	struct generic_ecam_pcie *pcie = dev_get_priv(dev);

	pcie->first_busno = dev_seq(dev);

	return 0;
}

static const struct dm_pci_ops pci_generic_ecam_ops = {
	.read_config	= pci_generic_ecam_read_config,
	.write_config	= pci_generic_ecam_write_config,
};

static const struct udevice_id pci_generic_ecam_ids[] = {
	{ .compatible = "pci-host-ecam-generic" /* PCI-E */ },
	{ .compatible = "pci-host-cam-generic", .data = TYPE_PCI },
	{ }
};

U_BOOT_DRIVER(pci_generic_ecam) = {
	.name			= "pci_generic_ecam",
	.id			= UCLASS_PCI,
	.of_match		= pci_generic_ecam_ids,
	.ops			= &pci_generic_ecam_ops,
	.probe			= pci_generic_ecam_probe,
	.of_to_plat	= pci_generic_ecam_of_to_plat,
	.priv_auto	= sizeof(struct generic_ecam_pcie),
};
