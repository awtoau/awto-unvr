#!/usr/bin/env python3
"""Publish a build-out-fedora build: regenerate tftp artifacts + sync the
module tree onto woomera. Must run while Fedora is up (needs SSH).

Phase 1 of 2 for a Fedora deploy - see scripts/_fedora_deploy.py for why this
is split from flash-nand.py (phase 2, needs the box at the U-Boot prompt
instead). Run this, THEN reset to U-Boot, THEN ./dev.py flash.
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
        "DONE - tftp artifacts + module tree both fresh from the same build-out. Next: reset to U-Boot, then ./dev.py flash"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
