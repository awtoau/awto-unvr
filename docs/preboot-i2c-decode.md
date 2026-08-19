# Preboot DW-i2c setup — does stock program IC_SDA_HOLD?

Bears on the s35390a ch0 bus wedge ([rtc-s35390a-fault.md] #78/#86): our DTS added
`i2c-sda-hold-time-ns = 300`. Question: does the stock boot chain program IC_SDA_HOLD
(DW-i2c reg 0x7c) on the pld bus, or rely on the reset default? Decoded from the
preboot blobs (static; `docs/nor-reference/preboot-s2-disassembly.asm`).

## Controller

- Bus = **`0xfd880000` = AL_I2C_PLD_BASE** (i2c-pld, Linux **i2c-0**) — confirmed:
  `i2c_eeprom_read` (S2 `0xf22044d0`) loads base from pool `[0xf2204510]=0xfd880000`.
  This is the bus the RTC (s35390a) + board EEPROM + DDR-SPD EEPROM sit on.

## What S2's i2c config writes — `s_i2c_xfer_a` @0xf2204694

DW-i2c register offsets; base = r2 = 0xfd880000, cfg struct = r1 = 0xf2205df8.

| reg | offset | value stock writes | meaning |
|---|---|---|---|
| IC_ENABLE | 0x6c | clear bit0 | disable before config (mandatory) |
| IC_CON | 0x00 | **0x63** | MASTER_MODE + SPEED=1 (standard, 100 kHz) + RESTART_EN + SLAVE_DISABLE (`cfg[0x19]=0x02` OR'd with 0x61; 0x71 variant adds 10-bit master) |
| IC_TAR | 0x04 | target addr (or 0x1000 flag) | slave target address |
| IC_SAR | 0x08 | cfg (slave-mode path only) | slave address |
| IC_RX_TL | 0x38 | cfg[0x08] | RX FIFO threshold |
| IC_TX_TL | 0x3c | cfg[0x0c] | TX FIFO threshold |
| **IC_SDA_HOLD** | **0x7c** | **NOT WRITTEN** | left at DW-i2c reset default |
| IC_SS_SCL_HCNT/LCNT | 0x14/0x18 | **NOT WRITTEN** | SCL timing left at reset default |
| IC_FS_SCL_HCNT/LCNT | 0x1c/0x20 | **NOT WRITTEN** | — |

Transfer (`al_i2c_perform_write` @0xf2204660 / `s_i2c_xfer_b` @0xf22046fc): FIFO writes
to IC_DATA_CMD (0x10), polls IC_STATUS (0x70). No further config registers touched.

## al_boot path — same HAL, same answer

- al_boot uses the Annapurna `al_hal_i2c.c` HAL (`al_i2c_perform_write` name matches).
- `al_i2c_init` in that HAL writes con / tar / sar / rx_tl / tx_tl — **not** sda_hold.
- IC_SDA_HOLD has its **own opt-in function** `al_i2c_hold_time_set`
  (`al_reg_write32(&regs->sda_hold, hold_ns*ref_clk_mhz/1000)`), called only if the
  firmware explicitly requests a hold time. No stock preboot path calls it for the pld
  bus (no absolute or offset-0x7c write to a 0xfd880000 base found).

## Conclusion for #78/#86

- **Stock does NOT program IC_SDA_HOLD on i2c-0.** It runs the DW-i2c at its hardware
  reset default SDA-hold, standard-mode 100 kHz, and never rewrites SCL timing either.
- So our `i2c-sda-hold-time-ns = 300` is a **Linux-side fix, not a copy of stock** —
  stock's own EEPROM/RTC accesses work at the reset default. The wedge is a property of
  the mainline `i2c-designware` driver's programming (it recomputes and writes SDA_HOLD
  from the DT, and 0 / a wrong value there is what drops the hold), not of a value stock
  sets and we lost. Match target = the controller reset default, not a stock-written
  number. Confirm the AL-324 DW-i2c reset default of reg 0x7c (IC_COMP_VERSION-dependent)
  before pinning 300 ns.
