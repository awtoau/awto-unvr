# NAND study: Micron MT29F8G08ABBCAH4 behind `al_nand`

Scope: what Linux can and cannot trust the NAND for, as of `main` @ `c71ecb2`.
Tracking: #208 (driver), #268 (exec_op / references), #166 (recovery), #30 (layout).

- Method: code read at `c71ecb2` + datasheet + vendor trees + **read-only** survey of the live box.
- Box: kernel `7.3.0-rc6-36cb313+`, up 2 d, a 245 GB rsync running on the SSD (load ~3.5).
- Measured 2026-10-10 11:00-13:45 +11:00 unless stated. Zero flash writes, zero erases.
- Tools added: `scripts/nand-survey.py` (bbm / ecc / eccflags), `scripts/nand_bch.py` (independent ECC model), `tests/test_nand_bch.py`.
- References: `sources/MT29F8G08ABBCAH4-micron-m71m-datasheet.pdf` (Rev L, 122 p), `/mnt/2tb/unvr-port-refs/urnvr-kernel-4.19.152/drivers/mtd/nand/raw/al_nand.c`, `/mnt/2tb/unvr-port-refs/UBNT-source-code/UNVR-1.3.35-GPL/{u-boot/board/annapurna-labs/common/al_nand.c,linux-arm64-unvr-4.1.37-ubnt/drivers/mtd/nand/al/al_nand.c}`, `/mnt/2tb/unvr-port-refs/linux-v7.3-fresh/drivers/mtd/nand/raw/`.

## 1. Status

| item | status | evidence |
|---|---|---|
| Chip identification, ONFI parse | KNOWN-GOOD | dmesg: `2c a3`, 1024 MiB, page 4096, OOB 224, erase 256 KiB, tPROG 600 us, tBERS 10000 us |
| Reads (data) | KNOWN-GOOD | `scripts/verify-nand-reads.py`: 16 MiB `linux_kernel` byte-identical to a U-Boot-taken backup, ECC 0/0 |
| HW ECC decode, strength t=8 / 512 B | KNOWN-GOOD | independent model matches 190,571 programmed pages exactly and 68 more to within one bit (1.52 M codewords), s5 |
| HW ECC single-bit correction | KNOWN-GOOD | chike chip page 262034 CW4: controller fixed byte 207 bit 0 (`0xc7` -> `0xc6`), predicted by the model, s5 |
| Factory bad-block detection | UNPROVEN (no positive case) | marker position/value match the datasheet by reading; 0/4096 blocks marked, so detection has never fired, s4 |
| Program (write) lands | UNPROVEN | no post-fix write has ever run; pre-fix writes were no-ops |
| Erase of a *programmed* block | UNPROVEN | only erase seen was of an already-erased block (2026-10-07) |
| ECC parity written by Linux == stock layout | UNPROVEN | no Linux-written page exists to check |
| Program/erase failures reach the MTD core | KNOWN-BROKEN (partly) | s3: timeouts, stale status reads, raw/OOB paths swallow errors |
| Read failures reach the MTD core | KNOWN-BROKEN | s3: `read_buf()` returns stale buffer as success on timeout |
| Bitflip reporting to MTD (`-EUCLEAN`) | KNOWN-BROKEN | s5: always 0 |
| Bad-block *marking* path | UNPROVEN | no bad block on this unit to exercise it |
| OOB layout reported to MTD | KNOWN-WRONG | s5: claims 220 ECC bytes, hardware uses 128 |
| `nand-write-test.py` as shipped | KNOWN-BROKEN (fixed in this change) | s7: exit status masked by pipes; criterion [2] cannot pass |
| Block lock / WP# state of the chip | UNPROVEN (inferred unlocked) | s4 |

## 2. Hardware facts

### Chip (datasheet Rev L)
- MT29F8G08ABBCAH4: 1.8 V, SLC, x8, 8 Gb, ONFI 1.0; datasheet lists no on-die ECC (host ECC required).
- Page 4096 + 224 spare; 64 pages/block; 4096 blocks (2 planes x 2048); 1 LUN.
- tR 25 us max; tPROG 200 us typ / 600 us max; tBERS 2 ms typ / 10 ms max (Table 33).
- NOP = 4 partial-page programs per page (Table 33, ONFI byte 110). The driver sets `NAND_NO_SUBPAGE_WRITE` (`al_nand_main.c:1018`); never reprogram a page.
- Required ECC: **8-bit per 540 bytes** (Table 21); ONFI byte 112 = 08h.
- Block endurance ONFI bytes 105-106 = `06h 04h` (6 x 10^4 cycles by ONFI encoding).
- Timing modes 0-4 supported (ONFI bytes 129-130 = `1Fh`).
- Block lock supported (1.8 V part); default-locked at power-up only if the LOCK pin is high (p.77).
- WP# low disables program/erase; status bit 7 = 0 means protected (Table 18); WP# may only change while idle, then wait tWW.

