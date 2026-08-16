# Annapurna Labs Alpine V2 (AL-324) — SoC research

Focus: overclocking / clocking / DVFS, datasheets/TRM, community RE & ports, other
products/users, key people/forums. Researched 2026-08-16.

- **Our part = AL-324**, quad Cortex-A57, ARMv8-A, self-reported `Device ID = a324`
  in U-Boot. 4 GiB DDR4, SPD I2C addr 0x57. See [hardware.md](hardware.md).
- Register-level facts below come from the **Annapurna HAL** (dual GPL-2.0 /
  commercial, © 2015 Annapurna Labs), cloned at `/mnt/2tb/unvr-port-refs/delroth-alpine_hal/`.
  There is **no public datasheet/TRM** — the HAL is the register documentation.

---

## 1. Overclocking / clocking / DVFS — evidence

### The mechanism exists; nobody has used it; there is no Linux driver for it
- **No Alpine cpufreq / DVFS / OPP driver exists anywhere.** Checked urnvr-kernel
  4.19.152, delroth qnap 6.0.6, UBNT GPL, linux 6.18/7.1.8 — only generic
  `virtual-cpufreq.c`. CPU frequency is **set once at boot** and fixed thereafter.
- **No community overclock report exists** for any Alpine V2 device (UNVR, UDM, QNAP
  TS-x32x). Searches surface only QNAP's marketing "CPU burst" and generic OC pages.
- So the A57 cores run at the **strapped** frequency (UNVR: 1.7 GHz) for the life of
  the boot. Changing it means either a different strap (pins/OTP) or runtime PLL
  reprogramming — see below.

### CPU PLL frequency is a 4-bit bootstrap strap (read-only)
Source: `drivers/pbs/al_hal_bootstrap.c` `al_bootstrap_cpu_pll_freq_get()`.
The strap register field decodes to a fixed CPU frequency. **Alpine V2** table
(`dev_id <= ALPINE_V2`):

| field | MHz | | field | MHz |
|---|---|---|---|---|
| 0x0 | bypass | | 0x8 | 2100 |
| 0x1 | 1000 | | 0x9 | 2200 |
| 0x2 | 1400 | | 0xA | 2300 |
| 0x3 | 1500 | | 0xB | 2400 |
| 0x4 | 1600 | | 0xC | 2500 |
| **0x5** | **1700 (UNVR)** | | 0xD | 2600 |
| 0x6 | 1800 | | 0xE | **2700 (max)** |
| 0x7 | 1900 | | 0xF | 2000 (default) |

- **UNVR silicon is strapped 0x5 = 1700 MHz.** The same decode covers 1800→2700 MHz.
  Whether a given AL-324 die is *binned/stable* above 1.7 GHz is unknown and untested.
- Alpine **V3** parts (`dev_id > V2`) decode the same field up to **3000 MHz** — not
  our chip, but shows the family headroom.
- The strap is a reflection of hardware pins/OTP, so it is **not a runtime knob**.

### PLL divider tables go to 3.0–3.2 GHz
Source: `drivers/ring/al_hal_pll_map.h`. For **Alpine V2 the relevant map is `v1`**
(REV1 in the HAL). `al_pll_freq_map_v1_100[]` (100 MHz ref) has divider tuples
`{nf, nr, od, bwadj}` for every step:
- 1700 MHz → `{33, 0, 1, 16}`; 2000 → `{19,0,0,9}`; up to **3000 `{29,0,0,14}`**,
  **3200 `{31,0,0,15}`**.
- v2/v3 maps (ref_div/fb/frac/post-div style) exist for Alpine V3 and are compiled
  out when `AL_DEV_ID <= ALPINE_V2`.
- Enum `al_hal_pll.h` tops out at `AL_PLL_FREQ_3200_000`.

