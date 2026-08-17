# UNVR vendor/official access paths — SSH, serial, firmware upload, update channel

Scope: Ubiquiti UNVR (Alpine V2 / AL-324, UniFi OS + Protect). Firmware 5.1.25
(`UNVR4.al324`). Goal: find an easier route to run our own kernel/firmware than
serial + NAND reflash.

Evidence marks: ✅ verified here (read the code/page) · 📄 vendor doc · ❓ inferred.

Firmware source: `sources/UNVR-5.1.25.bin`, squashfs @ offset 15242534 (zstd),
extracted read-only to scratch. Paths below are rootfs-relative.

## TL;DR — the highest-value finding

**The official SSH toggle IS the easy route to a root shell — no serial needed.**
- UniFi OS ships **OpenSSH `sshd`** (`/usr/sbin/sshd`, 864 KB) enabled at boot, gated
  by the web-UI toggle **Settings → Control Plane → Console → SSH** ✅.
- Default console login **`root` / `ui`** (older gen `root`/`ubnt`) 📄✅ — same for
  every unit, matches the cracked `/etc/shadow`.
- Toggle backend is `ubnt-systool sshd <true|false>` → `systemctl enable/restart ssh` ✅.
- With that root shell you do the **UrNVR file-swap** (replace the boot-partition
  `uImage`, which this gen boots **unsigned**) — bypassing `fwupdate` entirely.

**Neither web-upload nor update-channel MITM helps**, because both funnel through
`fwupdate`, which enforces an **RSA-SHA1 signature on the whole `.bin` container**
(`ERROR: Bad FW Image Signature`) ✅. The unsigned-uImage fact lives *below* that
check (U-Boot), so it only helps once you already have filesystem write access —
i.e. via the SSH shell, not via the vendor update path.

---

## Q1 — SSH / serial session

### SSH — YES, official, web-UI toggle. Confidence: HIGH ✅📄

