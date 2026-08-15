# UNVR boot-USB failure and recovery

Researched 2026-08-15. Applies to the UNVR (not UNVR-Pro) whose boot USB is
being imaged in `images/`.

## The failure is a known product defect

- UNVR keeps UniFi OS on an internal USB flash stick, glued in place.
- Symptom: GUI dies, then reboot sticks on flashing white LED, no network.
- Ubiquiti community thread: 38.3K views, 368 replies, now locked.
  <https://community.ui.com/questions/UNVR-stopped-responding-just-white-flashing-light/a051f869-8349-4a2a-a72a-ce3a8aa8c759>
- Ours is a Phison `UF3 8GB`; community reports the stock part is 8 GB and
  under-specced for the write load (constant logging).

## You do NOT need a disk image to restore the device

Repeatedly confirmed in the thread — the UNVR populates a **blank** USB itself:

- Power off, remove old USB (heat gun / screwdriver to break the glue).
- Fit a new good-quality USB. Community used Samsung Fit USB 3.1 32 GB.
- Temporarily pull the HDDs (precautionary).
- Boot. It formats and self-populates; allow up to 30 min.
- Power off, refit HDDs, boot, reconfigure.

Source: <https://www.incredigeek.com/home/unifi-unvr-recover-from-failed-usb-drive/>

**RAID/recordings live on the HDDs, not the USB**, and Protect re-reads the
array. Recordings generally survive.

**What a blank USB loses:** users and console config. A Protect backup restores
video groups but *not* users — invites must be resent. This is the one reason
our image matters: it is the only copy of the old `/data/etc` + unifi-core
state.

## Firmware from Ubiquiti

Queried `fw-update.ui.com` API 2026-08-15 (`filter=eq~~platform~~unvr`):

| | |
|---|---|
| Platform / product | `UNVR` / `unifi-nvr` |
| Version | `v5.1.25+84c48e7` |
| ubnt_version | `UNVR4.al324.v5.1.25.84c48e7.260710.1602` |
| Released | 2026-07-27 |
| Size | 786253430 bytes (~750 MiB) |
| sha256 | `74f2833356e832bd97f59bb3686eaedaf704335631b5daabedfce4d30bb222fc` |
| md5 | `3f661a852b6fb70946e2a94712df6923` |
| URL | <https://fw-download.ubnt.com/data/unifi-nvr/44dc-UNVR-5.1.25-4d75bc90-b895-4d8f-baff-382f24fd36cf.bin> |

Re-query: `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr`

- It is a **`.bin` firmware, not a raw USB image.** Ubiquiti publishes no
  flashable disk image for the boot stick.
- Our stick was last written 2022-10-24 (UniFi OS 1.x era). A fresh install
  lands on 5.1.25 — a large version jump, not a like-for-like restore.

## Recovery Mode (alternative path)

For "NVR & NVR-Pro" per <https://help.ui.com/hc/en-us/articles/360043360253-UniFi-Recovery-Mode>:

- Power off; hold Reset while reconnecting power; hold 5 s.
- Ethernet from PC to LAN port 1; set static 192.168.1.11/24.
- Browse `http://192.168.1.30` — **HTTP only**, browsers may force HTTPS.
- Firmware Update → upload the `.bin` above.

Needs a device that still boots far enough to enter recovery. A dead boot USB
generally does not, hence the USB swap being the usual fix.
