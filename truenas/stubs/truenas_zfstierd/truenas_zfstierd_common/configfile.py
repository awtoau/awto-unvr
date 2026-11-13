# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truenas_zfstierd.conf model + renderer (#356). No daemon reads it here; format is our own."""

from __future__ import annotations

import dataclasses


@dataclasses.dataclass(frozen=True)
class ZFSTierdGlobalConfig:
    max_concurrent_jobs: int = 2
    reporting_write_interval: int = 60
    rewrite_chunk_size: int = 1024
    reporting_callback_interval: int = 1
    max_used_percent: int = 80
    enabled: bool = False


def generate_config(cfg: ZFSTierdGlobalConfig) -> str:
    lines = [
        "# truenas_zfstierd stand-in (awto-unvr #356): no tiering daemon is installed"
    ]
    lines += [
        f"{k} = {str(v).lower() if isinstance(v, bool) else v}"
        for k, v in dataclasses.asdict(cfg).items()
    ]
    return "\n".join(lines) + "\n"
