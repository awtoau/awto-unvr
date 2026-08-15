#!/usr/bin/env python3
"""UNVR serial console: watch, log, and catch the U-Boot prompt.

WIRING - check before connecting:
  UNVR: 4-pin header on the PCB behind the SFP+ cage, middle of the board.
  Use THREE pins only: GND, TXD, RXD. **Do NOT connect the 3V3 pin** - the board
  supplies its own rail and back-feeding it can damage something.
  Adapter TX -> UNVR RX, adapter RX -> UNVR TX, GND -> GND. 3.3 V TTL.
  115200 8N1 (from U-Boot's own `loadbootargs`: console=ttyS0,115200).

Catching U-Boot: the autoboot window is ~2 s and easy to miss by hand. `--catch`
streams ESC continuously, so start it BEFORE powering on and the device lands at
the U-Boot prompt on its own.

Everything is timestamped to tmp/logs/unvr-console.log regardless of mode.

  ./scripts/unvr-console.py                     # watch + log
  ./scripts/unvr-console.py --catch             # spam ESC to stop autoboot
  ./scripts/unvr-console.py --until "=>"        # exit once a pattern appears
  ./scripts/unvr-console.py --interactive       # type to the device (Ctrl-] quits)
"""

import argparse
import sys
import termios
import tty
from datetime import datetime, timezone
from pathlib import Path

import serial

REPO = Path(__file__).resolve().parent.parent
LOGS = REPO / "tmp" / "logs"

# Serial read poll interval. Not a deadline - it is how often the loop wakes to
# check for input so ESC injection and pattern matching stay responsive.
# 50 ms is well under the ~2 s autoboot window we must react inside.
POLL = 0.05

# ESC injection cadence while --catch is active. U-Boot polls its console during
# the autoboot countdown; 20 ESC/s guarantees several land inside a 2 s window.
ESC_INTERVAL = 0.05

UBOOT_HINTS = (b"Hit any key", b"autoboot", b"=>", b"Marvell>>", b"BootROM", b"U-Boot")


def stamp():
    return datetime.now(timezone.utc).astimezone().isoformat(timespec="milliseconds")


# Bauds worth trying, likeliest first. 115200 is what U-Boot's own loadbootargs
# sets (console=ttyS0,115200); the rest are common vendor alternatives.
SWEEP_BAUDS = [115200, 57600, 38400, 9600, 19200, 230400, 460800, 921600]


def printable_ratio(b):
    if not b:
        return 0.0
    ok = sum(1 for c in b if 32 <= c < 127 or c in (9, 10, 13))
    return ok / len(b)


