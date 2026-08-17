# UNVR security posture vs Ubiquiti's public claims

Security-research analysis of the UNVR's **actual** boot/storage/recording security against
Ubiquiti's public "secure UniFi Protect recordings" posture. **No third-party PII in this
repo by design.** Mechanism-level only — not an attack playbook.

(Consolidated from issue #2, now closed in favour of this doc. Access paths:
[unvr-access-research.md](unvr-access-research.md); stock-unit programming: #66.)

## 1. Unsigned OS boot

- sysid ea16 boots `bootnand → dobootm=run bootunsign → bootm` — a legacy uImage verified
  by **CRC32 only**, no FIT/RSA/signature check compiled in. **Any modified kernel+rootfs
  boots.** RSA/eFuse checks exist but are **preboot-only** (gate SoC capability flags, not
  the OS kernel). See [nor-boot-chain.md](nor-boot-chain.md), [uboot-update-path.md](uboot-update-path.md).
- The *update container* (`.bin`) IS RSA-SHA1 signed and enforced on-device by `fwupdate` —
  but that only gates the **updater**, not what U-Boot boots. Write an unsigned uImage to
  the boot MTD directly and it runs (this is the basis of #66).

## 2. Data remanence on resale

- Unit received with the **prior owner's data fully intact** — local accounts, ~2,500 bcrypt
  hashes, Protect schedules, a personal photo in NAND. **No secure-erase at decommission.**
  (Specific PII deliberately excluded from this repo.)

## 3. At-rest exposure

- USERDEV overlay + config partition are plain **ext4**; the accounts DB (PostgreSQL) was
  readable **offline, no decryption**. Recordings-at-rest encryption **unconfirmed** (open).

## 4. Local root → recordings are tamperable with minimal on-box footprint

### Exposure set — NOT "anyone remotely"
Root is gated; state it precisely:
- **SSH is OFF by default**; the enable toggle (`ubnt-systool sshd true`, UI: Settings →
  Control Plane → Console → SSH) **requires UniFi OS admin auth**. Root creds (`root`/`ui`)
  don't help until it's on.
- Realistic reachers of root: an **admin / insider**, **stolen admin credentials**, or
  **physical access** (serial header behind the SFP cage; SPI-NOR recovery-mode telnet
  `ubnt:ubnt`; or pulling the drives). **Not** an unauthenticated remote attacker.

### No cryptographic barrier once root
- Recordings are **plain files on the SATA ext4 array** — root reads/modifies/deletes them
  directly; no app-layer gate.
- Root can **alter/erase the local logs** that would record the change → **local forensic
  footprint is minimal** (the same access does the deed and the cleanup).
- With unsigned boot, a tamperer can also run a modified kernel/userland — no integrity gate.

### Timeline fabrication (the evidentiary impact)
Because footage carries **no tamper-evidence**, local root in a **short window (~20 min)**
can not only **delete clips** but **fabricate a timeline** undetectably on the box:
- Remove the segment(s) covering an event so a period **appears uneventful** — e.g. an
  entry/intrusion that "never happened."
- Manipulate **timestamps / segment ordering / duration** ("time-stretching") so the
  recorded chronology no longer matches reality — a scene made to look **not entered** at the
  real time, or shifted outside a window of interest.
- Nothing on the device cryptographically binds a frame to a wall-clock time or to its
  neighbours, so these edits leave **no on-disk integrity mismatch** to detect.

### What DOES leave a trace (the "no footprint" caveat)
- **Enabling SSH is an admin action** typically logged in the **UniFi cloud activity/audit
  log** — an *off-device* record local root can't easily scrub.
- **Cloud-synced clips / off-device backup**, if configured, diverge from a locally-edited copy.
- **Only if** Protect hashes/signs clips at capture (keys unreachable by root) would on-disk
  tampering be *detectable*.

## The gap vs the public stance

The device gives **local root no cryptographic tamper-evidence over footage** and **no boot
integrity**. UNVR recordings are therefore trustworthy only insofar as (a) nobody gains
admin/physical access and (b) off-device audit exists — **not** cryptographically tamper-
evident on the box. For **evidentiary / chain-of-custody** use that is a real gap against a
"secure recordings" framing.

## Open questions / to verify

- [ ] Gather Ubiquiti's **public** claims about Protect recording security/privacy (via
  playwrong per the repo web rule) and pin the exact wording to compare against.
- [ ] Does Protect apply **integrity protection at capture** (per-clip hash/signature/HMAC,
  keys not reachable by root)? If not, footage has **no tamper-evidence** — the concrete
  claim to test.
- [ ] Are **recordings + metadata encrypted at rest** on the array? (HDDs currently absent.)
- [ ] What does the **cloud audit log** actually record (SSH-enable, config resets, deletions)
  — the only off-device footprint.
- [ ] Whether the boot MTD is **rw-writable from the SSH shell** on UniFi OS 4.x/5.x (#66).
- [ ] Responsible-disclosure consideration if a real gap vs stated posture is confirmed.

Relates to #1 (broader security analysis).
