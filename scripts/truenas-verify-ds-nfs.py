#!/usr/bin/env python3
# Copyright (C) Awto / Daniel Tyrrell
# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify the TrueNAS middleware patches on woomera (#356 phase C).

- nfs:  file-backed pool + share with expose_snapshots, mount from here, list .zfs/snapshot
- ldap: throwaway OpenLDAP (podman) + directoryservices LDAP join + ds_auth=true + login as LDAP user
- cleanup: remove everything either phase created (idempotent)

Usage: truenas-verify-ds-nfs.py {nfs|ldap|cleanup|all}
Log: tmp/logs/truenas-verify-ds-nfs.log
"""
import json
import os
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(os.environ.get("UNVR_REPO") or subprocess.run(
    ["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True,
    cwd=Path(__file__).resolve().parent).stdout.strip() or Path(__file__).resolve().parents[1])
LOG = REPO / "tmp/logs/truenas-verify-ds-nfs.log"
MNT = REPO / "tmp/nfsmnt"
IMG = "/var/tmp/awto-nfstest.img"
POOL, DS, SNAP = "awtotest", "awtotest/ds", "s1"
CT = "awto-ldaptest"
LDAP_PORT = 13389  # plain ldap; osixia ldaps cert is rejected by the box client ("No certificate was found")
BASE = "dc=awto,dc=test"
USER, PW = "alice", "alicepw-test"
GID = 20000
HOST_IP = "192.168.25.31"
SSH_TIMEOUT = 120  # a midclt job (pool/nfs/ds join) finishes in <30 s; 4x


def log(msg):
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(msg + "\n")
    print(msg, flush=True)


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=kw.pop("timeout", SSH_TIMEOUT), **kw)
    out = (r.stdout + r.stderr).strip()
    log(f"$ {' '.join(cmd)[:300]}\n{out}")
    return r.returncode, out


def box(cmd):
    rc, out = run([str(REPO / "dev.py"), "ssh", "--", cmd], cwd=REPO)
    return rc, "\n".join(l for l in out.splitlines() if "[dev.py]" not in l and not l.startswith("# woomera"))


def mc(method, *args):
    a = " ".join("'" + json.dumps(x) + "'" for x in args)
    return box(f"midclt call {method} {a}")


def job(method, *args):
    rc, out = mc(method, *args)
    return box(f"midclt call core.job_wait {out.splitlines()[-1].strip()}") if rc == 0 else (rc, out)  # job_wait rc!=0 on job failure


def result(name, ok, evidence):
    log(f"RESULT {name}: {'PASS' if ok else 'FAIL'} :: {evidence}")
    return ok


def nfs():
    box(f"zpool list {POOL} >/dev/null 2>&1 || (truncate -s 2G {IMG} && zpool create -f -m /mnt/{POOL} {POOL} {IMG})")
    box(f"zfs list {DS} >/dev/null 2>&1 || (zfs create {DS} && echo hello > /mnt/{DS}/f.txt && zfs snapshot {DS}@{SNAP})")
    # middleware only accepts share paths under pools in storage.volume; register the file pool by hand.
    box("midclt call datastore.query storage.volume '[[\"vol_name\",\"=\",\"awtotest\"]]' | grep -q awtotest || "
        "midclt call datastore.insert storage.volume '{\"vol_name\":\"awtotest\",\"vol_guid\":\"0\"}'")
    rc, share = mc("sharing.nfs.create", {"path": f"/mnt/{DS}", "expose_snapshots": True,
                                           "networks": ["192.168.25.0/24"], "maproot_user": "root",
                                           "maproot_group": "root"})
    ok_create = rc == 0 and '"expose_snapshots": true' in share
    result("nfs.create expose_snapshots (no licence error)", ok_create, share)
    job("service.control", "START", "nfs", {})
    _, exports = box("cat /etc/exports")
    ok_exp = "zfs_snapdir" in exports
    result("/etc/exports zfs_snapdir", ok_exp, exports.replace("\n", " "))
    MNT.mkdir(parents=True, exist_ok=True)
    run(["sudo", "mount", "-t", "nfs", f"192.168.25.40:/mnt/{DS}", str(MNT)])
    rc, ls = run(["ls", f"{MNT}/.zfs/snapshot/{SNAP}/"])
    return ok_create and ok_exp and result("client .zfs/snapshot listing", "f.txt" in ls, ls)


def ldap():
    run(["podman", "rm", "-f", CT])
    run(["podman", "run", "-d", "--name", CT, "-p", f"{LDAP_PORT}:389", "-e", "LDAP_ORGANISATION=awto",
         "-e", "LDAP_DOMAIN=awto.test", "-e", "LDAP_ADMIN_PASSWORD=adminpw", "-e", "LDAP_TLS=false",
         "docker.io/osixia/openldap:1.5.0"])
    for _ in range(30):  # container init ~5 s; poll 1 s steps via ldapsearch retry
        rc, _o = run(["podman", "exec", CT, "ldapsearch", "-x", "-H", "ldap://localhost", "-b", BASE,
                      "-D", f"cn=admin,{BASE}", "-w", "adminpw", "-s", "base"])
        if rc == 0:
            break
        time.sleep(1)
    ldif = f"""dn: ou=people,{BASE}
objectClass: organizationalUnit
ou: people

dn: ou=groups,{BASE}
objectClass: organizationalUnit
ou: groups

