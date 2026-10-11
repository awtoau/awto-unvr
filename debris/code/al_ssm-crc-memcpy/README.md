# al_ssm CRC/memcpy offload HAL (retired, #325)

- `al_hal_ssm_crc_memcpy.[ch]` (1,133 lines): descriptor builders for the CRC/checksum + memcpy engine.
- Only caller was `al_ssm_error_ints_unmask()` (error-IRQ unmask for crypto + raid + crc), itself uncalled in the Linux module. `al_ssm_main.c` registers crypto algorithms only.
- `al_hal_ssm_crypto-error-ints-retired.c`: `al_crypto_error_ints_{mask_get,unmask}`, callable only from that unmask.
- `al_ssm_error_ints_unmask` (cut from `al_hal_ssm.c`/`.h`); `al_raid_error_ints_unmask` stays (al_hal_ssm_raid.* is out of scope of this retirement).
- U-Boot's own al_ssm copy (`uboot-port/drivers/crypto/al_ssm/`) still calls its version; untouched.