### The PLL is runtime-reprogrammable via the HAL (but only firmware does it)
Source: `drivers/ring/al_hal_pll.c` `al_pll_freq_set(obj, freq, timeout)` →
`al_pll_freq_set_v1()` writes the PLL registers directly.
- `al_hal_pll.h`: three PLLs — **CPU PLL, North-Bridge PLL, South-Bridge PLL**.
  *"The CPU PLL drives the CPU cores, caches and local interrupt controllers."*
- Mechanically, a driver could call `al_pll_freq_set()` on the CPU PLL object to
  raise the core clock. **Risk:** relocking the live CPU PLL requires switching the
  cluster to bypass/alt clock mid-flight; the stock code path runs in preboot
  (`al_pll_init` @0x01023ed4, see [preboot-decompile.md](preboot-decompile.md)), not
  under Linux. No existing Linux code does this on Alpine.
- **Best-leads takeaway:** an overclock would be a *new* out-of-tree driver that
  reprograms the CPU PLL via this HAL, tested incrementally 1.7→1.8→1.9 GHz with a
  stability + thermal watch. Unexplored territory; no prior art to lean on.

### DDR clock / memory tuning
- **DDR PLL strap** (`al_bootstrap_ddr_pll_freq_get`): V2 field 0x6 = **1200 MHz**
  (DDR4-2400), 0x5 = 1333 MHz, others 533/667/800/933/1050/1066; default 800.
- **DDR HAL supports DDR4-800 … DDR4-3200** — enum `AL_DDR_FREQ_{800,1066,1333,1600,
  1866,2133,2400,2666,2933,3200}` (`ddr/`). Alpine V2 uses the `_alpine_v2` init/PHY
  reg files; t_xp comments reference 1866/2133/2400 MHz timings.
- DDR timings come from **SPD over I2C @0x57** + an Annapurna **DDR training "agent"**
  (CVOS), delegated in preboot; the raw PHY sequence is proprietary and not in the
  open blobs. See [preboot-decompile.md](preboot-decompile.md) §DDR.
- Memory-overclock would mean feeding different SPD/timing params to that agent —
  even less accessible than the CPU PLL.

### Rated max A57 frequency
- Marketed/shipped: **1.7 GHz** (all AL-324 products). ARM's A57 core is rated well
  above 2 GHz on suitable process; the bootstrap decode reaching 2.7 GHz suggests the
  IP block was designed with headroom. No datasheet states a guaranteed fmax for
  AL-324. Treat 1.7 GHz as a **product/binning choice**, not a proven silicon ceiling
  — but with zero community data above it.

---

## 2. Datasheets / TRM / register docs

- **No public Alpine V2 datasheet or TRM.** Annapurna kept them under NDA;
  annapurnalabs.com is defunct (Amazon). Bootlin (who did the upstreaming) never
  published register docs. Confirmed by absence across all searches.
- **The register documentation IS the HAL:** `delroth-alpine_hal` (© 2015 Annapurna,
  dual GPL/commercial). Contains full register headers: PLL, PBS, DDR ctrl/PHY v1/v2/v3,
  SerDes 25G, bootstrap map, addr-map, sys-fabric NB regs. This is the only
  register-level source and it is complete enough to have driven every port below.
  Path: `/mnt/2tb/unvr-port-refs/delroth-alpine_hal/`.
- Marketing-spec only (no registers): TechInfoDepot
  <https://techinfodepot.shoutwiki.com/wiki/Annapurna_Labs> — canonical Annapurna
  device/part table (AL-2xx…AL-73400). WikiDevi/DeviWiki mirror it.
- **No sources/ PDF added** — nothing worth saving exists as a public datasheet.

---

## 3. Community RE & ports — who leads

