---
name: mmio-dump-conflicting-mapping-crash
description: "scripts/mmio-dump/ (ioremap-based register dumper) crashes the kernel if the target region is already ioremap'd/pci_iomap'd by a live driver — don't dual-map"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-29T06:10:42.856Z
---

Tried `insmod mmio_dump.ko base=<addr already owned by a loaded driver>` on
woomera (2026-08-29, #23 investigation) to peek at al_dma's M2S queue
registers while al_dma.ko was loaded and already had that exact physical
range `pci_iomap()`'d. Result: `readl` inside mmio_dump triggered an EL1
SError (external abort) — full kernel panic, then the panic's own auto-reboot
hung too (matches [[no-plain-reboot-use-watchdog]]), requiring a hard power
cycle via the Sonoff outlet to recover.

**Why:** two independent `ioremap()`s of the same physical MMIO region can
get different memory attributes (device-nGnRnE vs device-nGnRE, etc.) unless
you're careful — the second mapping's access can fault at the bus level even
though the address is "the same" as one already working fine under the
owning driver's own mapping.

**How to apply:** `scripts/mmio-dump/` is genuinely useful for reading
registers a driver has NOT already claimed (its own README's use case: DDR
controller registers, nothing else has them mapped). Never point it at a
region a currently-loaded driver already owns via `ioremap`/`pci_iomap` —
check `/proc/iomem` first. To inspect a live driver's own already-mapped
registers safely, use a kprobe on a function that already dereferences the
SAME pointer the driver itself uses (proven working method same session —
attach at `/sys/kernel/tracing/kprobe_events`, fetch args via
`+offset(%xN)` chains, offsets from `gdb --batch -ex "print
&((struct foo*)0)->field"` against the built `.ko`'s DWARF info) — that
reads through the driver's existing, already-correct mapping instead of
creating a conflicting second one.
