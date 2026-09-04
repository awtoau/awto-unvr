#!/usr/bin/env python3
"""Publish a build-out-fedora build: regenerate tftp artifacts + sync the
module tree onto woomera. Must run while Fedora is up (needs SSH).

Phase 1 of 2 for a Fedora deploy. Phase 2 is `./dev.py deploy-ssd [--reboot]`
(#216: awto-uboot boots /boot/uImage off the SSD, so the deploy is an scp).
`./dev.py flash` is RECOVERY ONLY - it overwrites awto-uboot in NAND.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _fedora_deploy import log, publish_artifacts, sync_modules


def main() -> int:
    publish_artifacts()
    sync_modules()
    log(
        "DONE - tftp artifacts + module tree both fresh from the same build-out. "
        "Next: ./dev.py deploy-ssd --reboot (NOT ./dev.py flash - that overwrites "
        "awto-uboot in NAND, #216)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