| Who | Work | Where | Depth |
|---|---|---|---|
| **Bootlin** (Thomas Petazzoni) | Mainline arm64 Alpine V2 + Alpine MSIX irqchip (submitted 2016-02-08) | <https://bootlin.com/blog/initial-support-annapurna-arm64-platform/> | Originator of all mainline support; **abandoned 2019** (see quote) |
| **delroth** (Pierre Bourdon) | `alpine_hal` extraction, `al_eth-standalone`, `linux-qnap-tsx32x` (6.0.6) | <https://github.com/delroth> · <https://blog.delroth.net> | **Deepest external HAL/driver RE.** Everything downstream builds on his HAL |
| **bcyangkmluohmars** / **filefly** (mirror) | `linux-alpine-v2` — 6.12 LTS, UNVR+UDM DTS, al_eth/al_dma/al_ssm/al_sgpo, tested on real UNVR | <https://github.com/bcyangkmluohmars/linux-alpine-v2> · <https://codeberg.org/filefly/linux-alpine-v2> | **Most complete for UNVR.** Best starting point |
| **mornepousse** | `al_eth`/`al_thermal` forks, MDIO C22 for kernel ≥6.3 | github.com/mornepousse/al_eth-standalone | Keeps delroth's drivers building on modern kernels (2026) |
| **fabianishere** | `udm-kernel`, `udm-kernel-tools` — bootstrap custom kernels on UDM | <https://github.com/fabianishere/udm-kernel-tools> | Best `al_eth` revision (UDM 1.12.22): DT board params, GPIO SFP control |
| **riptidewave93** | `UNVR-NAS` (Debian+OMV builder), boot-chain + MTD + UART docs | <https://github.com/riptidewave93/UNVR-NAS> | Best install/removal docs; dormant |
| **UrNVR** | Buildroot OS + Ubiquiti GPL kernel 4.19.152 | <https://github.com/UrNVR> | GPL kernel archive; dormant |
| **rickmark** | UniFi security / boot-chain analysis (secure-boot break) | blog.rickmark.me (broken TLS → wayback) | Security angle, not clocking; corroborates our preboot RE |

- **Bootlin quote (2019-02-01, Petazzoni):** *"Bootlin is no longer working on
  upstreaming support for the Alpine platform… If you want a platform well-supported
  upstream, Alpine is probably not the best choice."* — explains the mainline gap.
- delroth's own summary: *"only partial upstream support, though Amazon seems
  interested in moving more parts to mainline"* (never materialised).
- Everyone is **dormant**; newest code is linux-alpine-v2 (Apr 2026) + mornepousse
  forks (Mar 2026). Expect to self-maintain. Full repo-activity trace:
  [prior-art.md](prior-art.md).

---

## 4. Products using the SoC

### AL-324 (Cortex-A57, Alpine V2) — same silicon as ours
- **Ubiquiti:** UNVR (board `ea1a`), UNAS, UDM-Pro (board `ea15`). All `a324`.
- **QNAP TS-x32x family**, all 1.7 GHz: TS-431P3, TS-432X/-432XU/-432PXU(-RP),
  TS-632X, TS-832X/-832XU/-832PX, TS-932X/-932PX, TS-1232XU. delroth's port targets
  these. StorageReview/OpenBenchmarking have benchmarks. Newest (2024): TS-432X/632X
  <https://www.cnx-software.com/2024/07/09/qnap-unveils-ts-432x-and-ts-632x-arm-based-nas-with-10gbe-networking-up-to-six-sata-bays/>.
- **Cross-pollination:** a fix on QNAP (delroth) or UDM (fabianishere) often applies
  to UNVR — same PLL/DDR/al_eth/al_dma. This is the main reason to track those repos.

### Correction to earlier notes
- **AL-314 ≠ AL-324.** TechInfoDepot lists **AL-314 as quad Cortex-A15 @1.4 GHz
  (Alpine V1)**, distinct from **AL-324 = quad A57 (Alpine V2)**. linux-alpine-v2's
  README labels the UNVR SoC "AL-314" — inconsistent with our device's own
  `Device ID = a324`. **Treat AL-324 as authoritative for UNVR/UDM.** The digits are
  conflated in the wild; key nothing on them (as porting-reference.md already warns).
