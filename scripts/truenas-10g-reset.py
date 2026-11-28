#!/usr/bin/env python3
"""Reset truenas' BCM57810 and bring the 10G port up.

    ./dev.py truenas-10g-reset --check     # report state, change nothing
    ./dev.py truenas-10g-reset --reset     # PCI remove + rescan, then up + address

The card's two functions share one hardware lock. **Port 1 (`07:00.1`) has no
module and never links**, but its DMA engine fails during init, wedging that
lock (`lock_status 0xffffffff`) and killing port 0's MDIO ~0.4 s after port 0
has already linked at 10 Gbps. The driver then misreports it as a power fault
on port 0 and latches the cage off; `bnx2x_open()` refuses both ports from then
on with "Recovery flow hasn't been properly completed".

A reboot does **not** clear it — the MCP keeps state across warm reset, and the
sequence repeats every boot. Only a PCI-level re-probe resets it. Details and
the full log: awto-au/awto-terminal#48.

Needs SSH to `admin` (key installed; `sudo` via `-tt` + password on stdin) and
`~/.config/truenas/env`.
"""

from __future__ import annotations

import argparse
import logging
import os
import subprocess
import sys
from pathlib import Path
from _repo import REPO as _REPO  # noqa: E402

ENV_FILE = Path.home() / ".config" / "truenas" / "env"
LOG_FILE = (_REPO / "tmp/logs/truenas-10g-reset.log")

PORT0, PORT1 = "0000:07:00.0", "0000:07:00.1"
IFACE = "enp7s0f0"
ADDR, PREFIX = "192.168.25.71", 24        # registry: awto-dan docs/network/ip-registry.toml

# Each step is one ssh round trip to a box one switch hop away; the slowest
# (rescan + driver re-probe) took ~8 s when measured. 45 s is ~5x that, the
# margin covering a re-probe that retries. On expiry the step is named and the
# run aborts rather than leaving the card half-removed.
SSH_TIMEOUT = 45


def _load_env() -> None:
	if not ENV_FILE.exists():
		sys.exit(f"missing {ENV_FILE}")
	for line in ENV_FILE.read_text().splitlines():
		line = line.strip().removeprefix("export ").strip()
		if line and not line.startswith("#") and "=" in line:
			k, _, v = line.partition("=")
			os.environ.setdefault(k.strip(), v.strip().strip("\"'"))


def sh(cmd: str, *, label: str) -> str:
	"""Run cmd as root on the NAS. sudo needs a TTY, hence -tt."""
	host = os.environ.get("TRUENAS_HOST", "192.168.25.64")
	pw = os.environ["TRUENAS_PASSWORD"]
	full = f"echo '{pw}' | sudo -S sh -c {subprocess.list2cmdline([cmd])}"
	logging.info("%s: %s", label, cmd)
	try:
		p = subprocess.run(
			["ssh", "-tt", "-o", "BatchMode=yes", f"admin@{host}", full],
			capture_output=True, text=True, timeout=SSH_TIMEOUT,
		)
	except subprocess.TimeoutExpired:
		sys.exit(f"timeout: {label} exceeded {SSH_TIMEOUT}s")
	out = "\n".join(l for l in p.stdout.splitlines() if not l.startswith("[sudo"))
	logging.info("%s -> %s", label, out.strip()[:400])
	return out.strip()


def state() -> dict[str, str]:
	"""Label each field at the source — ssh -tt interleaves the sudo prompt, so
	positional parsing silently shifts by one."""
	raw = sh(
		"echo __start__; " + "; ".join(
			f'echo "{k}=$(cat /sys/class/net/{IFACE}/{path} 2>/dev/null)"'
			for k, path in (
				("operstate", "operstate"),
				("carrier", "carrier"),
				("speed", "speed"),
				("rx_packets", "statistics/rx_packets"),
			)
		),
		label="state")
	out = {}
	for line in raw.splitlines():
		k, _, v = line.strip().partition("=")
		if k in ("operstate", "carrier", "speed", "rx_packets"):
			out[k] = v or "?"
	return {k: out.get(k, "?") for k in ("operstate", "carrier", "speed", "rx_packets")}


def cmd_check(args) -> None:
	s = state()
	print(f"{IFACE}: operstate={s['operstate']} carrier={s['carrier']} "
	      f"speed={s['speed']} rx_packets={s['rx_packets']}")
	bound = sh("echo __start__; ls /sys/bus/pci/drivers/bnx2x/ | grep ^0000 || true",
	           label="bound").replace("__start__", "")
	print("bnx2x bound:", " ".join(bound.split()) or "none")
	if s["operstate"] == "up" and s["carrier"] == "1":
		print("10G is up — nothing to do.")
	else:
		print("10G is down — re-run with --reset.")


