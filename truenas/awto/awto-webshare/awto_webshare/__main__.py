# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""awto-webshare [--plain]: serve on proxy.port (755) over TLS using middleware's rendered config."""

from __future__ import annotations

import argparse
import asyncio
import glob
import logging
import os
import signal
import ssl
import subprocess

from aiohttp import web

from . import config as cfgmod
from .app import State, make_app
from .pam import PamAuth

log = logging.getLogger("awto-webshare")
STATE_DIR = "/var/lib/awto-webshare"


def tls_context(cfg: cfgmod.Config) -> ssl.SSLContext:
    """TrueNAS Connect cert if middleware has one, else a self-signed cert made once (no TNC here)."""
    crt = sorted(glob.glob(os.path.join(cfg.cert_dir, cfg.cert_prefix + "*.crt")))
    key = [c[:-4] + ".key" for c in crt]
    if crt and os.path.exists(key[0]):
        cert, keyf = crt[0], key[0]
    else:
        cert, keyf = f"{STATE_DIR}/cert.pem", f"{STATE_DIR}/key.pem"
        if not os.path.exists(cert):
            os.makedirs(STATE_DIR, mode=0o700, exist_ok=True)
            subprocess.run(
                ["openssl", "req", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:prime256v1",
                 "-nodes", "-days", "3650", "-subj", "/CN=awto-webshare", "-keyout", keyf, "-out", cert],
                check=True, capture_output=True,
            )  # fmt: skip
            log.info("generated self-signed certificate %s", cert)
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.load_cert_chain(cert, keyf)
    return ctx


async def serve(plain: bool) -> None:
    cfg = cfgmod.load()

    def loader():
        return cfgmod.load()

    loader.mtime = cfgmod.mtime  # type: ignore[attr-defined]
    pam_cache: dict[str, PamAuth] = {}

    def authenticate(service: str, user: str, pw: str):
        return pam_cache.setdefault(service, PamAuth(service)).authenticate(user, pw)

    runner = web.AppRunner(make_app(State(loader, authenticate)))
    await runner.setup()
    ssl_ctx = None if plain else tls_context(cfg)
    addrs = [
        a for a in cfg.bind_addrs if not ("::" in cfg.bind_addrs and a == "0.0.0.0")
    ]  # "::" is dual-stack
    for a in addrs:
        await web.TCPSite(runner, a, cfg.port, ssl_context=ssl_ctx).start()
        log.info("listening on %s:%d", a, cfg.port)
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    loop.add_signal_handler(signal.SIGTERM, stop.set)
    loop.add_signal_handler(
        signal.SIGHUP, lambda: None
    )  # config is re-read when its files change
    await stop.wait()
    await runner.cleanup()


def main() -> int:
    ap = argparse.ArgumentParser(prog="awto-webshare")
    ap.add_argument(
        "--plain", action="store_true", help="HTTP, no TLS (behind a proxy / tests)"
    )
    logging.basicConfig(level=logging.INFO, format="%(name)s: %(message)s")
    asyncio.run(serve(ap.parse_args().plain))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
