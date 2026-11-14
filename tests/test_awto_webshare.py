# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""awto-webshare (truenas/awto/awto-webshare, #357): config, login policy with a fake PAM, file ops, escapes."""

from __future__ import annotations

import asyncio
import functools
import grp
import json
import os
import sys
from pathlib import Path

import pytest

pytest.importorskip("aiohttp")
from aiohttp.test_utils import TestClient, TestServer  # noqa: E402

PKG = Path(__file__).resolve().parent.parent / "truenas" / "awto" / "awto-webshare"
sys.path.insert(0, str(PKG))

from awto_webshare import config as cfgmod  # noqa: E402
from awto_webshare.app import State, make_app  # noqa: E402
from awto_webshare.pam import Identity  # noqa: E402


def arun(fn):
    """No pytest-asyncio on the gate host: run each async test in its own loop."""

    @functools.wraps(fn)
    def wrapper(env):
        asyncio.run(fn(env))

    return wrapper


H = {"X-WebShare": "1"}
MY_GROUP = grp.getgrgid(os.getgid()).gr_name


def write_cfg(tmp: Path, shares, groups) -> str:
    (tmp / "ws.json").write_text(json.dumps({"shares": shares}))
    auth = tmp / "auth.json"
    auth.write_text(json.dumps({
        "pam_service_name": "webshare", "allowed_groups": groups,
        "webshare_config_path": str(tmp / "ws.json"), "proxy": {"port": 755, "bind_addrs": ["::"]},
    }))  # fmt: skip
    return str(auth)


@pytest.fixture
def env(tmp_path):
    data = tmp_path / "data"
    (data / "sub").mkdir(parents=True)
    (data / "a.txt").write_text("0123456789")
    (tmp_path / "outside.txt").write_text("secret")
    os.symlink(tmp_path / "outside.txt", data / "link")
    os.symlink(tmp_path, data / "dirlink")
    (tmp_path / "homes").mkdir()
    (tmp_path / "homes" / "alice").mkdir()
    auth = write_cfg(
        tmp_path,
        [{"name": "data", "path": str(data), "is_home_base": False},
         {"name": "homes", "path": str(tmp_path / "homes"), "is_home_base": True}],
        [MY_GROUP],
    )  # fmt: skip

    def fake_pam(service, user, pw):
        assert service == "webshare"
        if pw != "good":
            return None
        gids = (
            [os.getgid()] if user == "alice" else [999999]
        )  # bob is in no allowed group
        return Identity(user, os.getuid(), os.getgid(), gids)

    state = State(lambda: cfgmod.load(auth), fake_pam)
    return state, data, tmp_path


async def client(state):
    c = TestClient(TestServer(make_app(state)))
    await c.start_server()
    return c


async def login(c, user="alice", pw="good"):
    return await c.post(
        "/api/login", json={"username": user, "password": pw}, headers=H
    )


@arun
async def test_login_policy(env):
    state, *_ = env
    c = await client(state)
    try:
        assert (await c.get("/api/shares")).status == 401
        assert (await login(c, pw="bad")).status == 401
        assert (
            await login(c, user="bob")
        ).status == 401  # PAM ok, not in allowed group
        assert (
            await c.post("/api/login", json={"username": "alice", "password": "good"})
        ).status == 403  # no CSRF header
        r = await login(c)
        assert (
            r.status == 200
            and "HttpOnly" in r.headers["Set-Cookie"]
            and "SameSite=Strict" in r.headers["Set-Cookie"]
        )
        assert [s["name"] for s in await (await c.get("/api/shares")).json()] == [
            "data",
            "homes",
        ]
        await c.post("/api/logout", headers=H)
        assert (await c.get("/api/shares")).status == 401
    finally:
        await c.close()


@arun
async def test_login_rate_limit(env):
    state, *_ = env
    c = await client(state)
    try:
        for _ in range(10):
            assert (await login(c, pw="bad")).status == 401
        assert (await login(c)).status == 429
    finally:
        await c.close()


