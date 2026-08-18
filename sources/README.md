# sources

Downloaded artifacts. Binaries are gitignored; this manifest is the tracked part.

## UNVR-5.1.25.bin

| | |
|---|---|
| URL | <https://fw-download.ubnt.com/data/unifi-nvr/44dc-UNVR-5.1.25-4d75bc90-b895-4d8f-baff-382f24fd36cf.bin> |
| Version | `v5.1.25+84c48e7` |
| ubnt_version | `UNVR4.al324.v5.1.25.84c48e7.260710.1602` |
| Released | 2026-07-27 |
| Retrieved | 2026-08-15T14:05:38+10:00 |
| Size | 786253430 bytes |
| sha256 | `74f2833356e832bd97f59bb3686eaedaf704335631b5daabedfce4d30bb222fc` |
| md5 | `3f661a852b6fb70946e2a94712df6923` |

Discovered via `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr`
(platform `UNVR`, product `unifi-nvr`).

### Telling a good copy from a bad one

- Exactly 786253430 bytes. Any other size is wrong.
- sha256 must match above. Verify: `./scripts/fetch-unvr-firmware.py verify`
- Not an HTML login/error page: `file` reports data, not HTML, and the first
  bytes are not `<!DOCTYPE`.

Fetched with curl by explicit approval - playwrong has no binary-download tool.
The stock checksum is what makes that safe; do not skip verification.

## lmk00338.pdf

| | |
|---|---|
| URL | <https://www.ti.com/lit/ds/symlink/lmk00338.pdf> |
| Doc | TI LMK00338 datasheet, SNAS636C |
| Revision | Rev C, July 2021 (first released Dec 2013) |
| Retrieved | 2026-08-18 (via playwrong `pdf`) |
| Size | 2038698 bytes |
| Why | Confirms U1 = LMK00338 (40-WQFN, 8-output HCSL clock fan-out buffer) |

### Telling a good copy from a bad one

- 34 pages (a truncated fetch has fewer). Not an HTML login page.
- Title present: "LMK00338 8-Output PCIe ... Clock Buffer and Level Translator".
- Pin table lists 40-lead WQFN, `OSCin`/`OSCout`, `CLKoutA0..A3` + `CLKoutB0..B3`.

## S-35390A — RTC (behind PCA9546 mux ch0)

| | |
|---|---|
| URL | <https://www.ablic.com/en/doc/datasheet/real_time_clock/S35390A_E.pdf> |
| Doc | ABLIC S-35390A 2-wire real-time clock datasheet |
| Revision | Rev.4.2_04 (© 2004-2018) |
| Retrieved | 2026-08-18 (via playwrong `pdf`) |
| Size | 741338 bytes |
| Why | RTC on i2c mux ch0 wedges the bus — POC/BLD "indefinite status", RESET, V_DET. See docs/rtc-s35390a-fault.md |

### Telling a good copy from a bad one

- 57 pages (a truncated fetch has fewer). Not an HTML login page.
- STATUS1 register (Fig 12): B0 POC, B1 BLD, B7 RESET.
- V_DET = 1.0 V typ (0.65–1.35 V), Table 6/7; communicate ≥ 1.3 V, Table 4.