### Controller (Alpine, `fa100000.nand`, DT reg 0x202000)
- Two single-address FIFO data paths; selected by which init is called. Detail: `docs/mtd.md` "Linux sees the NAND".
- Command sequencer FIFO (CMD / ADDRESS / DATA_READ_COUNT / DATA_WRITE_COUNT / SPARE_READ_COUNT / SPARE_WRITE_COUNT / WAIT_FOR_READY / WAIT_CYCLE_COUNT entries).
- HW BCH engine, config decoded from the PBS syscon at probe: `cw 512, ecc on, spare off 4100, page 4096, algo 1` (dmesg).
- Status flags: `UNCORR_ERR` and `CORR_ERR` are single sticky bits in `nfc_int_stat` (`al_hal_nand.c:918-958`); **no bit count**.
- PBS also carries a bad-block-marking method and two locations (`al_hal_nand.c:412-447`); the driver decodes and **never uses or logs them**.

### Live layout (`/proc/mtd` + sysfs, 2026-10-10)

| mtd | label | chip offset | size | flags |
|---|---|---|---|---|
| mtd0 | al_boot | 0x0 | 2 MiB | writeable |
| mtd1 | device_tree | 0x200000 | 1 MiB | writeable |
| mtd2 | linux_kernel | 0x300000 | 16 MiB | writeable |
| mtd3 | awto-uboot | 0x1300000 | 1 MiB | writeable (DT: "deliberately NOT read-only") |
| mtd4 | rootfs | 0x1400000 | 0x3eb00000 | writeable |
| mtd5 | chike | 0x3ff00000 | 1 MiB | writeable |
| mtd6-13 | NOR | - | - | erasesize 0x1000 |

- Partitions tile the chip exactly (4096 blocks: 8+4+64+4+4012+4).
- Every sysfs `oobavail` = 2, `ecc_step_size` 512, `ecc_strength` 8, `bitflip_threshold` 6, `subpagesize` 4096.
- `mtdN` numbers move with probe order; address by label.

### Rated vs running

| interface | rated max | board allows | running | gap reason |
|---|---|---|---|---|
| NAND bus timing | ONFI mode 4 (modes 0-4, tRC/tWC 30 ns at 1.8 V) | not tested | **mode 1** (`AL_NAND_MAX_ONFI_TIMING_MODE`, `al_nand_main.c:38`) | none recorded; vendor value. DT `max-onfi-timing-mode = <1>` (`dts/alpine-v2-ubnt-unvr-ea16.dts:795`) is **never read** |
| Read throughput | not established | - | 13.3 MB/s (`dd` 1 MiB of `chike`, 2026-10-10, PIO 32-bit FIFO reads, rsync running) | PIO loop; DMA path (`al_nand_data_buff_read_dma`) not imported |
| Program/erase | tPROG 600 us / tBERS 10 ms | - | chip-timed | n/a |

- A gap with no recorded reason is a defect per the global rules: timing mode needs a measured limit or a reason (not filed separately; see s9 step 8).

## 3. Write path, end to end (merged code)

### Call chain
- `nandwrite` -> `mtd_write` -> `nand_do_write_ops` (`nand_base.c:4308`) -> `nand_check_wp` (:4344) -> per page `nand_write_page` (:4270) -> `chip->ecc.write_page`.
- `ecc.write_page` = `al_nand_ecc_write_page` (`al_nand_main.c:618`):
  1. `ecc_set_enabled(1)` (:629); `write_err = 0` (:633)
  2. `nand_prog_page_begin_op` (:634): `cmd_ctrl(SEQIN)`, address cycles, `write_buf(4096)` (:431)
  3. `ret = write_err` (:638) -- the only check
  4. `cmdfunc(RNDIN, writesize + ecc_offset)` (:642); TX on, WP off; `WAIT_CYCLE_COUNT`; `SPARE_WRITE_COUNT(ecc_bytes)` (:652)
  5. `wait_cmd_fifo_empty` (:656); `out:` WP on, TX off, ECC off (:658-661)
  6. `nand_prog_page_end_op` (:668): `cmd_ctrl(PAGEPROG)` -> `WAIT_FOR_READY` -> `wait_dev_ready` (13 ms) -> `nand_wait()` (400 ms poll, status read) -> `-EIO` only if status FAIL
