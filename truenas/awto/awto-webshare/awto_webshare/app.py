# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""aiohttp app: login (PAM + group), session cookie, list/download/upload/mkdir/rm/rename within shares.

- All file I/O goes through worker.py in a child dropped to the user's uid/gid/groups (root only),
  so ZFS ACLs and modes decide; this process never touches share files itself.
- Not in v1: share links, MCP, passkeys, URL downloads, zip, search (design: awtoau/awto-unvr#357).
"""

from __future__ import annotations

import asyncio
import errno
import json
import os
import secrets
import sys
import time
from importlib import resources
from urllib.parse import quote

from aiohttp import web

from . import config as cfgmod
from .pam import Identity, in_allowed_group

SESSION_IDLE = 3600  # s; an abandoned browser tab must not keep a PAM session open
LOGIN_FAILS_MAX, LOGIN_FAIL_WINDOW = 10, 60  # per client IP per minute
CSRF_HEADER = "X-WebShare"  # SameSite=Strict cookie plus this on every non-GET
# cwd is "/", so the package must be importable by path (also when running from a source tree)
WORKER_ENV = {
    "PYTHONPATH": os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "PATH": "/usr/bin:/bin",
}
HTTP_ERR = {
    errno.ENOENT: 404, errno.EACCES: 403, errno.EPERM: 403, errno.EEXIST: 409, errno.ENOTDIR: 400,
    errno.EISDIR: 400, errno.EINVAL: 400, errno.ELOOP: 403, errno.ENOTEMPTY: 409, errno.EROFS: 403,
    errno.ENOSPC: 507, errno.EDQUOT: 507,
}  # fmt: skip


class Session:
    def __init__(self, ident: Identity):
        self.ident, self.touched = ident, time.monotonic()


class State:
    def __init__(self, cfg_loader, authenticator):
        self.load = cfg_loader
        self.auth = authenticator
        self._cfg, self._cfg_mtime = None, -1.0
        self.sessions: dict[str, Session] = {}
        self.fails: dict[str, list[float]] = {}

    def cfg(self) -> cfgmod.Config:
        if (
            self._cfg is None
            or getattr(self.load, "mtime", lambda: 0)() != self._cfg_mtime
        ):
            self._cfg = self.load()
            self._cfg_mtime = getattr(self.load, "mtime", lambda: 0)()
        return self._cfg

    def session(self, request: web.Request) -> Session:
        tok = request.cookies.get("ws_session", "")
        s = self.sessions.get(tok)
        now = time.monotonic()
        if s is None or now - s.touched > SESSION_IDLE:
            if s is not None:
                self.drop(tok)
            raise web.HTTPUnauthorized(text="login required")
        s.touched = now
        return s

    def drop(self, tok: str) -> None:
        s = self.sessions.pop(tok, None)
        if s:
            s.ident.close()


def share_root(cfg: cfgmod.Config, sess: Session, name: str) -> str:
    sh = cfg.share(name)
    if sh is None:
        raise web.HTTPNotFound(text="no such share")
    return os.path.join(sh.path, sess.ident.username) if sh.is_home_base else sh.path


async def run_worker(
    sess: Session, args: list[str], stdin: bytes | None = None, pipe_out: bool = False
):
    kw: dict = {}
    if os.geteuid() == 0 and sess.ident.uid != 0:
        kw = {
            "user": sess.ident.uid,
            "group": sess.ident.gid,
            "extra_groups": sess.ident.gids,
        }
    proc = await asyncio.create_subprocess_exec(
        sys.executable, "-m", "awto_webshare.worker", *args,
        stdin=asyncio.subprocess.PIPE if stdin is not None or pipe_out else asyncio.subprocess.DEVNULL,
        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, cwd="/", env=WORKER_ENV, **kw,
    )  # fmt: skip
    return proc


def fail(err: bytes) -> web.HTTPException:
    try:
        e = json.loads(err)
        code = HTTP_ERR.get(e["errno"], 500)
        return _http(code, e["msg"])
    except (ValueError, KeyError):
        return _http(500, "worker failed")


def _http(code: int, text: str) -> web.HTTPException:
    cls = {400: web.HTTPBadRequest, 403: web.HTTPForbidden, 404: web.HTTPNotFound, 409: web.HTTPConflict,
           507: web.HTTPInsufficientStorage, 500: web.HTTPInternalServerError}[code]  # fmt: skip
    return cls(text=text)


async def simple(sess: Session, args: list[str]) -> dict | list:
    proc = await run_worker(sess, args)
    out, err = await proc.communicate()
    if proc.returncode:
        raise fail(err)
    return json.loads(out)


def parse_target(request: web.Request) -> tuple[str, str]:
    return request.match_info["share"], request.match_info.get("path", "")


def csrf(request: web.Request) -> None:
    if (
        request.method not in ("GET", "HEAD")
        and request.headers.get(CSRF_HEADER) != "1"
    ):
        raise web.HTTPForbidden(text="missing " + CSRF_HEADER)


def make_app(state: State) -> web.Application:
    routes = web.RouteTableDef()

    @routes.get("/")
    async def index(request):
        html = (
            resources.files("awto_webshare")
            .joinpath("static/index.html")
            .read_text("utf-8")
        )
        return web.Response(text=html, content_type="text/html")

    @routes.post("/api/login")
    async def login(request):
        csrf(request)
        ip = request.remote or "?"
        now = time.monotonic()
        recent = [t for t in state.fails.get(ip, []) if now - t < LOGIN_FAIL_WINDOW]
        state.fails[ip] = recent
        if len(recent) >= LOGIN_FAILS_MAX:
            raise web.HTTPTooManyRequests(text="too many failed logins")
        try:
            body = await request.json()
            user, pw = str(body["username"]), str(body["password"])
        except (ValueError, KeyError, TypeError):
            raise web.HTTPBadRequest(text="username and password required") from None
        cfg = state.cfg()
        ident = await asyncio.get_running_loop().run_in_executor(
            None, state.auth, cfg.pam_service, user, pw
        )
        if ident is None or not in_allowed_group(ident, cfg.allowed_groups):
            if ident is not None:
                ident.close()
            state.fails[ip].append(now)
            raise web.HTTPUnauthorized(text="login failed")
        tok = secrets.token_urlsafe(32)
        state.sessions[tok] = Session(ident)
        resp = web.json_response({"username": ident.username})
        resp.set_cookie(
            "ws_session",
            tok,
            httponly=True,
            samesite="Strict",
            secure=request.secure,
            path="/",
        )
        return resp

    @routes.post("/api/logout")
    async def logout(request):
        csrf(request)
        state.drop(request.cookies.get("ws_session", ""))
        resp = web.json_response({})
        resp.del_cookie("ws_session", path="/")
        return resp

    @routes.get("/api/shares")
    async def shares(request):
        state.session(request)
        return web.json_response(
            [{"name": s.name, "home": s.is_home_base} for s in state.cfg().shares]
        )

    @routes.get("/api/list/{share}")
    @routes.get("/api/list/{share}/{path:.*}")
    async def list_(request):
        sess = state.session(request)
        share, rel = parse_target(request)
        root = share_root(state.cfg(), sess, share)
        return web.json_response(
            {"path": rel, "entries": await simple(sess, ["list", root, rel])}
        )

    @routes.get("/api/dl/{share}/{path:.+}")
    async def download(request):
        sess = state.session(request)
        share, rel = parse_target(request)
        root = share_root(state.cfg(), sess, share)
        st = await simple(sess, ["stat", root, rel])
        if st["type"] != "file":
            raise web.HTTPBadRequest(text="not a regular file")
        size, start, end, status = st["size"], 0, st["size"], 200
        rng = request.headers.get("Range")
        if rng and rng.startswith("bytes=") and "," not in rng:
            a, _, b = rng[6:].partition("-")
            try:
                if a == "":
                    start, end = max(0, size - int(b)), size
                else:
                    start, end = int(a), (min(int(b) + 1, size) if b else size)
            except ValueError:
                raise web.HTTPBadRequest(text="bad Range") from None
            if start >= size or start >= end:
                raise web.HTTPRequestRangeNotSatisfiable(
                    headers={"Content-Range": f"bytes */{size}"}
                )
            status = 206
        name = os.path.basename(rel)
        headers = {
            "Content-Length": str(end - start),
            "Accept-Ranges": "bytes",
            "Content-Disposition": f"attachment; filename*=UTF-8''{quote(name)}",
            "Content-Type": "application/octet-stream",
            "X-Content-Type-Options": "nosniff",
        }
        if status == 206:
            headers["Content-Range"] = f"bytes {start}-{end - 1}/{size}"
        proc = await run_worker(sess, ["read", root, rel, str(start), str(end - start)])
        resp = web.StreamResponse(status=status, headers=headers)
        await resp.prepare(request)
        await proc.stdout.readline()  # the worker's JSON header line, not body
        try:
            while chunk := await proc.stdout.read(1 << 20):
                await resp.write(chunk)
        finally:
            if proc.returncode is None:
                proc.kill()
            await proc.wait()
        await resp.write_eof()
        return resp

    @routes.put("/api/ul/{share}/{path:.+}")
    async def upload(request):
        sess = state.session(request)
        csrf(request)
        share, rel = parse_target(request)
        root = share_root(state.cfg(), sess, share)
        over = "1" if request.query.get("overwrite") == "1" else "0"
        proc = await run_worker(sess, ["write", root, rel, over], stdin=b"")
        try:
            async for chunk in request.content.iter_chunked(1 << 20):
                proc.stdin.write(chunk)
                await proc.stdin.drain()
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            try:
                proc.stdin.close()
            except Exception:  # noqa: BLE001
                pass
        out, err = await proc.communicate()
        if proc.returncode:
            raise fail(err)
        return web.json_response(json.loads(out))

    @routes.post("/api/mkdir/{share}/{path:.+}")
    async def mkdir(request):
        sess = state.session(request)
        csrf(request)
        share, rel = parse_target(request)
        return web.json_response(
            await simple(sess, ["mkdir", share_root(state.cfg(), sess, share), rel])
        )

    @routes.delete("/api/rm/{share}/{path:.+}")
    async def rm(request):
        sess = state.session(request)
        csrf(request)
        share, rel = parse_target(request)
        return web.json_response(
            await simple(sess, ["rm", share_root(state.cfg(), sess, share), rel])
        )

    @routes.post("/api/rename/{share}/{path:.+}")
    async def rename(request):
        sess = state.session(request)
        csrf(request)
        share, rel = parse_target(request)
        try:
            dst = str((await request.json())["to"])
        except (ValueError, KeyError, TypeError):
            raise web.HTTPBadRequest(text="to required") from None
        return web.json_response(
            await simple(
                sess, ["rename", share_root(state.cfg(), sess, share), rel, dst]
            )
        )

    app = web.Application(
        client_max_size=0
    )  # uploads are streamed to the worker, no size cap here
    app.add_routes(routes)
    return app
