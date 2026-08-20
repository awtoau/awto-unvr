# al_eth phylink+sfp migration — WIP, blocked, not deleted

Real, substantial driver work (compiles clean against the kernel source) implementing
Linux's generic `phylink`/`sfp` subsystem for eth2 (10G SFP+), replacing Annapurna's
vendor link-management state machine. Architecture doc: `eth-10g-phylink.md`.

## Why it's here, not in `modules/al_eth/`

Deploying it exposed a real bug in `scripts/_fedora_deploy.py`'s `sync_modules()`:
`rsync`+`depmod` both reported success, but `phylink.ko`/`sfp.ko` landed on the box at
0 bytes - a silent transfer failure `sync_modules()` never checked for. `al_eth.ko`
(which now hard-depends on `phylink.ko`) refused to load at all as a result, leaving
the box with zero network interfaces.

The sync integrity gap is now fixed (`_fedora_deploy.py`'s `_verify_sync_integrity()`,
checks for zero-byte `.ko` files post-sync) - but this migration itself was rolled
back to a known-good pre-migration state rather than re-attempted immediately, to
avoid debugging a large, entirely-unverified-on-hardware subsystem live on a box that
had just lost its only recovery network path.

## To pick this back up

1. Restore `al_eth_phylink.c`/`.h` to `modules/al_eth/`.
2. Re-apply the corresponding `modules/al_eth/al_eth.h`, `al_eth_main.c`,
   `al_init_eth_lm.c`/`.h`, `Makefile`, and `dts/alpine-v2-ubnt-unvr-ea16.dts` changes
   (not preserved here - re-derive from `eth-10g-phylink.md` and the deleted
   `al_eth_group_lm.c`/`.h`, `alpine_group_lm.c`/`.h` which are NOT deleted in the
   current tree, since the migration's deletion of those was also rolled back).
3. Build with `scripts/build-linux-71-fedora.py` (the config force-fix for
   `PHYLINK`/`SFP`/`PCS_XPCS`/`I2C_MUX_PCA954x` is already in place there).
4. Deploy with `./dev.py publish-fedora` - the sync integrity check will now catch a
   repeat of this exact failure instead of silently succeeding.
5. Boot and verify with `scripts/wait-for-boot.py` + `scripts/eth-link-status.py`
   before trusting any further link-training results.

Everything here is unverified on real hardware - it never got past module load.
