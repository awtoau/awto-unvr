# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truesearchd serve | index  (units: truesearchd.service, truesearch-indexer.service)."""

from __future__ import annotations

import argparse
import logging
import signal
import threading
import time

from .index import Index
from .server import Daemon, Server

log = logging.getLogger("truesearchd")
SOCKET = "/run/truesearchd/mdssvc.sock"
DB = "/var/lib/truesearchd/index.db"


def indexer_loop(index: Index, interval: float, stop: threading.Event) -> None:
    """Rescan each registered target when older than `interval`.

    interval 600 s: walk is the only change detector (no inotify); a stale hit costs one failed stat.
    Poll 5 s: pick up a newly registered target soon after the first OPEN.
    """
    while not stop.is_set():
        for tid, _ds, _share, root, last in index.targets():
            if time.time() - last > interval:
                t0 = time.time()
                n = index.scan(tid, root)
                log.info(
                    "scanned %s: %d entries in %.1f s",
                    tid.replace("\t", ":"),
                    n,
                    time.time() - t0,
                )
        stop.wait(5)


def main() -> int:
    ap = argparse.ArgumentParser(prog="truesearchd")
    ap.add_argument("mode", choices=["serve", "index"])
    ap.add_argument("--socket", default=SOCKET)
    ap.add_argument("--db", default=DB)
    ap.add_argument("--interval", type=float, default=600.0)
    a = ap.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(name)s: %(message)s")
    # `systemctl reload` (middleware truesearch.configure): targets register themselves, nothing to re-read
    signal.signal(signal.SIGHUP, lambda *_: None)
    index = Index(a.db)
    if a.mode == "index":
        indexer_loop(index, a.interval, threading.Event())
        return 0
    srv = Server(a.socket, Daemon(index))
    log.info("serving %s, index %s", a.socket, a.db)
    srv.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
