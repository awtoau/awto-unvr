# UNVR identity — Factory + EEPROM partitions

This unit: sysid `0xea16` (UNVR, no eMMC), serial `74acb941a811`.

- Held in two NOR partitions: **Factory** (mtd "Factory") and **EEPROM** (mtd "EEPROM").
- `ubnthal.ko` reads them and exposes decoded fields at `/proc/ubnthal/system.info`.
- Raw byte-offset field layout: see [nor-boot-chain.md](nor-boot-chain.md) (EEPROM
  also holds a 2048-bit RSA private key @0xe004 — inventory in `secrets.yaml`).

## Decoded identity (this unit)

Source: `/proc/ubnthal/system.info`, captured
[docs/hw-reference/20260816-104601/ubnthal.txt](hw-reference/20260816-104601/ubnthal.txt).

| Field | Value | Notes |
|---|---|---|
| systemid | `ea16` | board sysid → selects DTB / bootfrom=bootnand |
| shortname / name | `UNVR4` / `UniFi-NVR-4` | |
| serialno | `74acb941a811` | equals eth0 MAC |
| eth0.macaddr | `74:ac:b9:41:a8:11` | |
| eth1.macaddr | `74:ac:b9:41:a8:12` | serial + 1 |
| boardrevision | `29` | matches BOM `113-02832-29` |
| manufid | `0004` | |
| mfgweek | `202022` | 2020, week 22 |
| vendorid | `0777` | Ubiquiti |
| cpu | `AL324V2` | Annapurna Alpine V2 / AL-324 |
| cpuid | `411ed073` | Cortex-A57 MIDR |
| cpu_rev_id | `00010000` | |
| flashSize | `33554432` | 32 MiB SPI-NOR (MX25U25635F) |
| ramsize | `4294967296` | 4 GiB |
| qrid | `icvpQD` | QR/setup id |
| device.hashid | `08669c977e563b77` | derived |
| device.anonid | `94fc4bef-78a2-4c70-8866-9c977e563b77` | derived |
| regdmn | `0000ffff…` | regulatory domain |
| lcm_count | `0` | |

- MAC/serial are device IDs, cleared for publication by the owner.
- These are read-only identity/calibration values; not rewritten by firmware upgrades.
