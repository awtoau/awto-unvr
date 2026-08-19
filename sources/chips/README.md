# sources/chips — datasheet PDFs (TRACKED)

Chip datasheets for the UNVR board. **Tracked in git** (owner directive; `.gitignore`
has an explicit exception for `sources/chips/`), unlike the gitignored binaries in
`sources/`. Per-chip docs are in [`../../docs/chips/`](../../docs/chips/).

All PDFs fetched via playwrong (`mcp__playwrong__pdf`), never curl/wget.

| File | Chip | Origin URL | Revision | Good-copy check |
|---|---|---|---|---|
| `MX25U25635F.pdf` | Macronix MX25U25635F SPI-NOR | media.digikey.com mirror of macronix.com attachment 8663 | v1.5 (P/N PM1712) | **96 pages**; title-page "MX25U25635F DATASHEET"; contains "256Mb" + "1.8V" |
| `AR8031.pdf` | Atheros AR8031 1G PHY | media.digikey.com `ar8031_ds_(atheros)_rev1.0_aug2011.pdf` | Data Sheet v1.1, Aug 2011 | **118 pages**; "AR8031 Integrated 10/100/1000 Mbps Ethernet Transceiver" |
| `PCA9546A.pdf` | I²C 4-ch mux (TI 2nd-source of NXP PCA9546A) | ti.com `lit/ds/symlink/pca9546a.pdf` | TI Rev I, Jun 2022 | **43 pages**; "PCA9546A Low Voltage 4-Channel I2C and SMBus Switch" |
| `S-35390A.pdf` | ABLIC/Seiko S-35390A RTC | ablic.com `S35390A_E.pdf` | Rev 4.2 (2018) | **57 pages**; "S-35390A 2-WIRE REAL-TIME CLOCK" |
| `ADT7475.pdf` | ADI/onsemi ADT7475 fan/thermal | bdtic.com `DataSheet/ADI/ADT7475.pdf` | ADI Rev A | **68 pages**; "dBCool Remote Thermal Monitor and Fan Controller" |
| `MT29F8G08ABBCAH4-avaq-summary.pdf` | Micron MT29F8G08ABBCAH4 NAND | avaq.com uploads mirror | 1-page spec card (**not** full DS) | **1 page**; confirms "1.8V 8G-bit 1G x 8 63-Pin VFBGA" |
| `nxp-ethernet-macs-interfaces-controllers.pdf` | NXP QorIQ MAC/PCS overview (P/T/LS/LA/LX) — incl. **mEMAC**, MDIO ctrl, SerDes/PCS bring-up | community.nxp.com Layerscape attachment 9285 (P. Billings, DN Apps) | Rev 22-Oct-2020 | **79 pages**; title "ETHERNET- MACs, Interfaces, & Mgmt Controllers Overview"; covers mEMAC + MDC/MDIO. Ref for AL-324 10G-MAC=mEMAC-family (MorethanIP) analysis |
| `altera_tse_ug_ethernet.pdf` | Altera/Intel Triple-Speed Ethernet (TSE) MAC user guide — the **1G MAC core** front-ended by AL-UDMA on eth1 (10/100/1000 MAC, MDIO, FIFO, cmd_config) | intel.com/altera `ug_ethernet.pdf` | UG-01008, matches the MTIP TSE core | **178 pages**; title "Triple Speed Ethernet User Guide"; covers cmd_config / MDIO / 1000BASE. Ref for the al_eth 1G RGMII register semantics (#83) |

## Not obtainable automatically (documented from public specs instead)

| Chip | Why no PDF | Where the specs came from |
|---|---|---|
| **MT29F8G08ABBCAH4** (full DS) | Micron datasheet login-gated; all distributor hotlinks (Mouser/Verical/Farnell) 403/block or serve a wrong-density family doc | Running-kernel ground truth + Avaq 1-page card (above). Part page: micron.com/…/mt29f8g08abbcah4-it-c |
| **PCA9575** | NXP `docs/en/data-sheet/PCA9575.pdf` hotlink-protected (HTTP 404 to any non-browser fetch); distributor htmldatasheets 410/gone | NXP product page (full text), datasheet **Rev 5.0, 2023-05-31** — open URL in a browser |
| **AL-324 / Alpine V2** | NDA (Amazon/Annapurna), no public datasheet | HAL headers (`delroth/alpine_hal`), mainline DT, stock kernel |
| **ASM1042A** | ASMedia NDA, no public datasheet | lspci/lsusb, Linux xhci-pci quirks |
| **DDR4 DRAM** | part unconfirmed (soldered) | read SPD @ I²C 0x57 to identify, then fetch |
| **SFP+ optic** | module unconfirmed | `ethtool -m` / EEPROM 0x50 to identify, then fetch |
| **QorIQ DPAA RM** (full mEMAC register chapter) | only mirror found (supchen.cn) has an **expired TLS cert**; nxp.com RM login-gated | Register-level mEMAC facts taken from GPL source: u-boot `include/fsl_memac.h` + Linux `drivers/net/ethernet/freescale/fman/fman_memac.c`; NXP repo `nxp-qoriq/dpaa-im/fsl_memac.h` (deepwiki) |
