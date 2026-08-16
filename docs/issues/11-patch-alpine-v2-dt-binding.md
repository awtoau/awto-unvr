# [Patch 1] ARM64 DT binding for Alpine V2
labels: porting, devicetree, patch, upstream
---

First patch in the order. Binding docs for the Alpine V2 board/SoC compatibles.
Roadmap: [patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] `Documentation/devicetree/bindings/` entries for the UNVR board compatible and
  any new Alpine V2 node compatibles introduced by later patches.
- [ ] MAINTAINERS entry.
- [ ] Reuse existing `al,alpine-v2` where mainline already defines it (mainline
  `alpine-v2.dtsi` uses `compatible = "al,alpine-v2"`).

## Notes
- Vendor DT root is `annapurna-labs,alpine` / model "Annapurna Labs Alpine V2 UBNT"
  ([live.dts](../hw-reference/20260816-104601/live.dts)) — do NOT copy vendor
  compatibles blindly; align with mainline naming.

## Acceptance criteria
- [ ] `dtbs_check` passes for the new board DT (#13) against these bindings.

Blocks: #12, #13.
