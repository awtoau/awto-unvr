---
name: our-uboot-assume-nothing
description: our U-Boot must do ALL hardware init itself — never rely on stock/inbuilt U-Boot or preboot state
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-18T01:06:14.869Z
---

**Our U-Boot assumes NOTHING. It does every hardware-init step itself** — never rely on the stock (factory 2015.07) U-Boot, the inbuilt/mainline defaults, or the preboot (S2/al_boot) having already set something up.

**Why:** the end goal is a STANDALONE U-Boot (flashed to NOR, or entered straight from the preboot). Anything it inherits from a chainload disappears the moment it runs standalone. A chainload test where stock U-Boot already enumerated PCI + assigned the AHCI BAR at 0xfe0f0000 is a **crutch that hides missing init** — the test "passes" but the standalone build fails.

**How to apply:**
- Bring up the internal PCIe host ourselves (port `pcie-al-internal.c`, ~7 KB), enumerate PCI, assign BARs, configure AXI snoop (SMCC) — do NOT bind AHCI to a hardcoded MMIO BAR and hope it's live.
- For every subsystem (DDR, PCIe, SATA, NAND, eth): port/write the real init, don't assume the preboot left it usable. Verify from a cold/standalone path, not just chainload.
- Chainload is fine as a *fast iteration harness*, but a feature isn't done until it works from a from-scratch init with nothing pre-set. Test both.

See [[ssd-boot-blocked-stock-uboot-sata]] (the SATA case that surfaced this), [[test-every-change-never-assert]].