- `write_buf` -> `al_nand_wait_for_irq(BUF_WRRDY)` (20 ms) per codeword -> `al_nand_data_buff_write` (HAL :1668) -> 32-bit writes to `obj->data_buff_base` (HAL :1726-1730).
- Erase: `nand_erase_nand` -> `cmd_ctrl(ERASE1/ERASE2)` -> core `nand_wait()`; driver adds no wait of its own.

### Where a failure is swallowed

| # | point | file:line | effect |
|---|---|---|---|
| W1 | PAGEPROG ready timeout stored in `write_err`, never read | `al_nand_main.c:338-340`, return at :668 | the 13 ms bound is advisory; `nand_wait()` is the backstop. **Review defect 1 confirmed** |
| W2 | RNDIN FIFO timeout stored after the only check | :329 vs :638 | never read. Part of defect 1 |
| W3 | `write_err` set by *read-path* `cmd_ctrl` timeouts, cleared only in write_page | :129, :329, :633 | becomes a false failure the moment W1 is fixed naively. **Defect 2 confirmed** |
| W4 | `ecc.write_page_raw` and `ecc.write_oob` left at core defaults (`nand_write_page_raw`, `nand_write_oob_std`) | :862-866; `nand_base.c:5767-5771` | reachable by `nandwrite --raw`, bad-block marking, any OOB write; `write_buf` errors are recorded and never checked; no `len & 3` guard. **Defect 3 confirmed** |
| W5 | HAL write path never stores `cw_size_remaining`/`cw_count_remaining` back | `al_hal_nand.c:1683-1684`, guard :1686-1696 | "Not enough CW data" guard is dead on the write path (read path stores at :1658-1659). **Defect 4 confirmed** |
| W6 | `send_byte_count_cmd` has no error path | :250-258, call :652 | SPARE_WRITE_COUNT entry failure is invisible |
| W7 | **status read returns a stale byte on read failure** | :364-410 (early `return` :387-388, :398-403), :413-429 | `read_byte()` hands out `word_cache[0]` from an earlier read; `nand_wait()`/`nand_check_wp()`/`nand_erase_nand()` then see an old "ready, pass" status. **New** |
| W8 | no program-verify of the OOB/parity phase | - | a page programmed without parity reads back as erased (s5), so a readback-all-0xff cannot separate "not programmed" from "programmed without parity" |
| W9 | WP# asserted before PAGEPROG | :659 then :668 | inherited vendor order (`urnvr-kernel-4.19.152/.../al_nand.c:560-565`). Datasheet: WP# low disables program, and WP# may change only while idle, before a command sequence; here it changes mid-sequence. Vendor writes work, so unproven either way; first suspect if the first real write silently does nothing |

### Read path (same class)
- R1 `al_nand_read_buf` returns without copying on IRQ timeout (:387-388) or HAL failure (:398-403); `ecc_read_page` never checks (:564-570); `nand_read_page_op` returns 0. The caller's buffer keeps whatever was in it (kernel bounce memory) and the read succeeds. **New**
- R2 `ecc_read_page` returns 0 always (:601): `max_bitflips` = 0, so `-EUCLEAN` is unreachable (s5).
- R3 `oob_required` returns `-EINVAL` + `WARN_ON` (:551, :626): reachable from user space with data+OOB in one request; fails closed, but the comment (:547-550) is wrong that the core never asks.
- The unit's single observed error path (`dev_err ... irq timeout`) has never fired in dmesg this boot.

### Confirmed / refuted from the #208 review (comment 6091146775)
- Defects 1-4: **confirmed** at the lines above; no commit has touched `modules/al_nand` since `36cb313`.
- "Cannot write outside the requested partition": **holds** (driver does no address arithmetic; bounds in the MTD core).
- "ascending-vs-fixed address question is settled in favour of FIFO": agrees; `docs/mtd.md` still says "open".

## 4. Bad blocks

