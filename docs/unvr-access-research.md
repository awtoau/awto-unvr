# UNVR stock/official access paths — research

Six questions on how to get a shell / push our own kernel/firmware to the stock
UNVR (Alpine V2 / AL-324, UniFi OS + Protect). Method A = web (playwrong), method
B = reverse the stock firmware on disk.

Firmware reversed: `sources/UNVR-5.1.25.bin` → squashfs @ 15242534 (zstd),
`unsquashfs -o 15242534`. Kernel config pulled via
`scripts/analyse-unvr-firmware.py --extract` → `tmp/sections/kernel.config`
(IKCFG in-kernel, `CONFIG_IKCONFIG=y`). Firmware extracts are ephemeral (not
committed). Paths below written `fw:/...` = path inside the extracted rootfs.

Evidence marks: ✅ confirmed (read it here) · 〄 inferred · 📄 Ubiquiti/community claim.

Live update API (fetched): the whole update chain is confirmed reachable, see Q3.

---

## Q1 — SSH / serial shell (official)

**SSH: YES, official, web-UI toggle.** ✅

- Ubiquiti doc: SSH is a supported (if discouraged) access method. UniFi **Consoles
  (UDM Pro / UNVR / CloudKey): SSH disabled by default**; enable at
  **Settings → Control Plane → Console → check SSH**. Older UI:
  Settings → System. Username always `root`.
  Source: help.ui.com/hc/en-us/articles/204909374 (Connecting to UniFi with Debug
  Tools & SSH) — fetched ✅.
- **Default creds** (pre-adoption / factory): **`root` / `ui`** (older gen
  `root`/`ubnt`). Same doc ✅, and matches baked `/etc/shadow` (`credentials.md`).
- Port 22 (configurable, see Q4). Once adopted, the SSH password is whatever the
  setup wizard / Control-Plane panel set; a random one if unset.
- **Simple Mode alternative — "Debug" console**: UniFi Network device panel →
  Settings → bottom → Debug opens a browser terminal to the device without SSH.
  Same doc ✅. (Network devices, not the NVR host shell.)

Firmware side (method B) confirms it is a **full OpenSSH server**, not a stub:

- `fw:/usr/sbin/sshd` — OpenSSH 864 KB ✅.
- `fw:/lib/systemd/system/ssh.service` ✅ — `ExecStart=/usr/sbin/sshd -D`,
  `WantedBy=multi-user.target`, gated by
  `ConditionPathExists=!/etc/ssh/sshd_not_to_be_run`. Symlinked
  `fw:/etc/systemd/system/multi-user.target.wants/ssh.service` → enabled in image,
  but the runtime state is driven by the toggle (Q4), which does
  `systemctl enable/disable ssh`.
- `fw:/etc/ssh/sshd_config` ✅: `PermitRootLogin yes`, `PasswordAuthentication no`
  **but** `ChallengeResponseAuthentication yes` + `UsePAM yes` → root logs in via
  **PAM keyboard-interactive** (password prompt still works). `sshd_config.d/`
  empty; the toggle rewrites `PasswordAuthentication` when needed (Q4).

**Serial console: YES.** 📄/✅ (already established in this project)

- UNVR: 4-pin header behind the SFP+ cage, mid-PCB; GND/TXD/RXD (ignore 3V3),
  **115200 8N1**, 3.3 V TTL. UNVR Pro: near USP-RPS connector. (`sources.md` §8,
  `credentials.md`.)
- Login `root`/`ui` (al324 gen). Recovery-mode serial/telnet `root`/`ubnt`.

**Confidence: high.** SSH is the intended official host shell; enable in web UI,
log in as root. No serial teardown needed once enabled.

---

## Q2 — Upload firmware via the web interface

**Mechanism exists, but it is a signed `.bin` and the offline-upload UI is not
exposed for the NVR the way it is for adopted network devices.** ✅

Backend chain (method B), all in `fw:/usr/share/unifi-core/app/service.js`
(Node app, minified) + `fw:/sbin/ubnt-systool` (bash) ✅:

