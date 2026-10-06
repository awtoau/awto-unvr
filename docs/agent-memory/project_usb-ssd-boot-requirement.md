---
name: usb-ssd-boot-requirement
description: "Long-term requirement - the UNVR box should boot from the USB-attached SSD, not the internal SATA SSD; testing/benchmarking that path is a priority"
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-23T23:49:51.993Z
---

Long-term goal (stated 2026-08-24): the box should boot from the **USB-attached SSD**, not the internal SATA SSD it currently uses.

**Current state as of 2026-08-24:**
- A USB storage device is physically attached and confirmed working on **stock** U-Boot: ASMedia ASMT1051 USB-SATA bridge exposing a Micron 256GB SSD (`TFDDAK256TDL`, 238.4GB), enumerates cleanly via `usb start` -> `usb storage` at stock's `ALPINE_UBNT_NAS_ALL>` prompt.
- **Our custom U-Boot has zero USB support** (`uboot-port/configs/alpine_v2_unvr_defconfig` has no `CONFIG_USB*` symbols at all) - stock's xHCI driver proves the hardware path works, but our own port needs the Kconfig enabled (CONFIG_USB, USB_XHCI_HCD, USB_XHCI_PCI/DM_USB, USB_STORAGE, CMD_USB) before it can use it.
- Current durable boot path is the **internal SATA SSD** (Samsung 850, PARTUUID-based root) - see [[ssd-boot-blocked-stock-uboot-sata]].

**Why:** User's stated long-term requirement, not yet elaborated on further (reasoning not given - could be about swappability, avoiding the internal SATA bay for a data drive instead, or portability). Testing and benchmarking the USB SSD path is explicitly called out as important, not just "nice to have."

**How to apply:** When doing U-Boot port work ([[thorough-code-review-no-vendor-blind-trust]] applies to any USB driver porting too), prioritize enabling+testing USB storage boot on our own U-Boot over other secondary paths. Benchmark USB throughput (currently only confirmed at USB 2.0/480Mb/s negotiated speed via the ASMT1051 bridge - worth checking if that's a bridge limitation or an xHCI negotiation issue, since the controller itself reported "5 Gb/s" hub speed) against the existing SATA path before committing to it as the primary boot method.

**Authorized (2026-08-24):** the USB SSD (Micron 256GB via the ASMT1051 bridge) can be wiped and repurposed for booting - user's explicit go-ahead, not yet acted on.

**Real benchmark data (2026-08-24, stock U-Boot, filed as #137):** 128MiB `usb read` completed cleanly in 5.35s (~25 MB/s sustained) - meaningfully slower than the internal SATA SSD path (~62 MB/s, #92). A 512MiB `usb read` **hard hung stock U-Boot entirely** (no response to Ctrl-C or `reset` over serial, needed a physical/watchdog recovery). Root cause not yet known - could be U-Boot's USB storage driver, the ASMT1051 bridge, or an interaction. This is the bigger blocker: even setting throughput aside, a transfer-size hang would make USB boot unusable until understood or worked around (e.g. chunked reads).
