# MT29F8G08ABBCAH4 — Micron SLC NAND

- **Part:** Micron **MT29F8G08ABBCAH4** (`-IT:C` grade).
- **Function:** main NAND. Holds kernel + rootfs (mtd0–mtd4).
- **Confirmed:** kernel `Manufacturer ID 0x2c, Chip ID 0xa3` → decoded to this part; `NAND: 1024 MiB`. ✅ live.

## Key specs

- **Capacity:** 8 Gbit = **1024 MiB**, single-level cell (SLC).
- **Voltage: 1.8 V** — the `ABBC` variant is the 1.8 V part (Avaq card: "Parallel 1.8V"). Matters for programming.
- **Bus:** x8, ONFI (async parallel NAND).
- **Geometry:** page **4096 B + 224 B OOB**; block (erase) **256 KiB** (64 pages); erasesize `0x40000`.
- **ECC:** on-die/controller ECC in use; under 1.3.35 the controller was actively correcting 1–6 bitflips on kernel-partition reads (within budget — wear signal, watch it). See [nand-1.3.35.md](../nand-1.3.35.md).
- **Package:** 63-ball VFBGA.
- IDs: Mfr `0x2C` (Micron), device `0xA3`.

## How it connects here

- **Bus:** on-SoC NAND controller (al-nand) @ `0xfa100000` — [hardware.md#mmio-and-address-map](../hardware.md#mmio-and-address-map).
- **Driver:** `al_nand` (custom, out-of-tree) — not mainline. `compatible = annapurna-labs,al-nand`.
- **Role:** mtd0–mtd4 (kernel + rootfs); NOR holds the rest.

## Datasheet

- Local: [`sources/chips/MT29F8G08ABBCAH4-avaq-summary.pdf`](../../sources/chips/MT29F8G08ABBCAH4-avaq-summary.pdf) — **1-page spec card only** (Avaq), confirms 1.8 V / 8 Gbit / 1G×8 / VFBGA-63. **Not** the full datasheet.
- Full Micron datasheet is **login-gated** at micron.com and every distributor hotlink (Mouser/Verical/Farnell) returns a block page or the wrong-density family doc. Canonical part page: <https://www.micron.com/products/storage/nand-flash/slc-nand/part-catalog/part-detail/mt29f8g08abbcah4-it-c>
- On-chip specs above are ground-truth from the running kernel, so the missing PDF is not blocking.

## RE / repurpose notes

- **Programming voltage = 1.8 V.** External NAND reads (63-ball BGA) need a BGA socket/adapter + a 1.8 V-capable NAND programmer. In-circuit is impractical (parallel x8 + ALE/CLE/RE/WE).
- OOB is 224 B — set the reader's page/OOB to 4096+224 and ECC layout to match the al-nand controller, or ECC will mis-correct.
- Dumping is easier via the SoC: boot U-Boot / Linux and read mtd0–4, rather than desoldering.