dn: cn=tnadmins,ou=groups,{BASE}
objectClass: posixGroup
cn: tnadmins
gidNumber: {GID}
memberUid: {USER}

dn: uid={USER},ou=people,{BASE}
objectClass: inetOrgPerson
objectClass: posixAccount
objectClass: shadowAccount
uid: {USER}
cn: Alice Test
sn: Test
uidNumber: 20001
gidNumber: {GID}
homeDirectory: /var/empty
loginShell: /usr/sbin/nologin
userPassword: {PW}
"""
    p = subprocess.run(["podman", "exec", "-i", CT, "ldapadd", "-x", "-H", "ldap://localhost",
                        "-D", f"cn=admin,{BASE}", "-w", "adminpw"], input=ldif, capture_output=True, text=True)
    log(f"ldapadd rc={p.returncode} {p.stdout}{p.stderr}")

    rc, out = mc("system.general.update", {"ds_auth": True})
    rc2, cfg = mc("system.general.config")
    ok_auth = '"ds_auth": true' in cfg and "licen" not in out.lower() and "entitle" not in out.lower()
    result("system.general.update ds_auth=true", ok_auth, f"update: {out[:200]} | config ds_auth true={'\"ds_auth\": true' in cfg}")

    cfgd = {"service_type": "LDAP", "enable": True,
            "credential": {"credential_type": "LDAP_PLAIN", "binddn": f"cn=admin,{BASE}", "bindpw": "adminpw"},
            "configuration": {"server_urls": [f"ldap://{HOST_IP}:{LDAP_PORT}"], "basedn": BASE,
                              "validate_certificates": False,
                              # sssd refuses password auth over plain ldap:// without this
                              "auxiliary_parameters": "ldap_auth_disable_tls_never_use_in_production = true"}}
    rc, out = job("directoryservices.update", cfgd)
    result("directoryservices.update LDAP", rc == 0, out[:300])
    _, st = mc("directoryservices.status")
    log(f"ds status: {st}")
    _, gl = mc("group.query", [["name", "=", "tnadmins"]], {"extra": {"search_dscache": True}})
    log(f"group lookup: {gl[:300]}")
    rc, pv = mc("privilege.create", {"name": "awto-ldaptest", "ds_groups": [GID], "local_groups": [],
                                      "roles": ["FULL_ADMIN"], "web_shell": False})
    log(f"privilege.create: {pv[:300]}")
    # login as LDAP user over the websocket API from this host (basic auth REST is fallback)
    rc, me = run(["curl", "-sk", "-m", "20", "-u", f"{USER}:{PW}", "https://192.168.25.40/api/v2.0/auth/me"])
    ok_login = rc == 0 and USER in me
    if not ok_login:
        rc, me = ws_login()
        ok_login = USER in me
    return ok_auth and result("login as LDAP user", ok_login, me[:300])


def ws_login():
    code = (
        "import asyncio,json,ssl,sys\n"
        "import websockets\n"
        "async def m():\n"
        " c=ssl.create_default_context();c.check_hostname=False;c.verify_mode=ssl.CERT_NONE\n"
        " async with websockets.connect('wss://192.168.25.40/api/current',ssl=c) as w:\n"
        "  async def call(i,meth,p):\n"
        "   await w.send(json.dumps({'jsonrpc':'2.0','id':i,'method':meth,'params':p}));return json.loads(await w.recv())\n"
        f"  print(json.dumps(await call(1,'auth.login_ex',[{{'mechanism':'PASSWORD_PLAIN','username':'{USER}','password':'{PW}'}}])))\n"
        "  print(json.dumps(await call(2,'auth.me',[])))\n"
        "asyncio.run(m())\n")
    return run([sys.executable, "-c", code])


def cleanup():
    _, pv = mc("privilege.query", [["name", "=", "awto-ldaptest"]])
    try:
        for p in json.loads(pv or "[]"):
            mc("privilege.delete", p["id"])
    except ValueError:
        pass
    mc("system.general.update", {"ds_auth": False})
    job("directoryservices.update", {"service_type": None, "enable": False, "credential": None,
                                      "configuration": None})
    run(["podman", "rm", "-f", CT])
    _, shares = mc("sharing.nfs.query", [["path", "=", f"/mnt/{DS}"]])
    try:
        for s in json.loads(shares or "[]"):
            mc("sharing.nfs.delete", s["id"])
    except ValueError:
        pass
    run(["sudo", "umount", str(MNT)])
    box("midclt call datastore.query storage.volume '[[\"vol_name\",\"=\",\"awtotest\"]]'")
    _, vol = mc("datastore.query", "storage.volume", [["vol_name", "=", POOL]])
    try:
        for v in json.loads(vol or "[]"):
            mc("datastore.delete", "storage.volume", v["id"])
    except ValueError:
        pass
    box(f"zpool destroy -f {POOL} 2>/dev/null; rm -f {IMG}; zpool list; cat /etc/exports")
    _, g = mc("system.general.config")
    _, d = mc("directoryservices.config")
    log(f"after cleanup: ds_auth false={'\"ds_auth\": false' in g}; ds={d[:200]}")


if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    ok = True
    if what in ("nfs", "all"):
        ok &= nfs()
    if what in ("ldap", "all"):
        ok &= ldap()
    if what in ("cleanup", "all"):
        cleanup()
    sys.exit(0 if ok else 1)