- **Enable:** UniFi OS consoles ship SSH **disabled by default**; enable at
  **Settings → Control Plane → Console → check SSH** 📄
  (help.ui.com/hc/en-us/articles/204909374 — "UniFi Consoles: SSH is disabled by
  default. To enable it, navigate to Settings > Control Plane > Console and check
  SSH.").
- **Port:** 22 (standard; `sshd_config` has no non-default `Port`) ✅.
- **User:** always `root` for UniFi Consoles 📄.
- **Default password:** `root` / `ui` (older gen `root` / `ubnt`) 📄, matches the
  baked `/etc/shadow` ✅ (see `docs/credentials.md`). After setup/adoption the SSH
  password is set separately in the UI.
- **Auth mode** (`etc/ssh/sshd_config`) ✅:
  - `PermitRootLogin yes`
  - `PasswordAuthentication no` **but** `ChallengeResponseAuthentication yes` +
    `UsePAM yes` → password login works via PAM keyboard-interactive, not the
    plain password path. `ubnt-systool sshd-passwdauth true` flips
    `PasswordAuthentication` if needed.
  - `Subsystem sftp /usr/lib/openssh/sftp-server` present → SCP/SFTP works.
- **Service unit** `lib/systemd/system/ssh.service` ✅: `ExecStart=/usr/sbin/sshd -D`,
  `WantedBy=multi-user.target`, gated by
  `ConditionPathExists=!/etc/ssh/sshd_not_to_be_run`.
- Community-confirmed working on UNVR (e.g. unifi-common install over UNVR UniFi OS
  4.3.6, `sources.md` §6).

**Use it:** enable SSH in the UI, `ssh root@<unvr-ip>`, password `ui` (fresh) or the
UI-set password. That is a full root shell.

### Serial console — YES, documented. Confidence: HIGH 📄✅

- UNVR: 4-pin header behind the SFP+ cage, mid-PCB. GND/TXD/RXD (ignore 3V3),
  **115200 8N1**, 3.3 V TTL 📄 (`docs/sources.md` §8, `docs/hardware.md`).
- UNVR Pro: 4-pin near the USP-RPS / DC Power Backup connector 📄.
- Login same as SSH: `root` / `ui`. Recovery mode (hold reset ~10 s): telnet/serial
  `root` / `ubnt` 📄✅.

---

## Q2 — Manual firmware upload via the web UI

### YES, but signature-gated. Confidence: HIGH ✅📄

**Web-UI paths (vendor):**
- **Device panel URL paste** 📄 (help.ui.com/hc/en-us/articles/204910064): paste a
  `community.ui.com/releases` firmware link into the address bar in the device
  Settings panel; the console fetches + applies it.
- **Network cache** 📄: cache the update in the Network app, then "Update Available".

**Backend endpoint** — `unifi-core` (Node app,
`usr/share/unifi-core/app/service.js`) ✅:
- Route: **`POST /firmware/update`** (and `GET /firmware/update` for status), plus
  a `consoleGroups/firmware` variant. Registered as
  `Jr.post("/firmware/update",ge(4),Kne)` — `ge(4)` is a role/permission gate
  (owner/admin) ✅.
- Uploaded/downloaded image is written to
  **`/data/unifi-core/firmware/fwupdate.bin`** (const `GD="fwupdate.bin"`,
  `oa.join("unifi-core","firmware")`) ✅. Config `firmware.internalDir:
  /data/unifi-core/firmware`, `externalDir: /srv/unifi-core/firmware`
  (`app/config/default.yaml`) ✅.
- `unifi-core` then invokes the same updater as CLI: `ubnt-systool fwupdate <file>`.

**File format + signature check (the blocker)** ✅ — from `sbin/fwupdate` strings and
`sbin/ubnt-systool` `do_fwupdate()`:
- Container = Ubiquiti multi-part `.bin` (magic-tagged parts; `DEBUG: Part magic`).
- Verify chain: `ubnt-systool fwupdate` → `fwinfo -k` (parse/verify) → `ubntnas
  system upgrade` (or `fwupdate -c`). `fwupdate` does
  **`EVP_VerifyInit`/`EVP_VerifyUpdate`/`EVP_VerifyFinal` with `EVP_sha1` against an
  embedded RSA `-----BEGIN PUBLIC KEY-----`** → **RSA-SHA1 signature over each part**.
  On mismatch: **`ERROR: Bad FW Image Signature`**, abort. On pass it writes
  `/dev/mtdblock%d` (incl. uboot).
- **No signing key leaked**; we cannot forge a valid container. So a
  hand-built/custom `.bin` uploaded via the web UI is **rejected at `fwinfo`/`fwupdate`**.

**CLI equivalents (need the SSH shell from Q1)** 📄:
- `ubnt-systool fwupdate https://fw-download.ubnt.com/.../<img>.bin`
- or `scp file → /tmp/fwupdate.bin; ubnt-systool fwupdate /tmp/fwupdate.bin`
- Same signature check applies — only genuine Ubiquiti images pass.

**Verdict for our goal:** web-upload is NOT an easier custom-kernel route. It only
accepts signed vendor images. The escape hatch is *below* fwupdate (unsigned uImage,
reached from a root shell), not the upload endpoint.

---

## Q3 — Automatic-update firmware download

### Confirmed + expanded. Confidence: HIGH ✅

**Update-check API** — from `service.js` + `default.yaml` ✅:
- Base host (prod): **`https://fw-update.ubnt.com`** (`cloud.prd.fwUpdateUrl`).
  Dev/stg: `dev-fw-update.ubnt.com` / `stg-fw-update.ubnt.com`.
- Query: **`GET {fwUpdateUrl}/api/firmware-latest?product=<p>&channel=<c>&platform=<o>`**
  — code: `n=cloud[E.cloudEnv].fwUpdateUrl; s=pD("/api/firmware-latest",{product,channel,platform})`.
- `channel: release` default (`default.yaml` `firmware.channel`). Platform for us =
  `unvr`. Matches the known
  `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr` (the
  `.ui.com` host + `filter=` form is the public/CDN variant of the same API).
- Response schema: JSON list of `{version, channel, platform, sizeBytes, sha256,
  <cdn download url>}` — CDN link points at **`https://fw-download.ubnt.com/data/...`**
  (per vendor doc example) 📄.
- Related cloud config also hits `config.ubnt.com/cloudAccessConfig.json`,
  `static.ui.com/fingerprint/...` (device DB), none firmware-signing-relevant ✅.

**Authentication:** the update-check is an unauthenticated GET over HTTPS (public
API; the same URL works from a browser). Device identity/entitlement is not required
to *learn* the latest version.

**Is it signature-verified? YES — at the device, not the transport** ✅:
- Transport is HTTPS to `fw-update.ubnt.com` / `fw-download.ubnt.com` (TLS cert
  validation applies).
- Even if you defeat TLS (own CA on the box, DNS/redirect on-LAN), the downloaded
  image still passes through `fwupdate` → **RSA-SHA1 container signature check**
  (Q2). A custom image fails `Bad FW Image Signature`.

**Can an on-LAN MITM/redirect serve a custom image?**
- To serve a *different genuine Ubiquiti image* (e.g. force a specific/older signed
  version): **plausible** — redirect `fw-download.ubnt.com` to your host serving a
  real signed `.bin`; it will verify and flash. Useful only for
  downgrade/version-pinning, not custom code.
- To serve a *custom/unsigned image*: **NO** — blocked by the RSA-SHA1 check.

**Verdict:** update-channel MITM is not a custom-firmware route. Signature is
enforced on-device regardless of transport.

---

## Q4 — Vendor toggle to enable telnet / ssh / debug shell

### SSH: YES — first-class vendor toggle. Confidence: HIGH ✅

- **`ubnt-systool sshd <true|false>`** (`sbin/ubnt-systool`, `do_sshd()`) ✅:
  - enable → `systemctl enable ssh; systemctl --no-block restart ssh`
  - disable → `systemctl disable ssh; kill ssh; terminate live sshd sessions`
- Companion subcommands ✅: `sshd-authkeys [file]` (install `authorized_keys`),
  `sshd-passwdauth [true|false]` (flip `PasswordAuthentication`),
  `sshd-port [n]` (change port, validated by `sshd -t`).
- The web-UI SSH checkbox (Q1) and `unifi-core` call exactly this. So the "cloud
  portal / Protect UI SSH setting" == `ubnt-systool sshd true` == root shell.
- **This is the single highest-value access path**: root without the serial console.

### Debug console (UI) 📄
- "Simple Mode: Debug Console" — Settings → device → Debug — an in-app shell to the
  device, no SSH needed (help.ui.com/204909374). Also a root-capable shell via the UI.

### Telnet / dropbear / adb: NO general-purpose shell in the running OS ✅
- No `telnetd`, `dropbear`, or `adb` binary in the rootfs ✅ (only `sbin/fwupdate`,
  OpenSSH). `debug-shell.service` exists (stock systemd, early root shell on
  `/dev/tty9`) but is not enabled and needs local TTY access.
- **Recovery mode** (hold reset ~10 s at power-on) exposes **telnet `ubnt:ubnt`** on
  a separate SPI-NOR recovery kernel 📄✅ — a distinct, documented factory/recovery
  path, not a runtime toggle.

### No engineering/factory runtime backdoor found ✅
- Second UID-0 account `ui` is **locked** (`ui:!`) — not a login (prior RE).
- No hidden env flag beyond the `ubnt-systool sshd`/`sshd-*` family and the
  `sshd_not_to_be_run` condition file.

---

## Synthesis — easiest route to our own kernel

1. **Enable SSH** in the UI (or `ubnt-systool sshd true`) → `ssh root@unvr` (`ui`).
   No serial, no disassembly. HIGH confidence this works on a booting stock unit.
2. From that shell, do the **UrNVR file-swap**: back up `/dev/mtd*` + boot device,
   mount the boot partition, replace `uImage` with our unsigned image (this gen
   boots unsigned — U-Boot verification is skipped), keep the original beside it.
   **Does not touch `fwupdate`, so the RSA-SHA1 container signature is irrelevant.**
3. `fwupdate` / web-upload / update-MITM are all dead ends for *custom* code — all
   enforce the container signature. They only serve genuine signed Ubiquiti images.

Open/untested (needs a live box; ours has a failing USB):
- Whether the boot partition is mountable rw on UniFi OS 4.x/5.x from the SSH shell.
- Whether any block-write-protection module (UDM-Pro-only elsewhere) applies to UNVR
  (no evidence it does).

## Evidence index (firmware paths, rootfs-relative)
- `usr/sbin/sshd` (OpenSSH), `etc/ssh/sshd_config`, `lib/systemd/system/ssh.service`
- `sbin/ubnt-systool` — `do_sshd()` L854, `do_fwupdate()` L1312, `sshd-*` subcmds L26-29
- `sbin/fwupdate` — RSA `BEGIN PUBLIC KEY` / `EVP_sha1` / `EVP_VerifyFinal` /
  `ERROR: Bad FW Image Signature` / `/dev/mtdblock%d`
- `sbin/fwinfo`, `usr/bin/ubntnas` (`system upgrade`)
- `usr/share/unifi-core/app/service.js` — `POST /firmware/update` (`ge(4)`),
  `fwupdate.bin`, `firmware-latest` query builder
- `usr/share/unifi-core/app/config/default.yaml` — `cloud.prd.fwUpdateUrl`,
  `firmware.internalDir/externalDir/channel`

## Web sources
- help.ui.com/hc/en-us/articles/204909374 — SSH enable location + default creds ✅
- help.ui.com/hc/en-us/articles/204910064 — manual/advanced update (URL paste, SSH
  `ubnt-systool fwupdate`, `fw-download.ubnt.com` example) ✅
