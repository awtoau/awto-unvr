# al_ssm: vendor files with no build path (retired, #325)

- `al_hal_ssm_crypto_cfg.c` (2,382 lines): crypto-engine profile config (GMAC, inline CRC, LZ/Huffman compression). Dropped from the Makefile; its 7 exported functions had only header declarations as "callers". The matching 7 prototypes were removed from `al_hal_ssm_crypto.h`.
- `al_err_events.[ch]`, `al_err_events_internal.h`, `al_err_events_ssm.[ch]` (2,414 lines): vendor error-event collector. Never in the Makefile; nothing includes it.
- `al_hal_ssm_crc_regs.h`, `al_hal_unit_adapter_gen_regs.h` (1,869 lines): included only by the files above / by nothing.
- `al_ssm_main.c` registers AES/crypto algorithms only; no CRC offload, compression or GMAC consumer exists.
- Restore only to add one of those features (crypto_cfg is a prerequisite for inline CRC and compression).
