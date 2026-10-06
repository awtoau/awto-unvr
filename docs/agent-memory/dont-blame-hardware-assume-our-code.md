---
name: dont-blame-hardware-assume-our-code
description: "When something doesn't work, assume OUR code/config is at fault — never conclude \"faulty chip / dead cell / needs physical intervention\""
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-19T11:12:54.051Z
---

When debugging on awto-unvr (or any port), do NOT conclude the hardware is faulty — a bad chip, a flat/dead coin cell, "needs a physical power-cycle / pull the cell", etc. The default is that OUR code or config is wrong.

Concrete case: I repeatedly framed the s35390a RTC / ch0 i2c wedge as a hardware fault (chip latched, MS621 cell flat, "pull the coin cell to recover"). The user corrected sharply: **it is faulty CODE, not the chip and not the cell.** Stock Linux 5.1 talked to that exact RTC on that exact board — a part that answers under stock firmware is not faulty. The wedge is produced fresh by our code's ch0 handling (likely i2c SCL timing not matching stock's proven raw hcnt/lcnt counts). See [[i2c-rtc-sda-hold]] and docs/rtc-s35390a-fault.md (rewritten to say this).

**Why:** blaming hardware is a dead end — it stops the investigation and pushes an unfixable "physical intervention" conclusion, when the real fix is in code we control. It also wastes the user's time and reads as giving up. This is the same instinct as the project's [[our-uboot-assume-nothing]] rule.

**How to apply:** if there is a working reference (stock firmware, another OS, another device) that exercises the same hardware successfully, the hardware is good — the bug is the delta between that and our code/config. Find and close the delta. Only after every software/config avenue is exhausted AND there is positive evidence of a hardware defect may hardware be named — and even then, say "unconfirmed" and keep looking at code.
</content>
