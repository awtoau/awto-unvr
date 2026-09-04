#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Audit which hardware present on woomera has no driver bound.

Three failure modes it distinguishes:
  MISSING_CONFIG  driver is in-tree, config symbol not set
  NO_BIND         config set, device never probes (DT disabled/absent, bus down)
  NO_DRIVER       no in-tree driver exists at all

Sources: lspci -nnk, lsusb -t, /sys/bus/*/devices, /proc/mtd, dmesg, the DTS,
and the kernel .config. Read-only on the box; changes nothing.

Usage:
  ./scripts/audit-hw-coverage.py                 # collect from box + report
  ./scripts/audit-hw-coverage.py --from-cache    # re-report from tmp/hwaudit/

Log: tmp/logs/audit-hw-coverage.log
"""

from __future__ import annotations

import argparse
import logging
import re
import subprocess
import sys
from pathlib import Path

import _box

ROOT = Path(__file__).resolve().parent.parent
CACHE = ROOT / "tmp" / "hwaudit"
LOGDIR = ROOT / "tmp" / "logs"
DTS = ROOT / "dts" / "alpine-v2-ubnt-unvr-ea16.dts"
KCONFIG = Path("/mnt/2tb/unvr-port-refs/build-out-fedora/kbuild/.config")

# ssh round-trip on a healthy 100M LAN link is <1 s; these commands are pure
# sysfs/procfs reads. 60 s = ~60x, generous enough for a loaded box but still
# fires long before a wedged bus looks like a hang. On expiry: log the probe
# name and elapsed, continue with the remaining probes.
SSH_TIMEOUT_S = 60

# Probes: name -> remote shell command. Kept shell-simple so each is one
# ssh round trip. i2cdetect is deliberately NOT here: scanning the pca9546
# mux channels can wedge the s35390a on ch0 (docs/rtc-s35390a-fault.md, #86).
PROBES = {
    "lspci": "lspci -nnk",
    "lsusb": "lsusb; echo =====TREE; lsusb -t",
    "sysbus": (
        "for d in /sys/bus/*/devices/*; do "
        'drv=""; [ -e "$d/driver" ] && drv=$(readlink -f "$d/driver"); '
        'comp=""; [ -r "$d/of_node/compatible" ] && comp=$(tr "\\0" "," < "$d/of_node/compatible"); '
        'echo "$d|$drv|$comp"; done'
    ),
    "classes": (
        "echo ==rtc; ls /sys/class/rtc/ 2>&1; "
        "echo ==watchdog; ls /sys/class/watchdog/ 2>&1; "
        "echo ==leds; ls /sys/class/leds/ 2>&1; "
        'echo ==hwmon; for h in /sys/class/hwmon/hwmon*; do echo "$h $(cat $h/name 2>/dev/null)"; done; '
        'echo ==thermal; for t in /sys/class/thermal/thermal_zone*; do echo "$t $(cat $t/type 2>/dev/null)"; done; '
        "echo ==drm; ls /sys/class/drm/ 2>&1; "
        "echo ==sound; ls /sys/class/sound/ 2>&1"
    ),
    "mtd": "cat /proc/mtd",
    "filesystems": "cat /proc/filesystems",
    "netdev": (
        "for i in /sys/class/net/*; do n=$(basename $i); "
        'echo "$n drv=$(basename $(readlink -f $i/device/driver 2>/dev/null) 2>/dev/null) '
        'speed=$(cat $i/speed 2>/dev/null) oper=$(cat $i/operstate 2>/dev/null)"; done'
    ),
    "usbspeed": (
        "for d in /sys/bus/usb/devices/[0-9]*; do [ -f $d/speed ] && "
        'echo "$(basename $d) speed=$(cat $d/speed) $(cat $d/product 2>/dev/null)"; done'
    ),
    "dmesg_err": (
        "dmesg | grep -iE "
        "'error|fail|no driver|deferred|unable|cannot|timed out|not found|unsupported'"
    ),
    "firmware": "ls /lib/firmware/rtl_nic/ 2>&1; echo ==pkg; rpm -q linux-firmware 2>&1",
}


def ssh(cmd: str) -> str:
    """One ssh round trip. Returns stdout, '' on failure."""
    ip = _box.locate()
    if not ip:
        logging.warning("woomera not found on the LAN for %.60s", cmd)
        return ""
    _box.flush_failed_neighbours(ip)
    try:
        p = subprocess.run(
            _box.ssh_argv(ip, cmd=[cmd], batch=True),
            capture_output=True,
            text=True,
            timeout=SSH_TIMEOUT_S,
            cwd=ROOT,
        )
    except subprocess.TimeoutExpired:
        logging.error("ssh probe exceeded %ds limit: %.60s", SSH_TIMEOUT_S, cmd)
        return ""
    if p.returncode != 0:
        logging.warning(
            "ssh rc=%d for %.60s: %s", p.returncode, cmd, p.stderr.strip()[:200]
        )
    return p.stdout


def collect() -> dict[str, str]:
    CACHE.mkdir(parents=True, exist_ok=True)
    out = {}
    for name, cmd in PROBES.items():
        logging.info("probe: %s", name)
        text = ssh(cmd)
        (CACHE / f"{name}.txt").write_text(text)
        out[name] = text
    return out


def load_cache() -> dict[str, str]:
    return {
        n: (CACHE / f"{n}.txt").read_text() if (CACHE / f"{n}.txt").exists() else ""
        for n in PROBES
    }


def kconfig() -> dict[str, str]:
    """Parse .config into {SYMBOL: value}; unset symbols map to 'n'."""
    cfg = {}
    if not KCONFIG.exists():
        logging.warning("kernel .config not found at %s", KCONFIG)
        return cfg
    for ln in KCONFIG.read_text().splitlines():
        if m := re.match(r"^(CONFIG_\w+)=(.*)$", ln):
            cfg[m.group(1)] = m.group(2)
        elif m := re.match(r"^# (CONFIG_\w+) is not set$", ln):
            cfg[m.group(1)] = "n"
    return cfg


def dts_disabled_nodes() -> list[str]:
    """DTS nodes explicitly status='disabled' - a deliberate no-bind, not a gap."""
    if not DTS.exists():
        return []
    out, node = [], None
    for ln in DTS.read_text().splitlines():
        if m := re.match(r"\s*(?:\w+:\s*)?([\w@-]+)\s*\{", ln):
            node = m.group(1)
        if 'status = "disabled"' in ln and node:
            out.append(node)
    return out


def report(data: dict[str, str], cfg: dict[str, str]) -> None:
    p = print
    p("=" * 72)
    p("PCI devices with no driver bound")
    p("=" * 72)
    dev = None
    for ln in data["lspci"].splitlines():
        if re.match(r"^\S+:\S+\.\d", ln):
            if dev:
                p(f"  UNBOUND: {dev}")
            dev = ln
        elif "Kernel driver in use" in ln:
            dev = None
    if dev:
        p(f"  UNBOUND: {dev}")

    p("")
    p("=" * 72)
    p("USB interfaces with no driver bound")
    p("=" * 72)
    for ln in data["lsusb"].splitlines():
        if "Driver=[none]" in ln:
            p(f"  {ln.strip()}")

    p("")
    p("=" * 72)
    p("Platform/OF devices with an of_node and no driver")
    p("=" * 72)
    for ln in data["sysbus"].splitlines():
        parts = ln.split("|")
        if len(parts) != 3:
            continue
        path, drv, comp = parts
        if drv or not comp:
            continue
        if "/platform/" not in path and "/amba/" not in path:
            continue
        p(f"  {path.split('/')[-1]:32s} {comp.rstrip(',')}")

    p("")
    p("=" * 72)
    p("DTS nodes marked status=disabled (deliberate, not a gap)")
    p("=" * 72)
    for n in dts_disabled_nodes():
        p(f"  {n}")

    p("")
    p("=" * 72)
    p("Config symbols of interest")
    p("=" * 72)
    interesting = [
        "CONFIG_RTC_DRV_S35390A",
        "CONFIG_VFAT_FS",
        "CONFIG_EXFAT_FS",
        "CONFIG_NTFS3_FS",
        "CONFIG_XFS_FS",
        "CONFIG_F2FS_FS",
        "CONFIG_DRM_UDL",
        "CONFIG_EDAC_AL_MC",
        "CONFIG_SOUND",
        "CONFIG_SND_USB_AUDIO",
        "CONFIG_SENSORS_ADT7475",
        "CONFIG_ARM_SP805_WATCHDOG",
        "CONFIG_GPIO_PCA953X",
        "CONFIG_I2C_MUX_PCA954x",
        "CONFIG_SFP",
        "CONFIG_AT803X_PHY",
        "CONFIG_USB_UAS",
        "CONFIG_MTD_SPI_NOR",
        "CONFIG_EEPROM_AT24",
    ]
    for sym in interesting:
        v = cfg.get(sym, "<absent>")
        flag = "  " if v not in ("n", "<absent>") else "GAP"
        p(f"  {flag} {sym:32s} = {v}")

    p("")
    p("=" * 72)
    p("Filesystems the running kernel supports")
    p("=" * 72)
    fs = [ln.split()[-1] for ln in data["filesystems"].splitlines() if ln.strip()]
    p("  " + " ".join(sorted(fs)))
    for want in ("vfat", "msdos", "exfat"):
        if want not in fs:
            p(f"  GAP: '{want}' not supported by the running kernel")

    p("")
    p("=" * 72)
    p("MTD partitions (NAND absent => al-nand has no driver, #208)")
    p("=" * 72)
    p(data["mtd"].rstrip())

    p("")
    p("=" * 72)
    p("Network interfaces")
    p("=" * 72)
    p(data["netdev"].rstrip())

    p("")
    p("=" * 72)
    p("USB link speeds (480 = USB2, 5000 = USB3)")
    p("=" * 72)
    p(data["usbspeed"].rstrip())

    p("")
    p("=" * 72)
    p("Firmware")
    p("=" * 72)
    p(data["firmware"].rstrip())

    p("")
    p("=" * 72)
    p("dmesg: probe failures / timeouts")
    p("=" * 72)
    for ln in data["dmesg_err"].splitlines():
        if re.search(r"deferred|firmware load|no driver|-E[A-Z]+|failed to", ln, re.I):
            p(f"  {ln.strip()}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--from-cache",
        action="store_true",
        help="re-report from tmp/hwaudit/ without touching the box",
    )
    args = ap.parse_args()

    LOGDIR.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        handlers=[
            logging.FileHandler(LOGDIR / "audit-hw-coverage.log"),
            logging.StreamHandler(sys.stderr),
        ],
    )

    data = load_cache() if args.from_cache else collect()
    report(data, kconfig())
    return 0


if __name__ == "__main__":
    sys.exit(main())