- Firmware image path constants: `GD="fwupdate.bin"`, internal
  `/data/unifi-core/firmware/fwupdate.bin`, external
  `/srv/unifi-core/firmware/fwupdate.bin`, console-group copy
  `/data/unifi-core/consoleGroup/fwupdate.bin`; generic upload dir
  `/data/unifi-core/uploads`. (`service.js` `firmware.internalDir/externalDir` in
  `config/default.yaml` ✅.)
- nginx exposes `location ~* /api/consoleGroups/(backup|firmware)$` →
  `uos_api_backend` (`fw:/usr/share/unifi-core/http/site-shadow.conf` ✅) — the
  console-group firmware push endpoint (X-Group-Token auth). This is a
  cloud/group-driven firmware channel, not a plain "upload my .bin" form.
- Apply path: unifi-core → **`ubnt-systool fwupdate <file|url>`** ✅. That handler
  (`do_fwupdate`, `ubnt-systool` L1312) ✅:
  1. accepts a **local file OR a URL** (wget/curl download);
  2. `fwinfo -k` sanity, then on UNVR **`ubntnas system upgrade <file> -f
     --no-reboot`** (else fallback `fwupdate -c` + `fwextract` + `fw_move` to
     `/boot/fwupdate.bin`);
  3. reboots to apply.

**Signature check — this is the blocker.** ✅

- `fw:/sbin/fwupdate` strings ✅: `EVP_sha1`, `EVP_VerifyFinal`,
  `ERROR: Bad FW Image Signature`, `-s RSA public Key file`, writes
  `/dev/mtdblock%d` (incl. uboot). → **RSA + SHA1 signature over the UBNT
  container, verified before flashing.** A modified/unsigned `.bin` is rejected.
- So a web-upload of a *custom* image does not boot: format = UBNT container
  (`analyse-unvr-firmware.py`), and it must carry a valid Ubiquiti RSA signature
  we do not have. No signing key leaked (prior RE).

**Payoff for us:** the web/UI upload is **not** an easier route to custom
code — the signature gate is the same one U-Boot/`fwupdate` enforce. The
unsigned-uImage fact (this gen boots unsigned kernels from `/boot`) does **not**
help here, because `fwupdate`/`ubntnas` verify the *container* signature before
they ever write the kernel partition. Bypass is at the `/dev/mtd` layer instead
(see Q5), not the upload form.

**Confidence: high** on the chain + signature. **Medium** on "no offline-upload
button in the NVR UI" (backend supports file input; whether the shipped UI
surfaces a file picker for the console host is not UI-confirmed here).

---

## Q3 — Automatic-update firmware download

**Confirmed end-to-end, live.** ✅

Update-check (from `service.js` ✅):
```
GET {fwUpdateUrl}/api/firmware-latest?filter=eq~~product~~<product>
                                     &filter=eq~~platform~~<platform>
                                     &filter=eq~~channel~~<channel>
```
- `fwUpdateUrl = https://fw-update.ubnt.com` (prod; stg/dev variants
  `stg-fw-update`/`dev-fw-update`). `config/default.yaml` ✅.
- params built by `pD("/api/firmware-latest",{product,channel,platform})`,
  `Nf=(k,v)=>filter=eq~~${k}~~${v}` ✅. `product` = `unifi-nvr`
  (`firmware.product`, default `unifi-firmware`), `platform` = `UNVR`,
  `channel` = `release` (or beta/etc).
- Optional `Authorization` header = cloud token (`gd()`), added only if a cloud
  session exists; the check works unauthenticated.

Live response (playwrong-fetched
`https://fw-update.ubnt.com/api/firmware-latest?filter=eq~~product~~unifi-nvr&filter=eq~~platform~~UNVR&filter=eq~~channel~~release`) ✅:
```json
{"_embedded":{"firmware":[{"channel":"release","file_size":786253430,
 "md5":"3f661a852b6fb70946e2a94712df6923",
 "sha256_checksum":"74f2833356e832bd97f59bb3686eaedaf704335631b5daabedfce4d30bb222fc",
 "platform":"UNVR","product":"unifi-nvr",
 "tags":{"ubnt_version":"UNVR4.al324.v5.1.25.84c48e7.260710.1602"},
 "version":"v5.1.25+84c48e7",
 "_links":{"data":{"href":
   "https://fw-download.ubnt.com/data/unifi-nvr/44dc-UNVR-5.1.25-4d75bc90-....bin"}}}]}}
```
- **Download host/URL: `https://fw-download.ubnt.com/data/unifi-nvr/<hash>-UNVR-<ver>-<uuid>.bin`** ✅ (matches the file already in `sources/`).
- Response carries `md5` + `sha256_checksum` (integrity) and the exact
  `ubnt_version` string.
