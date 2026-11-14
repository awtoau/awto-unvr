# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""One filesystem operation, run as the logged-in user (the parent spawns this under Popen(user=,group=)).

- Every path is resolved component by component with O_NOFOLLOW from an fd of the share root, so a
  symlink or `..` cannot leave the share (userspace RESOLVE_BENEATH).
- argv: op root rel [arg]. list/stat -> JSON on stdout. read: JSON header line + raw bytes.
  write: body on stdin. Failure: exit 1, JSON {"errno","msg"} on stderr.
"""

from __future__ import annotations

import errno
import json
import os
import stat
import sys

CHUNK = 1 << 20


def split_rel(rel: str) -> list[str]:
    parts = [p for p in rel.split("/") if p != ""]
    if any(p in (".", "..") for p in parts) or "\0" in rel:
        raise OSError(errno.EINVAL, "bad path")
    return parts


def open_dir_beneath(root: str, parts: list[str]) -> int:
    fd = os.open(root, os.O_RDONLY | os.O_DIRECTORY)
    try:
        for p in parts:
            nfd = os.open(p, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=fd)
            os.close(fd)
            fd = nfd
        return fd
    except BaseException:
        os.close(fd)
        raise


def _entry(name: str, st: os.stat_result) -> dict:
    kind = (
        "dir"
        if stat.S_ISDIR(st.st_mode)
        else "file"
        if stat.S_ISREG(st.st_mode)
        else "other"
    )
    return {
        "name": name,
        "type": kind,
        "size": st.st_size,
        "mtime": st.st_mtime_ns // 10**9,
        "mode": st.st_mode & 0o7777,
    }


def op_list(root: str, rel: str) -> list[dict]:
    fd = open_dir_beneath(root, split_rel(rel))
    try:
        out = []
        for name in sorted(os.listdir(fd)):
            try:
                out.append(
                    _entry(name, os.stat(name, dir_fd=fd, follow_symlinks=False))
                )
            except OSError:
                continue
        return out
    finally:
        os.close(fd)


def _parent(root: str, rel: str) -> tuple[int, str]:
    parts = split_rel(rel)
    if not parts:
        raise OSError(errno.EINVAL, "path is the share root")
    return open_dir_beneath(root, parts[:-1]), parts[-1]


def op_stat(root: str, rel: str) -> dict:
    parts = split_rel(rel)
    if not parts:
        return _entry("", os.stat(root))
    dfd, name = _parent(root, rel)
    try:
        return _entry(name, os.stat(name, dir_fd=dfd, follow_symlinks=False))
    finally:
        os.close(dfd)


def op_read(root: str, rel: str, start: int, length: int | None, out) -> dict:
    dfd, name = _parent(root, rel)
    try:
        fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW, dir_fd=dfd)
    finally:
        os.close(dfd)
    with os.fdopen(fd, "rb") as f:
        st = os.fstat(fd)
        if not stat.S_ISREG(st.st_mode):
            raise OSError(
                errno.EISDIR if stat.S_ISDIR(st.st_mode) else errno.EINVAL,
                "not a regular file",
            )
        end = st.st_size if length is None else min(st.st_size, start + length)
        out.write(
            json.dumps(
                {
                    "size": st.st_size,
                    "start": start,
                    "end": end,
                    "mtime": st.st_mtime_ns // 10**9,
                }
            ).encode()
            + b"\n"
        )
        f.seek(start)
        left = max(0, end - start)
        while left:
            buf = f.read(min(CHUNK, left))
            if not buf:
                break
            out.write(buf)
            left -= len(buf)
    return {}


def op_write(root: str, rel: str, overwrite: bool, src) -> dict:
    dfd, name = _parent(root, rel)
    tmp = f".{name}.upload-{os.getpid()}"
    try:
        if not overwrite:
            try:
                os.stat(name, dir_fd=dfd, follow_symlinks=False)
                raise OSError(errno.EEXIST, "exists")
            except FileNotFoundError:
                pass
        fd = os.open(
            tmp, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o664, dir_fd=dfd
        )
        size = 0
        try:
            with os.fdopen(fd, "wb") as f:
                while buf := src.read(CHUNK):
                    f.write(buf)
                    size += len(buf)
                f.flush()
                os.fsync(f.fileno())
            if overwrite:
                os.rename(tmp, name, src_dir_fd=dfd, dst_dir_fd=dfd)
            else:
                os.link(
                    tmp, name, src_dir_fd=dfd, dst_dir_fd=dfd, follow_symlinks=False
                )
                os.unlink(tmp, dir_fd=dfd)
        except BaseException:
            try:
                os.unlink(tmp, dir_fd=dfd)
            except OSError:
                pass
            raise
        return {"size": size}
    finally:
        os.close(dfd)


def op_mkdir(root: str, rel: str) -> dict:
    dfd, name = _parent(root, rel)
    try:
        os.mkdir(name, 0o775, dir_fd=dfd)
    finally:
        os.close(dfd)
    return {}


def op_rm(root: str, rel: str) -> dict:
    dfd, name = _parent(root, rel)
    try:
        st = os.stat(name, dir_fd=dfd, follow_symlinks=False)
        if stat.S_ISDIR(st.st_mode):
            os.rmdir(name, dir_fd=dfd)  # empty directories only
        else:
            os.unlink(name, dir_fd=dfd)
    finally:
        os.close(dfd)
    return {}


def op_rename(root: str, rel: str, dst: str) -> dict:
    sfd, sname = _parent(root, rel)
    try:
        dfd, dname = _parent(root, dst)
        try:
            try:
                os.stat(dname, dir_fd=dfd, follow_symlinks=False)
                raise OSError(errno.EEXIST, "destination exists")
            except FileNotFoundError:
                pass
            os.rename(sname, dname, src_dir_fd=sfd, dst_dir_fd=dfd)
        finally:
            os.close(dfd)
    finally:
        os.close(sfd)
    return {}


def main(argv: list[str]) -> int:
    op, root, rel = argv[0], argv[1], argv[2]
    try:
        if op == "list":
            res = op_list(root, rel)
        elif op == "stat":
            res = op_stat(root, rel)
        elif op == "read":
            op_read(
                root,
                rel,
                int(argv[3]),
                None if argv[4] == "-" else int(argv[4]),
                sys.stdout.buffer,
            )
            return 0
        elif op == "write":
            res = op_write(root, rel, argv[3] == "1", sys.stdin.buffer)
        elif op == "mkdir":
            res = op_mkdir(root, rel)
        elif op == "rm":
            res = op_rm(root, rel)
        elif op == "rename":
            res = op_rename(root, rel, argv[3])
        else:
            raise OSError(errno.EINVAL, "bad op")
    except OSError as e:
        sys.stderr.write(
            json.dumps({"errno": e.errno or errno.EIO, "msg": e.strerror or str(e)})
        )
        return 1
    sys.stdout.write(json.dumps(res))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