- **MikroTik RB1100AHx4 / RB4011 are NOT the same SoC** — they use **AL21400
  (Cortex-A15, Alpine V1)**, not A57 V2. Earlier notes calling RB1100AHx4 "same SoC"
  are wrong; it shares the Alpine *brand*, not the V2 core. Drivers do not cross over
  the same way QNAP/UDM do.

### Other Alpine (V1 / Cortex-A15, related but not our core)
- Synology DS215+/DS416/DS715/DS1515/DS2015xs; Netgear R8900/R9000/XR700, ReadyNAS
  RN202/212; ASUS GT-AC9600; Check Point L-71W/L-72.

### Amazon / Annapurna lineage
- Amazon acquired Annapurna Labs 2015 → pivoted to **Graviton (AL73400, 16× Cortex-A72
  @2.3 GHz, 16 nm)** and never upstreamed the Alpine NAS-SoC drivers. This is the root
  cause of the whole out-of-tree situation. <https://en.wikipedia.org/wiki/AWS_Graviton>.

---

## 5. Key people / forums

- **Bootlin blog** — bootlin.com/blog/tag/annapurna — upstreaming history, patch links.
- **delroth** — delroth.net, blog.delroth.net, github.com/delroth — HAL + QNAP port.
- **fabianishere** — github.com/fabianishere — UDM kernel tooling.
- **rickmark** — blog.rickmark.me (cert broken, subject `fallback.tls.fastly.net`;
  use `https://web.archive.org/web/2024/https://blog.rickmark.me/untitled-3/`). Boot
  chain: al_boot = customised U-Boot in SPI-NOR (no ROM root-of-trust → chain break),
  stage2/stage3 + I2C `agent_wakeup v2.10` @ SPD addr 0x57, **signing keys embedded in
  the FDT** (an I2C device can rewrite the FDT and re-enter stage2). Matches our
  [preboot-decompile.md](preboot-decompile.md) findings independently.
- **r/qnap** — thread "Systems with Annapurna Labs Alpine AL 324"
  <https://www.reddit.com/r/qnap/comments/10lkmos/>.
- **OpenBenchmarking** — <https://openbenchmarking.org/s/Annapurna%20Labs%20Alpine%20AL324%20ARM%20Cortex-A57> — real AL-324 benchmark data if a perf baseline is wanted.
- Not found: any STH/serverbuilds/OpenWrt thread on clocking or overclocking Alpine V2.

---

## 6. Best leads to pursue

1. **CPU overclock = new out-of-tree driver** calling `al_pll_freq_set()` on the CPU
   PLL object (REV1/v1 map). Test 1.7→1.8 GHz first, watch al_thermal + stability.
   No prior art — highest-risk, highest-novelty. Bootstrap decode shows 2.7 GHz is
   at least *representable* on V2.
2. **Confirm the CPU PLL register base + object init** on Alpine V2 from the HAL
   (`al_hal_pll_map.h` instances, `nb_regs`) before touching hardware. Cross-check
   against our live capture / preboot `al_pll_init` @0x01023ed4.
3. **DDR is a harder target** — timings owned by the proprietary CVOS training agent;
   only SPD@0x57 is externally visible. Deprioritise vs CPU clock.
4. **Watch delroth + fabianishere + mornepousse** for any clock/thermal commits —
   same silicon, fixes cross over to UNVR.
5. **Baseline first:** pull an AL-324 benchmark from OpenBenchmarking and measure the
   stock 1.7 GHz UNVR before any OC, so any gain is quantified. (Perf is
   storage/CPU-bound well below 10G — see porting-reference.md.)
6. Public datasheet unlikely to ever surface; **the HAL is the reference** — invest in
   reading `al_hal_pll*`, `al_hal_bootstrap*`, `al_hal_ddr*` rather than hunting docs.