@arun
async def test_list_download_range(env):
    state, data, _ = env
    c = await client(state)
    try:
        await login(c)
        j = await (await c.get("/api/list/data")).json()
        by = {e["name"]: e for e in j["entries"]}
        assert (
            by["a.txt"]["size"] == 10
            and by["a.txt"]["type"] == "file"
            and by["sub"]["type"] == "dir"
        )
        assert (await c.get("/api/list/data/sub")).status == 200
        assert (await c.get("/api/list/data/nope")).status == 404
        assert (await c.get("/api/list/nope")).status == 404
        r = await c.get("/api/dl/data/a.txt")
        assert r.status == 200 and await r.read() == b"0123456789"
        r = await c.get("/api/dl/data/a.txt", headers={"Range": "bytes=2-4"})
        assert (
            r.status == 206
            and await r.read() == b"234"
            and r.headers["Content-Range"] == "bytes 2-4/10"
        )
        r = await c.get("/api/dl/data/a.txt", headers={"Range": "bytes=-3"})
        assert r.status == 206 and await r.read() == b"789"
        assert (
            await c.get("/api/dl/data/a.txt", headers={"Range": "bytes=50-"})
        ).status == 416
        assert (await c.get("/api/dl/data/sub")).status == 400
    finally:
        await c.close()


@arun
async def test_write_ops(env):
    state, data, _ = env
    c = await client(state)
    try:
        await login(c)
        assert (
            await c.put("/api/ul/data/sub/new.bin", data=b"x" * 3_000_000, headers=H)
        ).status == 200
        assert (data / "sub" / "new.bin").read_bytes() == b"x" * 3_000_000
        assert (
            await c.put("/api/ul/data/sub/new.bin", data=b"y", headers=H)
        ).status == 409  # exists
        assert (
            await c.put("/api/ul/data/sub/new.bin?overwrite=1", data=b"y", headers=H)
        ).status == 200
        assert (data / "sub" / "new.bin").read_bytes() == b"y"
        assert not [p for p in (data / "sub").iterdir() if "upload" in p.name]
        assert (
            await c.put("/api/ul/data/sub/x", data=b"y")
        ).status == 403  # no CSRF header
        assert (await c.post("/api/mkdir/data/d2", headers=H)).status == 200 and (
            data / "d2"
        ).is_dir()
        assert (await c.post("/api/mkdir/data/d2", headers=H)).status == 409
        r = await c.post(
            "/api/rename/data/sub/new.bin", json={"to": "d2/moved.bin"}, headers=H
        )
        assert r.status == 200 and (data / "d2" / "moved.bin").exists()
        assert (await c.delete("/api/rm/data/d2", headers=H)).status == 409  # not empty
        assert (await c.delete("/api/rm/data/d2/moved.bin", headers=H)).status == 200
        assert (await c.delete("/api/rm/data/d2", headers=H)).status == 200
        assert not (data / "d2").exists()
    finally:
        await c.close()


@arun
async def test_cannot_escape_share(env):
    state, data, tmp = env
    c = await client(state)
    try:
        await login(c)
        for rel in (
            "..%2Foutside.txt",
            "link",
            "dirlink/outside.txt",
            "sub/../../outside.txt",
        ):
            r = await c.get(f"/api/dl/data/{rel}")
            assert r.status in (400, 403, 404), rel
        assert (await c.get("/api/list/data/dirlink")).status in (400, 403)
        assert (
            await c.put("/api/ul/data/dirlink/pwn", data=b"x", headers=H)
        ).status in (400, 403)
        assert not (tmp / "pwn").exists()
        assert (
            await c.post("/api/rename/data/a.txt", json={"to": "../stolen"}, headers=H)
        ).status == 400
    finally:
        await c.close()


@arun
async def test_home_base_share(env):
    state, data, tmp = env
    c = await client(state)
    try:
        await login(c)
        assert (await c.put("/api/ul/homes/hi.txt", data=b"h", headers=H)).status == 200
        assert (tmp / "homes" / "alice" / "hi.txt").read_text() == "h"
    finally:
        await c.close()


def test_libpam_rejects_unknown_user():
    from awto_webshare.pam import PamAuth

    try:
        pam = PamAuth("login")
    except OSError:
        pytest.skip("libpam not available")
    assert pam.authenticate("no-such-user-awto", "x") is None
