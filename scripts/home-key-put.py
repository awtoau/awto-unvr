#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Install the tank/home key from secrets.yaml as /root/home.key (mode 600) on woomera, or shred it.

./dev.py home-key-put put|shred     # value goes over ssh stdin, never argv, never logged
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("home-key-put")
REMOTE_KEY = "/root/home.key"
# One short ssh exec on the LAN: <2 s expected; 30 s backstop for a stale ARP entry.
SSH_TIMEOUT_S = 30


def main() -> int:
    import yaml

    mode = sys.argv[1] if len(sys.argv) > 1 else ""
    ip = _box.require()
    if mode == "put":
        sec = yaml.safe_load(
            Path.home().joinpath(".config/awto/secrets.yaml").read_text()
        )
        key = sec["woomera-tank-home-key"]["password"]
        cmd = f"umask 077; cat > {REMOTE_KEY}; chmod 600 {REMOTE_KEY}"
        inp = key
    elif mode == "shred":
        cmd, inp = f"shred -u {REMOTE_KEY} 2>&1; ls {REMOTE_KEY} 2>&1 | head -1", ""
    else:
        print(__doc__)
        return 2
    r = subprocess.run(
        _box.ssh_argv(ip, cmd=[cmd], batch=True),
        input=inp,
        capture_output=True,
        text=True,
        timeout=SSH_TIMEOUT_S,
    )
    log(f"{mode}: rc={r.returncode} {r.stdout.strip()[-80:]} {r.stderr.strip()[-80:]}")
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
