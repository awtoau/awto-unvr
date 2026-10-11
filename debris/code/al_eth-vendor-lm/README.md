# al_init_eth_lm.c detect/establish state machines (retired, #325)

- `al_init_eth_lm-retired.c`: 40 symbols cut from `modules/al_eth/al_init_eth_lm.c` (not compilable as-is; excerpt blocks, each headed `/* ---- name ---- */`).
- Contents: SFP/QSFP EEPROM detect, link detection + establish step machines, Auto-FEC, rx-equalisation run, BR210/BR410/DS25 retimer drivers + tables, link check/pause/state getters.
- Why dead: phylink + sfp.c own presence, EEPROM, carrier (#113); board has `retimer_exist = false`; nothing calls them (`scripts/find-dead-code.py`).
- Kept in `al_init_eth_lm.c`: `al_eth_lm_init`, static TX/RX SerDes tables + `al_eth_serdes_static_{tx,rx}_params_set`, `static_parameters_{override,get}`, `debug_mode_set`.
- Lost with it (not re-implemented): lm_context->lock, DS125 retimer table, 1G-DA tables (never ported; see modernisation plan).
