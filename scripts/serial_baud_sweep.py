#!/usr/bin/env python3
"""Sweep baud rates on a loopback and dump exactly what comes back.

Why: a plain TX-RX wire is rate-agnostic - it must echo cleanly at EVERY baud.
Deterministic garbage at one rate means the RX clock differs from the TX clock,
so something active sits in the path (or the chip's baud table is broken).
Clean at exactly one rate => that is the real line rate.
"""

from __future__ import annotations

import argparse
import logging
import sys
import time
from pathlib import Path

import serial

DEFAULT_PORT = (
    "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0"
)
SWEEP = (1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600)
PAYLOAD = b"The quick brown fox jumps over the lazy dog 0123456789\r\n"

# Same budget as the loopback test: 1.25 x (wire time + 60 ms USB turnaround).
# On expiry: report bytes actually received (0 = nothing echoed at this rate).
USB_TURNAROUND_S = 0.060
TIMEOUT_MARGIN = 1.25

log = logging.getLogger("sweep")


def probe(port_path: str, baud: int) -> bytes | None:
    limit = TIMEOUT_MARGIN * (len(PAYLOAD) * 10 / baud + USB_TURNAROUND_S)
    try:
        with serial.Serial(port_path, baud, timeout=limit, write_timeout=limit) as port:
            port.reset_input_buffer()
            port.reset_output_buffer()
            port.write(PAYLOAD)
            port.flush()
            # Over-read: a rate mismatch can return MORE bytes than were sent.
            return port.read(len(PAYLOAD) * 2)
    except (serial.SerialException, OSError) as exc:
        log.error("%7d  open/IO failed: %s", baud, exc)
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument(
        "--repeat",
        type=int,
        default=3,
        help="probes per baud; >1 shows whether corruption is deterministic",
    )
    args = ap.parse_args()

    logdir = Path("tmp/logs")
    logdir.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(message)s",
        handlers=[
            logging.FileHandler(logdir / "serial_baud_sweep.log"),
            logging.StreamHandler(sys.stdout),
        ],
    )

    log.info("port %s", args.port)
    log.info("sent %d B: %r", len(PAYLOAD), PAYLOAD)
    log.info("%7s %5s %5s  %s", "baud", "got", "ratio", "first 16 bytes back")

    clean: list[int] = []
    for baud in SWEEP:
        seen: set[bytes] = set()
        for _ in range(args.repeat):
            got = probe(args.port, baud)
            if got is None:
                break
            seen.add(got)
        if got is None:
            continue

        sample = sorted(seen, key=len)[-1]
        if sample == PAYLOAD:
            clean.append(baud)
            log.info(
                "%7d %5d %5s  CLEAN - exact echo%s",
                baud,
                len(sample),
                "1.00",
                "" if len(seen) == 1 else "  (INTERMITTENT: varied across probes)",
            )
        elif not sample:
            log.info("%7d %5d %5s  nothing back", baud, 0, "-")
        else:
            ratio = len(PAYLOAD) / len(sample)
            log.info(
                "%7d %5d %5.2f  %s | %r%s",
                baud,
                len(sample),
                ratio,
                sample[:16].hex(" "),
                sample[:16],
                "  (deterministic)" if len(seen) == 1 else "  (varies per probe)",
            )
        time.sleep(
            0.05
        )  # let the chip settle between reopens; 50 ms >> its ~10 ms latency timer

    if clean:
        log.info("RESULT: clean echo at %s", clean)
        if len(clean) < len(SWEEP):
            log.info(
                "  Not clean everywhere -> not a plain wire; real line rate is above."
            )
    else:
        log.info("RESULT: no baud gave a clean echo")
    return 0 if clean else 1


if __name__ == "__main__":
    sys.exit(main())