### What the chip does (datasheet Error Management, p.101-102)
- Shipped erased; invalid blocks carry the mark **`00h` in the first spare location of the first page** (column 4096, OOB byte 0); other bytes of that page may or may not be programmed.
- NVB min 4016 of 4096; max 80 invalid per 8Gb die (ONFI bytes 103-104 = 50h); **block 0 guaranteed valid with ECC**.
- "Do not erase or program blocks marked invalid"; erasing a bad block can destroy the mark irrecoverably.
- Required: check the mark before any program/erase; always check status; use minimum ECC; use bad-block management and wear levelling.

### What the driver / core do
- `chip->bbt_options` is never set: **no on-flash BBT**. `nand_scan()` builds an in-RAM table by reading the marker at every probe (dmesg: "Scanning device for bad blocks", 0.74 s for 4096 blocks).
- Marker position: Micron init sets `NAND_BBM_FIRSTPAGE` (`nand_micron.c:497`); `badblockpos` = 0; "bad" = byte != 0xFF (`nand_base.c:253-282`). **This matches the datasheet.**
- The scan reads OOB through `ecc.read_oob` = `nand_read_oob_std` with the ECC engine off, so the marker is read raw. OOB[0:4] is never touched by the ECC engine (parity starts at OOB[4]).
- Stock U-Boot (`u-boot/board/annapurna-labs/common/al_nand.c:786`, `nand_scan_bbt_simulate` only under `CONFIG_AL_NAND_SIMULATE`) and the 4.1.37 / 4.19 vendor drivers also use the default in-RAM scan. The PBS BBM fields presumably serve the boot ROM (not verified).
- Core behaviour on bad blocks:
  - erase: `nand_erase_nand` refuses (`-EIO`, "attempt to erase a bad block").
  - **program: the core does not check.** `nand_do_write_ops` has no bad-block test; `dd`/`cat` to `/dev/mtdN`, raw clients and mtdblock write straight into a bad block. `nandwrite`, `flash_erase` and UBI skip them in user space / UBI.
  - marking: `nand_block_markbad_lowlevel` = erase (best effort) + `ecc.write_oob` (`nand_write_oob_std` -> `write_buf`, 224 B at column 4096, ECC off) -> W4 applies. Path never executed here.

### Survey of this unit (`scripts/nand-survey.py bbm`, 2026-10-10 13:28, read-only, ~12k raw OOB reads)
- Pages 0, 1 and 63 of all 4096 blocks: OOB[0:4] = `ff ff ff ff` in **every** block.
- In-RAM BBT (`MEMGETBADBLOCK`) bad: 0. sysfs `bad_blocks`: 0 on all six partitions. `bbt_blocks`: 0.
- No `Bbt0` / `1tbB` signature at OOB[8:12] anywhere: no on-flash BBT exists.
- Zero marks is inside the datasheet range (0-80). The tool's reads return real OOB (buffer starts zeroed, reads come back `ff`).

### Gaps
- No factory bad block exists, so skip-on-read, skip-on-erase and markbad are **exercised nowhere**.
- Whether any marker was lost to an earlier erase (datasheet warns it is unrecoverable) is **not establishable**; no pre-use OOB image exists (all backups in `git_debris/woomera-mtd/` are `nand read` data without OOB).
- PBS BBM method / locations are decoded and unlogged (`al_hal_nand.c:412-447`); the boot ROM's view of bad blocks is unknown.
- For #30 (UBI or any raw layout): bad-block handling is the consumer's job, and the driver reports nothing wrong today only because there is nothing to report.
- Block lock: vendor drivers never issue UNLOCK (23h/24h) or BLOCK LOCK READ STATUS (7Ah) (`grep` over both vendor `al_nand.c`), yet stock U-Boot programs this chip, so the chip is not default-locked (LOCK pin low/open). Direct 7Ah read not done.

## 5. ECC correctness

