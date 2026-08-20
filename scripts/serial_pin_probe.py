#!/usr/bin/env python3
"""Map which USB-serial pins are physically connected, without a meter.

No voltages - the PL2303/CP2102 have no ADC. Logic levels only, via termios:
  read  : CTS, DSR, DCD, RI (TIOCMGET)
  drive : RTS, DTR (TIOCMSET), TX (TIOCSBRK holds TX low; released = idle high)
  RX    : not directly readable - a low on RX raises a BREAK, delivered as 0x00

Break-based TX->RX continuity is **baud-independent**: it tests the wire as a
wire, with no framing or bit timing involved. That is the point of this script -
it answers "are these two pins joined" when a normal loopback cannot.

See docs/issues/38-ssh-normal-access.md sibling work; loopback history in
scripts/serial_loopback_test.py.
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

# Break must be held longer than one character frame for the receiver to call it a
# break. At 9600 8N1 a frame is 10 bits = 1.04 ms. The USB latency timer (~10 ms)
# dominates, so hold 50 ms (~48 frames, 5x the latency timer).
# On expiry of the read window: report "no break seen" = TX not joined to RX.
BREAK_HOLD_S = 0.050
SETTLE_S = 0.020  # modem-line change to status report: 2 USB control transfers ~10 ms

INPUTS = ("cts", "dsr", "cd", "ri")

log = logging.getLogger("pinprobe")


def read_inputs(port: serial.Serial) -> dict[str, bool]:
    return {name: bool(getattr(port, name)) for name in INPUTS}


def fmt(state: dict[str, bool]) -> str:
    return "  ".join(f"{k.upper()}={int(v)}" for k, v in state.items())


def tx_to_rx(port: serial.Serial) -> bool | None:
    """Hold TX low and look for a break (0x00) on RX. True = TX joined to RX."""
    port.reset_input_buffer()
    port.timeout = BREAK_HOLD_S
    port.break_condition = True
    got = port.read(4)
    port.break_condition = False
    time.sleep(SETTLE_S)
    port.reset_input_buffer()
    return bool(got) and all(b == 0 for b in got)


def drive_and_read(port: serial.Serial, out: str, level: bool) -> dict[str, bool]:
    setattr(port, out, level)
    time.sleep(SETTLE_S)
    return read_inputs(port)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument(
        "--baud",
        type=int,
        default=9600,
        help="only sets frame length for break detection; 9600 = 1.04 ms/frame",
    )
    args = ap.parse_args()

    logdir = Path("tmp/logs")
    logdir.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(message)s",
        handlers=[
            logging.FileHandler(logdir / "serial_pin_probe.log"),
            logging.StreamHandler(sys.stdout),
        ],
    )

    log.info("port %s @ %d baud", args.port, args.baud)
    try:
        port = serial.Serial(
            args.port, args.baud, timeout=BREAK_HOLD_S, write_timeout=1
        )
    except (serial.SerialException, OSError) as exc:
        log.error("open failed: %s", exc)
        return 2

    with port:
        port.rts = False
        port.dtr = False
        port.break_condition = False
        time.sleep(SETTLE_S)

        idle = read_inputs(port)
        log.info("")
        log.info("idle (RTS=0 DTR=0, TX high):   %s", fmt(idle))
        log.info("  a floating input usually reads 0; a driven-high one reads 1")

        log.info("")
        log.info("--- TX -> RX (break continuity, baud-independent) ---")
        joined = tx_to_rx(port)
        log.info(
            "  TX held low %.0f ms -> RX %s",
            BREAK_HOLD_S * 1e3,
            "saw BREAK (0x00): TX IS JOINED TO RX"
            if joined
            else "saw nothing: TX is NOT joined to RX",
        )

        log.info("")
        log.info("--- TX -> inputs (does TX reach CTS/DSR/CD/RI?) ---")
        port.break_condition = True
        time.sleep(SETTLE_S)
        tx_low = read_inputs(port)
        port.break_condition = False
        time.sleep(SETTLE_S)
        tx_high = read_inputs(port)
        log.info("  TX low : %s", fmt(tx_low))
        log.info("  TX high: %s", fmt(tx_high))
        for name in INPUTS:
            if tx_low[name] != tx_high[name]:
                log.info("  ** TX is connected to %s **", name.upper())

        log.info("")
        log.info("--- RTS / DTR -> inputs ---")
        for out in ("rts", "dtr"):
            lo = drive_and_read(port, out, False)
            hi = drive_and_read(port, out, True)
            setattr(port, out, False)
            log.info("  %s=0: %s", out.upper(), fmt(lo))
            log.info("  %s=1: %s", out.upper(), fmt(hi))
            for name in INPUTS:
                if lo[name] != hi[name]:
                    log.info("  ** %s is connected to %s **", out.upper(), name.upper())

        log.info("")
        if joined:
            log.info("RESULT: TX-RX continuity CONFIRMED (break detected)")
            return 0
        log.info("RESULT: TX-RX open - no break reached RX")
        log.info("  If a normal loopback ever echoed on this cable, the path is not a")
        log.info("  plain wire: a transceiver/level-shifter blocks DC but passes data.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
