# Vendor U-Boot info dump — read-only (2026-08-17)

`U-Boot 2015.07-alpine_db-2.21-HAL (Dec 16 2020)`, captured at the prompt via
`scripts/uboot-info.tcl` (all commands read-only). Reached U-Boot with the SP805
watchdog reset (`scripts/reboot-to-uboot.tcl`) — no power-cycle.

## Highlights

**Overclock / clock levers (built into vendor U-Boot):**
- **`cpu_set_speed`** — "Set CPU speed". The direct overclock lever — no modern U-Boot needed for a first test. Pair with the UART-as-clock-probe to verify.
- `dram_margins` — DDR RDQS/WDQS shmoo via the controller BIST. **DANGEROUS** — crashed U-Boot with a Synchronous Abort before; treat as read-with-care.
- `ddr_training_results`, `ddr_ecc_stats`, `ddr_ecc_poison` — DDR tuning/diag.
- `serdes`, `eth_1g_params_set`, `eth_link_training_enable`, `mdio`, `mii` — Ethernet/SerDes tuning (carry into Linux per `eth_1g_params_set` help).
- `thermal_get` / `thermal_init` / `dtt` — SoC thermal readout from U-Boot.
- **`reset`** — "Perform RESET of the CPU" (try as a clean reboot path vs the hung Linux `reboot`, #51 — alongside the working watchdog reset).

**Storage / boot (confirms our model):**
- NOR: `sf probe` → **`MX25U25635F … total 32 MiB`** (matches live + photo).
- NAND: `nand0`, erase 256 KiB, page 4096 B, OOB 224 B, **0 bad blocks** (MT29F8G08).
- `bootcmd` = our NAND boot (`nand read 0x02000000 0x1300000 0x1200000; …`).
- `bootcmd_orig` (vendor recovery) + `bootnand`/`bootspi`/`bootemmc`/`multiboot`.
- `bootemmc` = `usb start; ext4load usb 0 … uImage` — confirms **eMMC is over USB** (and `usb start` finds nothing → stranded, as probed).
- Update scripts show how the vendor writes flash: `kernelupd`/`spikernelupd`
  (NAND/NOR kernel), `dtupd` (DT @ NOR `0x81000`), `bootupd` (al_boot), `delenv`.
- NOR env at `env_offset=0x1c0000` (+redund `0x1d0000`); `pld_i2c_addr=0x57`.

**Reconcile / conflict:**
- **I2C mux:** U-Boot enumerates **`PCA9548@0x71`** with **8 channels** (buses 1–8),
  but the board photo reads **`TCA9546A`/PW546A** (4-channel) and the DTS uses
  `pca9546`. Likely a 4-ch part with U-Boot using a generic 8-ch label — but
  worth an explicit check (the extra 4 buses may be dead). ⚠
- `cpu_set_speed` + `thermal_get` present in U-Boot but not surfaced in Linux.

**Fan control (vendor, for reference):** `slowfan` = `i2c dev 4; i2c mw 0x2e 0x5c/0x5d/0x5e 0xe8; i2c mw 0x2e 0x30/0x31/0x32 $fanspeed` (ADT747x at **0x2e on i2c bus 4**, `fanspeed=0x50`). `resetled=gpio clear 37`; `preboot=ble;$resetled;run slowfan`.

**PCI topology:** `00.01/00.02` al_eth (1c36:0001), `00.04` al_ssm crypto (1c36:0022),
`00.05` al_dma/RAID (1c36:0022 cls 04), `00.08/00.09` SATA (1c36:0031).

**DRAM (bdinfo):** 4 banks × 0x40000000 = **4 GB**, at `0x0 / 0x40000000 /
0x80000000 / 0x200000000`. MACs `eth1 74:ac:b9:41:a8:11`, `eth2 …:12`.

## Notable commands (from `help`) worth remembering
`cpu_set_speed`, `cpu_aborts_enable_set`, `dram_margins`, `ddr_training_results`,
`serdes`, `mdio`/`mii`, `thermal_get`, `sgpo` (bay LEDs), `pca953x` (GPIO exp),
`muio_mux` (pin mux debug), `iodma_*` (RAID/memcpy accel), `flash_contents_*`
(TOC / boot-instance system), `is_nand_boot`, `reset`, `go` (chainload entry),
`bootm`/`bootz`, `ext4load`/`ext4ls`, `part`/`gpt`, `scsi*`.
Absent here: `mmc`, `mtdparts`, `clocks`, `date`.

## Full dump

Verbatim read-only capture (complete, incl. env MACs/IPs):
[`uboot-2015.07-alpine_db-2.21-HAL.txt`](uboot-2015.07-alpine_db-2.21-HAL.txt) —
the full `printenv`, `help`, `bdinfo`, `nand/sf/i2c/pci/dm` output.