- Target-version fetch: `GET {fwUpdateUrl}/api/firmware?filter=…product/platform/version/probability` (`service.js` `ws()`) ✅.

**Can an on-LAN MITM/redirect serve a custom image? NO.** ✅

- Transport is HTTPS to `fw-update.ubnt.com` / `fw-download.ubnt.com` (TLS; a
  transparent redirect needs a trusted cert). Even granting a redirect, the
  downloaded `.bin` is **RSA+SHA1-signed** and `fwupdate`/`ubntnas` verify it
  (Q2) → "Bad FW Image Signature". The API's `sha256_checksum` is a second gate.
- So MITM of the update channel is not a custom-boot path. Its only use would be
  pinning/holding a *legitimate* signed image.

**Confidence: high** (API fetched live; chain read in binary).

---

## Q4 — Stock toggle to enable TELNET / SSH / debug shell

**SSH: single stock toggle, no serial needed.** ✅ — highest-value Q1/Q4 result.

- The toggle is **`ubnt-systool sshd <true|false>`** (`fw:/sbin/ubnt-systool`
  `do_sshd`, L854) ✅:
  ```
  enable : systemctl enable ssh ; systemctl --no-block restart ssh
  disable: systemctl disable ssh ; systemctl kill ssh ; terminate ssh sessions
  status : systemctl -q is-enabled ssh
  ```
- unifi-core drives it: `fd(e)` → `Q("ubnt-systool",["sshd",String(e)])`; state
  read from `ucore/system-data.json` (`sshEnabled` + `hashedSshPassword`), gated
  by `pC.ssh` capability + a cloud **SSH agreement** (`agreementAcceptedAt`) and
  applied on boot by `KXe()` (`service.js` ✅).
- Password set via **`ubnt-systool sshpasswd set <crypt>`** →
  `echo "root:<hash>" | chpasswd -e` (L1919) ✅. Get = reads root line from
  `/etc/shadow`.
- Related stock sub-toggles (all in `ubnt-systool`) ✅:
  `sshd-port <n>` (rewrites `Port` in sshd_config), `sshd-passwdauth
  <true|false>` (rewrites `PasswordAuthentication`), `sshd-authkeys [file]`
  (installs `/root/.ssh/authorized_keys`). → key-based root login is a supported
  path.

**So from any root context you can just `ubnt-systool sshd true` (or plain
`systemctl enable --now ssh`) to get network root SSH.** The web toggle does
exactly this.

**Telnet: not a stock service, but the applet is on the box.** ✅

- No `telnetd`/`in.telnetd` systemd unit; not enabled. But
  `fw:/bin/busybox` includes the **`telnetd` and `telnet` applets** ✅ — a root
  shell can `busybox telnetd -l /bin/login -p <port>` manually. Not gated by
  anything but root (DAC); no SELinux/lockdown (Q5).
- Recovery mode (SPI-NOR `mtd5`, hold reset ~10 s) exposes **telnet `root`/`ubnt`**
  📄 — a separate stock debug path that survives a bad `/boot` uImage.

**Debug shell:** `fw:/lib/systemd/system/debug-shell.service` exists (systemd's
`/dev/tty9` root bash) but is not wired into a target — enable-able from root
only ✅.

No engineering/factory "unlock" flag beyond the above was found; there is a
second UID-0 account `ui` but it is **locked** (`ui:!` in `/etc/shadow`) ✅, and
root's hash is `$5$…` (SHA256 crypt) ✅ — not the enable path.

**Confidence: high.** The SSH toggle is the single best find: network root shell
with no console teardown.

---

## Q5 — Direct hardware access from userspace

