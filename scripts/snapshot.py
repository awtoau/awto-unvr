#!/usr/bin/env python3
"""Full hardware-state snapshot: SoC-internal registers (CCU, GIC, PLL ring,
PCIe ext0, bootstrap strap, DDR-ready/SRAM-agent block, all 6 GPIO banks) via
/dev/mem, plus NOR/MTD partition listing+checksums, plus a full I2C bus/mux
scan and known-device register reads via sysfs (runs on woomera).

One-shot "what does the box actually look like right now" dump - not a
targeted diagnostic like regdump.py (which this supersedes/includes for the
PCIe+GPIO section - keep both in sync with al_diag_dump() in alpine.c).
Every address here is cited against docs/hardware.md; nothing here is a
guess. Sections degrade gracefully (skip + log a warning) rather than abort
the whole snapshot if one piece is unavailable on a given boot.

Root only (raw /dev/mem MMIO read + i2c-tools + mtd sysfs).
"""

import logging
import mmap
import re
import struct
import subprocess
import sys
import time
from pathlib import Path

LOG_DIR = Path(__file__).resolve().parent.parent / "tmp" / "logs"
LOG_DIR.mkdir(parents=True, exist_ok=True)
STAMP = time.strftime("%Y%m%d-%H%M%S")
LOG_PATH = LOG_DIR / f"snapshot-{STAMP}.log"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)-5s %(message)s",
    handlers=[logging.FileHandler(LOG_PATH), logging.StreamHandler(sys.stdout)],
)
log = logging.getLogger(__name__)

_MEM_FD = None


def _mem_fd() -> int:
    global _MEM_FD
    if _MEM_FD is None:
        import os

        _MEM_FD = os.open("/dev/mem", os.O_RDONLY)
    return _MEM_FD


def read32(addr: int) -> int | None:
    try:
        page = addr & ~0xFFF
        off = addr & 0xFFF
        with mmap.mmap(_mem_fd(), 4096, offset=page, prot=mmap.PROT_READ) as m:
            return struct.unpack("<I", m[off : off + 4])[0]
    except OSError as e:
        log.warning("read32(0x%08x) failed: %s", addr, e)
        return None


def dump_regs(section: str, regs: list[tuple[str, int]]) -> None:
    log.info("--- %s ---", section)
    for name, addr in regs:
        val = read32(addr)
        if val is None:
            continue
        log.info("%-20s @ 0x%08x = 0x%08x", name, addr, val)


def run(cmd: list[str], timeout: float = 5.0) -> str | None:
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return r.stdout
    except (OSError, subprocess.TimeoutExpired) as e:
        log.warning("%s failed: %s", " ".join(cmd), e)
        return None


# --- SoC-internal registers, docs/hardware.md MMIO map -----------------

def dump_cpu() -> None:
    log.info("--- CPU ---")
    out = run(["uname", "-a"])
    if out:
        log.info("uname: %s", out.strip())
    try:
        cpuinfo = Path("/proc/cpuinfo").read_text()
        impl = re.search(r"CPU implementer\s*:\s*(\S+)", cpuinfo)
        part = re.search(r"CPU part\s*:\s*(\S+)", cpuinfo)
        cores = cpuinfo.count("processor\t:")
        log.info(
            "implementer=%s part=%s cores=%d",
            impl.group(1) if impl else "?",
            part.group(1) if part else "?",
            cores,
        )
    except OSError as e:
        log.warning("cpuinfo read failed: %s", e)


def dump_gic() -> None:
    # GICv3 distributor, docs/hardware.md:276. GICD_CTLR=+0x0, GICD_TYPER=+0x4,
    # GICD_ISENABLERn=+0x100+4n (first 3 words = IRQs 0-95).
    base = 0xF0200000
    dump_regs(
        "GIC-v3 distributor",
        [
            ("GICD_CTLR", base + 0x0),
            ("GICD_TYPER", base + 0x4),
            ("GICD_ISENABLER0", base + 0x100),
            ("GICD_ISENABLER1", base + 0x104),
            ("GICD_ISENABLER2", base + 0x108),
        ],
    )


