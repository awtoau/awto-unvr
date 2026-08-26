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
"""

import curses
import mmap
import os
import struct
import subprocess
import sys
import time

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


def render(stdscr, mem_fd: int) -> None:
    curses.curs_set(0)
    stdscr.nodelay(True)
    tick = 0
    pca20 = pca21 = sgpo = None

    while True:
        banks = read_soc_banks(mem_fd)
        bits = [(banks[p // 8] >> (p % 8)) & 1 for p in range(48)]

        if tick % SLOW_REFRESH_TICKS == 0:
            pca20 = gpioget_chip("gpiochip0", 16) or pca20
            pca21 = gpioget_chip("gpiochip1", 16) or pca21
            sgpo = gpioget_chip("gpiochip8", 32) or sgpo
        tick += 1

        stdscr.erase()
        row = 0
        stdscr.addstr(row, 0, "awto-unvr GPIO live monitor (q to quit) - "
                               "SoC lines 100ms, I2C expanders ~1s")
        row += 2

        # --- 48 SoC lines: table by port, columns = the 6 banks ---
        stdscr.addstr(row, 0, "SoC PL061 banks (port = bank number, row = bit):")
        row += 1
        header = "bit  " + "".join(f"port{p:<7}" for p in range(6))
        stdscr.addstr(row, 0, header)
        row += 1
        for bit in range(8):
            line = f"{bit:<5}"
            for bank in range(6):
                pin = bank * 8 + bit
                v = bits[pin]
                label, active_low, is_gpio = PIN_INFO[pin]
                active = (v == 0) if active_low else (v == 1)
                cell = f"{pin:>2}={v}{'*' if active else ' '}"
                line += f"{cell:<11}"
            try:
                stdscr.addstr(row, 0, line)
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
                stdscr.addstr(row, 0, f"  pin {pin:<2} {label:<28} {'ACTIVE' if active else '-'}")
            except curses.error:
                pass
            row += 1
        row += 1

        # --- I2C-expander / SGPO GPIO, same table-by-port idea ---
        for title, chip_vals, labels, ncols in (
            ("PCA9575 @0x20 (gpiochip0, 16 lines)", pca20, PCA9575_0X20_LINES, 4),
            ("PCA9575 @0x21 (gpiochip1, 16 lines, HDD bay control)", pca21, PCA9575_0X21_LINES, 4),
            ("SGPO (gpiochip8, 32 lines, bay-activity shift-reg)", sgpo, {}, 8),
        ):
            try:
                stdscr.addstr(row, 0, title + (":" if chip_vals else " - unavailable"))
            except curses.error:
                pass
            row += 1
            if not chip_vals:
                continue
            for start in range(0, len(chip_vals), ncols):
                line = ""
                for i in range(start, min(start + ncols, len(chip_vals))):
                    line += f"{i:>2}={chip_vals[i]}  "
                try:
                    stdscr.addstr(row, 0, line)
                except curses.error:
                    pass
                row += 1
            for i, (label, active_low) in sorted(labels.items()):
                if i >= len(chip_vals):
                    continue
                v = chip_vals[i]
                active = (v == 0) if active_low else (v == 1)
                try:
                    stdscr.addstr(row, 0, f"  line {i:<2} {label:<28} {'ACTIVE' if active else '-'}")
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
    try:
        mem_fd = os.open("/dev/mem", os.O_RDONLY)
    except (PermissionError, OSError) as e:
        print(f"/dev/mem open failed ({e}) -- run as root", file=sys.stderr)
        return 2

    curses.wrapper(render, mem_fd)
    return 0


if __name__ == "__main__":
    sys.exit(main())
