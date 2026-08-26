#!/usr/bin/env python3
"""Live full-screen GPIO monitor - table-by-port layout, all 48 direct SoC
PL061 lines plus the I2C-expander/SGPO GPIO this board also has, labeled,
like `top` (runs on woomera).

Two refresh rates: the 48 SoC lines read via /dev/mem every 100ms (fast,
per request). The I2C-attached devices (2x PCA9575 expander, SGPO shift
register) read via `gpioget` (their own bound kernel drivers - NOT a raw
i2c bus scan, which has repeatedly wedged the RTC's bus tonight; direct
reads to a known, already-bound device are safe, see docs/rtc-s35390a-
fault.md) on a slower ~1s cadence, since each gpioget call is a real i2c
transaction with real latency - polling those at 100ms would both be
unnecessary and risk saturating the bus.

Pin labels from docs/gpio-map.md (48 SoC lines) and docs/gpio-switches-
leds.md (PCA9575/SGPO) - keep in sync. gpiochip numbers confirmed live via
`gpiodetect` (2026-08-27): chip0=[0-0020] PCA9575@0x20, chip1=[0-0021]
PCA9575@0x21, chip2-7=the 6 PL061 banks, chip8=[fd8b4000.sgpo] (32 lines).
No chip for @0x29 (Pro bays 5-8) - confirms it's unpopulated on this SKU.

Root only (raw /dev/mem MMIO read + gpioget). Ctrl-C or q to quit.

--headless: same poll+log loop, no curses display - curses needs a real
controlling TTY, which a backgrounded/nohup'd process doesn't have. Use
this to leave the box monitoring GPIO changes unattended, e.g.:
    setsid nohup python3 gpio-top.py --headless < /dev/null > /dev/null 2>&1 &

--once: prints the same table as the curses UI, once, plain text, no
cursor-positioning/clear-screen codes - readable when captured over a
scripted request/response channel (e.g. a serial console driven by
console-send) rather than a real interactive TTY.
"""

import curses
import mmap
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

GPIO_BANK_BASE = [0xFD887000, 0xFD888000, 0xFD889000, 0xFD88A000, 0xFD88B000, 0xFD897000]
GPIODATA_ALL_OFF = 0x3FC  # PL061 address-mask trick: all 8 mask bits set

# docs/gpio-map.md per-ball table: pin -> (label, active_low, muxed_to_gpio)
PIN_INFO = {
    0: ("SFP 25G speed LED", True, True),
    1: ("spare (pull-H)", False, True),
    2: ("spare (pull-H)", False, True),
    3: ("spare (out-low)", False, True),
    4: ("spare (pull-H)", False, True),
    5: ("spare (pull-H)", False, True),
    6: ("NAND flash data", False, False),
    7: ("NAND flash data", False, False),
    **{p: ("NAND flash data bus", False, False) for p in range(8, 16)},
    16: ("NAND flash", False, False),
    17: ("NAND flash", False, False),
    18: ("NAND flash", False, False),
    19: ("NAND flash CS/WP", False, False),
    20: ("spare (pull-H)", False, True),
    21: ("SGPO (bay-activity LED)", False, False),
    22: ("spare", False, True),
    23: ("spare", False, True),
    24: ("SGPO", False, False),
    25: ("SGPO", False, False),
    26: ("UART2 RX (RPS RS-232)", False, False),
    27: ("UART2 TX (RPS RS-232)", False, False),
    28: ("spare (pull-H)", False, True),
    29: ("spare (pull-H)", False, True),
    30: ("ETH LED", False, False),
    31: ("ulogo_blue LED", True, True),
    32: ("spare (pull-H)", False, True),
    33: ("rps_prnt (RPS present)", False, True),
    34: ("12v_lp (RPS 12V sense)", False, True),
    35: ("spare (pull-H)", False, True),
    36: ("spare (pull-H)", False, True),
    37: ("ulogo_white LED", True, True),
    38: ("reset button", True, True),
    39: ("spare (pull-H)", False, True),
    40: ("spare (H)", False, True),
    41: ("spare (H)", False, True),
    42: ("hdd force-power-on-wa", False, True),
    43: ("spare (H)", False, True),
    44: ("spare (L)", False, True),
    45: ("spare (L)", False, True),
    46: ("spare (L)", False, True),
    47: ("spare (L)", False, True),
}