### Model (derived from stock-written pages, reproduced by `scripts/nand_bch.py`)
- Binary BCH over GF(2^15), primitive polynomial `x^15 + x + 1`, **t = 8**, one 512-byte codeword, message bytes MSB-first.
- Generator = lcm of minimal polynomials of a^1..a^16; degree 120 (matches HAL `15 * t`, `al_hal_nand.c:2069-2073`).
- Parity = remainder of `msg * x^120 mod g`, 120 bits.
- Stored in a 16-byte slot as four little-endian 32-bit words; the HAL's slot size is `ALIGN_UP(ceil(120/8), 4)` = 16 (`al_hal_nand.c:2075-2086`).
- The top byte of the first word (slot byte 3) is not parity; varies per codeword; **meaning not identified**; the decoder does not need it.
- Page layout: 8 codewords; OOB `[0,4)` = `ff`; slot *i* at OOB `[4+16i, 20+16i)`; **ECC occupies OOB `[4,132)` = 128 bytes**.
- Strength t=8 / 512 B meets the datasheet minimum (8 bit / 540 B) with margin on data coverage.
- The driver overrides the PBS-decoded algorithm and strength with the ONFI requirement (`al_nand_main.c:777-790`, vendor 4.19 `al_nand.c:776-781` does the same); the media agrees, the PBS value is never logged.

### Evidence (raw `nanddump --noecc --oob` vs model, 2026-10-10 13:29-13:45, rsync running)

| partition | pages | erased (raw all-0xff) | parity ok | 1-bit flip | other mismatch |
|---|---|---|---|---|---|
| al_boot | 512 | 512 | 0 | 0 | 0 |
| device_tree | 256 | 256 | 0 | 0 | 0 |
| linux_kernel | 4096 | 715 | 3381 | 0 | 0 |
| awto-uboot | 256 | 0 | 256 | 0 | 0 |
| rootfs | 256768 | 69924 | 186777 | 67 | 0 |
| chike | 256 | 98 | 157 | 1 | 0 |

- All 262144 pages classified; **0 pages with a mismatch the model cannot explain as a single flipped bit**.
- OOB[0:4] = `ff` on every programmed page. The bad-block marker bytes are never overwritten by the ECC engine.
- Three rootfs passes (13:31, 13:35, 13:46) gave 67, 66, 67 single-bit-flip pages: one marginal cell reads differently between passes.
- Flip pages are spread across the whole rootfs (page 2542 ... 186191); the model locates the bit (`byte N bit M`) in the log JSON (`tmp/logs/nand-survey-rootfs.json`).
- **Hardware correction is correct and the counter is real:** chike chip page 262034 CW4: model says byte 207 bit 0 flipped; `nanddump` raw byte `0xc7`, ECC-path byte `0xc6`, the only differing byte of the page; `corrected_bits` +1 per read (persistent flip, not transient). `eccflags` shows the controller's verdict `corrected=1, uncorrectable=0` on that page and 0/0 on neighbours.
- Erased pages: controller flags **uncorrectable=1** (`linux_kernel` pages 4000-4002); the driver converts that to "empty" (below).

### Where the ECC bytes really land vs what the driver claims
- Driver: ECC region `[ecc_offset, oobsize)` = `[4, 224)`, 220 bytes (`al_nand_main.c:846-853, 881-882`, ooblayout :481-493). Real: `[4, 132)`, 128 bytes.
- Consequences: `oobavail` = 2 (real free OOB: `[2,4)` + `[132,224)` = 94 B); `ecc.bytes` = 27 per step (meaningless); `SPARE_READ_COUNT`/`SPARE_WRITE_COUNT` are given **220** (:565-567, :652-654) where the HAL's own arithmetic says 8 x 16 = 128.
- `OOB[132:224)` after a write is whatever the controller emits for bytes past the parity:
  - 0xFF on `linux_kernel`, `chike` and every other page not written by stock U-Boot.
  - **A constant 92-byte pattern** on pages stock U-Boot `nand write` produced: all 256 pages of `awto-uboot` and 4416 pages of `rootfs` (pages 0-4351 = 0x1400000-0x2500000, pages 5120-5183 = 0x2800000-0x2840000: exactly the `flash-nand.py` kernel and DTB footprints). One distinct pattern per partition set. Parity and OOB[0:4] on those pages are correct and the controller reports 0/0 on them.
  - What the Linux driver writes there is **UNPROVEN**; harmless to ECC either way (nothing reads those bytes).
- U-Boot, 4.1.37 and 4.19 drivers all use the same 220 (`eccbytes = oob_size - ecc_loc`).

