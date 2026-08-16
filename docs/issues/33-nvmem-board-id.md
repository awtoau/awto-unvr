# [Patch] NVMEM board identity (base MAC / board ID / hwrev), replace ubnthal
labels: porting, nvmem, devicetree, patch
---

Stage 10 board-identity half (the SGPO/LED half is live #22). Expose board identity
via **DT compatible + RO-EEPROM + NVMEM**, no `ubnthal.ko`, no casual identity writes.
Roadmap: [Stage 10 row](../porting-roadmap.md#14-stage-plan-reconciliation).

## What identity we need
- Base MAC address (assigned to eth ifaces via `nvmem-cells`).
- Board ID / systemid (**ea16**, confirmed live —
  [ubnthal.txt](../hw-reference/20260816-104601/ubnthal.txt)), hwrev, device ID.

## Tasks
- [ ] Model the identity EEPROM as an `nvmem` provider (RO), on the DW i2c bus.
- [ ] Board `compatible` carries the model; no runtime string parsing of ubnthal.
- [ ] `nvmem-cells` feed MAC to the ethernet nodes (works with the new eth driver,
  live #19/#21).
- [ ] EEPROM strictly read-only — no identity writes from Linux.

## Constraints
- Device MAC/serial are private; keep out of any public commit/log.
- Replaces the vendor `ubnthal` identity blob entirely.

## Acceptance criteria
- [ ] eth ifaces get the correct base MAC from NVMEM, no ubnthal.
- [ ] Board identifies via DT compatible; identity partition never written.

Relates: SGPO/LED (live #22), SoC dtsi (live #16), board DT (live #17).
