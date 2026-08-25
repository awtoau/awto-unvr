# docs/ review — duplication + legacy/stale triage (2026-08-17)

Scope: every `.md` under `docs/` (~30 top-level + `chips/` + `issues/` + `hw-reference/`).
Ground-truth enforced from the task brief (Fedora-standalone-on-woomera, storage,
PHY=AR8033, gpio 33/34=RPS, ttyS2=RPS UART, cpu downclock-only, RPS populated, no BT,
reboot fix #51). ✅ APPLIED = safe fix done in this pass. Everything else is a
recommendation for the owner.

## ⚠ Concurrent editing during this review

The docs tree was being **actively restructured by another process while I reviewed**
(mtimes 15:55-16:14 today): `hardware.md`, `chips/README.md` rewritten, and a **new
`docs/components.md` "master component/connector/test-point catalog"** created at 16:12.
Several findings below were resolved mid-review (chips DRAM + SFP parts now identified).
My snapshot is therefore partly overtaken; I did NOT edit the files under active
restructure (`hardware.md`, `chips/`, `components.md`) to avoid clobbering that work.

New duplication axis introduced: **`components.md` (new master BOM) vs `hardware.md`
"Physical chip IDs — board-photo catalog" vs `chips/`** — three now overlap; components.md
claims single-source but hardware.md still carries its own chip catalog. Also a new
consistency note surfaced: components.md says **SoC silk is U2, not U1** (hardware.md
labels it "U1" as shorthand) and **`U1` is a separate unresolved QFP** — the two docs
must agree.

## Fixes APPLIED this pass (safe/unambiguous)

| File:line | Was | Now |
|---|---|---|
| `sources.md`:31 | UNVR "Ours. Board ID `ea1a` ✅" | `ea16` (no-eMMC variant; eMMC UNVR is ea1a) — contradicted ground truth |
| `porting-reference.md`:30 | "Board ID `ea1a` (UNVR)" | `ea16` (this UNVR; port targets ea1a) — roadmap:115 already flags the ea1a guess superseded |

## Per-doc table

status: current / stale / duplicate / legacy · action: keep / update / merge / trim / delete / flag

| Doc | Status | Issues found (line refs) | Recommended action |
|---|---|---|---|
| **project-status.md** | **STALE** | Dated 2026-08-16. "In flight: Fedora … Rootfs building on host now" (28-30) and "Next: Deploy Fedora to SSD" (44-51) are **DONE** — Fedora 44 boots standalone from NAND on woomera (fedora-on-ssd.md §PERSISTENT, 2026-08-17). Storage table (33-40) shows USB still `sda` "to be unplugged"; ground truth = USB removed, SSD is sda. Issue index (61-71) predates #43-#62. | **update** — rewrite Done/In-flight/Next/storage to current; refresh issue index. Highest-value stale doc. |
| **hardware.md** | current (mostly) | Self-flagged internal contradiction (486-493): line 203 says vendor cmdline has `reboot=cold`; sweep says `reboot=warm reboot=warm`. **But workflow.md:479 captured `reboot=cold` live on 4.1.37** — the two docs still disagree; unresolved. Line 437 labels 1G PHY "AR8031" in the delta section though the chip is AR8033 (resolved elsewhere). 494 lines — large; MMIO map + photo catalog + multi-boot deltas could each be their own doc but all cross-referenced. | **update** — resolve the reboot=cold/warm contradiction against the logs; normalise "AR8031"→"AR8033 (driver 8031)". Keep as the SoC/chip single-source. |
| **boot-flow.md** | current / **duplicate** | U-Boot env + bootunsign (11-53), sysid variant table (97-127), overlay/USERDEV/purify (141-157), cmdline+break= (158-190) all restated in workflow.md §7-8, nor-boot-chain.md, firmware-5.1.25.md. | **keep** but **trim** the U-Boot-env/netboot block (dup of nor-boot-chain.md §4 + workflow.md §8) to a cross-ref. |
| **bootloader.md** | current / **duplicate + stale open item** | TOC table (17-30) + "## Open" (226-231) flag the `preboot` row as "does not match … unresolved ❓" — **RESOLVED** in uboot-update-path.md §TOC-correction (69-73) and nor-boot-chain.md §1. sysid→DTB, unsigned-boot, GPL-drop all restated in nor-boot-chain.md. | **update** — mark the preboot-TOC row resolved (off 0x0 size 0x80000) and drop the dead open bullet; **merge** structural content into nor-boot-chain.md (see Duplication). |
| **nor-boot-chain.md** | current | The comprehensive boot-chain synthesis (explicitly "builds on" the others, 14-17) but restates much of them. §7 provenance overlaps licence-audit.md. | **keep** as the canonical boot-chain doc; make the older four link here instead of re-tabling. |
| **uboot-update-path.md** | current | Resolves bootloader.md's TOC open item (69-73). Overlaps nor-boot-chain.md §1/§4 (TOC, board table, env deltas). Uses word "honestly" (203) — banned per CLAUDE.md. | **keep**; trim TOC/board-table dup to cross-ref; drop "honestly". |
| **nand-1.3.35.md** | current | Explicitly 1.3.35-generation (correct, dated). Default-env + unsigned-boot + board-table blocks overlap boot-flow/bootloader/nor-boot-chain. | **keep** (generation-specific record); trim the env/unsigned-boot dup to cross-ref. |
| **firmware-5.1.25.md** | current | Container format + sections table duplicated verbatim with porting-reference.md §firmware-container (see Duplication). | **keep**; make the container-format table single-source (porting-reference.md or firmware-5.1.25.md), other links. |
| **upgrade-path.md** | current | Release ladder + API. Uses "stated honestly" (98) — banned word. Minor dup of upgrade-process.md prereqs. | **keep**; drop "honestly". |
| **upgrade-process.md** | current / **legacy-narrative** | Runbook of the 2026-08-15 ladder actually walked. Load-bearing gotchas are worth keeping; but it is a historical "what we did" narrative (against terse-current-only convention). Overlaps upgrade-path.md, uboot-update-path.md. | **keep** (real runbook + gotchas) but trim the narrative framing. |
| **workflow.md** | **legacy-narrative** | 648 lines — the single biggest doc; a chronological "everything done so far, in order" from cold start (rescue USB, fetch fw, console bring-up, find-on-LAN, prep-userdev). Much is superseded (device now runs Fedora, not the failing-USB initramfs). "Current state"/"What's next" tables (558-648) are the 2026-08-15 state, long overstaken. Device IP `.140` (484,566) is stale (issues/38 → `.149`). Heavy dup of recovery.md, boot-flow.md, firmware-5.1.25.md, upgrade-process.md. | **update/trim heavily** — this is the largest legacy narrative in the repo. Extract the still-true gotchas (console model §10, serial wiring, tio pty) into reference docs; retire the blow-by-blow. Owner decision. |
| **recovery.md** | current | USB-failure background + firmware API. Firmware-API block (36-56) overlaps upgrade-path.md and workflow.md §3. | **keep**; light trim of the duplicated API row. |
| **fedora-on-ssd.md** | **current** ✅ | Matches ground truth incl. §PERSISTENT standalone NAND boot (2026-08-17), reboot #51, root pw `unvr`. | **keep** — this is the current OS-state doc project-status should mirror. |
| **reboot-driver-handover.md** | current | al_reboot written/untested/#51 — matches ground truth. Says woomera at `192.168.25.140` (39) — **stale**, issues/38 has `.149`. | **keep**; fix the IP to `.149`. |
| **improvements-audit.md** | current | 7.1.8 config audit; issue-ready. Bootargs sample (10) uses `reboot=warm` (consistent w/ vendor, distinct from Fedora's `reboot=cold`). Findings largely now filed as #43-#45,#49-#50. | **keep**; optional note that items are ticketed (#43 etc.). |
| **gpio-switches-leds.md** | **current** ✅ | gpio 33/34=RPS sense, 37=ulogo_white, reset=gpio38 — matches ground truth (2026-08-17). | keep |
| **rps-subsystem.md** | **current** ✅ | ttyS2=RPS UART (not BT), RPS populated, U1 unresolved, i2c_gen disabled — matches ground truth. | keep |
| **overclock-and-caps.md** | **current** ✅ | cpu downclock lever, PLL VCO overclock, eFuse caps. Consistent w/ hw-reference uboot dump. | keep |
| **dt-gaps-hardware-of-record.md** | **current** ✅ | S2 confirms ttyS2 mislabel→RPS, no BT, RPS populated — matches ground truth. | keep |
| **linux-6.12-build.md** | current | First mainline build; foundational for 6.18/7.1 which record only deltas. | keep |
| **linux-6.18-build.md** | current | Delta doc; verified on hw. | keep |
| **linux-71-build.md** | current | Delta doc; verified on hw (latest stable). | keep |
| **benchmarks-6.12.md** | current | 6.12 storage sanity pass. 7.1 benchmarks tracked #41/#42 (not yet done). | keep |
| **porting-roadmap.md** | current | 8-phase plan; explicitly supersedes porting-reference's ea1a (115). Issue mapping (12-14) says drafts 31-37 "file as #31+" — **now filed** as GitHub #31-#37. | **keep**; minor: update the "not ticketed → new #NN" phrasing (those tickets now exist). |
| **porting-reference.md** | current (fixed) | ea1a→ea16 applied (30). Firmware-container + kernel-config blocks duplicated with firmware-5.1.25.md/workflow.md (see Duplication). Early-research (2026-08-15) reference. | keep; container/config tables → single-source. |
| **alpine-v2-research.md** | current | Line 137 "UNVR (board `ea1a`)" — ambiguous (real eMMC UNVR is ea1a; our unit ea16). Clock/DDR strap tables overlap overclock-and-caps.md. | **flag** — clarify ea16 vs ea1a for our unit; not auto-fixed (product-line context genuinely includes an ea1a variant). |
| **uefi.md** | current | EDK2 plan #39. Self-flagged "Honest gaps" heading + "stated honestly" (231, and §6 title) — banned word. | keep; rename "Honest gaps" → "Gaps + risks". |
| **sources.md** | current (fixed) | ea1a→ea16 applied (31). Master link register; overlaps prior-art.md (repo table) and porting-reference.md §sources. | keep; the repo-status table duplicates prior-art.md — pick one owner. |
| **prior-art.md** | current | Repo-activity trace. Repo table overlaps sources.md §2 heavily. | keep; de-dup vs sources.md. |
| **photo-catalog.md** | **current** ✅ | "Atheros switch"=AR8033 PHY confirmed; U1 unresolved — matches ground truth. | keep |
| **ghidra.md** | current | RE methodology; links hardware.md for the MMIO map (no dup). | keep |
| **preboot-decompile.md** | current | Function-level decompile; multi_dt/RSA overlap nor-boot-chain.md §3/§5 but at different depth. | keep |
| **licence-audit.md** | current | 5.1.25 GPL fingerprint; provenance overlaps nor-boot-chain.md §7 and sources.md §3 (cross-referenced). | keep |
| **credentials.md** | current | Vendor default creds. | keep |
| **identity-partitions.md** | current | ubnthal decode; byte-layout deferred to nor-boot-chain.md §6 (good single-source). | keep |
| **hw-reference/uboot-2015…HAL.md** | **current** ✅ | cpu_set_speed downclock, PCA9546 (not PCA9548), eMMC-over-USB stranded — matches ground truth. | keep |
| **chips/README.md** | **current** (just rewritten) | Now a datasheet-index; DRAM=Samsung K4A8G165WB and SFP=Finisar FTLX8571D3BCL both marked resolved (23-24,31-33). Points to new `components.md` as master BOM. | keep |
| **components.md** (NEW) | **current** | New master component/connector/TP catalog (created 16:12 mid-review). Overlaps hardware.md's physical-chip catalog — see concurrent-editing note. | keep; resolve the components.md ↔ hardware.md chip-catalog duplication (pick one owner). |
| **chips/dram-ddr4.md** | **STALE** | Still "Part: unknown — no marking read yet" (3) and "Candidates … to be confirmed" (22-24), while its siblings `chips/README.md`:23,31 + `components.md` + `hardware.md`:120,144 now confirm **Samsung K4A8G165WB ×4** (photo, and 0x57 is the AT24C64 identity EEPROM, not DDR SPD). The concurrent editor updated the index but not this deep-dive yet. | **update** to the confirmed Samsung part (left to the active editor / owner to avoid a concurrent clobber). |
| **chips/ar8033.md** | current | Matches ground truth (AR8033, driver-label 8031). | keep |
| **chips/** (adt7475, al-324, asm1042a, mt29f8g08…, mx25u25635f, pca9546a, pca9575, s-35390a, sfp-optic) | current | Not individually re-read; cross-checked vs chips/README + hardware.md. NOTE chips/README now IDs SFP = **Finisar FTLX8571D3BCL** (read live) — verify sfp-optic.md carries the same (index says resolved). | keep; confirm sfp-optic.md matches the resolved Finisar part. |
| **issues/*.md (00-21, 31-37)** | **DUPLICATE of filed GitHub issues** | Drafts 00-21 = GitHub **#4-#25**; drafts 31-37 = GitHub **#31-#37** (verified titles match via `gh issue list`). issues/README.md:12-14 still says 31-37 are "not filed — file as #31+" — **stale** (they are filed). | **flag only** (do NOT delete per brief). Recommend: issues/README note which NN → which live #; treat the drafts as superseded local copies. |
| **issues/38-ssh-normal-access.md** | current, **untracked, number-collides** | New (git-untracked) draft; number 38 collides with GitHub **#38** ("Chip docs… identify DRAM & SFP") which is a *different* issue. Content (woomera at .149) is resolved and correct. | **flag** — renumber/file separately; it is not GitHub #38. |
| **issues/README.md** | **STALE** | "Gap drafts 31-37 … not filed — file as live #31+" (12,40) — those are now GitHub #31-#37. | **update** the not-filed claim. |
| **photos/README.md** | current (not re-read in full) | Photo-tracking note. | keep |

## Duplication clusters (single-source + cross-ref recommended)

1. **Boot chain / TOC / U-Boot env / unsigned-boot / sysid→DTB** — spread across
   `bootloader.md`, `boot-flow.md`, `nor-boot-chain.md`, `uboot-update-path.md`,
   `nand-1.3.35.md`. nor-boot-chain.md is the natural single source (it already says it
   "builds on" the rest). Recommend: bootloader.md + boot-flow.md keep their unique RE
   method / initramfs content, but the TOC table, board table, env-delta table and
   "unsigned boot" verdict live once (nor-boot-chain.md) and the others link.

2. **Firmware container format + section table** — byte-identical block in
   `firmware-5.1.25.md` §1, `porting-reference.md §firmware-container-anatomy`, and
   `workflow.md §4`. Pick one owner (suggest firmware-5.1.25.md), others cross-ref.

3. **Stock kernel-config table** (MODULE_SIG/KEXEC/al_eth built-in…) — duplicated in
   `porting-reference.md §stock-kernel-config`, `workflow.md §6`, and `sources.md §11b`.
   Single-source it.

4. **initramfs overlay / USERDEV / purify_userdev / break= / cmdline options** — in
   `boot-flow.md` and `workflow.md §7` nearly verbatim. Single-source (boot-flow.md).

5. **Prior-art / repo-status tables** — `sources.md §2` and `prior-art.md` list the same
   repos with dates/stars. De-dup; one is the register, the other the status.

6. **Chip data** — `hardware.md` physical-chip catalog vs `chips/*` — mostly good
   (chips/README explicitly cross-checks hardware.md), EXCEPT DRAM (hardware.md
   identified it, chips/dram-ddr4.md still "unknown").

7. **GPL-withholding narrative** — `sources.md §3`, `prior-art.md`, `licence-audit.md`
   — already cross-referenced; licence-audit adds the 5.1.25-specific verdict, acceptable.

## Legacy / stale summary (ranked)

1. **project-status.md** — Fedora shown in-flight; it's done and standalone. Update.
2. **workflow.md** — 648-line chronological narrative, largely superseded (box now runs
   Fedora). Trim to keep only still-true gotchas; retire the rest.
3. **chips/dram-ddr4.md + chips/README.md** — DRAM "unknown" but identified (Samsung
   K4A8G165WB-BCRC) in hardware.md.
4. **issues/README.md + issues/*.md** — drafts 00-21/31-37 duplicate filed GitHub
   #4-#25/#31-#37; README still says 31-37 "not filed". Flag (don't delete).
5. **bootloader.md** — TOC `preboot` row + open item resolved elsewhere; mark resolved.
6. **Stale device IP `.140`** in workflow.md (484,566) and reboot-driver-handover.md (39)
   — should be `.149` (issues/38). Safe to fix once owner confirms the reservation.
7. **hardware.md reboot=cold/warm** self-contradiction, and it disagrees with
   workflow.md:479 — resolve against the console logs.
8. **Banned word "honest/honestly"** (CLAUDE.md rule) in `uboot-update-path.md`:203,
   `upgrade-path.md`:98, `uefi.md`:231 + §6 title "Honest gaps". Trivially fixable.
</content>
</invoke>
