#!/usr/bin/env python3
"""Live full-screen GPIO monitor - all 48 direct SoC PL061 lines, labeled,
refreshed at 100ms, like `top` (runs on woomera).

Pin labels/active-polarity from docs/gpio-map.md's live-read pinmux table
(2026-08-18) - keep the two in sync. Only lines actually muxed to GPIO mode
are meaningful; NAND/SGPO/UART2/ETH-LED muxed lines are shown but greyed,
since PL061 still reads *something* for them, just not a real GPIO value.

Root only (raw /dev/mem MMIO read). Ctrl-C to exit.
"""

import curses
import mmap
import struct
import sys
import time

GPIO_BANK_BASE = [0xFD887000, 0xFD888000, 0xFD889000, 0xFD88A000, 0xFD88B000, 0xFD897000]
GPIODATA_ALL_OFF = 0x3FC  # PL061 address-mask trick: all 8 mask bits set

# (label, active_low, muxed_to_gpio) - docs/gpio-map.md per-ball table
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

REFRESH_S = 0.1  # 100ms, per request - "like top"


def read32(mem_fd: int, addr: int) -> int:
    page = addr & ~0xFFF
    off = addr & 0xFFF
    with mmap.mmap(mem_fd, 4096, offset=page, prot=mmap.PROT_READ) as m:
        return struct.unpack("<I", m[off : off + 4])[0]


def read_all_banks(mem_fd: int) -> list[int]:
    return [read32(mem_fd, base + GPIODATA_ALL_OFF) & 0xFF for base in GPIO_BANK_BASE]


def render(stdscr, mem_fd: int) -> None:
    curses.curs_set(0)
    stdscr.nodelay(True)
    prev = [0] * 48

    while True:
        banks = read_all_banks(mem_fd)
        bits = [(banks[p // 8] >> (p % 8)) & 1 for p in range(48)]

        stdscr.erase()
        stdscr.addstr(0, 0, "awto-unvr GPIO live monitor - 48 SoC lines, 100ms refresh (q to quit)")
        stdscr.addstr(1, 0, f"{'pin':>3} {'bit':>3}  {'label':<28} state")
        row = 2
        for p in range(48):
            label, active_low, is_gpio = PIN_INFO[p]
            val = bits[p]
            active = (val == 0) if active_low else (val == 1)
            changed = val != prev[p]
            state = "ACTIVE" if active else "-"
            attr = curses.A_BOLD if changed else curses.A_NORMAL
            if not is_gpio:
                attr |= curses.A_DIM
            line = f"{p:>3} {val:>3}  {label:<28} {state}"
            try:
                stdscr.addstr(row, 0, line, attr)
            except curses.error:
                pass  # terminal too small for all 48 rows, skip overflow
            row += 1

        stdscr.refresh()
        prev = bits

        try:
            if stdscr.getkey() == "q":
                return
        except curses.error:
            pass
        time.sleep(REFRESH_S)


def main() -> int:
    try:
        import os

        mem_fd = os.open("/dev/mem", os.O_RDONLY)
    except (PermissionError, OSError) as e:
        print(f"/dev/mem open failed ({e}) -- run as root", file=sys.stderr)
        return 2

    curses.wrapper(render, mem_fd)
    return 0


if __name__ == "__main__":
    sys.exit(main())
