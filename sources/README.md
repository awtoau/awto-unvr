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

## chips/altera_tse_ug_ethernet.pdf

| | |
|---|---|
| URL | <https://www.manuallib.com/download//THREE-SPEED-ETHERNET-MEGACORE-FUNCTION-USER-MANUAL.PDF> |
| Doc | Altera Triple-Speed Ethernet MegaCore Function User Guide, UG-01008-3.0 |
| Revision | Software v12.0, June 2012 |
| Retrieved | 2026-08-19 (via playwrong `pdf`) |
| Size | 2516688 bytes |
| Why | Reference for the TSE MAC register map + `command_config` bits; the AL-324 1G "al_eth" MAC is this MTIP/TSE core (register layout + cmd_cfg bits identical to vendor `struct al_eth_mac_1g`). |

Other mirrors tried and blocked/dead: mouser.com (HTML block page), fpga.world
(no route). manuallib served the full document.

### Telling a good copy from a bad one

- 178 pages (a truncated fetch has fewer). Not an HTML block page.
- Title present: "Triple-Speed Ethernet MegaCore Function User Guide", UG-01008-3.0.
- Text contains `command_config`, `rx_section_full`, `mdio_phy0`, and the
  scatter-gather DMA descriptor section.
