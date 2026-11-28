#!/usr/bin/env python3
"""TrueNAS SCALE client — WebSocket JSON-RPC 2.0 against `wss://<host>/api/current`.

    ./dev.py truenas info
    ./dev.py truenas pools
    ./dev.py truenas disks
    ./dev.py truenas datasets [--pool p3]
    ./dev.py truenas shares
    ./dev.py truenas network
    ./dev.py truenas alerts
    ./dev.py truenas apps
    ./dev.py truenas update-status
    ./dev.py truenas upgrade [--version 26.0.0-BETA.3]
    ./dev.py truenas call pool.query '[]'

Host is 192.168.25.64 (`truenas.local`). SSH there is publickey-only, so the
API is the only password-driven way in.

**WebSocket, not REST.** TrueNAS 26 removes the REST v2.0 API entirely; this
speaks the JSON-RPC transport that both 25.10 and 26 serve.

Credentials come from ~/.config/truenas/env (chmod 600), same shape as
~/.config/hass-cli/env:

    export TRUENAS_HOST="192.168.25.64"
    export TRUENAS_USER="admin"
    export TRUENAS_PASSWORD="..."       # or TRUENAS_API_KEY="..." (preferred)

TLS uses a self-signed appliance cert, so verification is off by default —
--verify turns it back on once a real cert is installed.
"""

from __future__ import annotations

import argparse
import json
import logging
import os
import ssl
import sys
from pathlib import Path

import websocket

from _repo import LOGS

ENV_FILE = Path.home() / ".config" / "truenas" / "env"
LOG_FILE = LOGS / "truenas.log"

# The API is one switch hop away and answers system.info in <100 ms. 15 s is
# ~150x that, the margin spent on pool/dataset walks that touch every disk. A
# call still outstanding at 15 s is a wedged middleware worker, not a slow one;
# on expiry the command exits 1 naming the method and the limit.
TIMEOUT = 15.0
# An update applies in the background and the box reboots under us. Poll its
# job every 5 s: the whole apply runs 5-15 min, so 5 s is ~1/60th of it —
# frequent enough to show real progress, far too coarse to load the middleware.
JOB_POLL_INTERVAL = 5.0
SECRET_KEYS = {"password", "api_key", "old_password", "new_password", "otp_token", "token"}


def _redact(obj):
	"""Copy of call params with secret values masked, for logging (#72)."""
	if isinstance(obj, dict):
		return {k: "***" if k in SECRET_KEYS else _redact(v) for k, v in obj.items()}
	if isinstance(obj, (list, tuple)):
		return type(obj)(_redact(v) for v in obj)
	return obj


def _load_env(path: Path = ENV_FILE) -> None:
	"""Read `export K="V"` lines. Only fills what the environment has not set."""
	if not path.exists():
		return
	for line in path.read_text().splitlines():
		line = line.strip()
		if not line or line.startswith("#"):
			continue
		line = line.removeprefix("export ").strip()
		if "=" not in line:
			continue
		key, _, val = line.partition("=")
		os.environ.setdefault(key.strip(), val.strip().strip("\"'"))


class TrueNAS:
	"""One authenticated JSON-RPC connection. `call()` is the whole interface."""

	def __init__(self, host: str, verify: bool = False):
		self.host = host
		sslopt = {} if verify else {"cert_reqs": ssl.CERT_NONE, "check_hostname": False}
		self.ws = websocket.create_connection(
			f"wss://{host}/api/current", sslopt=sslopt, timeout=TIMEOUT
		)
		self._id = 0
		self._login()

	def _login(self) -> None:
		key = os.environ.get("TRUENAS_API_KEY")
		if key:
			cred = {"mechanism": "API_KEY_PLAIN",
			        "username": os.environ.get("TRUENAS_USER", "admin"), "api_key": key}
		else:
			pw = os.environ.get("TRUENAS_PASSWORD")
			if not pw:
				sys.exit(f"no TRUENAS_API_KEY or TRUENAS_PASSWORD — see {ENV_FILE}")
			cred = {"mechanism": "PASSWORD_PLAIN",
			        "username": os.environ.get("TRUENAS_USER", "admin"), "password": pw}
		res = self.call("auth.login_ex", cred)
		if res.get("response_type") != "SUCCESS":
			sys.exit(f"login failed: {res.get('response_type')}")

	def call(self, method: str, *params):
		self._id += 1
		logging.info("call %s %s", method, _redact(params) or "")
		self.ws.send(json.dumps({"jsonrpc": "2.0", "id": self._id,
		                         "method": method, "params": list(params)}))
		while True:
			try:
				msg = json.loads(self.ws.recv())
			except websocket.WebSocketTimeoutException:
				sys.exit(f"timeout: {method} did not answer within {TIMEOUT}s")
			if msg.get("id") != self._id:      # collections/events interleave
				continue
			if "error" in msg:
				err = msg["error"]
				sys.exit(f"{method}: {err.get('message')} {err.get('data', {}).get('reason', '')}")
			return msg["result"]

	def job(self, method: str, *params, label: str = ""):
		"""Start a long-running method and block until its job leaves RUNNING."""
		import time
		job_id = self.call(method, *params)
		last = None
		while True:
			j = self.call("core.get_jobs", [["id", "=", job_id]])[0]
			state, desc = j["state"], (j.get("progress") or {}).get("description")
			if (state, desc) != last:
				print(f"  [{state}] {desc or ''}".rstrip())
				last = (state, desc)
			if state not in ("RUNNING", "WAITING"):
				if state != "SUCCESS":
					sys.exit(f"{label or method} {state}: {j.get('error')}")
				return j.get("result")
			time.sleep(JOB_POLL_INTERVAL)


