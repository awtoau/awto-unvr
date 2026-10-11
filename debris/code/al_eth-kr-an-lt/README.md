# al_eth KR auto-negotiation / link-training (retired, #325)

- Vendor Clause-73 AN + Clause-72 LT: `al_init_eth_kr.[ch]`, `al_hal_eth_kr.[ch]`, `al_hal_an_lt_wrapper_regs.h` (~2,260 lines).
- Only caller was `al_eth_an_lt_execute()` in `al_eth_lm_link_establish_step()`; that state machine is retired (`../al_eth-vendor-lm/`).
- Why dead: the SFP+ port runs fixed 10.3125G with `auto-neg`/`link-training = "disabled"`, link managed by phylink + sfp.c (#113).
- Restore only for 10GBASE-KR backplane; mainline has no Clause 72 framework.
- Evidence table: `docs/vendor-code-modernisation.md` item 5.
