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

PCIE_EXT0_BASE = 0xFD800000

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
    dump_regs(
        "PBS bootstrap (raw - see read-ddr-bootstrap.py for decode)",
        [
            ("boot_strap", 0xFD8A8110),
        ],
    )


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
    small_labels = {
        "u-boot",
        "u-boot-env",
        "u-boot-env-redundant",
        "factory",
        "eeprom",
        "cksum",
    }
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


# --- PCI / USB / SCSI trees ------------------------------------------------


def dump_pci() -> None:
    log.info("--- PCI tree (lspci -vv) ---")
    out = run(["lspci", "-vv", "-nn"], timeout=15)
    if out:
        log.info("%s", out.strip())


def dump_usb() -> None:
    log.info("--- USB tree (lsusb -t) ---")
    out = run(["lsusb", "-t"])
    if out:
        log.info("%s", out.strip())
    log.info("--- USB verbose (lsusb -v) ---")
    out = run(["lsusb", "-v"], timeout=15)
    if out:
        log.info("%s", out.strip())


def dump_scsi() -> None:
    log.info("--- SCSI/storage tree ---")
    out = run(["lsscsi", "-v"])
    if out:
        log.info("lsscsi:\n%s", out.strip())
    else:
        out = run(["cat", "/proc/scsi/scsi"])
        if out:
            log.info("/proc/scsi/scsi:\n%s", out.strip())
    out = run(["lsblk", "-o", "NAME,SIZE,TRAN,MODEL,SERIAL,ROTA,TYPE"])
    if out:
        log.info("lsblk:\n%s", out.strip())


# --- I2C ------------------------------------------------------------------

# docs/hardware.md:373-379 + gpio-map.md - known I2C address -> device name,
# for the tree summary below. Keep in sync if a new device gets documented.
I2C_KNOWN_DEVICES = {
    0x20: "PCA9575 GPIO expander (SFP-1G-LED + straps)",
    0x21: "PCA9575 GPIO expander (bay pwr/present/fault)",
    0x29: "PCA9575 GPIO expander (Pro bays 5-8, unpopulated on this SKU)",
    0x2E: "ADT7475 fan/temp controller",
    0x30: "S35390A RTC",
    0x50: "SFP module EEPROM (SFF-8079/8472)",
    0x51: "SFP module EEPROM (SFF-8079/8472, page 1)",
    0x57: "AT24C64 identity EEPROM (MAC/serial) / DRAM SPD",
    0x71: "PCA9546 I2C mux (4 channels)",
}


def find_rtc_bus() -> str | None:
    """Which i2c-N hosts the s35390a RTC (addr 0x30), found via sysfs only -
    no bus traffic, so this is safe even while the bus is wedged. Matches
    `/sys/bus/i2c/devices/<N>-0030`."""
    for p in Path("/sys/bus/i2c/devices").glob("*-0030"):
        n = p.name.split("-")[0]
        if n.isdigit():
            return n
    return None


def dump_i2c_scan() -> None:
    log.info("--- I2C bus scan (all buses incl. pca9546 mux channels) ---")
    buses = run(["i2cdetect", "-l"])
    if not buses:
        return
    log.info("%s", buses.strip())

    rtc_bus = find_rtc_bus()
    if rtc_bus:
        log.info(
            "i2c-%s hosts the s35390a RTC (0x30) - a full i2cdetect scan on this "
            "bus has repeatedly wedged it live tonight (docs/rtc-s35390a-fault.md: "
            "phantom START/STOP mis-read holds SDA, unrecoverable in SW). "
            "SKIPPING the raw scan; read via the bound kernel driver instead "
            "(dump_i2c_devices()'s hwclock call, below).",
            rtc_bus,
        )

    tree: dict[str, list[str]] = {}
    for line in buses.splitlines():
        m = re.match(r"i2c-(\d+)", line)
        if not m:
            continue
        n = m.group(1)
        if n == rtc_bus:
            continue
        out = run(["i2cdetect", "-y", n])
        if not out:
            continue
        log.info("i2c-%s:\n%s", n, out.strip())
        found = []
        for row in out.splitlines()[1:]:
            cols = row.split(":", 1)
            if len(cols) != 2:
                continue
            try:
                row_base = int(cols[0].strip(), 16)
            except ValueError:
                continue
            for i, cell in enumerate(cols[1].split()):
                if cell in ("--", ""):
                    continue
                addr = row_base + i
                name = I2C_KNOWN_DEVICES.get(addr, "unknown")
                found.append(f"0x{addr:02x} ({name})")
        if found:
            tree[n] = found

    if rtc_bus:
        tree[rtc_bus] = ["0x30 (S35390A RTC) [via bound driver, not scanned]"]

    log.info("--- I2C summary tree ---")
    for n, devs in tree.items():
        log.info("i2c-%s: %s", n, ", ".join(devs))


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
            for f in sorted(hwmon.glob("temp*_input")) + sorted(
                hwmon.glob("fan*_input")
            ):
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
        gc = run(
            ["find", "/sys/bus/i2c/devices", "-iname", f"*{addr}*", "-path", "*gpio*"]
        )
        if gc:
            log.info("PCA9575 @0x%s gpiochip node(s):\n%s", addr[-2:], gc.strip())


