# Kernel boot-log fixes (7.1.8 woomera)

Root cause + fix per boot-log error. Source of record for the OOT module fixes.
The module sources are now vendored in-repo under `modules/` (single
source-of-truth, same pattern as the ea16 DTS); the fixes below live as tracked
source there. Tracked patches in `patches/` remain as per-fix diffs.

Captured from: `tmp/logs/woomera-boot-dmesg.log` (kernel 7.1.8-dirty).

## Bug 1 — UBSAN array-index-out-of-bounds in al_hal_iofic.c

- Owned/handled separately (coordinator). Not covered here.
- Summary: `struct al_iofic_regs` uses zero-length arrays (`ctrl[0]`,
  `grp_int_mod[0][32]`); `al_hal_iofic.c` indexes them with group 0..3 → UBSAN
  under `CONFIG_UBSAN_BOUNDS`. Fix = size arrays to `AL_IOFIC_MAX_GROUPS`=4
  while preserving register offsets (embedded by value in `udma_iofic_regs` →
  `udma_gen_regs_v3/v4`, so the reserved gaps must be made sizeof-relative).
- A worked, layout-verified variant is saved at
  `tmp/my-iofic-fix-for-coordinator.diff` (al_eth) for reference.

## Bug 2 — al_ssm crypto self-tests fail (data integrity)

File: `modules/al_ssm/al_ssm_main.c` (skcipher wrapper for the 7.1 crypto API).
The HAL (`al_hal_ssm_crypto*.c`) is correct register-level code — not touched.
Patch: `patches/al_ssm-crypto-cbc-iv-xts-tweak.patch`.

### 2a. cbc-aes: "wrong output IV" (test vector 0)

- Root cause: wrapper programs the INPUT IV (`enc_iv_in`/`iv_dma`) but never
  writes the UPDATED output IV back to `req->iv`. 7.1 skcipher requires
  `req->iv` = last ciphertext block on return (CBC chaining) or the self-test
  fails. Ciphertext itself was correct (testmgr reached the IV check).
- Fix: new `al_ssm_copy_result()` — after the result copy-back, for CBC set
  `req->iv` to the last cipher block. Encrypt → last block of output
  (`dst_virt`); decrypt → last block of input ciphertext (`src_virt`). Reads
  from the private bounce buffers, so in-place (`req->src == req->dst`) is
  correct even though `req->dst` gets overwritten first. Called from all three
  completion sites: `al_ssm_complete_one` (async), the inline sync fast-path in
  `al_ssm_enqueue`, and the backlog drain.
- Why correct: matches mainline CBC IV-chaining semantics; derived from
  ciphertext so it is HW-independent. XTS/others left unchanged (no IV chaining).

### 2b. xts-aes: "wrong result" (test vector 1, non-zero tweak)

- Root cause: wrapper never calls `al_crypto_xts_cfg_set()`, so the crypto
  engine's `xts_conf.tweak_calc_swap` register keeps its power-on value. The
  HAL default config is `swap_pre_inc_bytes = TRUE` — the byte-swap applied to
  the tweak SEED before it is AES-encrypted. Without it the seed byte order is
  wrong, so the tweak `T0 = AES(Key2, seed)` is wrong for any non-zero seed.
- Why vector 0 passed but vector 1 failed: XTS-AES vector 0 has an all-zero
  key/tweak/plaintext, so a wrong seed byte-swap is invisible (swap of 0 = 0)
  and the block-to-block tweak progression (post-increment) is exercised and
  proven correct. Vector 1 has a non-zero tweak seed (`0x33333333330…`) and
  distinct keys → the missing pre-increment swap corrupts `T0` → wrong
  ciphertext. Everything else in the SA is byte-correct (verified: IEEE key
  order Key1‖Key2, tweak key at SA word 20 within the single 256 B entry, fresh
  SA load each transaction).
- Fix: call `al_crypto_xts_cfg_set(&dev->ssm_dma, dev->unit_info.crypto_regs_base,
  NULL)` in probe after DMA init, before `crypto_register_skciphers`. NULL =
  the HAL's own documented default. This is the mandatory XTS init step the
  wrapper omitted; it uses the HAL, does not modify it.
