# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""libpam via ctypes (no extra Debian dependency) + the login policy: PAM, then group membership."""

from __future__ import annotations

import ctypes
import ctypes.util
import grp
import os
import pwd
from dataclasses import dataclass, field

PAM_SUCCESS = 0
PAM_PROMPT_ECHO_OFF, PAM_PROMPT_ECHO_ON = 1, 2


class _Msg(ctypes.Structure):
    _fields_ = [("style", ctypes.c_int), ("msg", ctypes.c_char_p)]


class _Resp(ctypes.Structure):
    _fields_ = [("resp", ctypes.c_void_p), ("retcode", ctypes.c_int)]


_CONV = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.POINTER(_Msg)),
    ctypes.POINTER(ctypes.POINTER(_Resp)),
    ctypes.c_void_p,
)


@dataclass
class Identity:
    username: str
    uid: int
    gid: int
    gids: list[int] = field(default_factory=list)
    _close: object = None

    def close(self) -> None:
        if self._close:
            self._close()
            self._close = None


def identity_of(username: str) -> Identity | None:
    try:
        pw = pwd.getpwnam(username)
    except KeyError:
        return None
    return Identity(
        pw.pw_name, pw.pw_uid, pw.pw_gid, os.getgrouplist(pw.pw_name, pw.pw_gid)
    )


def in_allowed_group(ident: Identity, allowed: tuple[str, ...]) -> bool:
    names = set()
    for gid in ident.gids:
        try:
            names.add(grp.getgrgid(gid).gr_name)
        except KeyError:
            pass
    return bool(names & set(allowed))


class PamAuth:
    """authenticate(username, password) -> Identity | None. Group policy is the caller's (app.py)."""

    def __init__(self, service: str):
        self.service = service
        self.pam = ctypes.CDLL(ctypes.util.find_library("pam") or "libpam.so.0")
        self.libc = ctypes.CDLL(ctypes.util.find_library("c") or "libc.so.6")
        self.libc.calloc.restype = ctypes.c_void_p
        self.libc.strdup.restype = ctypes.c_void_p
        self.pam.pam_start.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]

    def authenticate(self, username: str, password: str) -> Identity | None:
        pw = password.encode()

        @_CONV
        def conv(n, msgs, resp, _data):
            arr = self.libc.calloc(n, ctypes.sizeof(_Resp))
            resp[0] = ctypes.cast(arr, ctypes.POINTER(_Resp))
            for i in range(n):
                if msgs[i].contents.style in (PAM_PROMPT_ECHO_OFF, PAM_PROMPT_ECHO_ON):
                    resp[0][i].resp = self.libc.strdup(pw)
            return PAM_SUCCESS

        handle = ctypes.c_void_p()
        cstruct = ctypes.create_string_buffer(ctypes.sizeof(ctypes.c_void_p) * 2)
        ctypes.cast(cstruct, ctypes.POINTER(ctypes.c_void_p))[0] = ctypes.cast(
            conv, ctypes.c_void_p
        )
        if (
            self.pam.pam_start(
                self.service.encode(), username.encode(), cstruct, ctypes.byref(handle)
            )
            != PAM_SUCCESS
        ):
            return None
        ok = (
            self.pam.pam_authenticate(handle, 0) == PAM_SUCCESS
            and self.pam.pam_acct_mgmt(handle, 0) == PAM_SUCCESS
        )
        ident = identity_of(username) if ok else None
        if ident is None:
            self.pam.pam_end(handle, 0)
            return None
        # session is listed by middleware (truenas-session) until closed
        opened = self.pam.pam_open_session(handle, 0) == PAM_SUCCESS

        def close(
            handle=handle, conv=conv, cstruct=cstruct
        ):  # keep conv/cstruct alive until pam_end
            if opened:
                self.pam.pam_close_session(handle, 0)
            self.pam.pam_end(handle, 0)

        ident._close = close
        return ident
