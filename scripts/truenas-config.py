#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Idempotent TrueNAS config for woomera: tank/home, user dan, SMB + NFS, snapshots (#356, #358).

  ./dev.py truenas-config plan     # print the steps, touch nothing on the box
  ./dev.py truenas-config apply    # converge the box (safe to re-run; run after every reinstall)
  ./dev.py truenas-config verify   # NFS mount + SMB put/get + snapshot listing, cleans up

- Reinstall wipes the config DB: apply imports `tank` first when it is not imported.
- dan password: ~/.config/awto/secrets.yaml section `woomera-truenas-dan`, generated once, never printed.
- Everything goes through `midclt call` over ssh as root (_box.ssh_argv).
"""

from __future__ import annotations

import argparse
import json
import os
import secrets as pysecrets
import shlex
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _net import LAN_SUBNET  # noqa: E402
from _repo import REPO, TMP, make_log  # noqa: E402

log = make_log("truenas-config")

POOL, DATASET = "tank", "tank/home"
HOME_PATH = f"/mnt/{DATASET}"
USER, UID, GID = "dan", 1000, 1000
SECRET_SECTION = "woomera-truenas-dan"
SHARE = "home"
SECRETS = Path(os.path.expanduser("~/.config/awto/secrets.yaml"))
PUBKEYS = [Path.home() / ".ssh" / n for n in ("id_ed25519.pub", "id_rsa.pub")]
# (name, cron minute/hour, lifetime) - hourly kept 2 days, daily kept 30 days
SNAP_TASKS = [
    {"naming_schema": "auto-%Y%m%d.%H%M-2d", "lifetime_value": 2, "lifetime_unit": "DAY",
     "schedule": {"minute": "0", "hour": "*", "dom": "*", "month": "*", "dow": "*"}},
    {"naming_schema": "auto-%Y%m%d.%H%M-30d", "lifetime_value": 30, "lifetime_unit": "DAY",
     "schedule": {"minute": "30", "hour": "0", "dom": "*", "month": "*", "dow": "*"}},
]
# One midclt call: local ssh + middleware round trip. Expected <2 s on the LAN; 30 s = 15x is
# the host-process backstop for a stalled middleware. Expiry: raise with the method name.
CALL_TIMEOUT_S = 30
# Jobs (pool.import_pool, dataset create with chown): expected <10 s; 120 s backstop.
JOB_TIMEOUT_S = 120

DRY = False


def mc(method: str, *args, job: bool = False, write: bool = True):
    """midclt call over ssh -> parsed JSON (None for empty output)."""
    cmd = ["midclt", "call"] + (["-job"] if job else []) + [method]
    cmd += [json.dumps(a) for a in args]
    if DRY and write:
        log(f"[plan] {method} {json.dumps(args)[:200]}")
        return None
    ip = _box.require()
    argv = _box.ssh_argv(ip, "root", [shlex.join(cmd)], batch=True)
    try:
        r = subprocess.run(
            argv, capture_output=True, text=True,
            timeout=JOB_TIMEOUT_S if job else CALL_TIMEOUT_S,
        )
    except subprocess.TimeoutExpired as e:
        raise RuntimeError(f"{method}: no answer within {e.timeout}s") from e
    if r.returncode:
        raise RuntimeError(f"{method} failed: {r.stderr.strip()[-600:]}")
    out = r.stdout.strip()
    return json.loads(out) if out else None


def q(method: str, filters: list):
    return mc(method, filters, write=False) or []


# ---- secrets (values never logged) ------------------------------------------------------
def load_secrets() -> dict:
    import yaml

    return yaml.safe_load(SECRETS.read_text()) or {}


def dan_password(create: bool) -> str | None:
    import yaml

    data = load_secrets()
    sec = data.get(SECRET_SECTION)
    if sec and sec.get("password"):
        return sec["password"]
    if not create:
        return None
    data[SECRET_SECTION] = {
        "url": "smb://woomera (LAN IP via ./dev.py ssh --print)",
        "purpose": "woomera TrueNAS user dan (SMB/web); NFS uses uid 1000",
        "username": USER,
        "password": pysecrets.token_urlsafe(24),
    }
    SECRETS.write_text(yaml.safe_dump(data, sort_keys=False))
    SECRETS.chmod(0o600)
    log(f"secrets: created section {SECRET_SECTION}")
    return data[SECRET_SECTION]["password"]


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


def step_dataset() -> None:
    if q("pool.dataset.query", [["name", "=", DATASET]]):
        log(f"dataset {DATASET}: exists")
        return
    log(f"dataset {DATASET}: create")
    mc("pool.dataset.create", {"name": DATASET, "share_type": "GENERIC"})


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
            "username": USER, "full_name": USER, "uid": UID,
            "group": gid_row[0]["id"], "group_create": False,
            "home_create": True, "home_mode": "700",
            "password": dan_password(create=not DRY) or "<generated on apply>",
            **fields,
        },
    )


def step_smb() -> None:
    want = {
        "name": SHARE, "path": HOME_PATH, "enabled": True, "browsable": True,
        "purpose": "LEGACY_SHARE",
        "options": {"purpose": "LEGACY_SHARE", "shadowcopy": True, "path_suffix": "%U"},
    }
    row = q("sharing.smb.query", [["name", "=", SHARE]])
    if row:
        log(f"smb share {SHARE}: update")
        mc("sharing.smb.update", row[0]["id"], {k: v for k, v in want.items() if k != "name"})
    else:
        log(f"smb share {SHARE}: create")
        mc("sharing.smb.create", want)


def step_nfs() -> None:
    want = {
        "path": HOME_PATH, "networks": [LAN_SUBNET], "enabled": True,
        "mapall_user": USER, "mapall_group": USER, "expose_snapshots": True,
        "security": ["SYS"],
    }
    row = q("sharing.nfs.query", [["path", "=", HOME_PATH]])
    if row:
        log("nfs export: update")
        mc("sharing.nfs.update", row[0]["id"], want)
    else:
        log("nfs export: create")
        mc("sharing.nfs.create", want)


def step_services() -> None:
    for svc in ("cifs", "nfs"):
        mc("service.update", svc, {"enable": True})
        mc("service.control", "START", svc, {}, job=True)
        log(f"service {svc}: enabled + started")


def step_snapshots() -> None:
    have = q("pool.snapshottask.query", [["dataset", "=", DATASET]])
    for t in SNAP_TASKS:
        body = {
            "dataset": DATASET, "recursive": True, "enabled": True,
            "allow_empty": False, **t,
        }
        row = [h for h in have if h["naming_schema"] == t["naming_schema"]]
        if row:
            log(f"snapshot task {t['naming_schema']}: update")
            mc("pool.snapshottask.update", row[0]["id"], body)
        else:
            log(f"snapshot task {t['naming_schema']}: create")
            mc("pool.snapshottask.create", body)


STEPS = [step_pool, step_dataset, step_user, step_smb, step_nfs, step_services, step_snapshots]


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


def cmd_verify() -> int:
    ip = _box.require()
    fails: list[str] = []

    def check(name: str, ok: bool, detail: str = "") -> None:
        log(f"{'PASS' if ok else 'FAIL'} {name} {detail}".rstrip())
        if not ok:
            fails.append(name)

    for svc in ("cifs", "nfs"):
        r = q("service.query", [["service", "=", svc]])
        check(f"service {svc} running+enabled", bool(r) and r[0]["state"] == "RUNNING" and r[0]["enable"])
    check("snapshot tasks", len(q("pool.snapshottask.query", [["dataset", "=", DATASET]])) >= len(SNAP_TASKS))
    nfs = q("sharing.nfs.query", [["path", "=", HOME_PATH]])
    check("nfs expose_snapshots", bool(nfs) and nfs[0].get("expose_snapshots") is True)

    # NFS: mount here, write as uid 1000, check ownership on the box, snapshot visibility
    mnt = TMP / "nfs-home"
    mnt.mkdir(parents=True, exist_ok=True)
    marker = f"verify-{pysecrets.token_hex(4)}.txt"
    snap = f"{DATASET}@verify-{pysecrets.token_hex(3)}"
    mounted = sh(["sudo", "mount", "-t", "nfs", f"{ip}:{HOME_PATH}", str(mnt)]).returncode == 0
    check("nfs mount", mounted)
    if mounted:
        try:
            p = mnt / USER / marker
            if os.getuid() == UID:
                p.write_text("nfs-ok")
                check("nfs write+read", p.read_text() == "nfs-ok")
                owner = subprocess.run(
                    _box.ssh_argv(ip, "root", [f"stat -c %U {shlex.quote(f'{HOME_PATH}/{USER}/{marker}')}"], batch=True),
                    capture_output=True, text=True, timeout=CALL_TIMEOUT_S,
                ).stdout.strip()
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
            r = sh(base + ["-c", f"put {local} smb-test.txt; get smb-test.txt {back}; del smb-test.txt"])
            check("smb put/get", r.returncode == 0 and back.exists() and back.read_text() == "smb-ok")
        finally:
            for f in (auth, local, back):
                f.unlink(missing_ok=True)
    log("verify: " + ("OK" if not fails else f"FAILED {fails}"))
    return 1 if fails else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("cmd", choices=["plan", "apply", "verify"])
    a = ap.parse_args()
    if a.cmd == "verify":
        return cmd_verify()
    return cmd_apply(plan=a.cmd == "plan")


if __name__ == "__main__":
    sys.exit(main())
