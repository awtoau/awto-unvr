#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Idempotent TrueNAS config for woomera: tank/home, user dan, SMB + NFS, snapshots,
Spotlight (TrueSearch) + WebShare (#356, #357, #358).

  ./dev.py truenas-config plan     # print the steps, touch nothing on the box
  ./dev.py truenas-config apply    # converge the box (safe to re-run; run after every reinstall)
  ./dev.py truenas-config key-init # create the tank/home key section in secrets.yaml if absent
  ./dev.py truenas-config verify   # NFS mount + SMB put/get + snapshot listing, cleans up

- Reinstall wipes the config DB: apply imports `tank` first when it is not imported.
- tank/home is ZFS-native encrypted (aes-256-gcm, hex key). The key lives ONLY in secrets section
  `woomera-tank-home-key`; apply creates the dataset with it, or unlocks a locked one (reinstall
  rotates TrueNAS's pwenc secret, so its stored copy of the key is not trusted) (#370).
- dan password: ~/.config/awto/secrets.yaml section `woomera-truenas-dan`, generated once, never printed.
- Transport: TrueNAS JSON-RPC over wss://<box>/api/current with an API key (secrets section
  `woomera-truenas-api`); bootstrapped from truenas_admin's password when absent/rejected.
"""

from __future__ import annotations

import argparse
import json
import os
import secrets as pysecrets
import ssl
import subprocess
import sys
import time
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _net import LAN_SUBNET  # noqa: E402
from _repo import TMP, make_log  # noqa: E402

log = make_log("truenas-config")

POOL, DATASET = "tank", "tank/home"
HOME_PATH = f"/mnt/{DATASET}"
USER, UID, GID = "dan", 1000, 1000
SECRET_SECTION = "woomera-truenas-dan"
SHARE = "home"
WEBSHARE_GROUP = "truenas_webshare"  # built-in; awto-webshare's default allowed group
WEBSHARE_PORT = 755
SECRETS = Path(os.path.expanduser("~/.config/awto/secrets.yaml"))
PUBKEYS = [Path.home() / ".ssh" / n for n in ("id_ed25519.pub", "id_rsa.pub")]
# (name, cron minute/hour, lifetime) - hourly kept 2 days, daily kept 30 days
SNAP_TASKS = [
    {
        "naming_schema": "auto-%Y%m%d.%H%M-2d",
        "lifetime_value": 2,
        "lifetime_unit": "DAY",
        "schedule": {"minute": "0", "hour": "*", "dom": "*", "month": "*", "dow": "*"},
    },
    {
        "naming_schema": "auto-%Y%m%d.%H%M-30d",
        "lifetime_value": 30,
        "lifetime_unit": "DAY",
        "schedule": {"minute": "30", "hour": "0", "dom": "*", "month": "*", "dow": "*"},
    },
]
# One API call: LAN websocket round trip, expected <1 s; 30 s = 30x is the backstop for a stalled
# middleware. Expiry: raise naming the method.
CALL_TIMEOUT_S = 30
# Jobs (pool.import_pool, service.control): expected <10 s; 120 s backstop, polled via core.get_jobs.
JOB_TIMEOUT_S = 120
# Job-state poll: jobs here finish in seconds, 0.5 s poll adds <0.5 s latency.
JOB_POLL_S = 0.5
KEY_NAME = "awto-truenas-config"
KEY_SECTION = "woomera-truenas-api"
ADMIN_SECTION = "truenas-woomera"
HOME_KEY_SECTION = "woomera-tank-home-key"
HOME_ALGO = "AES-256-GCM"

DRY = False
_api = None


class AuthError(RuntimeError):
    pass


class Api:
    """Minimal TrueNAS JSON-RPC 2.0 client over wss://<box>/api/current (self-signed: no verify).
    truenas_api_client is not on PyPI, so this is a thin websockets.sync client."""

    def __init__(self, ip: str):
        import ssl

        from websockets.sync.client import connect

        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        self.ws = connect(
            f"wss://{ip}/api/current",
            ssl=ctx,
            open_timeout=CALL_TIMEOUT_S,
            max_size=64 * 2**20,
            legacy=True,
        )
        self.n = 0

    def call(self, method: str, *params, job: bool = False):
        self.n += 1
        self.ws.send(
            json.dumps(
                {
                    "jsonrpc": "2.0",
                    "id": self.n,
                    "method": method,
                    "params": list(params),
                }
            )
        )
        while True:
            try:
                msg = json.loads(self.ws.recv(timeout=CALL_TIMEOUT_S))
            except TimeoutError as e:
                raise RuntimeError(
                    f"{method}: no answer within {CALL_TIMEOUT_S}s"
                ) from e
            if msg.get("id") == self.n:
                break
        if "error" in msg:
            err = msg["error"]
            raise RuntimeError(
                f"{method} failed: {err.get('data', {}).get('reason') or err.get('message')}"
            )
        res = msg.get("result")
        return self.wait_job(method, res) if job else res

    def wait_job(self, method: str, job_id: int):
        import time

        t0 = time.monotonic()
        while time.monotonic() - t0 < JOB_TIMEOUT_S:
            rows = self.call("core.get_jobs", [["id", "=", job_id]])
            if rows and rows[0]["state"] in ("SUCCESS", "FAILED", "ABORTED"):
                if rows[0]["state"] != "SUCCESS":
                    raise RuntimeError(
                        f"{method} job {rows[0]['state']}: {rows[0].get('error')}"
                    )
                return rows[0].get("result")
            time.sleep(JOB_POLL_S)
        raise RuntimeError(f"{method}: job {job_id} not done within {JOB_TIMEOUT_S}s")


def secret(section: str) -> dict:
    return (load_secrets().get(section)) or {}


def append_section(section: str, fields: dict) -> None:
    """Append a new section as text: safe_dump would rewrite the whole file and drop its comments."""
    import yaml

    text = SECRETS.read_text()
    if not text.endswith("\n"):
        text += "\n"
    SECRETS.write_text(text + "\n" + yaml.safe_dump({section: fields}, sort_keys=False))
    SECRETS.chmod(0o600)
    log(f"secrets: created section {section}")


def drop_section(section: str) -> None:
    """Remove one top-level section by text (keeps the rest of the file byte-identical)."""
    import re

    text = SECRETS.read_text()
    new = re.sub(
        rf"^{re.escape(section)}:[^\n]*\n(?:[ \t]+[^\n]*\n|[ \t]*\n)*",
        "",
        text,
        flags=re.M,
    )
    if new != text:
        SECRETS.write_text(new)


def connect() -> Api:
    """API-key login; on failure (no key, or key gone after a reinstall) bootstrap a new key
    with one truenas_admin password login. DRY (plan): session login only, no key created."""
    ip = _box.require()
    key = secret(KEY_SECTION)
    if key.get("password"):
        api = Api(ip)
        r = api.call(
            "auth.login_ex",
            {
                "mechanism": "API_KEY_PLAIN",
                "username": key.get("username", "truenas_admin"),
                "api_key": key["password"],
            },
        )
        if r and r.get("response_type") == "SUCCESS":
            log("api: logged in with API key")
            return api
        log("api: stored API key rejected (reinstall?), bootstrapping")
    admin = secret(ADMIN_SECTION)
    if not admin.get("password"):
        raise AuthError(f"no API key and no [{ADMIN_SECTION}] password in secrets.yaml")
    api = Api(ip)
    r = api.call(
        "auth.login_ex",
        {
            "mechanism": "PASSWORD_PLAIN",
            "username": admin["username"],
            "password": admin["password"],
        },
    )
    if not r or r.get("response_type") != "SUCCESS":
        raise AuthError(
            f"password login as {admin['username']} failed: {r and r.get('response_type')}"
        )
    if DRY:
        log("[plan] api_key.create (skipped, session login only)")
        return api
    for old in api.call("api_key.query", [["name", "=", KEY_NAME]]):
        api.call("api_key.delete", old["id"])
    created = api.call(
        "api_key.create", {"name": KEY_NAME, "username": admin["username"]}
    )
    drop_section(KEY_SECTION)
    append_section(
        KEY_SECTION,
        {
            "url": f"wss://{ip}/api/current",
            "purpose": f"woomera TrueNAS API key '{KEY_NAME}' for scripts/truenas-config.py (recreated on reinstall)",
            "username": admin["username"],
            "password": created["key"],
        },
    )
    return api


def mc(method: str, *args, job: bool = False, write: bool = True):
    """TrueNAS API call. DRY: writes are only logged."""
    global _api
    if DRY and write:
        log(f"[plan] {method} {json.dumps(args)[:200]}")
        return None
    if _api is None:
        _api = connect()
    return _api.call(method, *args, job=job)


def q(method: str, filters: list):
    return mc(method, filters, write=False) or []


# ---- secrets (values never logged) ------------------------------------------------------
def load_secrets() -> dict:
    import yaml

    return yaml.safe_load(SECRETS.read_text()) or {}


def dan_password(create: bool) -> str | None:
    sec = secret(SECRET_SECTION)
    if sec and sec.get("password"):
        return sec["password"]
    if not create:
        return None
    pw = pysecrets.token_urlsafe(24)
    append_section(
        SECRET_SECTION,
        {
            "url": "smb://woomera (LAN IP via ./dev.py ssh --print)",
            "purpose": "woomera TrueNAS user dan (SMB/web); NFS uses uid 1000",
            "username": USER,
            "password": pw,
        },
    )
    return pw


def home_key(create: bool) -> str | None:
    """64-hex-char (32-byte) tank/home key from secrets; create=True generates + stores it FIRST."""
    sec = secret(HOME_KEY_SECTION)
    if sec.get("password"):
        return sec["password"]
    if not create:
        return None
    key = pysecrets.token_hex(32)
    append_section(
        HOME_KEY_SECTION,
        {
            "url": f"zfs://{DATASET} on woomera (keyformat=hex, {HOME_ALGO})",
            "purpose": f"ZFS native encryption key for {DATASET}; scripts/truenas-config.py unlocks with it (#370). LOSING IT LOSES THE DATA",
            "username": DATASET,
            "password": key,
        },
    )
    return key


# ---- steps ------------------------------------------------------------------------------
def step_pool() -> None:
    if q("pool.query", [["name", "=", POOL]]):
        log(f"pool {POOL}: imported")
        return
    found = mc("pool.import_find", job=True, write=False) or []
    match = [p for p in found if p.get("name") == POOL]
    if not match:
        raise RuntimeError(f"pool {POOL} neither imported nor importable: {found}")
    log(f"pool {POOL}: importing guid {match[0]['guid']}")
    mc("pool.import_pool", {"guid": match[0]["guid"]}, job=True)


def home_row() -> dict | None:
    rows = q("pool.dataset.query", [["name", "=", DATASET]])
    return rows[0] if rows else None


def step_dataset() -> None:
    row = home_row()
    if row:
        log(
            f"dataset {DATASET}: exists, encrypted={row['encrypted']} locked={row['locked']}"
        )
        if not row["encrypted"]:
            log(f"WARNING {DATASET} is not encrypted: migrate it (docs/truenas.md)")
        return
    log(f"dataset {DATASET}: create encrypted {HOME_ALGO}")
    key = home_key(create=not DRY) or "<generated on apply>"
    mc(
        "pool.dataset.create",
        {
            "name": DATASET,
            "share_type": "GENERIC",
            "encryption": True,
            "inherit_encryption": False,
            "encryption_options": {
                "generate_key": False,
                "key": key,
                "algorithm": HOME_ALGO,
            },
        },
    )


def store_db_key(key: str) -> None:
    """TrueNAS unlocks key-encrypted datasets at boot only from its own DB copy; keep one there so a
    plain reboot needs no apply. Secrets stays the source of truth (a reinstall wipes the DB copy)."""
    if mc(
        "datastore.query",
        "storage.encrypteddataset",
        [["name", "=", DATASET]],
        write=False,
    ):
        return
    log(f"dataset {DATASET}: store key in TrueNAS DB (boot auto-unlock)")
    mc("pool.dataset.change_key", DATASET, {"key": key, "generate_key": False})


def step_unlock() -> None:
    """Key from secrets.yaml, never from TrueNAS's DB (unreadable after a reinstall)."""
    row = home_row()
    if not row or not row["encrypted"]:
        log(f"dataset {DATASET}: not encrypted, nothing to unlock")
        return
    if not row["locked"]:
        key = home_key(create=False)
        if key and not DRY:
            store_db_key(key)
        log(f"dataset {DATASET}: already unlocked")
        return
    key = home_key(create=False)
    if not key:
        raise RuntimeError(
            f"{DATASET} is locked and secrets section {HOME_KEY_SECTION} is missing"
        )
    log(f"dataset {DATASET}: unlock with key from secrets")
    res = mc(
        "pool.dataset.unlock",
        DATASET,
        {"datasets": [{"name": DATASET, "key": key}], "recursive": True},
        job=True,
    )
    if res and res.get("failed"):
        raise RuntimeError(
            f"unlock {DATASET} failed: {json.dumps(res['failed'])[:200]}"
        )
    store_db_key(key)


def step_user() -> None:
    gid_row = q("group.query", [["gid", "=", GID]])
    if not gid_row:
        log(f"group {USER}: create gid {GID}")
        mc("group.create", {"name": USER, "gid": GID, "smb": False})
        gid_row = q("group.query", [["gid", "=", GID]]) if not DRY else [{"id": 0}]
    keys = [p.read_text().strip() for p in PUBKEYS if p.exists()]
    row = q("user.query", [["username", "=", USER]])
    fields = {
        "home": f"{HOME_PATH}/{USER}",
        "shell": "/usr/bin/bash",
        "smb": True,
        "sshpubkey": "\n".join(keys) or None,
        "ssh_password_enabled": False,
    }
    if row:
        log(f"user {USER}: update")
        mc("user.update", row[0]["id"], fields)
        return
    log(f"user {USER}: create uid {UID}")
    mc(
        "user.create",
        {
            "username": USER,
            "full_name": USER,
            "uid": UID,
            "group": gid_row[0]["id"],
            "group_create": False,
            "home_create": True,
            "home_mode": "700",
            "password": dan_password(create=not DRY) or "<generated on apply>",
            **{
                **fields,
                "home": HOME_PATH,
            },  # create: home = PARENT dir, dan/ is made under it
        },
    )


def step_smb() -> None:
    want = {
        "name": SHARE,
        "path": HOME_PATH,
        "enabled": True,
        "browsable": True,
        "purpose": "LEGACY_SHARE",
        "options": {"purpose": "LEGACY_SHARE", "shadowcopy": True, "path_suffix": "%U"},
    }
    row = q("sharing.smb.query", [["name", "=", SHARE]])
    if row:
        log(f"smb share {SHARE}: update")
        mc(
            "sharing.smb.update",
            row[0]["id"],
            {k: v for k, v in want.items() if k != "name"},
        )
    else:
        log(f"smb share {SHARE}: create")
        mc("sharing.smb.create", want)


def step_nfs() -> None:
    want = {
        "path": HOME_PATH,
        "networks": [LAN_SUBNET],
        "enabled": True,
        "mapall_user": USER,
        "mapall_group": USER,
        "expose_snapshots": True,
        "security": ["SYS"],
    }
    row = q("sharing.nfs.query", [["path", "=", HOME_PATH]])
    if row:
        log("nfs export: update")
        mc("sharing.nfs.update", row[0]["id"], want)
    else:
        log("nfs export: create")
        mc("sharing.nfs.create", want)


def step_search() -> None:
    log("smb search_protocols: SPOTLIGHT (truesearchd)")
    mc("smb.update", {"search_protocols": ["SPOTLIGHT"]})


def step_webshare() -> None:
    # user.update silently drops builtin groups (returns groups: []); group.update keeps them.
    grp = q("group.query", [["group", "=", WEBSHARE_GROUP]])
    user = q("user.query", [["username", "=", USER]])
    if grp and user and user[0]["id"] not in grp[0]["users"]:
        log(f"group {WEBSHARE_GROUP}: add {USER}")
        mc("group.update", grp[0]["id"], {"users": [*grp[0]["users"], user[0]["id"]]})
    want = {"name": SHARE, "path": HOME_PATH, "is_home_base": True}
    row = q("sharing.webshare.query", [["name", "=", SHARE]])
    if row:
        log(f"webshare {SHARE}: update")
        mc(
            "sharing.webshare.update",
            row[0]["id"],
            {"path": HOME_PATH, "is_home_base": True},
        )
    else:
        log(f"webshare {SHARE}: create")
        mc("sharing.webshare.create", want)


SERVICES = ("cifs", "nfs", "webshare")


def step_services() -> None:
    for svc in SERVICES:
        mc("service.update", svc, {"enable": True})
        mc("service.control", "START", svc, {}, job=True)
        log(f"service {svc}: enabled + started")


def step_snapshots() -> None:
    have = q("pool.snapshottask.query", [["dataset", "=", DATASET]])
    for t in SNAP_TASKS:
        body = {
            "dataset": DATASET,
            "recursive": True,
            "enabled": True,
            "allow_empty": False,
            **t,
        }
        row = [h for h in have if h["naming_schema"] == t["naming_schema"]]
        if row:
            log(f"snapshot task {t['naming_schema']}: update")
            mc("pool.snapshottask.update", row[0]["id"], body)
        else:
            log(f"snapshot task {t['naming_schema']}: create")
            mc("pool.snapshottask.create", body)


STEPS = [
    step_pool,
    step_dataset,
    step_unlock,
    step_user,
    step_smb,
    step_nfs,
    step_search,
    step_webshare,
    step_services,
    step_snapshots,
]


def cmd_apply(plan: bool) -> int:
    global DRY
    DRY = plan
    for s in STEPS:
        log(f"== {s.__name__}")
        if plan and s is not step_pool:
            # plan reads the box only for the pool step; later steps print their writes
            pass
        s()
    log("apply: done" if not plan else "plan: done (nothing changed)")
    return 0


# ---- verify -----------------------------------------------------------------------------
def sh(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    log("+ " + " ".join(cmd))
    return subprocess.run(cmd, capture_output=True, text=True, timeout=30, **kw)


# Spotlight: `index --once` walks dan's home (a few files) well under 1 s, then exits.
# Poll 0.5 s x 20 = 10 s cap; expiry -> FAIL naming the probe and the last mdsearch output.
SPOTLIGHT_POLL_S, SPOTLIGHT_POLLS = 0.5, 20


def spotlight_probe(
    ip: str, base: list[str], auth: Path, local: Path
) -> tuple[bool, str]:
    """Put a probe via SMB, force a rescan (indexer otherwise walks every 600 s), mdsearch it."""
    probe = f"awto-spotlight-{pysecrets.token_hex(4)}.txt"
    sh(base + ["-c", f"put {local} {probe}"])
    try:
        sh(_box.ssh_argv(ip, cmd=["truesearchd", "index", "--once"], batch=True))
        out = ""
        for _ in range(SPOTLIGHT_POLLS):
            r = sh(
                ["mdsearch", "-A", str(auth), ip, SHARE, f'kMDItemFSName=="{probe}"']
            )
            out = (r.stdout + r.stderr).strip()
            if probe in r.stdout:
                return True, ""
            time.sleep(SPOTLIGHT_POLL_S)
        return (
            False,
            f"{probe} not found after {SPOTLIGHT_POLL_S * SPOTLIGHT_POLLS:.0f}s: {out[-200:]}",
        )
    finally:
        sh(base + ["-c", f"del {probe}"])


def webshare_shares(ip: str, pw: str) -> list[str]:
    """Log in to awto-webshare as dan, return the share names it lists ([] on any failure)."""
    ctx = ssl.create_default_context()
    ctx.check_hostname, ctx.verify_mode = False, ssl.CERT_NONE  # self-signed on the box
    url = f"https://{ip}:{WEBSHARE_PORT}"
    hdr = {"X-WebShare": "1", "Content-Type": "application/json"}
    body = json.dumps({"username": USER, "password": pw}).encode()
    try:
        req = urllib.request.Request(f"{url}/api/login", body, hdr, method="POST")
        with urllib.request.urlopen(req, context=ctx, timeout=CALL_TIMEOUT_S) as r:
            cookie = r.headers["Set-Cookie"].split(";")[0]
        req = urllib.request.Request(f"{url}/api/shares", headers={"Cookie": cookie})
        with urllib.request.urlopen(req, context=ctx, timeout=CALL_TIMEOUT_S) as r:
            return [s["name"] for s in json.load(r)]
    except (OSError, KeyError, ValueError) as e:
        log(f"webshare: {e}")
        return []


def cmd_verify() -> int:
    ip = _box.require()
    fails: list[str] = []

    def check(name: str, ok: bool, detail: str = "") -> None:
        log(f"{'PASS' if ok else 'FAIL'} {name} {detail}".rstrip())
        if not ok:
            fails.append(name)

    for svc in SERVICES:
        r = q("service.query", [["service", "=", svc]])
        check(
            f"service {svc} running+enabled",
            bool(r) and r[0]["state"] == "RUNNING" and r[0]["enable"],
        )
    check(
        "snapshot tasks",
        len(q("pool.snapshottask.query", [["dataset", "=", DATASET]]))
        >= len(SNAP_TASKS),
    )
    row = home_row()
    algo = (row or {}).get("encryption_algorithm") or {}
    check(
        f"{DATASET} encrypted ({HOME_ALGO.lower()})",
        bool(row) and row["encrypted"] and algo.get("value") == HOME_ALGO,
        str(algo.get("value")),
    )
    check(
        "key loaded (unlocked)",
        bool(row) and row["key_loaded"] and not row["locked"],
    )
    check("secrets key section present", bool(home_key(create=False)))
    nfs = q("sharing.nfs.query", [["path", "=", HOME_PATH]])
    check("nfs expose_snapshots", bool(nfs) and nfs[0].get("expose_snapshots") is True)

    # NFS: mount here, write as uid 1000, check ownership on the box, snapshot visibility
    mnt = TMP / "nfs-home"
    mnt.mkdir(parents=True, exist_ok=True)
    marker = f"verify-{pysecrets.token_hex(4)}.txt"
    snap = f"{DATASET}@verify-{pysecrets.token_hex(3)}"
    mounted = (
        sh(["sudo", "mount", "-t", "nfs", f"{ip}:{HOME_PATH}", str(mnt)]).returncode
        == 0
    )
    check("nfs mount", mounted)
    if mounted:
        try:
            p = mnt / USER / marker
            if os.getuid() == UID:
                p.write_text("nfs-ok")
                check("nfs write+read", p.read_text() == "nfs-ok")
                st = mc("filesystem.stat", f"{HOME_PATH}/{USER}/{marker}", write=False)
                owner = st.get("user") if st else None
                check("nfs file owned by dan on box", owner == USER, owner)
            else:
                check("nfs write as uid 1000", False, f"running as uid {os.getuid()}")
            mc("pool.snapshot.create", {"dataset": DATASET, "name": snap.split("@")[1]})
            names = sh(["ls", str(mnt / ".zfs" / "snapshot")]).stdout
            check("nfs .zfs/snapshot lists snapshot", snap.split("@")[1] in names)
        finally:
            sh(["rm", "-f", str(mnt / USER / marker)])
            sh(["sudo", "umount", str(mnt)])
            try:
                mc("pool.snapshot.delete", snap)
            except RuntimeError as e:
                log(f"snapshot cleanup: {e}")

    # SMB: authfile in tmp/, mode 600, removed after
    pw = dan_password(create=False)
    check("secrets section present", bool(pw))
    if pw:
        auth = TMP / "smb-auth"
        auth.write_text(f"username = {USER}\npassword = {pw}\n")
        auth.chmod(0o600)
        local = TMP / "smb-test.txt"
        local.write_text("smb-ok")
        back = TMP / "smb-back.txt"
        try:
            base = ["smbclient", f"//{ip}/{SHARE}", "-A", str(auth)]
            check("smb list", sh(base + ["-c", "ls"]).returncode == 0)
            r = sh(
                base
                + [
                    "-c",
                    f"put {local} smb-test.txt; get smb-test.txt {back}; del smb-test.txt",
                ]
            )
            check(
                "smb put/get",
                r.returncode == 0 and back.exists() and back.read_text() == "smb-ok",
            )
            hits = spotlight_probe(ip, base, auth, local)
            check("spotlight (truesearchd) finds probe", hits[0], hits[1])
            ws = webshare_shares(ip, pw)
            check("webshare login + list", SHARE in ws, str(ws))
        finally:
            for f in (auth, local, back):
                f.unlink(missing_ok=True)
    log("verify: " + ("OK" if not fails else f"FAILED {fails}"))
    return 1 if fails else 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    ap.add_argument("cmd", choices=["plan", "apply", "verify", "call", "key-init"])
    ap.add_argument(
        "args", nargs="*", help="call: METHOD [JSON param ...], e.g. system.info"
    )
    ap.add_argument("--job", action="store_true", help="call: wait for the job result")
    a = ap.parse_args()
    if a.cmd == "verify":
        return cmd_verify()
    if a.cmd == "key-init":  # key must be in secrets.yaml before it encrypts anything
        log(
            f"key section {HOME_KEY_SECTION}: {'present' if home_key(create=True) else 'MISSING'}"
        )
        return 0
    if a.cmd == "call":
        if not a.args:
            ap.error("call needs METHOD")
        res = connect().call(a.args[0], *(json.loads(p) for p in a.args[1:]), job=a.job)
        log(json.dumps(res, indent=2, default=str))
        return 0
    return cmd_apply(plan=a.cmd == "plan")


if __name__ == "__main__":
    sys.exit(main())
