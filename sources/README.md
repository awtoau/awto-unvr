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

## SFF-8431.pdf

| | |
|---|---|
| URL | <https://www.f-tone.com/wp-content/uploads/2026/01/SFF-8431.pdf> |
| Doc | SFF-8431, "SFP+ 10 Gb/s and Low Speed Electrical Interface" (SFI) |
| Revision | Rev 4.1, 2009-07-06, plus Rev 4.1 Addendum 2013-09-15 (archived; live content split into SFF-8418 + SFF-8419) |
| Retrieved | 2026-09-04 (via playwrong `pdf`) |
| Size | 875908 bytes |
| Why | Defines the host TX compliance point for a 10G SFP+ port: point **B**, host output at the Host Compliance Board. Sets what a scope-based host TX-eq validation actually measures (#TBD 10G TX eq tuning). |

### Telling a good copy from a bad one

- 136 pages (a truncated fetch has fewer). Not an HTML login page.
- Contains "3.3.1 HOST COMPLIANCE POINTS" and "The compliance points are B and C."
- Contains "Table 12 Host Transmitter Output Jitter and Eye Mask Specifications at B"
  with TJ 0.28 UI p-p, DDJ 0.1 UI, DDPWS 0.055 UI, UJ 0.023 UI RMS, eye mask
  X1 0.12 / X2 0.33 UI, Y1 95 / Y2 350 mV.

## tek-55W-29390-0-sfp-plus-compliance.pdf

| | |
|---|---|
| URL | <https://download.tek.com/document/55W-29390-0.pdf> |
| Doc | Tektronix, "Ethernet SFF-8431 SFP+ SFF-8635 QSFP+ Compliance and Debug Testing" (55W-29390-0) |
| Revision | 2014-02-20 |
| Retrieved | 2026-09-04 (via playwrong `pdf`) |
| Size | 7377183 bytes |
| Why | Names the equipment a real SFF-8431 host-TX compliance run needs: HCB/MCB fixtures, >16 GHz real-time scope, TWDPc option. Evidence that scope-based TP-B compliance is not reproducible in this lab. |

### Telling a good copy from a bad one

- 31 pages (a truncated fetch has fewer). Not an HTML block page.
- Contains "15 Defined Measurements for Host Tx Compliance", "Host Compliance Board (HCB)",
  "TWDPc", and the ">16GHz Oscilloscope will meet rise time requirements" slide.

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

## SFF-8472-ver12.4.pdf

| | |
|---|---|
| URL | <https://www.optcore.net/wp-content/uploads/SFF-8472-ver12.4.pdf> |
| Doc | SFF-8472, Specification for Management Interface for SFP+ |
| Revision | Rev 12.4, 2021-03-31 (SNIA SFF TA TWG) |
| Retrieved | 2026-09-04 (via playwrong `pdf`) |
| Size | 823843 bytes |
| Why | Authority for the SFP+ A0h/A2h memory map. Used to establish that SFF-8472 defines **no** loopback control at any revision (grep "loopback" = 0 hits). |

### Telling a good copy from a bad one

- 43 pages. Cover reads "SFF-8472 Rev 12.4  March 31, 2021".
- Contains section headings "10.2 Vendor Specific Locations [Address A2h, Bytes 120-126]",
  "10.5 Vendor Specific Control Function Locations [Address A2h, Page 00h / 01h, Bytes 248-255]",
  "10.6 Variable Receiver Decision Threshold Control [Address A2h, Page 02h, Bytes 130-131]".
- Zero occurrences of "loopback" — that absence is the finding, so a copy that
  has any is not this document.

## fs-sfp-10gsr-85-datasheet-text.pdf

| | |
|---|---|
| URL | <https://img-en.fs.com/file/datasheet/customized-10g-sr.pdf> |
| Doc | FS "10GBASE-SR SFP+ 850nm 300m DOM Transceiver", P/N SFP-10GSR-85 |
| Revision | undated; footnotes reference SFF-8431 Rev 4.1 and the Finisar FTLX8573D3BTL reference design |
| Retrieved | 2026-09-04 (via playwrong `pdf`) |
| Size | 1689342 bytes |
| Why | The optical numbers used for the fibre-loopback power budget: PAVE -5..-1 dBm, RX sensitivity (OMA) -11.1 dBm, stressed sensitivity -7.5 dBm, max input power +0.5 dBm; and the explicit "limiting module" statement (no retimer/CDR). |

### Telling a good copy from a bad one

- 15 pages, text-extractable (`pdftotext` yields >20 kB).
- Contains "The transceiver is a "limiting module", i.e., it employs a limiting receiver."
  and the note naming `FTLX8573D3BTL`.
- Section IV "Optical Characteristics" table must show `RSENS1 -11.1 dBm` and `PMAX +0.5 dBm`.

## fs-sfp-10gsr-85-datasheet.pdf

| | |
|---|---|
| URL | <https://resource.fs.com/mall/resource/sfp-10gsr-85-datasheet.pdf> |
| Doc | FS marketing-styled datasheet for the same SFP-10GSR-85 |
| Retrieved | 2026-09-04 (via playwrong `pdf`) |
| Size | 3902179 bytes |
| Why | The copy FS links from the product page. Kept only for provenance — **the tables are rendered as images**, `pdftotext` yields ~570 bytes of stray glyphs. Use `fs-sfp-10gsr-85-datasheet-text.pdf` above for numbers. |

### Telling a good copy from a bad one

- 14 pages. `pdftotext` output is near-empty — that is correct for this file, not a truncation.
