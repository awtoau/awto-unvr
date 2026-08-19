# al_eth 1G TX: M2S engine ABORTs on the descriptor-read from DRAM (UNRESOLVED)

Port: our U-Boot v2026.07, chainloaded from stock, Alpine V2 / AL-324, eth1 (1G RGMII,
PCI 1c36:0001, rev_id 2). `ping`/`tftp` TX never completes.

## Symptom (on-box, reproducible)
- Every send: `TX completion timeout: 1 descs left after 100000 us`.
- Diagnostic dump (`al_eth_dm_dump_tx`, BAR0=0xfe000000):
  - `post-init`: `m2s.state=0x00000000` — engine CLEAN after init.
  - `doorbell`/`timeout`: `m2s.state=0x00002222` — all 4 M2S sub-engines (comp_ctrl/
    stream_if/data_rd/desc_pref) = 2 = ABORT.
  - `drtp` increments (doorbell lands) but `drhp`/`dcp`/`crhp` = 0, `q_tx_pkt` = 0.
- `dcp` never leaves 0 -> the engine NEVER fetches the descriptor. The ABORT is on the
  **descriptor-prefetch read** to `drbp = 0x00000000bfb59b80` (high DRAM, top of the
  U-Boot heap: relocaddr=0xbff54000, sp=0xbfb41650).
- M2S abort log `0xfe000214..0x220` = **all zeros** -> no response captured = a **pure
  AXI timeout** (~10 ms, gen_axi.cfg_1=5000000), NOT a DECERR/SLVERR. The read gets no
  responder.

## Ruled OUT on hardware (each = a build + cold-cycle + ping)
- **Snoop attribute** — eth SMCC (config 0x110, read back live at ECAM 0xfbc08110):
  `0x0` (default), `0x1` (SNOOP_OVR, non-coherent), `0x3` (SNOOP_OVR|EN, coherent) all
  abort identically.
- **FLR** — `al_eth_flr_rmn` before init present AND removed: both abort.
- **UDMA submaster config** — `desc_rd_cfg1=0x01030000`, `desc_rd_cfg2=0x32400000`,
  `ostand=0x10101010` = benign reset defaults, identical to stock (which works).
  `al_udma_m2s_axi_set` is never called for rev_id 2 in ours OR stock.
- **tgtid / tgtaddr** — not programmed for eth in ours or stock; disabled-by-default =
  raw address pass-through, cannot misroute 0xbfb59b80.
- **Queue reset / inherited-dirty state** — engine is clean at init (state=0), so there
  is nothing dirty to reset; `al_udma_state_set(DISABLE)` does NOT clear the ABORT
  (verified live: writing 0x2 to 0xfe000204 leaves state=0x2222).

## The decisive fact
**AHCI DMA to high DRAM WORKS** — `scsi scan` enumerates both 8 TB drives with full ATA
IDENTIFY (a DMA to a high-DRAM buffer), on the SAME bus-0 internal-PCIe fabric, coherent
(board `al_snoop_one` sets AHCI SMCC=0x3). So the fabric, CCI/CCU coherency, and
high-DRAM reachability are all functional. The failure is **specific to the eth UDMA
master** — it alone cannot get a response to a DRAM read.

## Low-address diagnostic — RUN (2026-08-19): master NOT routed to DRAM
Pinned the TX descriptor ring at low DRAM (desc_block = 0x10000000; confirmed live
`drbp=0x10000000, drbp_high=0`). Result: **still aborts identically** (state=0x2222,
dcp=0). So it is NOT address/window specific — **the eth UDMA master cannot reach DRAM at
any address**, while AHCI (same bus-0 internal-PCIe fabric) can.

Conclusion: the eth needs a per-function master-enable / routing that our ECAM-only PCI
path never applies and AHCI gets elsewhere. Prime suspects:
- **`al_unit_adapter_init` is STUBBED** (hal/.../al_eth_stubs.c) — it configures the eth
  unit's AXI master / ROB / fabric bridge; stubbed = the UDMA master is never wired to the
  fabric. `ap.unit_adapter=NULL` in al_eth_dm_dma_init skips it and logs "non optimal
  adapter configuration". This is the leading candidate now.
- The `al_hal_pcie` host-bridge bring-up our ECAM-only PCI path skips (alpine.c note) —
  stock's full host-bridge init may establish the eth function's inbound routing.
Next: read the REAL al_unit_adapter_init (delroth-alpine_hal) — what fabric/master regs it
programs — and either un-stub it for the eth unit or replicate the specific master-enable.

## Notes / gotchas
- **Never `md` 0xfe001038** (drtp_inc, write-only) — reading it data-aborts U-Boot.
- Test loop is gated by a deadlock: a failed ping leaves the eth in ABORT, which
  survives warm reset (change_state can't clear it), so stock's auto-chainload tftp then
  fails -> every build load needs a physical cold-cycle. Stock does not map the eth
  (BAR0 base=0, mem-space disabled), so it can't be un-wedged from the stock prompt.
- Files: `uboot-port/drivers/net/al_eth/al_eth_dm.c` (al_eth_dm_dma_init, the dump
  helper, al_eth_dm_snoop_setup), `hal/udma/al_hal_udma_main.c` (al_udma_set_defaults,
  al_udma_m2s_axi_set), `hal/udma/al_hal_udma_config.c`.
</content>