def on_bus() -> bool:
	"""`lspci -s` exits 0 with no output when the slot is empty, so match the
	device id in its output rather than trusting the exit code."""
	return "14e4:168e" in sh("echo __start__; lspci -nn -s 07:00.0", label="on_bus")


def iface_exists() -> bool:
	return "yes" in sh(f"echo __start__; test -e /sys/class/net/{IFACE} && echo yes || echo no",
	                   label="iface_exists")


def configure(args) -> None:
	"""Address + routing. Both routing parts are required: the main-table metric
	keeps the subnet on eno1, the policy rule puts .71-sourced traffic on 10G.
	Either alone regresses a direction (#59)."""
	sh(f"ip link set {IFACE} mtu 9000 up", label="up")
	if not args.address:
		return
	sh(f"ip addr replace {ADDR}/{PREFIX} dev {IFACE}", label="addr")
	sh(f"ip route del 192.168.25.0/24 dev {IFACE} 2>/dev/null; "
	   f"ip route add 192.168.25.0/24 dev {IFACE} proto kernel scope link "
	   f"src {ADDR} metric 2000 2>/dev/null; "
	   f"ip route replace 192.168.25.0/24 dev {IFACE} src {ADDR} table 10; "
	   f"ip rule add from {ADDR} table 10 priority 100 2>/dev/null; true",
	   label="routes")


def cmd_reset(args) -> None:
	"""Re-probe ONLY when the card is actually wedged.

	An unconditional remove+rescan killed a healthy card on 2026-10-12 — the
	rescan does not reliably re-enumerate, so `remove` is not idempotent (#70).
	"""
	if not on_bus():
		sys.exit("07:00.0 absent from the PCI bus — nothing to re-probe. "
		         "Needs a power cycle; a rescan will not recover it (#70).")

	s = state()
	if s["operstate"] == "up" and s["carrier"] == "1":
		print(f"{IFACE} already up at {s['speed']} Mb/s — no re-probe needed")
		configure(args)
		print("10G up.")
		return

	wedged = iface_exists() and "Recovery flow" in sh(
		"echo __start__; dmesg | grep -c 'Recovery flow' >/dev/null; "
		"dmesg | grep -o 'Recovery flow' | tail -1", label="wedged_check")
	if not wedged:
		# Down for some other reason (no carrier, cage unpowered). A remove here
		# risks losing the card for a problem a re-probe would not fix.
		sys.exit(f"{IFACE} is down but not in the known wedged state "
		         f"(operstate={s['operstate']}). Not removing the card — "
		         f"check `dmesg | grep bnx2x` first.")

	print("card wedged in recovery — removing both functions and rescanning")
	sh(f"echo 1 > /sys/bus/pci/devices/{PORT0}/remove; "
	   f"echo 1 > /sys/bus/pci/devices/{PORT1}/remove", label="remove")
	sh("echo 1 > /sys/bus/pci/rescan", label="rescan")

	if not iface_exists():
		sys.exit(f"RESCAN FAILED — {IFACE} did not reappear. The card is off the "
		         f"bus and needs a power cycle (#70).")
	configure(args)
	s = state()
	print(f"{IFACE}: operstate={s['operstate']} carrier={s['carrier']} speed={s['speed']}")
	if s["operstate"] != "up" or s["carrier"] != "1":
		sys.exit("link did not come up — check `dmesg | grep bnx2x` on the box")
	print("10G up.")


def main() -> None:
	LOG_FILE.parent.mkdir(parents=True, exist_ok=True)
	logging.basicConfig(filename=LOG_FILE, level=logging.INFO,
	                    format="%(asctime)s %(levelname)s %(message)s")
	_load_env()
	ap = argparse.ArgumentParser(description=__doc__,
	                             formatter_class=argparse.RawDescriptionHelpFormatter)
	g = ap.add_mutually_exclusive_group(required=True)
	g.add_argument("--check", action="store_true")
	g.add_argument("--reset", action="store_true")
	ap.add_argument("--address", action="store_true",
	                help=f"also assign {ADDR}/{PREFIX} (registry-allocated)")
	args = ap.parse_args()
	(cmd_reset if args.reset else cmd_check)(args)


if __name__ == "__main__":
	main()
