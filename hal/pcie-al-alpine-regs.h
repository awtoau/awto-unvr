/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Alpine V2 internal-PCIe SMCC/APP_CONTROL register definitions.
 *
 * Pure register data (offsets/masks/values), no kernel-specific types
 * or macros - shared verbatim, not just cross-checked, by this SoC's
 * three independent bootloader/firmware/OS ports:
 *   - Linux (canonical origin): drivers/pci/controller/pcie-al-internal.c
 *     in our fork (/mnt/2tb/unvr-port-refs/linux-v7.3-fresh,
 *     github.com/awto-au/linux), commit f755b2f5dc75.
 *   - U-Boot: uboot-port/board/annapurna/alpine/alpine.c (this repo) -
 *     scripts/uboot-build.py stages this exact file alongside it.
 *   - EDK2/UEFI: Platform/Ubiquiti/UNVR/Drivers/AlPcieSnoopFixDxe/
 *     (this repo) - included directly, no staging needed.
 *
 * This is the awto-unvr-side copy of the Linux fork's canonical file -
 * keep byte-identical; a plain diff against the Linux fork's copy is
 * the whole "audit" for this file (see
 * docs/audits/audit-edk2-pcie-glue.md, which still covers the
 * external-PCIe0 registers that have no Linux equivalent to share).
 *
 * Copyright (c) 2025 secfirstnas-rs project
 */

#ifndef PCIE_AL_ALPINE_REGS_H
#define PCIE_AL_ALPINE_REGS_H

/*
 * AXI Sub-Master Configuration & Control (SMCC) registers.
 * Located at PCI config space offsets within each device's 4KB window.
 *
 * Each device has up to 4 AXI sub-masters. Each sub-master's SMCC
 * register block is AL_ADAPTER_SMCC_BUNDLE_SIZE bytes apart.
 *
 * Sub-master 0: offset 0x110
 * Sub-master 1: offset 0x130
 * Sub-master 2: offset 0x150
 * Sub-master 3: offset 0x170
 */
#define AL_ADAPTER_SMCC			0x110
#define AL_ADAPTER_SMCC_BUNDLE_SIZE	0x20
#define AL_ADAPTER_SMCC_CONF_SNOOP_OVR	(1U << 0)
#define AL_ADAPTER_SMCC_CONF_SNOOP_EN	(1U << 1)
#define AL_ADAPTER_SMCC_SNOOP_BITS	(AL_ADAPTER_SMCC_CONF_SNOOP_OVR | \
					 AL_ADAPTER_SMCC_CONF_SNOOP_EN)

/* Number of AXI sub-masters per device */
#define AL_ADAPTER_SMCC_NUM_SUBMASTERS	4

/*
 * Application Control register.
 * The stock driver sets the lower 10 bits to 0x3ff on ALL devices,
 * and clears bits [15:10] (via MOVK instruction replacing the lower
 * 16 bits with 0x03ff, preserving upper 16 bits).
 */
#define AL_ADAPTER_APP_CONTROL		0x220
#define AL_ADAPTER_APP_CONTROL_LO16	0x03ff

/*
 * Device slot threshold for sub-master configuration:
 * - Slot <= 5: SMCC snoop is configured on ALL 4 sub-masters
 * - Slot >  5: SMCC snoop is configured on sub-master 0 only
 * ALL devices get APP_CONTROL configured regardless of slot.
 */
#define AL_INTERNAL_SLOT_THRESHOLD	5

/* Annapurna Labs PCI vendor ID */
#define PCI_VENDOR_ID_ANNAPURNA_LABS	0x1c36

#endif /* PCIE_AL_ALPINE_REGS_H */
