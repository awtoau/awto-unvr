# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Reads the files middleware renders: etc_files/webshare-auth/config.json and webshare/config.json."""

from __future__ import annotations

import dataclasses
import json
import os

AUTH_CONFIG = "/etc/webshare-auth/config.json"


@dataclasses.dataclass(frozen=True)
class Share:
    name: str
    path: str
    is_home_base: bool = False


@dataclasses.dataclass(frozen=True)
class Config:
    pam_service: str = "webshare"
    allowed_groups: tuple[str, ...] = ("truenas_webshare",)
    shares: tuple[Share, ...] = ()
    port: int = 755
    bind_addrs: tuple[str, ...] = ("0.0.0.0", "::")
    cert_dir: str = "/etc/certificates"
    cert_prefix: str = "truenas_connect"

    def share(self, name: str) -> Share | None:
        return next((s for s in self.shares if s.name == name), None)


def load(auth_path: str = AUTH_CONFIG) -> Config:
    with open(auth_path) as f:
        auth = json.load(f)
    with open(auth.get("webshare_config_path", "/etc/webshare/config.json")) as f:
        ws = json.load(f)
    proxy = auth.get("proxy", {})
    return Config(
        pam_service=auth.get("pam_service_name", "webshare"),
        allowed_groups=tuple(auth.get("allowed_groups", ())),
        shares=tuple(
            Share(s["name"], s["path"], bool(s.get("is_home_base")))
            for s in ws.get("shares", ())
        ),
        port=int(proxy.get("port", 755)),
        bind_addrs=tuple(proxy.get("bind_addrs") or ("0.0.0.0", "::")),
        cert_dir=proxy.get("cert_path", "/etc/certificates"),
        cert_prefix=proxy.get("cert_prefix", "truenas_connect"),
    )


def mtime(auth_path: str = AUTH_CONFIG) -> float:
    try:
        return max(
            os.stat(auth_path).st_mtime,
            os.stat(
                json.load(open(auth_path)).get(
                    "webshare_config_path", "/etc/webshare/config.json"
                )
            ).st_mtime,
        )
    except (OSError, ValueError):
        return 0.0
