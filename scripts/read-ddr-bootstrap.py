#!/usr/bin/env python3
"""Read the Alpine V2 PBS bootstrap strap register directly via /dev/mem (runs on woomera).

Decodes the running clock tree, esp. ddr_pll_freq -- the one live-open field
in docs/ddr-config-reverse.md §7 (EEPROM/SPD only bounds DDR to <=1866 MT/s;
this reads the actual running strap). Same decode as the U-Boot `bootstrap`
command (uboot-port/board/annapurna/alpine/alpine.c) -- both port
al_bootstrap_parse()/al_hal_bootstrap.c (delroth-alpine_hal), Alpine V2
branches only (this board is fixed AL-324/Alpine V2). Keep the two in sync.

PBS regfile base 0xfd8a8000 (AL_SB_BASE 0xfc000000 + AL_SB_PBS_OFF 0x1880000
+ AL_PBS_REGFILE_OFF 0x28000); boot_strap @ +0x110 (al_hal_pbs_regs.h).
Field layout: al_hal_bootstrap_map.h. Value tables: al_hal_bootstrap.c
al_bootstrap_{cpu,ddr}_pll_freq_get() / al_bootstrap_sb_pll_and_clk_freq_get().
Root only (raw /dev/mem MMIO read).
"""

import mmap
import struct
import sys

PBS_BASE = 0xFD8A8000
BOOT_STRAP_OFF = 0x110

# al_hal_bootstrap_map.h field layout: (shift, mask)
F_CPU_PLL = (0, 0xF)
F_DDR_PLL = (4, 0x7)
F_SB_PLL = (7, 0x3)
F_SB_CLK = (9, 0x3)
F_BOOT_DEV = (15, 0x7)
F_DEBUG_MODE = (18, 0x1)
F_REF_CLK = (19, 0x1)
F_CPU_EXIST = (20, 0x3)


def field(reg: int, spec: tuple[int, int]) -> int:
    shift, mask = spec
    return (reg >> shift) & mask


# al_bootstrap_cpu_pll_freq_get(), dev_id<=ALPINE_V2 table (index = CPU_PLL field)
CPU_PLL_TBL = [
    0, 1_000_000_000, 1_400_000_000, 1_500_000_000, 1_600_000_000,
    1_700_000_000, 1_800_000_000, 1_900_000_000, 2_100_000_000, 2_200_000_000,
    2_300_000_000, 2_400_000_000, 2_500_000_000, 2_600_000_000, 2_700_000_000,
    2_000_000_000,
]

# al_bootstrap_ddr_pll_freq_get(), Alpine V2 arm (index = NB/DDR_PLL field)
DDR_PLL_TBL = [
    0, 1_066_666_666, 666_666_666, 1_300_000_000,
    933_333_333, 1_050_000_000, 1_200_000_000, 800_000_000,
]

SB_CLK_TBL = [250_000_000, 375_000_000, 428_000_000, 500_000_000]

BOOT_DEV_NAMES = [
    "UART-CLI", "UART(2000000bps)", "NAND", "reserved",
    "UART(115200bps)", "SPI(M3)", "UART(1000000bps)", "SPI(M0)",
]


def decode(reg: int) -> dict:
    ref_clk = 100_000_000 if field(reg, F_REF_CLK) else 25_000_000

    cpu_pll = CPU_PLL_TBL[field(reg, F_CPU_PLL)] or ref_clk
    ddr_pll = DDR_PLL_TBL[field(reg, F_DDR_PLL)] or ref_clk

    sb_field = field(reg, F_SB_PLL)
    if sb_field == 0:
        sb_pll = ref_clk
        sb_clk = ref_clk  # bypass: clk follows pll
    else:
        sb_pll = 3_000_000_000 if sb_field == 1 else 1_500_000_000
        sb_clk = SB_CLK_TBL[field(reg, F_SB_CLK)]

    return {
        "pll_ref_clk_freq": ref_clk,
        "cpu_pll_freq": cpu_pll,
        "ddr_pll_freq": ddr_pll,
        "sb_pll_freq": sb_pll,
        "sb_clk_freq": sb_clk,
        "boot_device": BOOT_DEV_NAMES[field(reg, F_BOOT_DEV)],
        "debug_mode": "disabled" if field(reg, F_DEBUG_MODE) else "enabled",
        "cpu_exist_field": field(reg, F_CPU_EXIST),
    }


def main() -> int:
    try:
        with open("/dev/mem", "rb", 0) as f:
            m = mmap.mmap(f.fileno(), 4096, offset=PBS_BASE, prot=mmap.PROT_READ)
            reg = struct.unpack("<I", m[BOOT_STRAP_OFF : BOOT_STRAP_OFF + 4])[0]
    except (PermissionError, OSError) as e:
        print(f"/dev/mem read failed ({e}) -- run as root", file=sys.stderr)
        return 2

    d = decode(reg)
    print(f"boot_strap reg = 0x{reg:08x}  (PBS 0x{PBS_BASE:08x} + 0x{BOOT_STRAP_OFF:03x})")
    for name in ("pll_ref_clk_freq", "cpu_pll_freq", "ddr_pll_freq", "sb_pll_freq", "sb_clk_freq"):
        hz = d[name]
        print(f"  {name:<18} {hz:>12d} Hz  ({hz / 1e6:.1f} MHz)")
    print(f"  ddr data rate      {d['ddr_pll_freq'] * 2 / 1e6:.0f} MT/s  (DDR = 2x clock)")
    print(f"  boot_device        {d['boot_device']}")
    print(f"  debug_mode         {d['debug_mode']}")
    print(f"  cpu_exist field    {d['cpu_exist_field']}  (0=1 core, 1=2, 3=4)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