**Very open. From a root shell the SoC is wide open — this is the real bypass of
the signed-update path.** ✅ All from `tmp/sections/kernel.config` (this image's
own `CONFIG_IKCONFIG`) unless noted.

- **/dev/mem: unrestricted MMIO.** `CONFIG_DEVMEM=y` **and
  `# CONFIG_STRICT_DEVMEM is not set`** ✅ → `/dev/mem` maps arbitrary physical
  addresses incl. SoC registers (clock/PLL/pinmux 0xf00xxxxx, DBGEN
  0xf0070008, thermal, 0xfd8xxxxx). **`busybox devmem` applet present**
  (`fw:/bin/busybox`, strings `/dev/mem` `devmem`) ✅. → arbitrary MMIO peek/poke
  from a shell, no driver. Gate = **root only** (no STRICT_DEVMEM, no SELinux, no
  lockdown).
- **MTD → userspace NAND/NOR reflash without U-Boot.** `CONFIG_MTD=y`,
  `MTD_BLOCK=y`, `MTD_SPI_NOR=y`, `MTD_NAND=m` ✅. Tools present:
  `fw:/usr/sbin/{flashcp,nandwrite,nanddump,flash_erase,flash_eraseall,mtd_debug,mtdinfo}`
  ✅. → **write `/dev/mtdN` (kernel, uboot, recovery) directly from a shell**,
  bypassing the RSA-signed `fwupdate`/`ubntnas` container check entirely. This is
  the cleanest custom-image route given a root shell: `flashcp`/`nandwrite` our
  own uImage/uboot. (Dump first — prior RE and `sources.md` §12.)
- **Raw I2C.** `CONFIG_I2C_CHARDEV=y` → `/dev/i2c-*` ✅. `busybox`
  `i2cget/i2cset/i2cdump/i2cdetect` present ✅. Stock daemons already use it:
  `fw:/sbin/rpsd` (`/dev/i2c-`), `fw:/usr/sbin/sfpd` (`/dev/i2c-%d`) ✅ — though
  per #64, `rpsd`'s i2c access is a no-op on this specific ea16 board: `i2c_gen`
  isn't a real bus here (pins 30/31 are muxed to ETH-LED/ulogo_blue, not i2c),
  so there's no RPS monitor to reach via i2c regardless. `sfpd` (module EEPROM
  on the mux'd `i2c_pld` bus) is the one genuinely useful daemon-shadowed target
  here (stop it first to avoid contention).
- **SPI:** `CONFIG_SPI_SPIDEV=y` → `/dev/spidev*` possible ✅.
- **GPIO:** `CONFIG_GPIO_SYSFS=y` (sysfs `/sys/class/gpio`) ✅; Python libgpiod
  `fw:/usr/lib/python3/dist-packages/gpiod*.so` present ✅.
- **debugfs:** `CONFIG_DEBUG_FS=y` ✅ (register/knob exposure available).
- **UIO / VFIO: NOT available.** `# CONFIG_UIO is not set`; no VFIO in config ✅.
  No `/dev/uio*` or vfio-pci hand-off of al_eth/al_ssm/SATA to userspace. (Use the
  in-kernel drivers or /dev/mem instead.)
- **Watchdog:** `/dev/watchdog` via the Alpine WDT (see hardware docs); not the
  focus here.

**Only gate anywhere is DAC (root).** No SELinux, no kernel lockdown, no
`STRICT_DEVMEM`, no module-sig (`# CONFIG_MODULE_SIG is not set`) — so also
`insmod` of our own `.ko` (`busybox insmod` present).

**Payoff ranking (given a root shell):**
1. **`flash_erase`+`nandwrite`/`flashcp` on `/dev/mtdN`** → reflash NAND/NOR
   (kernel/uboot/recovery) with no signature check. *The* custom-boot route.
