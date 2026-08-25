# docs/ update pass — change-log (2026-08-17)

Applied the triage in `tmp/logs/doc-review.md` per owner brief. Rule: terse ≠ lossy —
moved facts to the single-source doc + cross-ref, never dropped a fact/number/path/gotcha.
Public repo: no new secrets or private `/mnt`/`/home` paths added. No doc deleted.

## 1. project-status.md — full rewrite to CURRENT state
- Dated 2026-08-16 → 2026-08-17.
- **Fedora 44 standalone-boot** moved In-flight → **Done** (NAND kernel @0x1300000 +
  DTB @0x2800000, SSD rootfs, no netboot/UEFI/GRUB; U-Boot can't read SATA). #40 closed.
- Added this session's deliverables as Done: components.md (130-photo BOM), rps-subsystem.md,
  gpio-switches-leds.md, I2C/SPI scan (i2c_gen disabled, 0x57 device), JTAG-candidate header.
- `al_reboot` SP805 driver → new "In flight / open" (written, untested, #51).
- Storage table rewritten: USB removed, SSD = sda (sda1 ESP / sda2 unvr-root PARTUUID),
  2×8TB = sdb/sdc. Naming-shift note kept.
- Next-list rewritten (al_reboot build/test first; physical macro chases added).
- Issue index updated: #38/#40/#47/#62 marked CLOSED; added #51/#60/#44/#49/#50.
- Reboot-hang gotcha (#51, power-cycle) added.

## 2. workflow.md — kept history, brought up to date (item 2)
- Title/intro reframed as the "cold-start bring-up narrative (2026-08-15)"; points to
  project-status.md for current state. Chronology (steps 1–14) untouched.
- §12 IP `192.168.25.140` (line ~484): kept as historical capture but annotated as the
  vendor-initramfs / U-Boot `ipaddr` tftp context; note the Fedora DHCP lease may differ,
  don't treat .140 as woomera's current IP. (Genericized the assertion, kept the number.)
- "Current state" table relabelled "(2026-08-15 snapshot — SUPERSEDED)"; done items marked
  (netboot exercised, USB unplugged, MTD dumps taken, Device now runs Fedora) with cross-refs.
- Appended "## Since then — milestones (2026-08-16 → 17)": kernel ports, Fedora standalone,
  boot-chain reverse, MTD dumps, hardware catalogue, al_reboot — all cross-referenced.

## 3. hardware.md — dedup vs components.md (item 3)
- Replaced the "Physical chip IDs — board-photo catalog" TABLE with a one-line pointer to
  components.md (master BOM). KEPT the cross-doc corrections / resolved-opens / reshoot notes
  below it (those are not in the BOM — no fact lost).
- Main-silicon SoC designator U1 → **U2** (silk), with note that `U1` is a separate
  unresolved QFP — now agrees with components.md.
- Multi-boot section: chip label "AR8031" → **AR8033** ("driver Atheros 8031" = mislabel,
  same PHY ID); verbatim driver string kept.
- Fan-chip breadcrumb: stale "recheck 7475 vs 7476" → resolved to **ADT7475** (per components.md).
- All other sections (storage-media, board-variants, MMIO/address map, boot-time obs, sensors,
  live-capture mapping) untouched.

## 4. Boot-chain duplication → nor-boot-chain.md canonical (item 4)
- **bootloader.md**: added a "Canonical boot chain … nor-boot-chain.md" banner at top
  (RE tables kept as the evidence view). Preboot TOC row + Open bullet marked RESOLVED
  (item 5, below). 1G-PHY Open bullet marked RESOLVED (AR8033).
- **boot-flow.md**: collapsed the "signature verification is skipped" block to a concise
  statement + cross-ref to nor-boot-chain.md §5 (full per-stage trust model). Added a
  canonical-env pointer (§4) above the env excerpt (excerpt kept for the netboot recipe).
- **uboot-update-path.md**: added canonical cross-ref to nor-boot-chain.md §1 after the TOC
  table; noted this doc's §Env is the single source the boot-chain doc links back to (so its
  env-delta table was NOT trimmed). No "honestly" found (concurrent editor already removed it).
- **nand-1.3.35.md**: labelled the mtd5 default-env as the 1.3.35/OLD env + cross-ref to
  nor-boot-chain.md §4 and uboot-update-path.md §Env. Unsigned-boot section: kept the
  1.3.35-specific fact (no bootsign at all) + cross-ref to §5 for the full trust model.
  (Facts verified present in the owner before adding refs; generation-specific content kept.)

## 5. bootloader.md — preboot TOC resolved (item 5)
- TOC table `preboot` row: was "off 0x080000 size 0 — does not match … unresolved ❓" →
  **RESOLVED**: off 0x000000 size 0x080000 (holds S2+al_boot+stage3 incl. payload @0x21000),
  cross-ref uboot-update-path.md#toc-correction / nor-boot-chain.md §1.
- "## Open" first bullet struck through / marked resolved (same cross-ref).

## 6. Device IP made generic (item 6)
- reboot-driver-handover.md:39 `192.168.25.140` → "on its LAN IP (DHCP lease … not the
  U-Boot ipaddr/tftp address)".
- workflow.md occurrences handled in item 2 above. No doc now asserts .140/.149 as woomera's
  current IP; .140 retained only where it is explicitly the U-Boot/tftp context.

## Ground-truth cross-check (all consistent after edits)
Fedora standalone-boots from NAND; U-Boot can't read SATA · NOR=MX25U25635F(32MB),
NAND=MT29F8G08(1GB), eMMC populated-but-hostless · 1G PHY=AR8033 (driver "8031") · no
Ethernet switch (U1 unresolved) · I2C mux=4-ch PCA9546 · i2c_gen disabled · gpio33/34=RPS
sense (not SW1/SW2) · ttyS2=RPS UART via MAX3221 (not BT) · RPS populated · no Bluetooth ·
DRAM=Samsung K4A8G165WB ×4 · SFP=Finisar FTLX8571 · reboot fix=al_reboot (untested, #51).

## Flagged only — NOT edited (per brief: keep history)
- **docs/issues/*.md** local drafts duplicate filed GitHub issues:
  - drafts 00–21 = GitHub #4–#25; drafts 31–37 = GitHub #31–#37 (superseded local copies).
  - **issues/README.md** still says "Gap drafts 31–37 … not filed — file as live #31+" —
    STALE (they are filed as #31–#37). Owner to update the not-filed claim.
  - **issues/38-ssh-normal-access.md** is git-untracked and its number collides with GitHub
    **#38** ("identify DRAM & SFP", a different issue). Content (woomera on its LAN IP,
    resolved) is fine; needs renumbering/filing separately.
- **chips/dram-ddr4.md** still "Part: unknown" while chips/README + components.md + hardware.md
  confirm Samsung K4A8G165WB ×4 — left to the active editor/owner to avoid a concurrent clobber.
- Banned word "honest/honestly": none found in docs/ now (already cleaned).
- Concurrent editor was active on nand-1.3.35.md during this pass (one edit reported a
  mid-flight on-disk change; re-read before the dependent edit — applied cleanly).
