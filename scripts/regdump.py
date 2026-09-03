#!/usr/bin/env python3
"""Read the #140 PCIe ext0 diagnostic registers + all 6 GPIO banks directly via
/dev/mem, from a running Linux userspace (runs on woomera).

Userspace twin of `al_diag_dump()` (uboot-port/board/annapurna/alpine/alpine.c,
also the `aldiag` U-Boot command) -- same register list, same addresses, so a
dump can be taken without a reboot into U-Boot. Keep the two in sync.

Root only (raw /dev/mem MMIO read).
"""

import logging
import mmap
import struct
import sys
from pathlib import Path

LOG_DIR = Path(__file__).resolve().parent.parent / "tmp" / "logs"
LOG_DIR.mkdir(parents=True, exist_ok=True)
LOG_PATH = LOG_DIR / "regdump.log"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)-5s %(message)s",
    handlers=[logging.FileHandler(LOG_PATH), logging.StreamHandler(sys.stdout)],
)
log = logging.getLogger(__name__)

# al_diag_dump()'s register list, alpine.c - keep addresses in sync
PCIE_EXT0_BASE = 0xFD800000
PCIE_REGS = [
    ("DEV_REV_ID", PCIE_EXT0_BASE + 0x16C),
    ("GLOBAL_CONF", PCIE_EXT0_BASE + 0xC8),
    ("PF_SEL", PCIE_EXT0_BASE + 0x1B0),
    ("MASTER_ARCTL", PCIE_EXT0_BASE + 0x14),
    ("MASTER_AWCTL", PCIE_EXT0_BASE + 0x18),
    ("PBS_PCIE_CONF_1", 0xFD8A80E4),
    ("CFG_TARGET_BUS", PCIE_EXT0_BASE + 0x30),
    ("PORT_INIT", PCIE_EXT0_BASE + 0x2000),
    ("DEBUG_INFO_0", PCIE_EXT0_BASE + 0x2080),
    ("CFGHDR_VID_DID", PCIE_EXT0_BASE + 0x10000),
    ("CFGHDR_CMD_STAT", PCIE_EXT0_BASE + 0x10004),
    ("PORT_LINK_CTRL", PCIE_EXT0_BASE + 0x10700 + 0x10),
    ("GEN2_CTRL", PCIE_EXT0_BASE + 0x10700 + 0x10C),
    ("VC0_POSTED_Q", PCIE_EXT0_BASE + 0x10700 + 0x48),
    ("VC0_NONPOST_Q", PCIE_EXT0_BASE + 0x10700 + 0x4C),
    ("AXI_SLAVE_ERR", PCIE_EXT0_BASE + 0x10700 + 0x1D0),
]

# PL061 GPIODATA, address-mask trick: offset 0x3FC = all 8 mask bits set,
# i.e. the read returns the real port value, not a masked subset.
GPIO_BANK_BASE = [
    0xFD887000,
    0xFD888000,
    0xFD889000,
    0xFD88A000,
    0xFD88B000,
    0xFD897000,
]
GPIO_GPIODATA_ALL_OFF = 0x3FC

# LTSSM state names, al_hal_pcie.h - only the early ones this issue has hit
LTSSM_NAMES = {
    0x0: "DETECT_QUIET",
    0x1: "DETECT_ACT",
    0x2: "POLL_ACTIVE",
    0x3: "POLL_COMPLIANCE",
    0x4: "POLL_CONFIG",
    0x5: "PRE_DETECT_QUIET",
    0x6: "DETECT_WAIT",
    0x11: "L0",
    0x12: "L0S",
}


def read32(mem_fd: int, addr: int) -> int:
    page = addr & ~0xFFF
    off = addr & 0xFFF
    with mmap.mmap(mem_fd, 4096, offset=page, prot=mmap.PROT_READ) as m:
        return struct.unpack("<I", m[off : off + 4])[0]


def main() -> int:
    try:
        mem_fd = __import__("os").open("/dev/mem", __import__("os").O_RDONLY)
    except (PermissionError, OSError) as e:
        log.error("/dev/mem open failed (%s) -- run as root", e)
        return 2

    log.info("=== regdump: PCIe ext0 + GPIO (#140) ===")
    for name, addr in PCIE_REGS:
        val = read32(mem_fd, addr)
        log.info("%-15s @ 0x%08x = 0x%08x", name, addr, val)

    ltssm_val = read32(mem_fd, PCIE_EXT0_BASE + 0x2080)
    ltssm_state = (ltssm_val & 0x1F8) >> 3
    log.info(
        "  -> LTSSM state 0x%x (%s)", ltssm_state, LTSSM_NAMES.get(ltssm_state, "?")
    )

    for i, base in enumerate(GPIO_BANK_BASE):
        val = read32(mem_fd, base + GPIO_GPIODATA_ALL_OFF) & 0xFF
        log.info("GPIO bank %d    @ 0x%08x = 0x%02x", i, base, val)

    log.info("=== regdump done (also written to %s) ===", LOG_PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