2. **`busybox devmem`** → set DBGEN, poke clock/pinmux/thermal regs directly.
3. **`busybox i2cset/i2cget` on `/dev/i2c-*`** → SFP EEPROM + RTC/mux (i2c_pld);
   not `i2c_gen`/RPS monitor - no real bus there on this board (#64).

**Confidence: high** (kernel config is this image's own; tools verified in rootfs).

---

## Q6 — Stock file-transfer / remote services (push a custom image)

What ships, whether it listens, and whether we can push with it. Firmware = `fw:`.
`busybox` applets from `fw:/bin/busybox` ✅.

| Service | In firmware | Default state | Use to push? |
|---|---|---|---|
| **OpenSSH sshd** (scp/sftp) | `fw:/usr/sbin/sshd` ✅ | **disabled**, one toggle (Q4) | **Yes — best.** `ubnt-systool sshd true`, then `scp`/`sftp` in. Key auth via `sshd-authkeys`. |
| ssh.socket | `fw:/lib/systemd/system/ssh.socket` ✅ | present (socket-activation alt) | same as sshd |
| **telnetd** | busybox applet ✅ | no unit, off | Yes, manual: `busybox telnetd` from root (cleartext) |
| **tftp client** | `fw:/bin/tftp` + busybox `tftp` ✅ | n/a (client) | **Yes — pull our kernel** from a LAN TFTP server onto the box |
| tftpd (server) | busybox `tftpd`? (not confirmed) | off | n/a |
| ftp client | apt method only `fw:/usr/lib/apt/methods/ftp`; busybox `ftpget/ftpput` ✅ | n/a | ftpget/ftpput usable from root |
| ftpd/vsftpd | **absent** ✅ | — | — |
| **rsync** | `fw:/usr/bin/rsync` ✅ | client (no rsyncd unit) | Yes, over ssh: `rsync -e ssh` |
| **nc / netcat** | `fw:/bin/nc`, `fw:/bin/netcat`, busybox `nc` ✅ | n/a | Yes, ad-hoc `nc` file transfer |
| curl / wget | `fw:/usr/bin/{curl,wget}` + busybox `wget` ✅ | client | **Yes — pull image over HTTP(S)** |
| busybox httpd | applet ✅ | off | serve files off the box if needed |
| socat | **absent** ✅ | — | — |
| xinetd | **absent** ✅ | — | — |

Stock update/transfer endpoints:
- `ubnt-systool fwupdate <url>` ✅ accepts a **URL** (wget/curl) — but still
  signature-checked (Q2). Not a custom-image path.
- `/api/consoleGroups/firmware` nginx endpoint (Q2) — cloud/group push, signed.
- No inform/adopt firmware-push usable for custom code (signed).

**U-Boot-level transfer** (from `sources/.../UNVR-1.3.35-GPL/u-boot`, `sources.md`
§3, and boot docs) 〄/✅:
- `tftpboot` / `tftpput` (DHCP + TFTP), `dhcp`, `nfs`, and serial
  `loadb`/`loadx`/`loady` (kermit/xmodem/**ymodem**). Standard Ubiquiti Alpine V2
  U-Boot config carries these.
- **`loady`** = ymodem receive over the **same serial console we already have** →
  push a uImage with zero extra infra. **`tftpput`** = pull files *off* the device
  over TFTP (dumps). `tftpboot` = fetch our kernel to RAM and `bootm`.

**Easiest "push a file / custom image" methods, ranked (we already have root
serial + LAN):**
1. **Enable sshd (`ubnt-systool sshd true`) → `scp` the file in.** One command,
   network, keys supported. Then reflash via `/dev/mtd` (Q5) or drop a uImage in
   `/boot`.
2. **`busybox wget`/`curl`/`tftp` from the existing root shell** to pull the
   image off a LAN server — no service to enable at all.
3. **U-Boot `loady`** (ymodem over the serial we already use) or **`tftpboot`** —
   bootloader-level, good for one-shot RAM-boot testing without touching flash.
4. `nc`/`rsync-over-ssh` as fallbacks.

Note all of these move *bytes*; the boot itself is unsigned at the U-Boot/uImage
layer (this gen boots unsigned kernels — linux-alpine-v2 TODO ✅, and `fwupdate`'s
signature gate is above the raw `/dev/mtd` write), so a hand-placed uImage /
mtd-write **runs without a signature**. The only signed gate is the
`fwupdate`/`ubntnas` *container* path (Q2/Q3), which we simply don't use.

**Confidence: high** on presence/state; **medium** on the exact U-Boot command
set for *our* 5.1.25 build (read from the 1.3.35 GPL U-Boot + community, not our
running env).

---

## Bottom line for the port

- **Get a shell the easy way:** enable SSH in the web UI (Settings → Control Plane
  → Console → SSH) or `ubnt-systool sshd true`; log in `root` / (set password).
  No serial teardown. (Q1/Q4)
- **Do NOT expect the web upload or update channel to boot custom code** — both go
  through RSA+SHA1 container verification in `fwupdate`/`ubntnas`. (Q2/Q3)
- **The custom-boot route is below that gate:** from the root shell, `/dev/mtd*` +
  mtd-utils reflash NAND/NOR unsigned, or drop an unsigned uImage in `/boot`; the
  SoC is fully reachable via `/dev/mem` (no STRICT_DEVMEM) and `/dev/i2c-*`. (Q5)
- **Move the bytes** with scp (enable sshd) or wget/tftp already on the box, or
  U-Boot `loady`/`tftpboot`. (Q6)

---

## Our deployment — reaching the Fedora box (woomera)

Not the stock image: our Fedora-44 build on the SSD (kernel `7.1.8-dirty`). SSH is
the intended default — `build-fedora-rootfs.py` ships sshd enabled + `PermitRootLogin
yes` ([fedora-on-ssd.md](fedora-on-ssd.md) L65). Serial console was bring-up only.

- **Address: DHCP lease on `enp0s1`, it MOVES.** Never write it down — resolve it.
  | When | Address |
  |---|---|
  | 2026-08-16T22:33 | `192.168.25.149/24` |
  | 2026-08-18T08:34 | `192.168.25.106/24` (.149 reassigned to a non-Ubiquiti host) |
  NOT the netboot-era static `.140`: `build-fedora-rootfs.py` runs systemd-networkd
  DHCP on all ethernet, so the box takes a lease; `.140` never applied to the Fedora
  rootfs. `ssh` to a stale address fails **refused** (another host answers) or
  **timeout** (nobody there) — neither means the box is down.
- MAC `74:ac:b9:41:a8:11` (OUI 74:AC:B9 = Ubiquiti) — the only stable handle.
  Fedora 44 aarch64, `SSH-2.0-OpenSSH_10.2`, password auth OK as `root`.
- **Find it: `python3 scripts/find-woomera-ssh.py`** — sweeps by MAC OUI, prints the
  current address. Run this instead of trusting any address in these docs.
- Found 2026-08-16T22:33+10:00 from `192.168.25.145/24` via
  `scripts/find-woomera-ssh.py` + MAC OUI (no serial). Gotcha: Fedora does not tag
  its SSH banner, so the distro-string heuristic is useless — MAC OUI is the
  discriminator. Ruled out on the way: `.140` ping/ssh (timeout), `ssh woomera` (no
  DNS), Ubiquiti UDP 10001 discovery (no answer), mDNS (none); a 22/443 sweep found
  13 hosts but named none.

Open hardening (was draft issue 38, not filed to GitHub):
- ~~Hostname is `fedora`~~ — **done**, reports `woomera` as of 2026-08-18T08:34.
- Root password still the build default (`build-fedora-rootfs.py` L67, marked CHANGE
  THIS) + `PermitRootLogin yes` → any LAN host can root the box over SSH.
- **Clock is wrong** — box said `22:35` when the host said `08:34` (~10 h out). No
  RTC battery / no NTP sync. Breaks file mtimes, build stamps and log correlation.
- Key-based login not set up — `ssh-copy-id` blocked by the agent permission
  classifier 2026-08-16; needs the owner to run/grant it.
- DHCP lease, not a reservation — address moves on its own (what rotted `.140`).
  Router reservation is the cheapest durable fix.
- `woomera` does not resolve — add `/etc/hosts` or `~/.ssh/config` Host block.
- [workflow.md](workflow.md) still says `192.168.25.140/24` — update after pinning.
- Acceptance: `ssh woomera` by name, key-based, no prompt; address survives reboot;
  serial needed only when the network is down.