# --- English decode summary ------------------------------------------------

# docs/gpio-map.md per-ball table - (label, active_low, muxed_to_gpio).
# Keep in sync with scripts/gpio-top.py's PIN_INFO (same source doc).
GPIO_PIN_LABELS = {
    0: ("SFP 25G speed LED", True),
    31: ("ulogo_blue LED", True),
    33: ("rps_prnt (RPS present)", False),
    34: ("12v_lp (RPS 12V sense)", False),
    37: ("ulogo_white LED", True),
    38: ("reset button", True),
    42: ("hdd force-power-on-wa", False),
}


def dump_english_decode() -> None:
    log.info("--- English decode summary ---")

    rev = read32(PCIE_EXT0_BASE + 0x16C)
    if rev is not None:
        log.info(
            "PCIe ext0 controller revision: dev_id_val=%d -> %s",
            (rev >> 16) & 0xFFFF,
            {0: "REV_ID_2", 2: "REV_ID_3", 4: "REV_ID_4"}.get(
                (rev >> 16) & 0xFFFF, "?"
            ),
        )

    ltssm_val = read32(PCIE_EXT0_BASE + 0x2080)
    if ltssm_val is not None:
        state = (ltssm_val & 0x1F8) >> 3
        up = "LINK UP" if state in (0x11, 0x12) else "link NOT trained"
        log.info("PCIe ext0 link: LTSSM 0x%x -> %s", state, up)

    tb = read32(PCIE_EXT0_BASE + 0x30)
    if tb is not None:
        log.info(
            "PCIe ext0 CFG_TARGET_BUS: mask=0x%x target_bus=%d",
            tb & 0xFF,
            (tb >> 8) & 0xFF,
        )

    cmd = read32(PCIE_EXT0_BASE + 0x10004)
    if cmd is not None:
        c = cmd & 0xFFFF
        flags = [
            n for bit, n in ((0, "IO"), (1, "Mem"), (2, "BusMaster")) if c & (1 << bit)
        ]
        log.info("PCIe ext0 CFGHDR command: %s", "|".join(flags) or "(none enabled)")

    spec = read32(0xF0090004)
    s3 = read32(0xF0094000)
    s4 = read32(0xF0095000)
    if spec is not None:
        log.info(
            "CCU speculation_ctrl=0x%x (7 = speculative fetches disabled from masters)",
            spec,
        )
    if s3 is not None:
        log.info(
            "CCU cluster0 snoop: %s (bit0=%d)",
            "ENABLED" if s3 & 1 else "disabled",
            s3 & 1,
        )
    if s4 is not None:
        log.info(
            "CCU cluster1 snoop: %s (bit0=%d)",
            "ENABLED" if s4 & 1 else "disabled",
            s4 & 1,
        )

    gctlr = read32(0xF0200000)
    if gctlr is not None:
        log.info(
            "GIC-v3 GICD_CTLR: Group0=%s Group1=%s",
            "enabled" if gctlr & 1 else "disabled",
            "enabled" if gctlr & 2 else "disabled",
        )

    # Bootstrap strap - reuse read-ddr-bootstrap.py's real decode table,
    # loaded by path since its filename has a dash (not import-able directly).
    try:
        import importlib.util

        spec_mod = importlib.util.spec_from_file_location(
            "read_ddr_bootstrap", Path(__file__).parent / "read-ddr-bootstrap.py"
        )
        rdb = importlib.util.module_from_spec(spec_mod)
        spec_mod.loader.exec_module(rdb)
        strap = read32(0xFD8A8110)
        if strap is not None:
            d = rdb.decode(strap)
            log.info(
                "Bootstrap: cpu_pll=%.0fMHz ddr_pll=%.0fMHz (%.0f MT/s) sb_pll=%.0fMHz "
                "boot_device=%s debug_mode=%s",
                d["cpu_pll_freq"] / 1e6,
                d["ddr_pll_freq"] / 1e6,
                d["ddr_pll_freq"] * 2 / 1e6,
                d["sb_pll_freq"] / 1e6,
                d["boot_device"],
                d["debug_mode"],
            )
    except Exception as e:
        log.warning("bootstrap decode failed: %s", e)

    log.info("GPIO pins of interest:")
    for i, base in enumerate(
        [0xFD887000, 0xFD888000, 0xFD889000, 0xFD88A000, 0xFD88B000, 0xFD897000]
    ):
        val = read32(base + 0x3FC)
        if val is None:
            continue
        for bit in range(8):
            pin = i * 8 + bit
            if pin not in GPIO_PIN_LABELS:
                continue
            label, active_low = GPIO_PIN_LABELS[pin]
            v = (val >> bit) & 1
            active = (v == 0) if active_low else (v == 1)
            log.info("  pin %-2d %-28s %s", pin, label, "ACTIVE" if active else "-")


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
    dump_pci()
    dump_usb()
    dump_scsi()
    dump_i2c_scan()
    dump_i2c_devices()
    dump_english_decode()
    log.info("=== snapshot done, log at %s ===", LOG_PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
