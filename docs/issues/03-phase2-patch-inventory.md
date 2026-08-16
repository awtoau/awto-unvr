# [Phase 2] Inventory & classify the existing patch set
labels: porting, phase-2, analysis
---

Classify every relevant file across the vendor trees, so we know what is genuinely
Annapurna-specific vs. what mainline already handles.
Roadmap: [Phase 2](../porting-roadmap.md#phase-2--inventory-the-existing-patch-set).

## Sources to inventory
- `UrNVR/unvr-kernel` (Ubiquiti GPL 4.19.152)
- `bcyangkmluohmars/linux-alpine-v2` (6.12 port)
- `delroth/al_eth-standalone`, `delroth/alpine_hal`, `delroth/linux-qnap-tsx32x`
- See [docs/prior-art.md](../prior-art.md), [docs/porting-reference.md](../porting-reference.md).

## Classification buckets (per file)
- [ ] already mainline
- [ ] mainline driver needing a compatible string
- [ ] mainline driver needing Alpine quirks
- [ ] shared third-party IP with register differences
- [ ] genuinely Annapurna-specific
- [ ] obsolete board policy that belongs in DT
- [ ] unnecessary for the UNVR

## Rules
- Compatible strings must describe **actually-compatible** hardware — do not label
  the MAC "Altera"/"NXP" on resemblance (that's #01's job to establish).

## Live baseline (genuinely custom, no mainline driver)
`al_eth`, `al_dma`, `al_ssm`, `al_nand`, `al_thermal`, `al-sgpo`, internal-PCIe
glue. Everything else in the mapping table is mainline —
[hardware.md](../hardware.md#live-capture-mainline-driver-mapping).

## Acceptance criteria
- [ ] A table (finding doc) with every file bucketed + one-line rationale.
- [ ] The "genuinely Annapurna-specific" bucket becomes the driver work list.