def human(n: float | int | None) -> str:
	if n is None:
		return "-"
	for unit in ("B", "K", "M", "G", "T", "P"):
		if abs(n) < 1024:
			return f"{n:.1f}{unit}"
		n /= 1024
	return f"{n:.1f}E"


def cmd_info(api: TrueNAS, args) -> None:
	i = api.call("system.info")
	g = api.call("system.general.config")
	print(f"hostname   {i['hostname']}")
	print(f"version    {i['version']}")
	print(f"product    {i.get('system_product')} (serial {i.get('system_serial')})")
	print(f"cpu        {i['model']} — {i['physical_cores']} cores")
	print(f"memory     {human(i['physmem'])}")
	print(f"uptime     {i['uptime']}")
	print(f"loadavg    {', '.join(f'{x:.2f}' for x in i['loadavg'])}")
	print(f"timezone   {g.get('timezone')}")
	print(f"ui         http {g.get('ui_port')} / https {g.get('ui_httpsport')}")


def cmd_pools(api: TrueNAS, args) -> None:
	for p in api.call("pool.query"):
		used, total = p.get("allocated"), p.get("size")
		pct = f"{100 * used / total:.0f}%" if used and total else "-"
		print(f"{p['name']:<16} {p['status']:<10} {human(used)}/{human(total)} ({pct})  healthy={p.get('healthy')}")
		for vdev in p.get("topology", {}).get("data", []):
			kids = [c.get("disk") or c.get("device") for c in vdev.get("children", [])] or [vdev.get("disk")]
			print(f"    data  {vdev.get('type')}  {' '.join(str(k) for k in kids)}")
			# pool 'healthy' stays True with a FAULTED raidz member (#69): check members
			for c in vdev.get("children", []):
				s = c.get("stats", {})
				if c.get("status") != "ONLINE" or any(s.get(k) for k in ("read_errors", "write_errors", "checksum_errors")):
					print(f"    !! {c.get('disk') or c.get('guid')} {c.get('status')} "
					      f"r/w/c {s.get('read_errors')}/{s.get('write_errors')}/{s.get('checksum_errors')}")


def cmd_disks(api: TrueNAS, args) -> None:
	for d in api.call("disk.query"):
		print(f"{d['name']:<8} {human(d.get('size')):>8}  {d.get('model','')}  serial={d.get('serial','')}  pool={d.get('pool') or '-'}")


def cmd_datasets(api: TrueNAS, args) -> None:
	for ds in api.call("pool.dataset.query"):
		if args.pool and not ds["name"].startswith(args.pool):
			continue
		used = ds.get("used", {}).get("parsed")
		avail = ds.get("available", {}).get("parsed")
		print(f"{ds['name']:<40} {ds['type']:<10} used={human(used):>8} avail={human(avail):>8}")


def cmd_shares(api: TrueNAS, args) -> None:
	print("== SMB ==")
	for s in api.call("sharing.smb.query"):
		print(f"  {s['name']:<20} {s['path']:<40} enabled={s['enabled']} ro={s.get('ro')}")
	print("== NFS ==")
	for s in api.call("sharing.nfs.query"):
		print(f"  {s.get('path') or s.get('paths')}  enabled={s['enabled']}")
	print("== iSCSI targets ==")
	for t in api.call("iscsi.target.query"):
		print(f"  {t['name']}  {t.get('alias') or ''}")