def dump_ccu() -> None:
    # CCU (Cache Coherency Unit), docs/hardware.md:279, #97/uboot-ccu-coherency.md.
    base = 0xF0090000
    dump_regs(
        "CCU (coherency)",
        [
            ("speculation_ctrl", base + 0x4),
            ("slaves[3].snoop_ctrl (cluster0)", base + 0x4000),
            ("slaves[4].snoop_ctrl (cluster1)", base + 0x5000),
        ],
    )


def dump_pll() -> None:
    # PLL ring, docs/hardware.md:343 - SB/NB/CPU PLL setup_0/status_7.
    base = 0xFD860000
    dump_regs(
        "PLL ring",
        [
            ("SB_PLL setup_0", base + 0xB00),
            ("SB_PLL status_7", base + 0xB00 + 0x1C),
            ("NB_PLL setup_0", base + 0xC00),
            ("NB_PLL status_7", base + 0xC00 + 0x1C),
            ("CPU_PLL setup_0", base + 0xD00),
            ("CPU_PLL status_7", base + 0xD00 + 0x1C),
            ("thermal", base + 0xA00),
        ],
    )


def dump_bootstrap() -> None:
    # PBS boot_strap, same register scripts/read-ddr-bootstrap.py decodes fully.
    dump_regs("PBS bootstrap (raw - see read-ddr-bootstrap.py for decode)", [
        ("boot_strap", 0xFD8A8110),
    ])


def dump_ddr_sram() -> None:
    # shared_parameters SRAM, docs/hardware.md:344. CPU-resume magic is the
    # cold/warm ground-truth test point found in #138.
    base = 0xFBFF4000
    dump_regs(
        "DDR-ready / SRAM-agent block",
        [
            ("SRAM_DEV_INFO_ADDRESS", base + 0x100),
            ("SRAM_CPU_RESUME_ADDRESS (cold/warm magic)", base + 0x120),
            ("magic_num/ddr_size", base + 0x150),
            ("SRAM_AGENT_ADDRESS", base + 0x200),
        ],
    )


def dump_pcie_ext0_gpio() -> None:
    # Same list as regdump.py / al_diag_dump() in alpine.c - kept in sync.
    base = 0xFD800000
    dump_regs(
        "PCIe ext0 (#140)",
        [
            ("DEV_REV_ID", base + 0x16C),
            ("GLOBAL_CONF", base + 0xC8),
            ("PF_SEL", base + 0x1B0),
            ("MASTER_ARCTL", base + 0x14),
            ("MASTER_AWCTL", base + 0x18),
            ("PBS_PCIE_CONF_1", 0xFD8A80E4),
            ("CFG_TARGET_BUS", base + 0x30),
            ("PORT_INIT", base + 0x2000),
            ("DEBUG_INFO_0", base + 0x2080),
            ("CFGHDR_VID_DID", base + 0x10000),
            ("CFGHDR_CMD_STAT", base + 0x10004),
            ("PORT_LINK_CTRL", base + 0x10700 + 0x10),
            ("GEN2_CTRL", base + 0x10700 + 0x10C),
            ("VC0_POSTED_Q", base + 0x10700 + 0x48),
            ("VC0_NONPOST_Q", base + 0x10700 + 0x4C),
            ("AXI_SLAVE_ERR", base + 0x10700 + 0x1D0),
        ],
    )
    ltssm_val = read32(base + 0x2080)
    if ltssm_val is not None:
        log.info("  -> LTSSM state 0x%x", (ltssm_val & 0x1F8) >> 3)

    log.info("--- GPIO banks (docs/hardware.md:242-251) ---")
    for i, gbase in enumerate(
        [0xFD887000, 0xFD888000, 0xFD889000, 0xFD88A000, 0xFD88B000, 0xFD897000]
    ):
        val = read32(gbase + 0x3FC)  # PL061 address-mask trick, all 8 bits
        if val is not None:
            log.info("GPIO bank %d @ 0x%08x = 0x%02x", i, gbase, val & 0xFF)


