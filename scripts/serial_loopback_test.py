#!/usr/bin/env python3
"""Physical TX-RX loopback test for a USB-serial adapter.

- Writes patterns at several baud rates, reads them back, compares byte-for-byte.
- Pass at every baud => TX shorted to RX. Nothing back => open loop (or wrong pins).
- Partial/garbled => marginal wiring or a driver echoing rather than a real short.
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
BAUDS = (9600, 115200, 921600)
CONSOLE_BAUDS = (115200, 9600, 57600, 38400, 19200, 230400)

# Deadline = 1.25 x worst case, worst case = wire time + USB turnaround.
#   wire time  = nbytes * 10 bits / baud (8N1)
#   turnaround = 60 ms: PL2303 latency timer (~10 ms) each way + host scheduling slack
# On expiry: log port, baud, limit, elapsed, bytes actually received.
USB_TURNAROUND_S = 0.060
TIMEOUT_MARGIN = 1.25

PATTERNS = {
    "counting": bytes(range(256)),
    "alternating": b"\x55\xaa" * 32,
    "ascii": b"The quick brown fox jumps over the lazy dog 0123456789\r\n",
}

log = logging.getLogger("loopback")


def deadline_for(nbytes: int, baud: int) -> float:
    return TIMEOUT_MARGIN * (nbytes * 10 / baud + USB_TURNAROUND_S)


def run_pattern(port: serial.Serial, name: str, payload: bytes) -> bool:
    limit = deadline_for(len(payload), port.baudrate)
    port.timeout = limit
    port.reset_input_buffer()
    port.reset_output_buffer()

    t0 = time.monotonic()
    port.write(payload)
    port.flush()
    got = port.read(len(payload))
    elapsed = time.monotonic() - t0

    if got == payload:
        log.info(
            "  %-12s %3d B OK   in %6.1f ms (limit %.1f ms)",
            name,
            len(payload),
            elapsed * 1e3,
            limit * 1e3,
        )
        return True

    if not got:
        log.error(
            "  %-12s %3d B TIMEOUT after %.1f ms (limit %.1f ms) - 0 bytes back",
            name,
            len(payload),
            elapsed * 1e3,
            limit * 1e3,
        )
    else:
        first_bad = next(
            (i for i, (a, b) in enumerate(zip(payload, got)) if a != b), len(got)
        )
        log.error(
            "  %-12s %3d B MISMATCH: got %d B, first diff at byte %d "
            "(sent %r, got %r), %.1f ms",
            name,
            len(payload),
            len(got),
            first_bad,
            payload[first_bad : first_bad + 4],
            got[first_bad : first_bad + 4],
            elapsed * 1e3,
        )
    return False


def check_modem_lines(port: serial.Serial) -> None:
    """Toggle RTS/DTR and report which inputs follow - shows a handshake loopback.

    Deadline 125 ms = 1.25 x 100 ms worst case (two USB control transfers ~10 ms
    each plus the adapter's interrupt status report and host scheduling).
    On expiry: report the line as not following.
    """
    limit = 0.125
    for out_name, in_names in (("rts", ("cts",)), ("dtr", ("dsr", "cd"))):
        for in_name in in_names:
            follows = True
            for level in (True, False, True, False):
                setattr(port, out_name, level)
                t0 = time.monotonic()
                while getattr(port, in_name) != level and time.monotonic() - t0 < limit:
                    pass
                if getattr(port, in_name) != level:
                    follows = False
                    break
            log.info(
                "  %s -> %s: %s",
                out_name.upper(),
                in_name.upper(),
                "follows (looped)" if follows else "does not follow (not connected)",
            )
    port.rts = False
    port.dtr = False


def repeat_loop(port_path: str, baud: int, interval: float, rounds_max: int = 0) -> int:
    """Probe once per interval until interrupted; log every round, shout on change.

    Interval default 1.0 s: matches the timescale of a human moving a jumper, and
    the probe itself is ~80 ms at 115200 so the line sits idle between rounds.
    Reopens the port each round if it vanishes (unplug/replug).
    """
    payload = PATTERNS["ascii"]
    log.info(
        "repeat mode: %s @ %d baud, %d B every %.1f s, %s",
        port_path,
        baud,
        len(payload),
        interval,
        f"{rounds_max} rounds" if rounds_max else "Ctrl-C to stop",
    )

    port: serial.Serial | None = None
    prev: bool | None = None
    rounds = 0
    passes = 0
    try:
        while not rounds_max or rounds < rounds_max:
            rounds += 1
            try:
                if port is None:
                    port = serial.Serial(port_path, baud, timeout=1, write_timeout=1)
                    log.info("port opened")
                now = run_pattern(port, f"round {rounds}", payload)
            except (serial.SerialException, OSError) as exc:
                if port is not None:
                    port.close()
                port = None
                log.error("  round %d: port I/O failed: %s", rounds, exc)
                now = False

            passes += now
            if prev is not None and now != prev:
                log.warning(
                    ">>> CHANGE: loopback %s <<<", "APPEARED" if now else "LOST"
                )
            prev = now

            time.sleep(interval)
    except KeyboardInterrupt:
        log.info("interrupted")
    finally:
        if port is not None:
            port.close()

    log.info(
        "RESULT: %d/%d rounds echoed - %s",
        passes,
        rounds,
        "loopback PRESENT"
        if passes == rounds
        else "NO loopback"
        if passes == 0
        else "INTERMITTENT",
    )
    return 0 if passes == rounds else 1


def listen(port_path: str, baud: int, window: float) -> int:
    """Passive receive - write nothing, report anything that arrives on RX.

    Window is caller-set (default 3 s): long enough to catch a ~1 Hz periodic
    talker. On expiry with no bytes: RX line is silent, which is the result.
    """
    log.info("listen %.1f s @ %d baud (passive, nothing transmitted)", window, baud)
    try:
        with serial.Serial(port_path, baud, timeout=window) as port:
            port.reset_input_buffer()
            t0 = time.monotonic()
            got = port.read(4096)
            elapsed = time.monotonic() - t0
    except (serial.SerialException, OSError) as exc:
        log.error("  open/IO failed: %s", exc)
        return 1

    if not got:
        log.info("  silent: 0 bytes in %.1f s", elapsed)
        return 1
    log.info("  GOT %d bytes in %.1f s: %s", len(got), elapsed, got[:64].hex(" "))
    log.info("  as text: %r", got[:64])
    return 0


def send_probe(
    port_path: str, baud: int, payload: bytes, window: float, repeats: int
) -> int:
    """Send bytes to a real device and report whatever it answers.

    Not a loopback test - anything back here came from the far end.
    Window default 1.0 s = 1.25 x a generous 800 ms for an embedded device to
    emit a prompt/banner. On expiry with nothing: the device is silent to this.
    """
    log.info(
        "probe %s @ %d baud: send %r, listen %.1f s, %d time(s)",
        port_path,
        baud,
        payload,
        window,
        repeats,
    )
    try:
        with serial.Serial(port_path, baud, timeout=window, write_timeout=1) as port:
            port.reset_input_buffer()
            port.reset_output_buffer()
            answered = 0
            for i in range(1, repeats + 1):
                t0 = time.monotonic()
                port.write(payload)
                port.flush()
                got = port.read(4096)
                elapsed = time.monotonic() - t0
                if got:
                    answered += 1
                    log.info(
                        "  try %d: %d bytes in %.0f ms", i, len(got), elapsed * 1e3
                    )
                    log.info("    hex : %s", got[:96].hex(" "))
                    log.info("    text: %r", got[:96])
                else:
                    log.info("  try %d: silent (%.0f ms)", i, elapsed * 1e3)
    except (serial.SerialException, OSError) as exc:
        log.error("  open/IO failed: %s", exc)
        return 1

    log.info("RESULT: %d/%d probes answered", answered, repeats)
    return 0 if answered else 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument(
        "--baud",
        type=int,
        action="append",
        help="repeatable; default 9600/115200/921600",
    )
    ap.add_argument(
        "--lines", action="store_true", help="also check RTS/DTR handshake loopback"
    )
    ap.add_argument(
        "--repeat", action="store_true", help="probe continuously until Ctrl-C"
    )
    ap.add_argument(
        "--interval", type=float, default=1.0, help="seconds between repeat rounds"
    )
    ap.add_argument(
        "--rounds",
        type=int,
        default=0,
        help="stop after N repeat rounds (0 = until Ctrl-C)",
    )
    ap.add_argument(
        "--listen",
        type=float,
        metavar="SECONDS",
        help="passive receive only: report anything arriving on RX",
    )
    ap.add_argument(
        "--send",
        nargs="?",
        const="\r\n",
        metavar="TEXT",
        help="send TEXT (default CR LF) to a real device, report its reply",
    )
    ap.add_argument(
        "--window", type=float, default=1.0, help="seconds to wait for a reply"
    )
    ap.add_argument("--tries", type=int, default=3, help="how many times to send")
    args = ap.parse_args()

    logdir = Path("tmp/logs")
    logdir.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)-5s %(message)s",
        handlers=[
            logging.FileHandler(logdir / "serial_loopback_test.log"),
            logging.StreamHandler(sys.stdout),
        ],
    )

    bauds = args.baud or list(BAUDS)
    log.info("port %s", args.port)

    probe_baud = bauds[0] if args.baud else 115200

    if args.send is not None:
        payload = args.send.encode().decode("unicode_escape").encode("latin-1")
        probe_bauds = args.baud or list(CONSOLE_BAUDS)
        answered = [
            b
            for b in probe_bauds
            if send_probe(args.port, b, payload, args.window, args.tries) == 0
        ]
        log.info("SWEEP RESULT: answered at %s", answered or "no baud rate")
        return 0 if answered else 1

    if args.listen is not None:
        return listen(args.port, probe_baud, args.listen)

    if args.repeat:
        return repeat_loop(
            args.port, bauds[0] if args.baud else 115200, args.interval, args.rounds
        )

    results: dict[int, bool] = {}
    for baud in bauds:
        log.info("baud %d", baud)
        try:
            with serial.Serial(args.port, baud, timeout=1, write_timeout=1) as port:
                results[baud] = all(
                    [
                        run_pattern(port, name, payload)
                        for name, payload in PATTERNS.items()
                    ]
                )
                if args.lines and baud == bauds[0]:
                    check_modem_lines(port)
        except serial.SerialException as exc:
            log.error("  open/IO failed: %s", exc)
            results[baud] = False

    ok = [b for b, r in results.items() if r]
    bad = [b for b, r in results.items() if not r]
    if not bad:
        log.info("RESULT: loopback PRESENT - all patterns echoed at %s", ok)
        return 0
    if not ok:
        log.info("RESULT: NO loopback - nothing echoed at any baud")
        return 1
    log.info("RESULT: MARGINAL - ok at %s, failed at %s", ok, bad)
    return 2


if __name__ == "__main__":
    sys.exit(main())