def sweep(port):
    """Probe each baud with a CR and score the reply. A serial line at the wrong
    rate returns high-entropy bytes; at the right rate it returns mostly ASCII.

    Listen window is 0.4 s per baud: a device echoes or prompts within a few ms,
    so this is ~100x the expected response time and the whole sweep takes ~4 s.
    On no reply at any baud, the line is silent - a wiring or power problem, not
    a baud problem."""
    import time
    print(f"# sweeping {port} - sending CR at each baud, scoring the reply\n", flush=True)
    results = []
    for baud in SWEEP_BAUDS:
        try:
            s = serial.Serial(port, baud, bytesize=8, parity="N", stopbits=1, timeout=0.1)
        except serial.SerialException as e:
            print(f"  {baud:>7}  cannot open: {e}", flush=True)
            continue
        s.reset_input_buffer()
        s.write(b"\r")
        s.flush()
        got = b""
        end = time.monotonic() + 0.4
        while time.monotonic() < end:
            got += s.read(4096)
        s.close()
        r = printable_ratio(got)
        results.append((r, baud, got))
        show = got[:60].decode("ascii", "replace").replace("\n", "\\n").replace("\r", "\\r")
        print(f"  {baud:>7}  {len(got):>5} B  printable {r:5.0%}  {show!r}", flush=True)

    if not any(len(g) for _, _, g in results):
        print("\n# NOTHING received at any baud.", flush=True)
        print("# That is a wiring/power issue, not a baud issue:", flush=True)
        print("#   - is the adapter's RX on the device's TX? (try swapping)", flush=True)
        print("#   - is GND connected?", flush=True)
        print("#   - is the device powered?", flush=True)
        return None
    best = max(results, key=lambda x: (x[0], len(x[2])))
    print(f"\n# best: {best[1]} baud, {best[0]:.0%} printable", flush=True)
    if best[0] < 0.7:
        print("# still mostly non-printable - could be a floating RX line picking up noise,", flush=True)
        print("# or a level mismatch (5 V adapter on a 3.3 V UART).", flush=True)
    return best[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--catch", action="store_true", help="stream ESC to interrupt autoboot")
    ap.add_argument("--until", default=None, help="exit once this text appears")
    ap.add_argument("--interactive", action="store_true", help="forward your keystrokes (Ctrl-] to quit)")
    ap.add_argument("--sweep", action="store_true", help="probe common bauds and report which gives ASCII")
    ap.add_argument("--hex", action="store_true", help="show raw hex alongside text")
    ap.add_argument("--send", action="append", default=[],
                    help="send this line (CR appended) after opening; repeatable")
    a = ap.parse_args()

    if a.sweep:
        sweep(a.port)
        return 0

    LOGS.mkdir(parents=True, exist_ok=True)
    logfile = LOGS / "unvr-console.log"

    try:
        ser = serial.Serial(a.port, a.baud, bytesize=8, parity="N", stopbits=1, timeout=POLL)
    except serial.SerialException as e:
        sys.exit(f"cannot open {a.port}: {e}\nIs the adapter plugged in? Are you in the dialout group?")

    print(f"# {a.port} @ {a.baud} 8N1 - logging to {logfile}", flush=True)
    if a.catch:
        print("# --catch: streaming ESC. Power on the UNVR NOW.", flush=True)
    if a.interactive:
        print("# --interactive: your keystrokes go to the device. Ctrl-] to quit.", flush=True)
    print("# Ctrl-C to stop.\n", flush=True)

    old = None
    stdin_fd = sys.stdin.fileno()
    if a.interactive and sys.stdin.isatty():
        old = termios.tcgetattr(stdin_fd)
        tty.setraw(stdin_fd)

    import select
    import time
    buf = b""
    last_esc = 0.0
    hinted = False

    try:
        with logfile.open("ab") as lf:
            lf.write(f"\n=== {stamp()} open {a.port} @ {a.baud} ===\n".encode())
            lf.flush()
            for line in a.send:
                ser.write(line.encode() + b"\r")
                ser.flush()
                lf.write(f"[sent] {line!r}\n".encode())
            while True:
                data = ser.read(4096)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
                    lf.write(data)
                    lf.flush()
                    buf = (buf + data)[-8192:]

                    if not hinted and any(h in buf for h in UBOOT_HINTS):
                        hinted = True
                        sys.stderr.write(f"\n[{stamp()}] U-Boot detected\n")
                        sys.stderr.flush()

                    if a.until and a.until.encode() in buf:
                        sys.stderr.write(f"\n[{stamp()}] matched {a.until!r} - exiting\n")
                        return 0

                if a.catch and time.monotonic() - last_esc >= ESC_INTERVAL:
                    ser.write(b"\x1b")
                    last_esc = time.monotonic()

                if a.interactive and select.select([sys.stdin], [], [], 0)[0]:
                    ch = sys.stdin.buffer.read(1)
                    if ch == b"\x1d":  # Ctrl-]
                        sys.stderr.write("\n[quit]\n")
                        return 0
                    ser.write(ch)
    except KeyboardInterrupt:
        sys.stderr.write("\n[interrupted]\n")
        return 0
    finally:
        if old is not None:
            termios.tcsetattr(stdin_fd, termios.TCSADRAIN, old)
        ser.close()


if __name__ == "__main__":
    sys.exit(main())
