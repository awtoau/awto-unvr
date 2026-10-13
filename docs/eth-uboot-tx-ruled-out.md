# U-Boot al_eth TX: what is ruled out, and what is not (#253)

**Read this before touching snoop, SMCC, axcache, ROB, cache maintenance, FLR or
buffer placement in `uboot-port/drivers/net/al_eth/`.** Snoop alone has been
tested five separate times across three sessions. Three commits each claimed to
be "THE TX-completion fix" and all three failed: `92f5706` (cache), `092fcde`
(FLR), `d69b711` (snoop). Do not add a fourth without a real `tftpboot`.

Scope: the 10G port (`alu_eth_10g`, PCI 1c36:0002) in **our U-Boot**. The 1G is
fixed (`30e7c65`, RGMII_ID — it was never a TX bug). Linux drives both ports at
line rate, so the silicon is good and the delta is our U-Boot code.

Current signature (2026-10-13, build `awto-a65d476-dirty`):

```
alu_eth_10g: TX completion timeout: 1 descs left after 13 us (limit 13 us, 342B @ 10000 Mbps)
alu_eth[timeout]: m2s.state=00000001 tx0 drhp=40000001 drtp=40000001 dcp=40000001 crhp=40000000 qpkt=00000001
alu_eth[axi-rd-err]: attr=00000000 addr_to=0 comp_err=0 comp_to=0 ... (all zero)
[eth tx] warn: dma state didn't change to Disable, engine still Normal (not quiescent)
alu_eth_10g: al_eth_adapter_stop failed: -110
```

`drhp == drtp == dcp` all advance, `crhp` never does. **The engine consumes the
descriptor and the completion never arrives.** Every AXI error register is zero,
so the read is not erroring — it is not being answered.

## Ruled out — do not re-test

| Hypothesis | Verdict | Evidence |
|---|---|---|
| **SMCC snoop** | No. Tested 5x: added (`d69b711`), removed (`d4f176a`), swept 0x0/0x1/0x3 on the 1G ("all abort identically"), live `setpci` diff vs working Linux (byte-identical), bundles 1-3 unwritten to match Linux (identical failure). | `d69b711`, `d4f176a`, `d41ed23`, `30e7c65`, #253 2026-10-13 |
| **axcache** (`0x220`) | No. `0x3ff`, identical to the working AHCI and to live Linux. Never swept to another value — the claim is match-not-sweep. | #90 body, `d41ed23`, `30e7c65` |
| **ROB** (`0x240`) | No, and this one has a negative control: al_ssm reads `0x00010001` (read+write ROB enabled) **by hardware reset default with zero driver config**, and `iodma` hangs too. ROB was never the gate. | `eth-tx-abort.md` 2026-08-24 |
| **Cache maintenance** | No, structurally: `al_udma_cdesc_get_all()` reads the completion head from the **`crhp` register**, not DRAM. Cache state cannot affect a TX completion. Also covers stock's `CONFIG_SYS_DCACHE_OFF`. | `30e7c65` |
| **Low-DRAM buffer placement** | No. Ring pinned low, `drbp` confirmed live, still hangs. Kept because it matches stock/iodma — *necessary-not-sufficient at best*. See the warning below. | `d41ed23`, `02b723b` |
| **FLR** | No, and this is now the best-evidenced elimination on the 10G: `want_flr = false`, rebuilt, SSD-chainloaded, **byte-for-byte the same failure**. Also tested both directions on the 1G in Aug. | #253 2026-10-13; `02b723b` |
| **`al_unit_adapter_init` stub** | No. Replicated on hardware (still hangs), then disproven structurally: Linux's *working* al_ssm never sets `.unit_adapter`, al_eth's Linux HAL has no such field, and the string does not exist anywhere in the vendor stage-3 binary. | `d41ed23`, `eth-tx-abort.md` |
| **AXI error tracking** (are we just not seeing it?) | No. Enabled live, re-ran the failing transfer: error attributes and latched fail-address **stayed all-zero**. The read is never answered at all, not merely unmonitored. | `eth-tx-abort.md` 2026-08-24 |
| **tgtid / tgtaddr misrouting** | Not programmed for eth in ours *or* stock; disabled = raw pass-through, cannot misroute. | `eth-tx-abort.md` |
| **UDMA submaster cfg / ostand** | Benign reset defaults, identical to stock. `al_udma_m2s_axi_set` is never called for rev_id 2 in ours or stock. | `eth-tx-abort.md` |
| **Queue-init vs state-set ordering** | Disproven from vendor's own *working* `iodma` source, which uses our order, not Linux's. | `eth-tx-abort.md` |
| **UDMA HAL vintage/drift** | Per-line diff vs al_ssm: byte-identical bodies. Then 8 UDMA HAL functions matched at register-write level against the Ghidra decompile of the real vendor binary. | `eth-tx-abort.md` |
| **Kernel PCIe host-bridge doing something extra** | All 298 lines of `pcie-al-internal.c` read: exactly 3 register writes (SMCC SM0, SM1-3, APP_CONTROL). No ATU, no stream-ID table, no IOMMU. `board_late_init()` already replicates it. | `eth-tx-abort.md` |
| **eth-specific** | No. `iodma` on al_ssm (a different al_udma) reproduced the identical signature. | `eth-tx-abort.md`, `156c8d6` |
| **Hardware / Linux affected** | No. Linux: 10G at 9.38 Gbit/s, `tx_errors 0`, `tx_dropped 0`. | #234, `issue-reconciliation.md:150` |

