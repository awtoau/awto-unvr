# Board photo sweep — consolidated findings (2026-08-17)

Full Claude-vision pass over all 130 `docs/photos/*.jpg` (5 parallel agents, 26 each).
Per-photo detail: `tmp/logs/photo-catalog/batch-0*-findings.md`. Confirmed known chips
are in [hardware.md](hardware.md#physical-chip-ids--board-photo-catalog-2026-08-17); this
doc is the NEW / notable / open items.

## ★ JTAG lead — the strongest candidate yet

**An unpopulated 2-row plated-through-hole header on the AL-324 (U2) TOP EDGE**, by
silk `C24/C25/C7835/C7836`, amid the `TP34-49` cluster. **Independently flagged by two
batches** (photos `20260816_231327`, `_232437`, `_232446`, `_232501`). Geometry (a 2×7-8
header hugging the SoC) is a classic JTAG/SWD footprint — silk is plain TP numbers, no
signal names. **Needs a straight-down macro shot to confirm pinout.** Secondary
candidate: a **4-pin inline header near `FD5`** (pin-1 square pad, photo `_231436`-area).
No `TCK/TDO/TMS/TDI/TRST/TX/RX/BOOT` silk anywhere on the board.

## ⚠ The "Ethernet switch" question — UNRESOLVED (conflicting reads)

The board *may* carry an extra networking/controller chip, but the agents disagreed —
**do not treat as settled:**
- **`U1`** — a ~32-40-pin QFP with its **own oscillator `OSC1`**, in the **SATA/USB
  corner** next to the ASM1042A. Read variously as a **Marvell 88E6xxx managed switch**
  (batch 3) OR an **ASM1061-class PCIe→SATA / 2nd USB3 bridge** (batch 1). Marking worn
  on every frame. **Needs a clear macro to settle.**
- A batch-4 "**Atheros 48-pin switch**" (photo `_231702`) is **CONFIRMED = the AR8033
  1G PHY (U51)**, not a switch (inspected the photo): a ~48-pin Atheros QFN sitting by
  its **25 MHz crystal `Y8`** in the RTC-cluster area. A real Atheros/QCA switch
  (AR8327/QCA8337) is a **148+ pin** device — this is far too small. So **no second
  Atheros switch exists.**
- **Why it matters:** the AL-324 already has 2 native SATA controllers + 2 Ethernet MACs,
  so a NAS with **one RJ45 + one SFP** shouldn't need an extra switch or SATA controller.
  The owner's instinct ("only one port — why a switch?") is the right question. **U1's
  true identity is the open item** — a macro of U1 resolves it.

## New / notable parts (not previously cataloged)

- **`U1`** (see above) — mystery QFP + OSC1, SATA/USB corner. **Priority ID.**
- **Power tree fully mapped:** `VR1` = SoC-core VRM (uP1708/UB3 controller, `R002`=2 mΩ /
  `R005`=5 mΩ current-sense shunts, `1R0`/`R33` chokes, 330 µF/6.3 V + 100 µF/47 µF-35 V
  caps); `VR2` = Anpec-class buck (`ADEN`, 1R0 inductor); `VR7`, `VR13`; distributed
  load-switch/LDO net (`C07J` ×3, `AVW`, SOT-23 regs). No single big PMIC — point-of-load.
- **`Q53x` DPAK MOSFET at `MB_DCIN`** — main 12 V input power-path switch.
- **M-TEK `G241035XG`** — Gigabit LAN magnetics module (RJ45 side).
- **RTC coin cell = SII `MS621`/`HS621` rechargeable Li** (NOT a CR2032 — corrects earlier).
- Unidentified small ICs to chase: `U5052` (P617A MSOP), `U13` QFN (by reset), `U16`,
  `U48` (RPS ORing-monitor candidate), `UB17/UB18` (`ES41`), `UB21`, `UB14`.

## Connectors / named nets

- **`MB_DCIN`** — main 12 V DC input (Molex-type, ~4-pin; needs a straight-on shot for count).
- **`BP DCOUT1`/`BP_DCOUT2`** — backplane DC-power outputs; **`JB4` (RPS IN)** + `J84`.
- **`J86` "BP CONTROL"** — 2×8 (16-pin) backplane-control header (presence/LED/power-seq).
- **`J92` "RST BTN CONN"** — reset-button cable; `J87`/`J88`/`J44`/`J46` misc board-to-board.
- **Power-sequencing silk:** `HDD1/2/3/4_PWROFF`, `DDR2_PWROFF`, `PWROFF` — per-drive +
  DDR staggered power gating.
- PCB: contract-mfr **MSI**; UL `E248779 94V-0`.

## Test points

All **generic `TPnnn`** — no signal-named pads anywhere in 130 photos. Deliberate probe
clusters (unlabeled) worth a look: the **`TP34-49` group at the SoC** (with the JTAG-
candidate header above), a **6-TP row by the NAND** (`TP93-103` + series R), and
`TP287-292`. None are inline 0.1″/2 mm headers except the SoC-edge candidate.

## Open items (ranked)

1. **Macro the SoC-top-edge unpopulated header** → confirm/deny JTAG/SWD (our best lead).
2. **Macro `U1` (SATA/USB corner, by OSC1)** → Marvell switch vs ASM1061 vs other — settles
   the "is there a switch" question.
3. Macro `MB_DCIN` contacts (pin count) and the `RPS IN`/`JB4` blades (rail map).
4. ID `U48` (RPS ORing monitor), `U5052`, `U13`, `U16`.