- Runtime validation is the parent's follow-up (reboot + rerun self-tests).

Build: `al_ssm.ko` rebuilds clean against `linux-v7.1.8`
(`tmp/logs/build-al_ssm-final.log`). Pre-existing frame-size/fallthrough
warnings in the HAL are unrelated and untouched.

## Bug 3 — pca953x 0-0029 probe fails -121 (EREMOTEIO)

File: `dts/alpine-v2-ubnt-unvr-ea16.dts` (tracked here).

- Root cause: `i2c_gpio2: gpio@29` (PCA9575 #2) described a chip that is NOT
  populated on the 4-bay UNVR — it drives Pro bays 5-8 (see
  `docs/dt-gaps-hardware-of-record.md`, "unpopulated, correctly absent"). The
  pca953x driver probes it and the I2C transfer NAKs → -121.
- Fix: node removed (correct board description; chip absent). No other node
  references `&i2c_gpio2` (verified). `gpio@21` (the populated PCA9575 driving
  bays 1-4) is retained.

## timer0 — "deferred probe pending: (reason unknown)"

File: `dts/alpine-v2-ubnt-unvr-ea16.dts`.

- Root cause: `fd890000.timer` = SP804 `timer0` in `alpine-v2.dtsi`, an
  `arm,sp804`/`arm,primecell` device. The dtsi gives it `clocks = <&sbclk>` but
  no `clock-names = "apb_pclk"`, so the AMBA bus pclk lookup never resolves and
  the device defers probe forever.
- Fix: `&timer0 { status = "disabled"; }` override. Linux uses the ARMv8 arch
  timer (`armv8-timer`) as clocksource/clockevent; SP804 timer0 is unused, and
  disabling it matches timer1/2/3 (already disabled in the dtsi).
- DTB rebuilds clean via kernel cpp+dtc (`make dtbs`); decompile confirms
  `timer@1890000 { status = "disabled" }` and no `gpio@29`
  (`tmp/logs/build-dtb.log`).

## Benign — confirmed, NOT changed

- `of_irq_parse_pci: failed with rc=-22` on al_ssm/al_eth/al_dma: they use
  MSI-X, not INTx (documented in the DTS). Normal.
- pca953x `supply vcc not found, using dummy regulator` (gpio@21): no regulator
  wired; harmless dummy-regulator fallback.
- RCU `Setting shift to …` / boot tuning: normal boot messages.
- `ata5 / ata7: link is slow to respond, please be patient (ready=0)`: ata5+ata7
  are the two 8 TB WDC WD82PURZ-85TEUY0 (WD Purple); they spin up in ~15 s then
  link at 6.0 Gbps (ata5.00/ata7.00 ATA-9). ata3 = Samsung 850 EVO SSD (boot);
  ata1/2/4/6/8 empty (link down). AHCI correctly waiting for spinning disks —
  normal, not a defect.

## Module-source tracking gap — CLOSED

- OOT module sources vendored in-repo: `modules/{al_eth,al_ssm,al_dma,al_sgpo,al_reboot}`
  (source-only: `.c/.h/Makefile`; artifacts gitignored via `modules/.gitignore`).
- Vendored copies carry the fixes as tracked source: iofic UBSAN (Bug 1) in the
  three `al_hal_iofic_regs.h` + `al_hal_udma_iofic_regs.h`; al_ssm crypto (Bug 2)
  in `al_ssm/al_ssm_main.c`.
- Build scripts (`build-linux-71-fedora.py`, `build-linux-71-ea16.py`,
  `build-linux-618-ea16.py`) copy each OOT module FROM `modules/` (like the DTS).
  al_sgpo's 7.1 `gpio_chip.set`-returns-int fix stays a build-time adaptation
  (`adapt_sgpo`/`adapt_module`, idempotent str-replace); vendored al_sgpo.c is pristine.
- PORT tree `linux-alpine-v2/modules/<m>` replaced with symlinks -> `modules/<m>`,
  so no consumer reads a stale copy and drift can't recur. rtl8370mb left in PORT
  (no build compiles it; not vendored).
- Vendored via `scripts/vendor-modules.py`.