### The one argument that survives re-reading

Both ports program **byte-identical** values in this entire register group, and
one works in U-Boot while the other does not:

| offset | 00:01.0 (1G, works) | 00:02.0 (10G, fails) |
|---|---|---|
| SMCC[0x110] | `00000003` | `00000003` |
| SMCC[0x130/0x150/0x170] | `00000000` | `00000000` |
| CONF_2[0x114] | `00000000` | `00000000` |
| axcache[0x220] | `000003ff` | `000003ff` |
| ROB[0x240] | `00010001` | `00010001` |

Read with `setpci` on a Linux doing 10 Gbps, 2026-10-13. **This register group
cannot be the discriminator** — even a perfect match with Linux would not
distinguish the two ports. Prefer this framing over "byte-identical to Linux",
which invites the rebuttal "but we write bundles 1-3 and Linux does not" (that
rebuttal was acted on 2026-10-13; identical failure).

## Retracted evidence still quoted elsewhere

- **"AHCI DMA works on the same fabric, so the fabric routes DMA."** Retracted:
  `uboot-port/drivers/ata/ahci.c` is the *generic* U-Boot AHCI driver with zero
  al_udma references — a structurally different master. It was never good
  evidence either way. **#90's and #234's bodies still carry the un-retracted
  version, and both are closed**, so the correction is not visible there.
- **"The al_udma master cannot reach DRAM at any address" (`02b723b`).** Rested
  on the AHCI comparison above and on a 1G diagnosis `30e7c65` disproved. The 1G
  now tftpboots at 4.2 MiB/s *through that same master*, so it demonstrably does
  reach DRAM. See the code warning below.
- **"Leading hypothesis: FLR wipes board params."** Dead as of 2026-10-13. Still
  asserted in `docs/issue-clusters.md`, #323 step 2, `issue-reconciliation.md:147`
  and `docs/agent-memory/eth-udma-tx-hang.md`.

## Contradictions live in the code right now