# docs/gpio-switches-leds.md - I2C-expander/SGPO GPIO, line -> (label, active_low)
PCA9575_0X20_LINES = {  # gpiochip0, 16 lines
    2: ("SFP+ 1G link LED (sfp_1g)", False),
    3: ("input (strap/status)", False),
    7: ("input (strap/status)", False),
    9: ("input (strap/status)", False),
    10: ("input (strap/status)", False),
    12: ("board strap/status/present", False),
    14: ("board strap/status/present", False),
    15: ("board strap/status/present", False),
}
PCA9575_0X21_LINES = {  # gpiochip1, 16 lines
    **{i: (f"bay{i+1} pwr-enable", False) for i in range(0, 4)},
    **{i: (f"bay{i-3} present", True) for i in range(4, 8)},
    **{i: (f"bay{i-11} fault LED", False) for i in range(12, 16)},
}

REFRESH_S = 0.1  # 100ms for the fast SoC-GPIO section, per request
SLOW_REFRESH_TICKS = 10  # I2C expanders/SGPO: every 10th fast tick (~1s)


def read32(mem_fd: int, addr: int) -> int:
    page = addr & ~0xFFF
    off = addr & 0xFFF
    with mmap.mmap(mem_fd, 4096, offset=page, prot=mmap.PROT_READ) as m:
        return struct.unpack("<I", m[off : off + 4])[0]


def read_soc_banks(mem_fd: int) -> list[int]:
    return [read32(mem_fd, base + GPIODATA_ALL_OFF) & 0xFF for base in GPIO_BANK_BASE]


def gpioget_chip(chip: str, n_lines: int) -> list[int] | None:
    """One `gpioget` call, all lines at once - a real i2c/register read via
    the bound kernel driver, not a bus scan. Returns None on any failure
    (chip busy/absent) so the caller can show stale/placeholder data rather
    than crash the whole display.

    libgpiod v2.x CLI (confirmed live, 2026-08-27): needs `--chip` (chip
    name is NOT a positional arg like v1.x), and prints `"N"=active` /
    `"N"=inactive` per line, not raw 0/1 - e.g.
    `"0"=active "1"=active "2"=inactive`. Parsed accordingly.
    """
    try:
        out = subprocess.run(
            ["gpioget", "--chip", chip, *[str(i) for i in range(n_lines)]],
            capture_output=True, text=True, timeout=2,
        )
        if out.returncode != 0:
            return None
        return [1 if tok.endswith("=active") else 0 for tok in out.stdout.split()]
    except (OSError, subprocess.TimeoutExpired, ValueError):
        return None


def log_changes(log_f, source: str, prev: list[int] | None, cur: list[int],
                 labels: dict, changed_at: dict | None = None) -> None:
    """Append one line per changed GPIO to the change log - only on actual
    transitions, not every poll, so the log stays meaningful over a long
    run rather than growing at the full 100ms/1s poll rate. If given,
    also stamps changed_at[(source, i)] = monotonic() for the curses UI's
    recency colouring - shared so the log and the display never disagree
    about when something last changed."""
    if prev is None:
        return
    for i, (old, new) in enumerate(zip(prev, cur)):
        if old == new:
            continue
        if changed_at is not None:
            changed_at[(source, i)] = time.monotonic()
        if log_f is None:
            continue
        label = labels.get(i, ("",))[0] if isinstance(labels.get(i), tuple) else labels.get(i, "")
        ts = time.strftime("%Y-%m-%dT%H:%M:%S%z")
        log_f.write(f"{ts} {source} line {i:<2} {label:<28} {old} -> {new}\n")
    if log_f is not None:
        log_f.flush()


