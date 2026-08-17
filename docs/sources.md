# Sources — UNVR repurposing

Master link register. Kept for history: everything found, with what it is, how
current, and whether we verified it.

**Verification marks**

| Mark | Meaning |
|---|---|
| ✅ | Verified in this project — we read the code/API/page ourselves |
| 📄 | Reported by a source we trust but not independently checked |
| ❓ | Claimed, unverified, treat as a lead |

Dates are last-commit (`pushed_at`), not `updated_at` — `gh search` reports the
latter and it is bumped by stars, which misleads.

Companion docs: [porting-reference.md](porting-reference.md) (technical),
[prior-art.md](prior-art.md) (project status), [recovery.md](recovery.md) (the
failing boot USB).

---

## 1. Hardware family — what else is Alpine V2

The port target is **Annapurna Labs (Amazon) Alpine V2**, quad Cortex-A57.
Amazon bought Annapurna in 2015, pivoted to Graviton, and never upstreamed the
drivers. Anything sharing this SoC shares the porting problem — and the fixes.

| Device | Notes |
|---|---|
| **Ubiquiti UNVR** | Ours. Board ID `ea16` ✅ (no-eMMC variant; the eMMC UNVR is `ea1a`). 4-bay, 10G SFP+ + 1G, 4 GB RAM |
| Ubiquiti UNVR Pro | Same family, different UART location and eMMC boot |
| Ubiquiti UDM-Pro | Board ID `ea15` ✅. **Much larger community — most technique flows from here** |
| Ubiquiti UNAS / UNAS Pro | 📄 same SoC family |
| QNAP TS-x32x | ✅ Same SoC. **QNAP publishes GPL sources** |
| MikroTik RB1100AHx4 | 📄 RouterOS, limited reuse |
| Synology (AL-314/AL-514 models) | 📄 Synology publishes GPL, but their Alpine kernel is 3.10.105 — **older, not useful** |

`unifi-unvr-emu` shims board sysid `0xea16` as "UNVR4" ✅, against `ea1a` measured
on real hardware by linux-alpine-v2 — so **UNVR variants differ**. Check yours
before trusting any board-ID-keyed guide.

## 2. Kernel ports — ranked by usefulness

