# Annapurna Labs Alpine V2 (AL-324) — SoC research

Focus: overclocking / clocking / DVFS, datasheets/TRM, community RE & ports, other
products/users, key people/forums. Researched 2026-08-16.

- **Our part = AL-324**, quad Cortex-A57, ARMv8-A, self-reported `Device ID = a324`
  in U-Boot. 4 GiB DDR4, SPD I2C addr 0x57. See [hardware.md](hardware.md).
- Register-level facts below come from the **Annapurna HAL** (dual GPL-2.0 /
  commercial, © 2015 Annapurna Labs), cloned at `/mnt/2tb/unvr-port-refs/delroth-alpine_hal/`.
  There is **no public datasheet/TRM** — the HAL is the register documentation.

---

## 1. Overclocking / clocking / DVFS — survey

**Register-exact procedure, strap/ceiling tables, PLL formats, eFuse caps analysis:
canonical doc is [overclock-and-caps.md](overclock-and-caps.md).** PLL divider
map is **REV2** (`al_pll_freq_set_v2`, `setup_0` bitfields) — full detail there.

Survey findings that stay here (not overclock-and-caps' focus):
- **No Alpine cpufreq / DVFS / OPP driver exists anywhere.** Checked urnvr-kernel
  4.19.152, delroth qnap 6.0.6, UBNT GPL, linux 6.18/7.1.8 — only generic
  `virtual-cpufreq.c`. CPU frequency is **set once at boot** and fixed thereafter.
- **No community overclock report exists** for any Alpine V2 device (UNVR, UDM, QNAP
  TS-x32x). Searches surface only QNAP's marketing "CPU burst" and generic OC pages.
- DDR memory-overclock would mean feeding different SPD/timing params to the closed
  CVOS training agent — see [preboot-decompile.md](preboot-decompile.md) §DDR and
  [ddr-config-reverse.md](ddr-config-reverse.md) for how that agent is fed.

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

### Part-number disambiguation
- **AL-314 ≠ AL-324.** TechInfoDepot lists **AL-314 as quad Cortex-A15 @1.4 GHz
  (Alpine V1)**, distinct from **AL-324 = quad A57 (Alpine V2)**. linux-alpine-v2's
  README labels the UNVR SoC "AL-314" — inconsistent with our device's own
  `Device ID = a324`. **Treat AL-324 as authoritative for UNVR/UDM.** The digits are
  conflated in the wild; key nothing on them (as porting-reference.md already warns).
- **MikroTik RB1100AHx4 / RB4011 are NOT the same SoC** — they use **AL21400
  (Cortex-A15, Alpine V1)**, not A57 V2. Shares the Alpine *brand*, not the V2 core.
  Drivers do not cross over the same way QNAP/UDM do.

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

1. **CPU overclock = new out-of-tree driver** doing the register-exact procedure in
   [overclock-and-caps.md](overclock-and-caps.md) §1 (setup_0 target values, RELOCK
   dance). Test 1.7→1.8 GHz first, watch al_thermal + stability. No prior art —
   highest-risk, highest-novelty.
2. Register base + `setup_0` format are now confirmed (REV2, not REV1) — see
   overclock-and-caps.md; no further HAL archaeology needed before a first live test.
3. **DDR is a harder target** — timings owned by the proprietary CVOS training agent;
   only SPD@0x57 is externally visible. Deprioritise vs CPU clock.
4. **Watch delroth + fabianishere + mornepousse** for any clock/thermal commits —
   same silicon, fixes cross over to UNVR.
5. **Baseline first:** pull an AL-324 benchmark from OpenBenchmarking and measure the
   stock 1.7 GHz UNVR before any OC, so any gain is quantified. (Perf is
   storage/CPU-bound well below 10G — see porting-reference.md.)
6. Public datasheet unlikely to ever surface; **the HAL is the reference** — invest in
   reading `al_hal_pll*`, `al_hal_bootstrap*`, `al_hal_ddr*` rather than hunting docs.
