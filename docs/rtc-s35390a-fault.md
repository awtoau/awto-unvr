# s35390a / ch0 i2c wedge — FIXED. Cause: mainline's `IC_ENABLE.ABORT`.

**Root cause:** mainline Linux's `__i2c_dw_disable()` issues a hardware `IC_ENABLE.ABORT`
(`drivers/i2c/busses/i2c-designware-common.c:623-668`), which terminates the transfer
mid-byte. The S-35390A answers a mid-frame abort (datasheet Fig 46) by holding SDA low until
clocked through — the wedge.

**Fix:** `snps,no-enable-abort` DT opt-out on the `i2c_pld` node, plus the two-line driver
patch that honours it. Commits `b497cfd` (fix), `3a30b18` (400 kHz), `959e3df` (RTC enabled).

- The abort is on the **normal** path, not an error path: `i2c-designware-master.c:197` calls
  `__i2c_dw_disable()` from `i2c_dw_xfer_init()`, i.e. the start of every transfer.
  `emptyfifo_hold_master` is true for us, so `MST_ON_HOLD` genuinely asserts.
- `grep -c DW_IC_ENABLE_ABORT drivers/i2c/busses/i2c-designware-common.c`: vendor 4.19.152 = 0,
  mainline v7.3 = 2. Our U-Boot has none either (`designware_i2c.c:36-47` only polls
  `ic_enable_status`) — which is why U-Boot's RTC access always worked while Linux's wedged, on
  identical hardware, timings and chip.
- Provenance, all post-dating this board's 4.19 vendor kernel by 4+ years: `2409205acd3c`
  (2023-09-11, introduced), `5d69d5a00f80` (2024-09-13), `8de3e97f3d3d` (2024-11-01),
  `f5cfe0a71588` (2026-05-27). Ubiquiti has no workaround because the code did not exist.
  `2409205acd3c` fixes a *master*-holds-SCL-low stall by aborting; on this board the abort's
  mid-frame STOP puts the *slave* into holding-SDA-low.

## Verified on hardware (2026-09-04)

- Full `i2cdetect -y -r 1` scan of ch0 — the operation that wedged the bus every previous time —
  completes with **0 `controller timed out`**. All eight s35390a addresses 0x30-0x37 respond;
  adt7475 still reads.
- 15 full scans across ch0/ch1/ch3 at **400 kHz** (the parts' rated max, up from the 100 kHz
  picked to dodge the wedge). SFP EEPROM reads fine.
- `i2ctransfer -y 1 r1@0x40` (empty address) → clean `Remote I/O error` NAK, bus untouched.
- `rtc-s35390a 1-0030: registered as rtc0`; `/dev/rtc0` present, `hwclock -r` reads correctly.

Datasheet: [sources/chips/S-35390A.pdf](../sources/chips/S-35390A.pdf) (ABLIC Rev.4.2), chip doc
[docs/chips/s-35390a.md](chips/s-35390a.md). Single source for the ch0 wedge — other docs link here.

## Reference kernel

Stock is **4.19.152** (`urnvr-kernel-4.19.152`, `CONFIG_RTC_DRV_S35390A=y` in
`alpine_ubnt_unifi_defconfig:525`), not the 4.1.37 GPL drop — 4.1.37 does not set that symbol at
all and is a stale artifact. Take every stock-vs-ours diff against 4.19.152.

Stock drove ch0 successfully every boot: `docs/hw-reference/20260816-104601/dmesg.txt:324,353`
(registered + a successful clock read), `live.dts:264-267` (`s35390a@30`, no `status` property).

## Ruled out — do not re-test

| Hypothesis | Verdict |
|---|---|
| Faulty chip / flat MS621 coin cell | No. Stock read it every boot on this exact unit. |
| SCL hcnt/lcnt timing (`i2c-*-scl-*cnt-raw`) | No. Vendor 4.19 computes the counts exactly like mainline; nothing reads those props from OF. They are vestigial in stock. |
| `i2c-scl-falling-time-ns` | Applied and insufficient — the scan still wedged with lcnt above stock's. |
| SDA **RX** hold bit (`171e23e150ac`) | No. Vendor 4.19 force-sets it too and does not wedge. `snps,no-sda-rx-hold` moved us *away* from stock; removed. |
| `i2c-mux-idle-disconnect` | No. Removed (matching stock), scan still wedged until the ABORT fix landed. |
| `i2c_dw_set_mode()` pair | No-ops here: we register no i2c slave, so both calls early-return. |
| Generic NAK-on-empty-address | No. ch3 NAKs a nonexistent address cleanly and survives. |
| Bit-bang GPIO bus recovery | Impossible *and* the wrong frame. pld SCL/SDA are dedicated pads, not MUIO-muxable (see below). |

## pld SCL/SDA GPIO routability — NO

AL-324 MUIO pin-mux (`delroth-alpine_hal/drivers/pbs/al_hal_muio_mux_map.h`, enum
`include/pbs/al_hal_muio_mux.h`) has exactly one i2c entry: `AL_MUIO_MUX_IF_I2C_GEN` = I2C_B
(i2c_gen 0xfd894000) on pins 30/31. No I2C_A/pld entry anywhere; pld SCL/SDA (0xfd880000) are
dedicated pads. Cross-checked against the live 48-ball mux read (`docs/gpio-map.md`).

## If it ever wedges again

Recovery is a **cold power cycle** (`./dev.py power-cycle`), not a reboot — a warm reset never
removes power from the s35390a, so it comes back still holding SDA. Measured: 123 timeouts inside
2 minutes of uptime after `systemctl reboot` from a wedged state; 0 after a power cycle.

A wedge is an outage, not cosmetic — SFP i2c reads fail long enough that `sfp.c` concludes the
module was pulled and the 10G link drops.
