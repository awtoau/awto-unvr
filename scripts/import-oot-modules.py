#!/usr/bin/env python3
"""Import the out-of-tree Annapurna kernel-module sources INTO this repo.

The repo copy under modules/ is the tracked source-of-truth; ALL build scripts
(71 / 618 / 612) copy FROM here directly — NO symlinks. The PORT tree is a
read-only upstream reference, used only for the FIRST import of a new module.

Fixes captured as tracked source (were untracked mods in the linux-alpine-v2 repo):
  * iofic UBSAN: al_eth/al_ssm/al_dma al_hal_iofic_regs.h -> ctrl[4] +
    sizeof-relative rsrvd1 (out-of-bounds grp_ctrl access).
  * al_ssm crypto: al_ssm_main.c -> al_crypto_xts_cfg_set + IV writeback.

Source-only: .c/.h/Makefile/Kbuild/README.md. Build artifacts (.o/.ko/.cmd/.mod*
/Module.symvers/modules.order) are skipped and gitignored under modules/.

Safe/idempotent: never overwrites an existing tracked modules/<m> (which carries
our fixes) — it imports only a module not yet in the repo.

Run: python scripts/import-oot-modules.py  (logs to tmp/logs/import-oot-modules.log)
"""

import os
import pathlib
import shutil
import sys

REPO = "/mnt/2tb/git/awto-unvr"
PORT = "/mnt/2tb/unvr-port-refs/linux-alpine-v2/modules"
# Imported: the OOT al_* module source trees. rtl8370mb/al_thermal excluded (no
# build script compiles them; al_thermal doesn't exist).
MODULES = ("al_eth", "al_ssm", "al_dma", "al_sgpo", "al_reboot")

SRC_EXT = {".c", ".h"}
SRC_NAMES = {"Makefile", "Kbuild", "README.md"}
GITIGNORE = """\
# Build artifacts from out-of-tree `make M=...` — never committed.
# Source (.c/.h/Makefile/Kbuild) is the tracked imported copy.
*.o
*.ko
*.mod
*.mod.c
*.mod.o
.*.cmd
*.cmd
Module.symvers
modules.order
.tmp_versions/
"""

LOG = os.path.join(REPO, "tmp/logs/import-oot-modules.log")
_logf = None


def log(m):
    print(m, flush=True)
    if _logf:
        _logf.write(m + "\n")
        _logf.flush()


def is_source(name):
    if name.endswith(".mod.c"):  # modpost-generated stub, a build artifact
        return False
    return os.path.splitext(name)[1] in SRC_EXT or name in SRC_NAMES


def import_one(m):
    src = os.path.join(PORT, m)
    dst = os.path.join(REPO, "modules", m)
    if not os.path.isdir(src):
        log(
            f"{m}: not in PORT; repo modules/{m} is the source of truth (already imported)"
        )
        return 0
    if os.path.isdir(dst):
        log(
            f"{m}: modules/{m} already tracked (carries our fixes) - NOT overwriting from PORT"
        )
        return 0
    os.makedirs(dst, exist_ok=True)
    n = 0
    for name in sorted(os.listdir(src)):
        sp = os.path.join(src, name)
        if not os.path.isfile(sp) or not is_source(name):
            continue
        shutil.copy2(sp, os.path.join(dst, name))
        n += 1
    log(f"{m}: imported {n} source files -> modules/{m}")
    return n


def main():
    global _logf
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    _logf = open(LOG, "a")
    log("=== import-oot-modules ===")
    os.makedirs(os.path.join(REPO, "modules"), exist_ok=True)
    total = 0
    for m in MODULES:
        total += import_one(m)
    gi = os.path.join(REPO, "modules/.gitignore")
    pathlib.Path(gi).write_text(GITIGNORE)
    log(f"wrote {gi}")
    log(f"DONE: {total} source files across {len(MODULES)} modules")


if __name__ == "__main__":
    sys.exit(main())