def cmd_network(api: TrueNAS, args) -> None:
	for nic in api.call("interface.query"):
		addrs = [f"{a['address']}/{a['netmask']}" for a in nic.get("state", {}).get("aliases", []) if a.get("type") in ("INET", "INET6")]
		print(f"{nic['name']:<10} {nic.get('state', {}).get('link_state'):<14} {' '.join(addrs)}")


def cmd_alerts(api: TrueNAS, args) -> None:
	alerts = [a for a in api.call("alert.list") if not a.get("dismissed")]
	if not alerts:
		print("no active alerts")
	for a in alerts:
		print(f"[{a['level']}] {a['formatted']}")


def cmd_apps(api: TrueNAS, args) -> None:
	for a in api.call("app.query"):
		upg = a.get("upgrade_available")
		print(f"{a['name']:<24} {a.get('state'):<12} {a.get('version','')}"
		      + ("  UPGRADE AVAILABLE" if upg else ""))


def cmd_update_status(api: TrueNAS, args) -> None:
	s = api.call("update.status")
	cur = s["status"]["current_version"]
	print(f"train      {cur['train']} (profile {cur['profile']})")
	new = s["status"].get("new_version")
	if not new:
		print("up to date")
		return
	m = new["manifest"]
	print(f"available  {new['version']}  train {m['train']}  profile {m['profile']}  {human(m['filesize'])}")
	dl = s.get("update_download_progress") or {}
	print(f"download   {dl.get('percent')}% — {dl.get('description')}")


def cmd_upgrade(api: TrueNAS, args) -> None:
	"""Apply an update. TrueNAS makes a boot environment first — that is the rollback."""
	s = api.call("update.status")["status"]
	new = s.get("new_version")
	if not new:
		print("already up to date")
		return
	target = args.version or new["version"]
	print(f"{s['current_version']['train']} → {target} ({new['manifest']['train']})")
	params = {"resume": args.resume}
	if args.version:
		params["version"] = args.version
		# TrueNAS requires `train` whenever `version` is pinned.
		params["train"] = new["manifest"]["train"]
	api.job("update.run", params, label="update.run")
	print("applied — reboot to boot the new version (`./dev.py truenas call system.reboot`)")


def cmd_call(api: TrueNAS, args) -> None:
	params = json.loads(args.params) if args.params else []
	print(json.dumps(api.call(args.method, *params), indent=2, default=str))


def main() -> None:
	LOG_FILE.parent.mkdir(parents=True, exist_ok=True)
	logging.basicConfig(filename=LOG_FILE, level=logging.INFO,
	                    format="%(asctime)s %(levelname)s %(message)s")
	_load_env()

	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("--host", default=os.environ.get("TRUENAS_HOST", "192.168.25.64"))
	ap.add_argument("--verify", action="store_true", help="verify TLS (needs a real cert)")
	sub = ap.add_subparsers(dest="cmd", required=True)

	sub.add_parser("info").set_defaults(fn=cmd_info)
	sub.add_parser("pools").set_defaults(fn=cmd_pools)
	sub.add_parser("disks").set_defaults(fn=cmd_disks)
	p = sub.add_parser("datasets"); p.add_argument("--pool"); p.set_defaults(fn=cmd_datasets)
	sub.add_parser("shares").set_defaults(fn=cmd_shares)
	sub.add_parser("network").set_defaults(fn=cmd_network)
	sub.add_parser("alerts").set_defaults(fn=cmd_alerts)
	sub.add_parser("apps").set_defaults(fn=cmd_apps)
	sub.add_parser("update-status").set_defaults(fn=cmd_update_status)
	p = sub.add_parser("upgrade")
	p.add_argument("--version", help="target version (default: what update.status offers)")
	p.add_argument("--resume", action="store_true", help="accept a warning update.run raised as EAGAIN")
	p.set_defaults(fn=cmd_upgrade)
	p = sub.add_parser("call"); p.add_argument("method"); p.add_argument("params", nargs="?")
	p.set_defaults(fn=cmd_call)

	args = ap.parse_args()
	args.fn(TrueNAS(args.host, verify=args.verify), args)


if __name__ == "__main__":
	main()