| Repo | Kernel | Last commit | ★ | Notes |
|---|---|---|---|---|
| [bcyangkmluohmars/linux-alpine-v2](https://github.com/bcyangkmluohmars/linux-alpine-v2) | **6.12 LTS** ✅ | 2026-04-15 ✅ | 3 | **Best starting point.** UNVR DTS + defconfig, tested on real UNVR |
| [delroth/linux-qnap-tsx32x](https://github.com/delroth/linux-qnap-tsx32x) | **6.12.12** ✅ | 2025-05-14 ✅ | 8 | Same SoC via QNAP. Full 2.1 GB tree. Branches: `qnap-tsx32x`, `wip-nic`, `old-6.0.6` ✅ |
| [delroth/al_eth-standalone](https://github.com/delroth/al_eth-standalone) | 5.5 base | 2025-03-05 ✅ | 6 | Upstream of the al_eth port |
| [mornepousse/al_eth-standalone](https://github.com/mornepousse/al_eth-standalone) | ≥6.3 | **2026-03-20** ✅ | 0 | **Fork AHEAD of delroth** — MDIO C22 callbacks for kernel ≥ 6.3 |
| [mornepousse/al_thermal-standalone](https://github.com/mornepousse/al_thermal-standalone) | modern | 2026-03-20 ✅ | 0 | Alpine thermal sensor |
| [delroth/alpine_hal](https://github.com/delroth/alpine_hal) | — | 2022-02-02 ✅ | 3 | Annapurna HAL extraction, 6 MB |
| [codeberg filefly/linux-alpine-v2](https://codeberg.org/filefly/linux-alpine-v2) | ❓ | ❓ | — | Non-GitHub mirror, not yet checked |

**delroth's `wip-nic` branch is stale** — head commit "wip: al_eth driver",
2022-01-09 ✅. His 6.12.12 tree therefore has the SoC but not the NIC; the
working al_eth on a modern kernel is linux-alpine-v2's out-of-tree module.

### linux-alpine-v2 contents ✅

Patches: `pcie-al-internal.c` (AXI SMCC snoop + APP_CONTROL), `pcie-al-dbi-fix.c`,
a `quirks.c` snippet.
Modules: `al_eth`, `al_dma` (RAID5/6 XOR/PQ), `al_ssm` (AES-XTS/CBC), `al_sgpo`
(bay LEDs), `rtl8370mb` (UDM Pro switch).
DTS: `alpine-v2-ubnt-unvr.dts`, `alpine-v2-ubnt-udmpro.dts`, `ubnt-udm-pro.dts`.
Configs: `unvr_defconfig`, `udmpro_defconfig`, cross-compile `Dockerfile`.
Docs: `KERNEL-MATRIX.md`, `ANALYSIS.md`, `RTL8370MB.md`, `SWITCH-API.md`, `TODO.md`.
Also `image/build-image-v2.sh` **and `build-image-v3.sh`** — Alpine V3 work exists.
CI: `.github/workflows/build.yml`.

**Its `docs/TODO.md` is in German** ✅ and contains load-bearing facts:
- **U-Boot signature verification is currently SKIPPED** — secure-boot chain is a
  TODO, not enforced. Big deal for booting your own kernel.
- XDP support in al_eth planned; IPsec via `al_ssm` HW crypto; "10G encrypted
  routing on hardware Ubiquiti sold as 1G".
- PCA9575 GPIO mapping **incomplete — only pins 4, 5, 8 known**.
- HDD power control actually uses `gpio3:5`, not `pca9575:5`. SATA LED uses
  `gpio2:6`, not `sgpo:6`. Stock differs from the port here.
- Alpine Linux rootfs idles at ~110 MB RAM.

## 3. GPL source — the real situation

Ubiquiti is obliged under GPL-2.0 and does not comply. GPL links were removed
from ui.com in the download redesign; `opensource-requests@ui.com` requests go
unanswered for months to years ✅. Raised on the curl mailing list 2026-02-18 as
a violation pattern ✅.

| Source | What | Status |
|---|---|---|
| [UrNVR/unvr-kernel](https://github.com/UrNVR/unvr-kernel) | Ubiquiti UNVR kernel, **Linux 4.19.152** ✅, 187 MB, full `drivers/net/ethernet/al/` ✅. Single commit `badfd862` 2022-02-24 "Initial upload of linux-image-4.19.152-ui-alpine" ✅. Has `alpine_ubnt_nas_defconfig`, `drivers/power/ubnt-hdd-pwrctl.c` ✅ | **No in-tree provenance.** Firmware release inferred ≈2.3.14 from sibling repo, not proven ✅ |
| [NeccoNeko/UBNT-source-code](https://github.com/NeccoNeko/UBNT-source-code) | `UNVR-1.3.35-GPL/`, kernel **4.1.37**, **U-Boot source**, package lists ✅. Requested 2020-12-12, delivered 2021-01-11 ✅ | Old-gen HAL (`al_hal_*`, no `al_mod_*`) ✅ |
| **[archive.org `unifi-udr-gpl-archives`](https://archive.org/details/unifi-udr-gpl-archives)** | **11.08 GB, 34 files**, uploaded 2022-12-13 ✅. UDM/UDM-Pro/UDM-SE/UDR GPL + kernel tarballs. `.source` sidecars show origin was a `secure-uploads.ui.com` GPL-request delivery ✅ | **Contains NO UNVR — confirmed** ✅ |
| [Ubiquiti firmware API](https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~UNVR) | Versions, sizes, sha256, CDN links. **No GPL archives exposed** ✅ | Live |
| FSF compliance | <https://www.fsf.org/licensing/compliance> | Escalation path |

### Which consoles are actually Alpine V2 ✅

| Console | SoC | GPL kernel |
|---|---|---|
| UNVR | Alpine V2 | 4.19.152 (UrNVR mirror only) |
| UDM (base) | **Alpine V2** ✅ | `linux-arm64-dream-udm-4.19.152-ui-alpine-udm` |
| UDM-Pro | **Alpine V2** ✅ | `linux-arm64-dream-4.19.152-ui-alpine` |
| UDM-SE | **Alpine V2** ✅ | same tree as UDM-Pro |
| UISP Console | **Alpine V2** ✅ | via `clansty/OpenWRT-UISP-Console` |
| **UDR** | **MediaTek — NOT Alpine** ✅ | `linux-mediatek-4.4.198-ui-mtk`. Its 3.2 GB / 2.9 GB archives are **irrelevant to al_eth** |

### THE FINDING: a newer `al_eth` exists for the same silicon ✅

No newer *kernel base* — everything Alpine is 4.19.152. But **two distinct newer
`al_eth` revisions** exist. `al_eth.c` by size / sha256:

| Source | Bytes | sha256 (head) |
|---|---|---|
| `UrNVR/unvr-kernel` (UNVR, Feb 2022) | 318,887 | `08088c66…` |
| **`fabianishere/udm-kernel` `flavour/stock-v4.19` (UDM, UbiOS 1.12.22)** | **326,611** | `fa2744d6…` |
| UDM-Pro 2.4.27 GPL (archive.org) | 324,405 | `47177b75…` |
| `thezim/debbox-kernel` (UDM-SE 2.4.9) | 324,405 | `47177b75…` — **byte-identical to UDM-Pro** |
| `clansty/OpenWRT-UISP-Console` `files-4.19` | 326,611 | **byte-identical to UDM 1.12.x** |

**The UDM 1.12.x variant is the most capable published `al_eth` for Alpine V2.**
Diff vs the UNVR tree: **+525 / −247 lines**, adding:

- **Device-tree board parameters** (`al_mod_eth_board_of_lm`, `_of_flow_ctrl`)
  instead of EEPROM-only board params
- **GPIO-descriptor SFP control** — `mod-def0`, `los`, `tx-fault`, `tx-disable`
- Generic-I2C SFP access shims
- Refactored MDIO bus setup/teardown, separate `al_mod_eth_phy_init`
- `vzalloc`/`vfree` for TX/RX ring `buffer_info` (UNVR and 2.4.27 use `kzalloc`)
- LM-aware ethtool `set_settings` applying link config on the fly

Provenance is Ubiquiti's: on `flavour/stock-v4.19` the only commits touching
`drivers/net/ethernet/al/` are `ubnt: Changeset for v1.10.0-8`, `v1.10.0-12`,
`v1.10.4`, `1.11.10-23`, **`1.12.22`** (2022-06-06) ✅.

**Caveat — divergent branches, not a version ladder** ✅. The UNVR tree has a file
the UDM trees lack (`al_mod_eth_lm_retimer_ds125.c`), and its
`al_mod_hal_eth_main.c` is *larger* (282,301 vs 282,085 vs 282,056). 21 files
differ. Treat 1.12.x as a **feature-superset of the driver glue**, not strictly
newer everywhere. The DT board params and GPIO SFP control are what a modern port
wants; the UNVR retimer file may still be needed for our SFP cage.

**Irony worth noting:** `fabianishere/udm-kernel` was written off as abandoned for
its *tooling*. Its value is the **kernel tree**, which is the best al_eth source
found.

### U-Boot source ✅

`NeccoNeko/UBNT-source-code` includes the **full Ubiquiti Alpine V2 U-Boot tree**
— the richest artefact in that repo. `configs/` carries the whole family:
`alpine_v2_64_ubnt_nas_defconfig` (**UNVR**), `..._nas_pro_`, `..._nas_hd_`,
`..._nas_ai_`, `..._udm_pro_se_`, `..._uxgp_`, `..._uxgp_lte_`, `..._uxg_hd_`,
`..._udc_`, `..._u6wall_`, `..._plus_`, `..._rm_`, plus `board/annapurna-labs/`
and `clean-alpine-v{1,2,3}*.sh`. `snapshot.commit` is an unexpanded
`$Format:%H %cD$` placeholder, so no upstream commit ID is recoverable.

**Missing everywhere** ✅ — no public mirror of any UNVR GPL drop newer than
**1.3.35 / kernel 4.1.37**. The 4.19.152 UNVR tree in `UrNVR/unvr-kernel` has no
corresponding published tarball anywhere. Current firmware is `UNVR4.al324` 5.1.25.

**New lead:** `compliance@ui.com` (alongside `opensource-requests@ui.com`) —
`divinehawk/utr-gpl` claims a 2026 GPL delivery via that address 📄. Its stated
date is internally inconsistent, so treat as unconfirmed, but it suggests requests
are *sometimes* still honoured.

## 4. OS replacement projects

| Repo | Last commit | ★ | Notes |
|---|---|---|---|
| [riptidewave93/UNVR-NAS](https://github.com/riptidewave93/UNVR-NAS) | 2024-12-08 ✅ | 113 | Debian 12 + OpenMediaVault. **Dormant ~20 months**; issue #33 confirms stopped ✅ |
| [UrNVR/urnvr](https://github.com/UrNVR/urnvr) | 2022-02-26 ✅ | 17 | Buildroot, **SSH install, no UART**. Targets firmware 2.3.14 |
| [NeccoNeko/UNVR-diy-os](https://github.com/NeccoNeko/UNVR-diy-os) | 2020-12-23 ✅ | 28 | Hardware specs, serial settings, USB removal, boot logs |

**UNVR-NAS is richer than its README** ✅ — it also ships
`scripts/ubnt-fw-parse.py` (their container parser),
`overlay/kernel/arch/arm64/configs/alpine_v2_defconfig`,
`scripts/docker/build_kernel.sh`, `tools/ubnteeprom/` (Go), and
`tools/ubnt-mtd-lock/` (kernel module). So he **does** build a kernel — his GPL
complaint was about the *latest* source, not any source.

## 5. Firmware container tooling

| Repo | What | Status |
|---|---|---|
| [riptidewave93/UNVR-NAS](https://github.com/riptidewave93/UNVR-NAS) `scripts/ubnt-fw-parse.py` | Container parser | ✅ exists, under agent analysis |
| [authrequest/ubnt-fw-extract](https://github.com/authrequest/ubnt-fw-extract) | Extraction utility | ❓ |
| [solar-pwr/ubnt-fw-tools](https://github.com/solar-pwr/ubnt-fw-tools) | `ubnt-split` / `ubnt-merge` | ❓ |
| [dream-x/unifi-fw-mirror](https://github.com/dream-x/unifi-fw-mirror) | API query, hash verify, local archive | ❓ |

Our own parser: [../scripts/analyse-unvr-firmware.py](../scripts/analyse-unvr-firmware.py).
Format decoded so far in [porting-reference.md](porting-reference.md).

## 6. Kernel/boot technique on related consoles

**Investigated 2026-08-15. Conclusion: kexec is NOT the safer path — it is a
prerequisite bring-up project of its own. Use UrNVR's install shape instead.**

| Repo | ★ | Last commit | Verdict |
|---|---|---|---|
| [fabianishere/udm-kernel-tools](https://github.com/fabianishere/udm-kernel-tools) | 339 | 2022-12-14 ✅ | **Dead end for UNVR** — see below |
| [fabianishere/udm-kernel](https://github.com/fabianishere/udm-kernel) | 135 | 2022-09-30 ✅ | Abandoned. Ubiquiti 4.19 GPL + WireGuard/CAKE. Not mainline |
| [fabianishere/kexec-mod](https://github.com/fabianishere/kexec-mod) | 42 | 2023-04-01 ✅ | The actual mechanism: ARM64 kexec as an LKM. Unmaintained, SoC-sensitive |
| [fabianishere/udm-unlock](https://github.com/fabianishere/udm-unlock) | 23 | 2021-02-11 ✅ | **2 commits total.** UDM-Pro only, PoC |
| [zacs/ckg2_server](https://github.com/zacs/ckg2_server) | **0** | 2026-08-14 ✅ | **Wrong hardware** — CK Gen2 is Qualcomm APQ8053, not Alpine |
| [jnovack/cloudkey](https://github.com/jnovack/cloudkey) | 27 | 2026-07-30 ✅ | Active, but front-panel OLED daemon only. No kernel content |
| [ubiquity-os/ubiquity-os-kernel](https://github.com/ubiquity-os/ubiquity-os-kernel) | 3 | 2026-04-21 ✅ | **CONFIRMED UNRELATED** — a GitHub App webhook router in Deno. Crypto/DAO org. Not Ubiquiti |
| [unifi-utilities/unifi-common](https://github.com/unifi-utilities/unifi-common) | **4362** | 2026-07-31 ✅ | **Most alive project here.** `on_boot.d` persistence, targets UniFi OS 4.x+ |
| [unifi-common-addons](https://github.com/unifi-utilities/unifi-common-addons) | 32 | 2026-08-11 ✅ | Active. ISP/IPv6 workarounds, nspawn. **No kernel/boot addon** |
| [seahorsepip/udm-utilities](https://github.com/seahorsepip/udm-utilities) | **0** | 2021-07-03 ✅ | Abandoned fork of unifi-common. **Ignore** |

### Why udm-kernel-tools fails for UNVR ✅

- Owner-confirmed broken above UDM firmware **1.12.33**. Issue #93 (2023-08-25):
  *"This project does not support the latest firmware (2.x/3.x)."* PR #90 to add
  2.x/3.x support is **open and unmerged since 2023-04-02**, blocked on not having
  the 3.x GPL kernel sources.
- Structurally welded to the **UDM 1.x split** — `unifi-os shell`, `ssh-proxy`,
  `/etc/unifi-os/ssh_proxy_port`. UniFi OS 2.x+ removed that architecture.
- Ships **per-firmware prebuilt `.ko`s** built against each firmware's
  `/proc/config.gz` + matching GPL source. Every firmware needs a rebuild.
- **Zero UNVR mentions** in the entire repo. Feature requests for UDM-SE and
  UXG-Pro went unanswered — it never grew past UDM/UDM-Pro.
- `kexec-mod` has never been reported working on Alpine V2; its one public port
  to other silicon works in QEMU but **fails on real hardware** (issue #2).

### unifi-common DOES work on UNVR ✅

Issue [#600](https://github.com/unifi-utilities/unifi-common/issues/600),
2025-07-27: installed on a **UNVR running UniFi OS 4.3.6 / Protect 6.0.47** —
works, but requires the **manual install**, because `remote_install.sh`'s
`udm_model()` case statement enumerates gateways only and has no UNVR entry.
Gives root persistence across reboots; **nothing toward a custom kernel**.

### Block-device write protection ✅

Confirmed Ubiquiti ships a module marking block devices write-protected — but
that is `udm-unlock`'s reason to exist and it is **UDM-Pro only**. No evidence it
applies to UNVR: both UNVR install routes write the boot device with no unlock
step. If you do hit it, `udm-unlock` is the right technique, wrong binary.

Compatibility warning ✅: installers written for UniFi OS 1.x/2.x do not work on
4.x/5.x.

## 7. Protect off-box — different problem, do not confuse

Runs Ubiquiti's Protect **userland** on other arm64 hardware. Does **not** emulate
Alpine V2 and is not a path to booting the UNVR.

| Repo | Last commit | ★ | Notes |
|---|---|---|---|
| [dciancu/unifi-protect-unvr-docker-arm64](https://github.com/dciancu/unifi-protect-unvr-docker-arm64) | 2026-08-04 ✅ | 167 | **The live one.** 71 issues, actively maintained |
| [rjmotion/unifi-unvr-emu](https://github.com/rjmotion/unifi-unvr-emu) | 2026-07-26 ✅ | 1 | Fork of snowsnoot. `BREAK-LOG.md`, `COMPATIBILITY.md` ✅ |
| [snowsnoot/unifi-unvr-arm64](https://github.com/snowsnoot/unifi-unvr-arm64) | 2025-02-24 ✅ | 45 | |
| [markdegrootnl/unifi-protect-arm64](https://github.com/markdegrootnl/unifi-protect-arm64) | 2023-06-14 ✅ | 145 | **ARCHIVED** ✅. Issue #17 + PR #18 cover running it on x86 |

`unifi-unvr-emu`'s `COMPATIBILITY.md` ✅ is a genuinely useful artifact: it marks
every component Native / Patched / Shimmed / Substitute / Excluded, so you can
tell which measured behaviour is real. Its finding that matters for us — **the
storage gap is caused by absent enclosure i2c (`ui-ese`, `ui-hdd-pwrctl`, bay slot
mapping), not a missing disk.** No block device fixes it; the bay is the gate.

## 8. Hardware, repair, recovery

- UART, UNVR: 4-pin header behind the SFP+ cage, mid-PCB. GND/TXD/RXD only,
  ignore 3V3. **115200 8N1** 📄. Verify voltage before connecting — expect 3.3 V TTL.
- UART, UNVR Pro: 4-pin near the USP-RPS / DC Power Backup connector 📄.
- Boot USB is **glued** — hair dryer + dental floss, or heat gun 📄.
- [Incredigeek USB replacement guide](https://www.incredigeek.com/home/unifi-unvr-recover-from-failed-usb-drive/) ✅
- [Ubiquiti Recovery Mode](https://help.ui.com/hc/en-us/articles/360043360253-UniFi-Recovery-Mode) ✅
- [Community thread, 368 replies — USB failure is a known defect](https://community.ui.com/questions/UNVR-stopped-responding-just-white-flashing-light/a051f869-8349-4a2a-a72a-ce3a8aa8c759) ✅
- MTD map and u-boot env procedure: see [porting-reference.md](porting-reference.md).

## 9. Protect APIs — using the box as shipped

Do not modify firmware; extend it.

| Project | What |
|---|---|
| [uilibs/uiprotect](https://github.com/uilibs/uiprotect) | Python API behind Home Assistant |
| [hjdhjd/unifi-protect](https://github.com/hjdhjd/unifi-protect) | TypeScript API, realtime events, encoded livestream |
| [ep1cman/unifi-protect-backup](https://github.com/ep1cman/unifi-protect-backup) | Automatic event-recording export |
| [homebridge-unifi-protect](https://github.com/hjdhjd/homebridge-unifi-protect) | HomeKit integration |
| Scrypted UniFi Protect plugin | Rebroadcast + automation |

## 10. Local mirrors on this machine

- Mainline Linux — has `arch/arm64/boot/dts/amazon/alpine-v2*`, `irq-alpine-msi.c`,
  `pcie-al.c`; **no `al_eth`** ✅
- QEMU — `hw/arm/` has **no Alpine machine model**, only `virt` and `sbsa-ref` ✅

## 11. Known gaps

- No modern UNVR GPL drop anywhere public. Newest confirmed kernel source is
  4.19.152 (UrNVR mirror, 2022).
- linux-alpine-v2 open issue #1: **AHCI port 2 fails to link up on warm reboot,
  both controllers** ✅ — unresolved, hits a 4-bay NAS directly.
- PCA9575 GPIO map incomplete (pins 4, 5, 8 only).
- Nobody actively maintains a UNVR OS port.

## 11a. Install route — decided 2026-08-15

**Use UrNVR's install shape, not UNVR-NAS's, and not kexec.** ✅

| Route | Shape | Risk |
|---|---|---|
| **UrNVR** | From running stock OS over SSH: back up `/dev/mtd*` + `/dev/boot`, `mount /dev/boot1`, `cp uImage uImage.bkp`, `scp` new `uImage`, reboot | **One file replaced, original kept beside it.** No UART, no u-boot env change, no whole-device dd |
| UNVR-NAS | UART → u-boot → `setenv rootfs`/`bootargsextra`/`saveenv` → recovery → `dd if=debian-UNVR.img of=/dev/boot bs=4M` | Whole-device overwrite + bootloader env edit |
| kexec | Port `kexec-mod` to Alpine V2 first | A whole prerequisite project |

**Recovery net** ✅: the UNVR keeps a **recovery kernel in SPI-NOR (`mtd5`)**,
reached by holding reset ~10 s at power-on, telnet with `ubnt:ubnt`. It lives in
a *different flash device* from the boot partition, so it survives a bad `uImage`
— giving most of kexec's "power-cycle to recover" property without kexec.

**Caveats** ✅: UrNVR pins firmware **2.3.14** (2022) and is 4.5 years stale. Its
setup mode `sgdisk -d`'s partitions 2–5 — destructive — so keep `setup=0` until a
kernel boots. Whether `/dev/boot1` is still mountable rw on UniFi OS 4.x/5.x is
**untested and is the first experiment**.

**U-Boot signature check** ✅: linux-alpine-v2's TODO records verification as
*currently skipped* rather than satisfied. How it is skipped is undocumented and
could bite when dropping an unsigned `uImage`.

## 11b. Open questions — ANSWERED offline 2026-08-15 ✅

Done without a running device: the kernel has `CONFIG_IKCONFIG=y`, so its config
is embedded in the image. `scripts/analyse-unvr-firmware.py --extract` pulls it
out of `sources/UNVR-5.1.25.bin`.

| Question | Answer |
|---|---|
| Module signing enforced? | **No** — `# CONFIG_MODULE_SIG is not set`. You can insmod your own modules |
| Kernel lockdown? | **No** — absent entirely |
| kexec available? | **No** — `# CONFIG_KEXEC is not set`. Confirms kexec needs `kexec-mod`, which is unported to Alpine V2 → **kexec is out** |
| al_eth a module? | **No** — `CONFIG_NET_AL_ETH=y`, built in. No `.ko` to lift |

Remaining checks that still need a live box (ours may not boot — failing USB):

```
cat /proc/mtd
mount /dev/boot1 /mnt     # then try writing a scratch file
```

## 12. Mining plan

1. Pull the confirmed GPL drops locally: `UrNVR/unvr-kernel` (4.19.152) and
   `NeccoNeko/UBNT-source-code` (1.3.35 / 4.1.37).
2. Diff their `drivers/net/ethernet/al/` against linux-alpine-v2's out-of-tree
   `al_eth` — establish what changed and which is newest.
3. Check UDM-Pro/UDR GPL archives on Internet Archive for a newer Annapurna HAL.
4. Extract `vmlinuz` + DTB from our 5.1.25 image; compare its DTB against
   `alpine-v2-ubnt-unvr.dts` and its config against `unvr_defconfig`.
5. Evaluate kexec (udm-kernel-tools) as a no-write bring-up path.
6. Only then touch u-boot env or the boot device — and only after verified dumps.

**Preserve before you write.** Full verified dumps of `/dev/mtd*` and the boot
device first. Our own boot USB is already failing — see [recovery.md](recovery.md).
