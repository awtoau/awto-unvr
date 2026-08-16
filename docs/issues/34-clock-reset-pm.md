# [Driver] Clock / reset-controller / PM (Stage 11)
labels: porting, clock, reset, pm
---

Stage 11. Clocks are mostly fixed (live #16 folds fixed-clocks into the dtsi); this
issue covers the **reset-controller driver** and **PM**, which are not otherwise
ticketed.
Roadmap: [Stage 11 row](../porting-roadmap.md#14-stage-plan-reconciliation).

## Clocks
- [ ] Clock inventory from live DT (refclk 100MHz, sbclk 500MHz, nbclk, cpuclk).
- [ ] CCF provider ONLY for clocks Linux must gate/rate-change; fixed-clock first.
  (No clock-controller node in stock DT → likely fixed-clocks suffice — confirm.)

## Reset controller
- [ ] Small reset-controller driver for blocks that need explicit reset assert/deassert
  (PCIe glue, SerDes, eth). Expose via `#reset-cells` so DT consumers use `resets=`.

## PM (last)
- [ ] cpuidle, runtime PM, PCIe/SATA link power management (LPM), suspend.
- [ ] Correctness before power savings; each added state re-runs the boot/IO tests.

## Out of scope
- **CPU overclock is SEPARATE** from this upstream bring-up series
  ([overclock-and-caps.md](../overclock-and-caps.md)) — do not fold OC in here.

## Acceptance criteria
- [ ] DT consumers resolve `resets=` against the new controller.
- [ ] PM states enabled one at a time, each with a clean boot + storage/net test.

Relates: SoC dtsi (live #16), PCIe glue (live #18).