- **`alu_eth_core.h:57-59` / `alu_eth_core.c:50-52`** assert "the al_udma master
  decodes only the LOW window" as fact. The measurement cited proved low
  placement does **not** fix the hang; `d41ed23` was honest ("likely
  necessary-not-sufficient"). High-DRAM placement has never been tested on the
  *working* 1G to see whether the restriction exists at all.
- **`alu_eth_core.c:415` sets `AL_ETH_TX_FLAGS_NO_SNOOP`** because "the fabric
  never answers the snoop", while `alu_eth_unit_adapter_setup()` 80 lines earlier
  programs `SNOOP_OVR | SNOOP_EN` on all four sub-masters. Two mutually
  exclusive theories of the fabric, both shipped. al_dma concluded the opposite
  (`3b2b44a`, "drop NO_SNOOP (#23)", recorded as a hardware fact). NO_SNOOP is
  probably cargo from the `d69b711` -> `d4f176a` thrash and has not been
  re-tested since the `0x2222`-ABORT model collapsed.
- **`alu_eth_core.c:163-164`** cites "the same settings the working AHCI uses on
  this fabric" — the retracted appeal above. That comment sent a session down
  this dead end on 2026-10-13.

## The wire test: run once, on deleted code, verdict not trustworthy

`scripts/uboot-eth-wire-test.py --port 10g` **was** run, 2026-09-05 07:44
(`tmp/logs/uboot-eth-wire-test.log`):

```
VERDICT: 0 frame(s) from 74:ac:b9:41:a8:12 on the wire -> TX NEVER LEAVES THE MAC
```

The 1G control in the same log worked (`9 frame(s) ... TX REACHES THE WIRE`), so
the harness is sound. But the verdict has three unrecorded caveats:

1. It captured on `enp7s0`, whose relevance to the 10G's L2 path was never
   established — and #170 proved an interface that answers does not tell you
   which port carried the traffic. "0 frames" may mean "wrong listener".
2. It ran the since-**deleted** `al_eth_dm_10g` driver. Per #253 2026-10-09:
   "no measurement on this issue has ever exercised the code that would now fail."
3. It predates the `crhp` discovery. "0 frames on the wire" plus "descriptor
   consumed" is a genuinely interesting combination nobody has reconciled.

The unqualified verdict has been requoted twice since. A do-not-re-test entry
built on it would be worse than none. **Re-run it on current code with a capture
interface verified to be on the 10G segment.**

Also: the script is not registered in `dev.py`, so browsing commands will not
find it.

## Not yet tested — where to go next

Ranked.

1. **Where `crhp` is written vs where the completion ring was allocated.** The
   explicit ask from the one good dump (#253, 2026-09-07), never acted on, now
   36+ days old. `alu_eth_q_params()` sets `cdesc_phy_base = cdesc`; nothing has
   verified the engine agrees. This is the highest-value lead and it matches the
   signature exactly (descriptor consumed, completion never written).
2. **`al_eth_mac_start()`'s return value on the 10G.** Flagged 2026-09-06 as
   "the change I would watch" — a MAC that silently failed to enable transmit
   gives descriptors-consumed-no-completion, and would also explain 0 frames on
   the wire. Never confirmed on hardware.
3. **Re-run the wire test** on current code, capture interface verified. See above.
4. **Decode `m2s.state=00000001`** per `al_hal_udma_regs_m2s.h`. Never done for
   the current value. `30e7c65` showed a bad decode of this register caused a
   three-session misdiagnosis (`0x1000` was read as a hang; it is `desc_pref=1`,
   Normal).
5. **Re-test NO_SNOOP** now the ABORT model is dead, and reconcile with al_dma's
   opposite conclusion (#23).
6. **Cold boot vs chainload.** Every #253 measurement has been chainloaded.
   `docs/agent-memory/our-uboot-assume-nothing.md`: "chainload pre-state is a
   crutch."
7. **`eth diag 2`** post-start on the current `alu_*` code — never read.

## The TX bound is too tight (diagnostic quality, not the cause)

`limit 13 us` is reached at 13-14 us. `ALU_ETH_TX_DMA_ALLOW_US = 10` is labelled
in-tree as "an unmeasured estimate (#226)" (`alu_eth_core.c:38`). It correctly
replaced #226's ~8000x bound, but 1.0x the theoretical minimum makes a genuine
failure indistinguishable from a slow completion. It omits descriptor fetch and
completion writeback, which dominate at 342 B. Derive as wire time + measured
per-descriptor overhead, then 1.25x, and print all three terms.

This does not change the diagnosis: `crhp` never advances at all.

## Where else this is written

- `docs/agent-memory/eth-udma-tx-hang.md` — the previous canonical list, right
  structure, stale since 2026-09-05, referenced by nothing. Superseded by this file.
- `docs/eth-tx-abort.md` — the deepest evidence base (vendor Ghidra, HAL diffs,
  al_ssm cross-check, the SMCC sweep) but titled for the **1G** and **#90**, both
  closed, and built on an `m2s.state=0x2222` ABORT model that no longer occurs.
  Read it for the Aug evidence, not for the current diagnosis.
- #253 — the issue. Its body's "do not re-test" list is correct but unsourced.