### Why "sha256 matches, `corrected_bits` is 0" does not prove the write
- Write and read both use `ecc_offset` and the same engine; a layout mistake would be consistent with itself.
- The counters are single sticky bits per page (`CORR_ERR`, `UNCORR_ERR`); `corrected_bits` counts **pages**, not bits.
- A page programmed *without* parity (the exact shape of the original bug and of W6/W8): controller flags uncorrectable, `al_nand_oob_is_empty()` (:520-538, :580-586) sees <4 zero bits in the OOB, `memset(buf, 0xff)` and **no error, no counter**. Readback is all-0xff: indistinguishable from "never programmed".
- Parity at the wrong offset: loud (`ecc_failures`++, `-EBADMSG`) if the OOB is non-empty, silent 0xff if it looks empty.
- sha256 match with `corrected_bits` > 0 (reviewer's case): <= 8 disagreeing bits per codeword: marginal cell, or a few wrong parity bits.
- Detect with: raw read (`nanddump -n -o`) + an encoder that is not the controller: `scripts/nand-survey.py ecc`. It names the failing codeword and separates "cell flip" from "parity wrong".

### ECC reporting defects
- `ecc_read_page` returns 0 (:601) -> `max_bitflips` = 0 -> `-EUCLEAN` never; `bitflip_threshold` 6 unreachable; UBI would never scrub. Needs a conservative value (e.g. `ecc.strength` when `CORR_ERR` is set) since the controller gives no count.
- `ecc_stats.corrected++` per page (:596-599), not per bit.
- Erased-page test looks only at the OOB and then clobbers the data (:580-586). Mainline `nand_check_erased_ecc_chunk()` checks data and parity, tolerating up to the strength.
- `read_page` cannot return the raw OOB with corrected data (`oob_required`), s3 R3.

## 6. The `exec_op` port (#268)

### Why
- `exec_op()` returns `int` per operation: W1, W2, W3, W6, W7, R1 disappear as a class (no `void` callback to hide an error in).
- Core helpers (`nand_status_op`, `nand_read_oob_op`, `nand_prog_page_*_op`, `nand_change_*_column_op`) replace hand-rolled command sequences; `write_page_raw` / `write_oob` defaults then inherit the error paths (W4).
- `WAITRDY` carries the core's own `timeout_ms`: removes the "400 ms accidental backstop".
- The header comment (`al_nand_main.c:12-14`, "no exec_op equivalent") is wrong; see mapping.

### Mapping

| legacy | exec_op | note |
|---|---|---|
| `cmd_ctrl(CLE)` | `NAND_OP_CMD_INSTR` | one `CMD` entry |
| `cmd_ctrl(ALE)` | `NAND_OP_ADDR_INSTR` | one `ADDRESS` entry per cycle |
| `read_buf`, `read_byte` | `NAND_OP_DATA_IN_INSTR` | `DATA_READ_COUNT` + FIFO loop; lengths not a multiple of 4 (status 1, ID 5-6) need a bounce word; deletes `word_cache` |
| `write_buf` | `NAND_OP_DATA_OUT_INSTR` | TX on, WP off, `DATA_WRITE_COUNT`, FIFO writes; restore TX/WP after the op's `WAITRDY` |
| PAGEPROG tail of `cmd_ctrl` (:331-344) | `NAND_OP_WAITRDY_INSTR` | `WAIT_FOR_READY` + poll bounded by `timeout_ms`; fixes W9 ordering naturally |
| `dev_ready` | `WAITRDY` | no polling callback |
| `select_chip` | `op->cs` | `al_nand_dev_select(cs)` per op |
| `nand_set_timing_mode` / table | `.setup_interface` (optional) | core picks the SDR mode and sends SET FEATURES; replaces :148-176, :710-729; also the place to retire the mode-1 cap |
| `ecc.read_page` hooks | `nand_read_page_op(page, writesize + ecc_offset, NULL, 0)`, driver `SPARE_READ_COUNT`, `nand_change_read_column_op(0, buf, writesize)` | RNDOUT == change_read_column |
| `ecc.write_page` hooks | `nand_prog_page_begin_op`, `nand_change_write_column_op(writesize + ecc_offset, NULL, 0)`, driver `SPARE_WRITE_COUNT`, `nand_prog_page_end_op` | RNDIN == change_write_column |

### The OOB/ECC phase-ownership problem
- In HW-ECC mode the controller needs the parity phase *between* two NAND operations: read = `SPARE_READ_COUNT` after the page load and before the data `DATA_IN`; write = `SPARE_WRITE_COUNT` after the data `DATA_OUT` and before PAGEPROG.
- No `exec_op` instruction expresses a controller-internal entry, so the ECC hooks must interleave direct HAL calls between two `exec_op` calls. This works only if every `exec_op` drains its FIFO entries (including the data phase) before returning, so the interleaved entry lands in order.
- ECC enable must be scoped to the hooks; `exec_op` itself always runs ECC-off (status, ID, ONFI page, SET FEATURES). The same discipline already exists (:554-561, :980, :1086).
- `oob_required` can be served by a second raw `nand_read_oob_op` with ECC off (closes R3), at the cost of an extra read.
- tADL/tWHR: the `WAIT_CYCLE_COUNT` after RNDIN (:645) must map to the instruction's `delay_ns`.

### Effort and risk (estimates, not measured)
- Estimate: 6 legacy callbacks plus their helpers (~230 of 1161 lines) -> `exec_op` + parser patterns (~250 lines); 2 ECC hooks (~90 lines); optional `setup_interface` (~80). About 1-2 days of coding; box time dominates (one box, shared, no CI hardware).
- Risk to the bootloader: **none until the first write**; the driver is a module and boot reads NAND through stock U-Boot. A bad port at worst loses NAND from Linux until reload/reboot.
- Risk of regression: moderate; probe (ONFI page via `exec_op`) is the fragile step. Regression net that exists today: `verify-nand-reads.py`, `nand-survey.py ecc` over `linux_kernel` (3381 programmed pages, 2 s), `nand-write-test.py`.
- Sequencing: port **after** the legacy write is proven once (separates "hardware cannot program" from "port bug"), or fix W1-W7 first and port second.

## 7. `scripts/nand-write-test.py` audit

| finding | effect | state |
|---|---|---|
| every command ends `\| tail`; `box_sh` returned `tail`'s status | `nandwrite` exit status was never observed; criterion [1] `write_rc == 0` always true | fixed: `set -o pipefail` |
| criterion [2] re-wrote the *same* pattern and expected non-zero | NAND reports pass for identical data (NOP=4); the test fails on a correct driver and cannot show a failed write is detected | demoted to info line |
| target "erased" precondition read through the ECC engine | a page programmed without parity reads as 0xff, so it passes as erased | added raw check (`nand-survey.py ecc`, 256 pages erased) |
| `ok` ignored the target's post-erase state | erase of a *programmed* block never verified | added [5]: target checksum back to pre-test value |
| sha256 + `ecc_failures/corrected_bits == 0` only | s5 | added [4]: independent BCH check of the 64 written pages |

- Still not covered: a *failed* write being reported (needs a fault the chip reports as FAIL, or driver fault injection; none exists), the bad-block marking path, `write_page_raw`/`write_oob`, cross-reading by stock U-Boot.
- Verified 2026-10-10 without `--write`: precondition and checksum phases run; no write was run.

## 8. `docs/mtd.md` against this study

- Right: chip/identity table, partition rationale, FIFO-vs-window explanation, HAL read/write asymmetry, reads proven, `awto-uboot` timing registers, dump/restore advice for NOR.
- Stale / wrong:
  - Header and tables: "12 MTD partitions", NAND `mtd8-11`; live is 14 (NAND mtd0-5, NOR mtd6-13). The "Linux sees the NAND" block lists 5 NAND partitions and NOR `mtd5-12`.
  - Table row `mtd10 rootfs` says the first MiB is `awto-uboot` and "fix the DT label before any write path lands": done (`awto-uboot` is its own partition).
  - "whether the +0x0 aperture is genuinely ascending-addressable is **open**": settled by the 16 MiB read match.
  - "A failed program now returns `-EIO`": true only for a device-reported FAIL (s3).
  - "three defects found and fixed": four more were found in review and are open; new ones in s3/s5.
  - `flash-nand.py` section ("verified safe", addresses): the script now refuses without `--i-know-this-overwrites-awto-uboot`; the 0x1300000 kernel layout it describes is gone.
  - "Page reads are one 32-bit read per 4 bytes ... unmeasured": 13.3 MB/s measured.
  - "Capture ... `dd if=/dev/mtdN`": drops OOB and bad-block information; `flashcp` is a NOR tool, NAND restore is `flash_erase` + `nandwrite`.
- Omits: bad blocks / BBT, ECC algorithm + OOB layout, `oobavail` 2, no bitflip reporting, read-error swallowing, rated-vs-running timing, stale OOB tail on U-Boot-written pages, WP#/block lock, `max-onfi-timing-mode` unused.
- Not edited in this change.

## 9. What to do next, in order

`W` = flash write, needs the user's explicit approval. All others are read-only or host-only.

1. **Done here:** independent ECC model, bad-block/ECC survey tool, `nand-write-test.py` repaired. No `W`.
2. Fix W1-W3, W6, W7, R1 in the legacy driver: carry one error code through the op (reset per operation, checked after `nand_prog_page_end_op` and in `ecc_read_page`), make `read_byte`/`read_buf` poison on failure (return `0x00`/fail status, never a stale word), set `ecc.write_page_raw`/`ecc.write_oob` to checked wrappers, store HAL accounting back. Small, local. Needs a build and a module reload (or reboot) on the box (`al_nand` only). Risk: low; a mistake loses NAND in Linux, not the bootloader. No `W`.
3. Fix reporting (s5): return a conservative `max_bitflips` on `CORR_ERR`; replace the empty-page test with `nand_check_erased_ecc_chunk()` over data + parity; `ecc_bytes` = 128 for `SPARE_*_COUNT` (the HAL's own generator uses 8 x 16, `al_hal_nand.c:1110`) and in the ooblayout (free OOB `[2,4)` + `[132,224)`). That the controller accepts 128 is UNPROVEN and it changes what is written, so land it with the first write test, not before. No `W`.
4. Mark `awto-uboot` and `linux_kernel` `read-only` in the DT for the bring-up (needs a DTB deploy + reboot; the box is busy with the rsync). Cuts the blast radius of any address bug to `al_boot` / `device_tree` / `chike`. No `W`.
5. **`W`**: `python3 scripts/nand-write-test.py --write` (target `device_tree`, 1 block erase + 64 pages, pre-verified raw all-0xff). Pass = [1],[3],[4],[5] and `awto-uboot` checksum unchanged. Risk: lowest practical (driver does no address arithmetic; `device_tree` is 16 MiB of `linux_kernel` below `awto-uboot`; U-Boot takes the DTB from NOR). Failure modes: nothing lands (W9 WP ordering, first suspect), controller hang (cold power cycle). Do step 2 first or the failure will read as success.
6. **`W`**: read the same block from **stock U-Boot** (`nand read` at `0x200000`), compare to the pattern: independent decoder on a Linux-written page. Needs serial console and a reboot. Risk: reboot only.
7. **`W` (optional, irreversible)**: exercise bad-block marking on one block of `al_boot`. Marking cannot be undone from Linux (core refuses to erase a bad block) and burns 1 of 4096 blocks. Skip unless #30 puts UBI on this chip; alternative is review + a host-side mock of `nand_markbad_bbm`.
8. exec_op port (s6), then the timing mode question: measure mode 2-4 on reads with `verify-nand-reads.py` + `nand-survey.py ecc`, record the limit or the reason.
9. Decisions for #30 before any layout work: bad-block policy (skip vs UBI), reserve `awto-uboot` + `device_tree` blocks that are known good, whether to log the PBS BBM method.

## 10. Not established

- Whether a program issued by `al_nand` lands at all (never run).
- Whether erase works on a programmed block.
- What bytes the Linux driver writes to OOB `[132,224)`.
- The meaning of slot byte 3 (top byte of each parity slot).
- Whether any factory bad-block marker was lost before this study (no pre-use OOB image).
- PBS-decoded BBM method/locations and PBS ECC strength (decoded, never logged; boot ROM behaviour unknown).
- Direct block-lock status (7Ah never issued).
- Why `rootfs` has 66-67 single-bit-flip pages (retention, read disturb, or marginal cells); the count changed by one between passes.
- Read throughput limit and the ONFI mode the board can sustain.
- Behaviour on a read timeout (R1): never observed, code-read only.

## 11. Reproduce

- `python3 scripts/nand-survey.py bbm` -- marker + BBT survey (~4 s).
- `python3 scripts/nand-survey.py ecc --label linux_kernel --pages all` -- independent ECC check (~2 s); `rootfs` ~2.5 min (box busy: the ~182 s limit can expire under load; rerun).
- `python3 scripts/nand-survey.py eccflags --label chike --start-page 145 --pages 3` -- controller verdict per page via kretprobes (removed on exit).
- Detail JSON: `tmp/logs/nand-survey-<label>.json`; log: `tmp/logs/nand-survey.log`.
- `pytest tests/test_nand_bch.py` -- model vs stock-written codewords (`tests/data/nand-stock-cw.json`).