# --- NOR/MTD ------------------------------------------------------------

def dump_mtd() -> None:
    log.info("--- MTD / NOR partitions ---")
    out = run(["cat", "/proc/mtd"])
    if not out:
        return
    log.info("%s", out.strip())
    # Checksum only the small partitions (u-boot/env/factory/eeprom/cksum) -
    # skip kernel/config/rootfs-scale ones, this is a snapshot not a backup.
    small_labels = {"u-boot", "u-boot-env", "u-boot-env-redundant", "factory", "eeprom", "cksum"}
    for line in out.splitlines()[1:]:
        m = re.match(r'(mtd\d+):\s+(\S+)\s+\S+\s+"(.+)"', line)
        if not m:
            continue
        dev, size_hex, label = m.groups()
        if label not in small_labels:
            continue
        sha = run(["sha256sum", f"/dev/{dev}"], timeout=15)
        if sha:
            log.info("  %-24s %-8s %s", label, dev, sha.split()[0])


# --- I2C ------------------------------------------------------------------

def dump_i2c_scan() -> None:
    log.info("--- I2C bus scan (all buses incl. pca9546 mux channels) ---")
    buses = run(["i2cdetect", "-l"])
    if not buses:
        return
    log.info("%s", buses.strip())
    for line in buses.splitlines():
        m = re.match(r"i2c-(\d+)", line)
        if not m:
            continue
        n = m.group(1)
        out = run(["i2cdetect", "-y", n])
        if out:
            log.info("i2c-%s:\n%s", n, out.strip())


def dump_i2c_devices() -> None:
    log.info("--- Known I2C device state (docs/hardware.md:373-379) ---")

    # RTC s35390a @ i2c 0x30
    out = run(["hwclock", "--rtc=/dev/rtc0", "-r"])
    if out:
        log.info("RTC (s35390a): %s", out.strip())

    # ADT7475 hwmon (fan/temp) - read whatever hwmon dir binds to adt7475
    for hwmon in Path("/sys/class/hwmon").glob("hwmon*"):
        name_f = hwmon / "name"
        if name_f.exists() and "adt7475" in name_f.read_text():
            log.info("ADT7475 hwmon: %s", hwmon)
            for f in sorted(hwmon.glob("temp*_input")) + sorted(hwmon.glob("fan*_input")):
                try:
                    log.info("  %-16s %s", f.name, f.read_text().strip())
                except OSError:
                    pass

    # Identity EEPROM at24c64 @ 0x57 - MAC/serial already known-decoded
    # elsewhere; just confirm the device node exists here, don't re-dump.
    ee = run(["find", "/sys/bus/i2c/devices", "-iname", "*0057*"])
    if ee:
        log.info("EEPROM (at24c64 @0x57) sysfs node(s):\n%s", ee.strip())

    # PCA9575 GPIO expanders @ 0x20/0x21/0x29
    for addr in ("0020", "0021", "0029"):
        gc = run(["find", "/sys/bus/i2c/devices", "-iname", f"*{addr}*", "-path", "*gpio*"])
        if gc:
            log.info("PCA9575 @0x%s gpiochip node(s):\n%s", addr[-2:], gc.strip())


def main() -> int:
    log.info("=== awto-unvr full hardware snapshot, %s ===", STAMP)
    dump_cpu()
    dump_gic()
    dump_ccu()
    dump_pll()
    dump_bootstrap()
    dump_ddr_sram()
    dump_pcie_ext0_gpio()
    dump_mtd()
    dump_i2c_scan()
    dump_i2c_devices()
    log.info("=== snapshot done, log at %s ===", LOG_PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