# Recency colour thresholds (seconds since last change) -> curses color
# pair index, per request: <1s red, <5s orange(-ish), <10s yellow.
RECENCY_TIERS = [(1.0, 1), (5.0, 2), (10.0, 3)]


def recency_attr(changed_at: dict, key: tuple, colors_ok: bool = True) -> int:
    if not colors_ok:
        return curses.A_NORMAL
    ts = changed_at.get(key)
    if ts is None:
        return curses.A_NORMAL
    age = time.monotonic() - ts
    for max_age, pair in RECENCY_TIERS:
        if age < max_age:
            return curses.color_pair(pair) | curses.A_BOLD
    return curses.A_NORMAL


def new_poll_state() -> dict:
    return {
        "tick": 0,
        "pca20": None, "pca21": None, "sgpo": None,
        "prev_bits": None, "prev_pca20": None, "prev_pca21": None, "prev_sgpo": None,
        "changed_at": {},  # (source, line_index) -> monotonic() of last change
    }


def poll_tick(mem_fd: int, log_f, state: dict) -> list[int]:
    """One poll cycle: always reads the 48 fast SoC lines, reads the slow
    I2C-expander/SGPO chips only every SLOW_REFRESH_TICKS-th call, logs any
    real changes either way (and stamps state["changed_at"] for the
    curses UI's recency colouring). Shared between the curses UI and
    --headless mode so the two never drift apart. Returns the current
    `bits` list."""
    ca = state["changed_at"]
    banks = read_soc_banks(mem_fd)
    bits = [(banks[p // 8] >> (p % 8)) & 1 for p in range(48)]

    log_changes(log_f, "SoC", state["prev_bits"], bits, PIN_INFO, ca)
    state["prev_bits"] = bits

    if state["tick"] % SLOW_REFRESH_TICKS == 0:
        new_pca20 = gpioget_chip("gpiochip0", 16)
        if new_pca20:
            log_changes(log_f, "PCA9575@0x20", state["prev_pca20"], new_pca20, PCA9575_0X20_LINES, ca)
            state["prev_pca20"] = state["pca20"] = new_pca20
        new_pca21 = gpioget_chip("gpiochip1", 16)
        if new_pca21:
            log_changes(log_f, "PCA9575@0x21", state["prev_pca21"], new_pca21, PCA9575_0X21_LINES, ca)
            state["prev_pca21"] = state["pca21"] = new_pca21
        new_sgpo = gpioget_chip("gpiochip8", 32)
        if new_sgpo:
            log_changes(log_f, "SGPO", state["prev_sgpo"], new_sgpo, {}, ca)
            state["prev_sgpo"] = state["sgpo"] = new_sgpo
    state["tick"] += 1
    return bits


def run_once(mem_fd: int, log_f) -> None:
    """Print the same table as the curses UI once, plain text, no cursor
    positioning/clear-screen codes - so it's readable when captured over a
    scripted request/response channel (e.g. a serial console driven by
    console-send) rather than a real interactive TTY, where curses output
    comes back as scrambled escape sequences."""
    state = new_poll_state()
    bits = poll_tick(mem_fd, log_f, state)
    pca20, pca21, sgpo = state["pca20"], state["pca21"], state["sgpo"]

    print("SoC PL061 banks (port = bank number, row = bit):")
    header = "bit  " + "".join(f"port{p:<7}" for p in range(6))
    print(header)
    for bit in range(8):
        cells = []
        for bank in range(6):
            pin = bank * 8 + bit
            v = bits[pin]
            active = (v == 0) if PIN_INFO[pin][1] else (v == 1)
            cells.append(f"{pin:>2}={v}{'*' if active else ' '}")
        print(f"{bit:<5}" + "".join(f"{c:<11}" for c in cells))
    print("* = active per docs/gpio-map.md polarity. Labels below for named pins:")
    for pin in sorted(PIN_INFO):
        label, active_low, is_gpio = PIN_INFO[pin]
        if "spare" in label or "NAND" in label:
            continue
        v = bits[pin]
        active = (v == 0) if active_low else (v == 1)
        print(f"  pin {pin:<2} {label:<28} {'ACTIVE' if active else '-'}")

    for title, chip_vals, labels, ncols in (
        ("PCA9575 @0x20 (gpiochip0, 16 lines)", pca20, PCA9575_0X20_LINES, 4),
        ("PCA9575 @0x21 (gpiochip1, 16 lines, HDD bay control)", pca21, PCA9575_0X21_LINES, 4),
        ("SGPO (gpiochip8, 32 lines, bay-activity shift-reg)", sgpo, {}, 8),
    ):
        print(title + (":" if chip_vals else " - unavailable"))
        if not chip_vals:
            continue
        for start in range(0, len(chip_vals), ncols):
            print("".join(f"{i:>2}={chip_vals[i]}  " for i in range(start, min(start + ncols, len(chip_vals)))))
        for i, (label, active_low) in sorted(labels.items()):
            if i >= len(chip_vals):
                continue
            v = chip_vals[i]
            active = (v == 0) if active_low else (v == 1)
            print(f"  line {i:<2} {label:<28} {'ACTIVE' if active else '-'}")


def run_headless(mem_fd: int, log_f) -> None:
    """No curses, no display - just the poll+log loop, safe to run as a
    detached background daemon (curses needs a real controlling TTY, which
    a backgrounded/nohup'd process doesn't have - this mode exists so the
    box can be left monitoring GPIO changes unattended)."""
    state = new_poll_state()
    while True:
        poll_tick(mem_fd, log_f, state)
        time.sleep(REFRESH_S)


def init_colors() -> bool:
    """Returns False (and leaves recency_attr() a no-op) if this terminal
    doesn't support colour at all - some serial-console terminfo entries
    lack even the default-colour extension, or (confirmed live on this
    box's console) report curses.COLORS as -1 without start_color()
    itself raising - checked explicitly since that's not an exception.
    Not fatal either way."""
    try:
        curses.start_color()
    except curses.error:
        return False
    if curses.COLORS < 8:
        return False
    try:
        curses.use_default_colors()
        bg = -1
    except curses.error:
        bg = curses.COLOR_BLACK  # fall back to an explicit background
    try:
        curses.init_pair(1, curses.COLOR_RED, bg)      # <1s
        orange = 208 if curses.COLORS >= 256 else curses.COLOR_YELLOW  # <5s
        curses.init_pair(2, orange, bg)                # true orange on 256-color
        curses.init_pair(3, curses.COLOR_YELLOW, bg)   # <10s
        return True
    except curses.error:
        return False


def render(stdscr, mem_fd: int, log_f) -> None:
    curses.curs_set(0)
    stdscr.nodelay(True)
    colors_ok = init_colors()
    state = new_poll_state()

    while True:
        bits = poll_tick(mem_fd, log_f, state)
        pca20, pca21, sgpo = state["pca20"], state["pca21"], state["sgpo"]
        ca = state["changed_at"]

        stdscr.erase()
        row = 0
        stdscr.addstr(row, 0, "awto-unvr GPIO live monitor (q to quit) - "
                               "SoC lines 100ms, I2C expanders ~1s. "
                               "red<1s orange<5s yellow<10s since last change")
        row += 2

        # --- 48 SoC lines: table by port, columns = the 6 banks ---
        stdscr.addstr(row, 0, "SoC PL061 banks (port = bank number, row = bit):")
        row += 1
        header = "bit  " + "".join(f"port{p:<7}" for p in range(6))
        stdscr.addstr(row, 0, header)
        row += 1
        for bit in range(8):
            try:
                stdscr.addstr(row, 0, f"{bit:<5}")
            except curses.error:
                pass
            for bank in range(6):
                pin = bank * 8 + bit
                v = bits[pin]
                active = (v == 0) if PIN_INFO[pin][1] else (v == 1)
                cell = f"{pin:>2}={v}{'*' if active else ' '}"
                col = 5 + bank * 11
                try:
                    stdscr.addstr(row, col, f"{cell:<11}", recency_attr(ca, ("SoC", pin), colors_ok))
                except curses.error:
                    pass
            row += 1
        row += 1
        stdscr.addstr(row, 0, "* = active per docs/gpio-map.md polarity. Labels below for named pins:")
        row += 1
        for pin in sorted(PIN_INFO):
            label, active_low, is_gpio = PIN_INFO[pin]
            if "spare" in label or "NAND" in label:
                continue
            v = bits[pin]
            active = (v == 0) if active_low else (v == 1)
            try:
                stdscr.addstr(row, 0, f"  pin {pin:<2} {label:<28} {'ACTIVE' if active else '-'}",
                              recency_attr(ca, ("SoC", pin), colors_ok))
            except curses.error:
                pass
            row += 1
        row += 1

        # --- I2C-expander / SGPO GPIO, same table-by-port idea ---
        for source, title, chip_vals, labels, ncols in (
            ("PCA9575@0x20", "PCA9575 @0x20 (gpiochip0, 16 lines)", pca20, PCA9575_0X20_LINES, 4),
            ("PCA9575@0x21", "PCA9575 @0x21 (gpiochip1, 16 lines, HDD bay control)", pca21, PCA9575_0X21_LINES, 4),
            ("SGPO", "SGPO (gpiochip8, 32 lines, bay-activity shift-reg)", sgpo, {}, 8),
        ):
            try:
                stdscr.addstr(row, 0, title + (":" if chip_vals else " - unavailable"))
            except curses.error:
                pass
            row += 1
            if not chip_vals:
                continue
            for start in range(0, len(chip_vals), ncols):
                col = 0
                for i in range(start, min(start + ncols, len(chip_vals))):
                    cell = f"{i:>2}={chip_vals[i]}  "
                    try:
                        stdscr.addstr(row, col, cell, recency_attr(ca, (source, i), colors_ok))
                    except curses.error:
                        pass
                    col += len(cell)
                row += 1
            for i, (label, active_low) in sorted(labels.items()):
                if i >= len(chip_vals):
                    continue
                v = chip_vals[i]
                active = (v == 0) if active_low else (v == 1)
                try:
                    stdscr.addstr(row, 0, f"  line {i:<2} {label:<28} {'ACTIVE' if active else '-'}",
                                  recency_attr(ca, (source, i), colors_ok))
                except curses.error:
                    pass
                row += 1
            row += 1

        stdscr.refresh()

        try:
            if stdscr.getkey() == "q":
                return
        except curses.error:
            pass
        time.sleep(REFRESH_S)


def main() -> int:
    headless = "--headless" in sys.argv
    once = "--once" in sys.argv

    # This box's serial-getty defaults TERM=vt220 (confirmed live) - a real
    # VT220 never had color, so its terminfo entry has none and curses.COLORS
    # comes back -1. ANSI colour codes work fine over any serial link; it's
    # purely a terminfo-selection issue. Force a colour-capable entry unless
    # the caller already asked for something specific (e.g. testing a
    # different terminal type intentionally).
    if os.environ.get("TERM") in (None, "", "vt220"):
        os.environ["TERM"] = "xterm-256color"

    try:
        mem_fd = os.open("/dev/mem", os.O_RDONLY)
    except (PermissionError, OSError) as e:
        print(f"/dev/mem open failed ({e}) -- run as root", file=sys.stderr)
        return 2

    log_dir = Path(__file__).resolve().parent.parent / "tmp" / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / f"gpio-top-changes-{time.strftime('%Y%m%d-%H%M%S')}.log"
    print(f"logging GPIO changes to {log_path}")
    with open(log_path, "a") as log_f:
        if once:
            run_once(mem_fd, log_f)
        elif headless:
            run_headless(mem_fd, log_f)
        else:
            curses.wrapper(render, mem_fd, log_f)
    return 0


if __name__ == "__main__":
    sys.exit(main())
