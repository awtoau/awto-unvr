#!/usr/bin/env python3
"""Relocate untracked Woomera MTD flash dumps out of the git tree, verifiably.

- Source:  images/mtd/**  (untracked/git-ignored .img + unpacked artifacts)
- Dest:    /mnt/2tb/git_debris/woomera-mtd/  (mirrored per-snapshot structure)
- Non-regenerable evidence: verify sha256 at dest == source BEFORE deleting source.
- Tracked README.md manifests are never touched (they stay as the index).

Safety: per-file copy -> re-hash dest -> compare -> only then delete source.
On any mismatch: STOP, leave source intact, exit non-zero.
Log: tmp/logs/move-woomera-mtd.log
"""
import hashlib
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO = Path("/mnt/2tb/git/awto-unvr")
SRC_ROOT = REPO / "images/mtd"
DEST_ROOT = Path("/mnt/2tb/git_debris/woomera-mtd")
LOG = REPO / "tmp/logs/move-woomera-mtd.log"

CHUNK = 1 << 20  # 1 MiB read blocks for hashing


def log(msg: str) -> None:
    line = f"{datetime.now(timezone.utc).astimezone().isoformat()} {msg}"
    print(line)
    with LOG.open("a") as fh:
        fh.write(line + "\n")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(CHUNK), b""):
            h.update(block)
    return h.hexdigest()


def main() -> int:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    log("=== move-woomera-mtd RUN ===")

    if not DEST_ROOT.parent.is_dir():
        log(f"STOP: dest parent missing: {DEST_ROOT.parent}")
        return 2

    # Untracked-but-not-ignored (should be none) + git-ignored (the dumps).
    def git_list(*args: str) -> list[str]:
        out = subprocess.run(
            ["git", "-C", str(REPO), "ls-files", *args, "images/mtd/"],
            capture_output=True, text=True, check=True,
        ).stdout
        return [l for l in out.splitlines() if l]

    tracked = set(git_list())
    untracked = git_list("--others", "--exclude-standard")
    ignored = git_list("--others", "--ignored", "--exclude-standard")
    to_move = sorted(set(untracked) | set(ignored))

    log(f"tracked files (kept in place): {len(tracked)}")
    log(f"untracked-non-ignored: {len(untracked)}")
    log(f"git-ignored (dumps): {len(ignored)}")
    log(f"files to relocate: {len(to_move)}")

    # Guard: never move a tracked file.
    overlap = set(to_move) & tracked
    if overlap:
        log(f"STOP: tracked files appear in move set: {sorted(overlap)}")
        return 3

    moved: list[tuple[str, str, int]] = []  # (relpath, sha256, size)
    total_bytes = 0
    for rel in to_move:
        src = REPO / rel
        # rel is like images/mtd/<snap>/<file>; mirror under DEST_ROOT sans 'images/mtd/'
        sub = Path(rel).relative_to("images/mtd")
        dst = DEST_ROOT / sub
        if not src.is_file():
            log(f"STOP: source not a file: {src}")
            return 4
        size = src.stat().st_size
        src_hash = sha256(src)
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        dst_hash = sha256(dst)
        if dst_hash != src_hash:
            log(f"STOP: sha256 MISMATCH {rel}\n  src={src_hash}\n  dst={dst_hash} -- source kept")
            return 5
        if dst.stat().st_size != size:
            log(f"STOP: size mismatch {rel} src={size} dst={dst.stat().st_size} -- source kept")
            return 6
        # verified -> safe to delete source
        src.unlink()
        moved.append((rel, src_hash, size))
        total_bytes += size
        log(f"OK  {size:>11} {src_hash[:16]}.. {rel} -> DELETED src")

    log(f"DONE: relocated+verified {len(moved)} files, {total_bytes} bytes")

    # Write manifest of hashes at dest for durable verification.
    manifest = DEST_ROOT / "SHA256SUMS.txt"
    with manifest.open("w") as fh:
        fh.write(f"# Woomera MTD dumps relocated from {SRC_ROOT} on "
                 f"{datetime.now(timezone.utc).astimezone().date().isoformat()}\n")
        fh.write(f"# {len(moved)} files, {total_bytes} bytes\n")
        for rel, h, size in moved:
            sub = Path(rel).relative_to("images/mtd")
            fh.write(f"{h}  {sub}\n")
    log(f"wrote manifest: {manifest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
