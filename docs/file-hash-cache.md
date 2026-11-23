# File hash cache (#378)

Lazy SHA-256 of every file on woomera's data filesystems, stored in the file's own `user.*` xattrs. Lets a client ask "is this the same file?" without rereading it.

- Script: `scripts/file-hash-cache.py`; box copy `/usr/local/bin/file-hash-cache.py`.
- Filesystems: `/srv/mirror`, `/srv/single`.
- Scope today: xattr schema + CLI + heartbeat. **No HTTP API yet** (see Not built).

## How it works
- systemd timer per filesystem: `awto-filehash@mirror`, `awto-filehash@single` (unit files written by `install`).
- One pass walks the tree in sorted order and, per regular file:
  - hash present and mtime+size match → fresh, one stat, skipped.
  - hash missing or mtime/size differ → stale; hash it, write xattrs.
  - modified < 60 s ago → "settling", skipped (still being written).
  - changed while being hashed → "settling", retried next pass.
- Skipped dirs: `#recycle`, `.tools`, `.snapshots`. Symlinks and non-regular files ignored.
- Pass stops after the time budget; the next pass resumes (fresh files cost a stat).- Runs at `Nice=19`, `IOSchedulingClass=idle`; shares the mirror with bees and any job a session starts.

| Setting | Value | Why |
|---|---|---|
| Budget per pass | 600 s | bounded pass; resumable |
| Gap between passes | 5 min after a pass ends | `OnUnitInactiveSec` |
| `TimeoutStartSec` | 720 s | 1.2 × budget; last file may overrun it |
| Settle window | 60 s | covers a slow SMB/NFS client flush |
| Stall threshold | 2 × (budget + gap) = 30 min | heartbeat older than this = not running |

## xattr schema (the data API)
| Attribute | Value | Meaning |
|---|---|---|
| `user.sha256` | 64 hex chars | SHA-256 of the file content |
| `user.sha256.mtime_ns` | integer | mtime when hashed |
| `user.sha256.size` | integer | size when hashed |
| `user.sha256.at` | epoch seconds | when the hash was computed |

- **Fresh** = `mtime_ns` and `size` equal the file's current values. **Stale** = they differ. **Unhashed** = no attributes.
- Hash age = now − `user.sha256.at`; a hash can be fresh and old.
- Reading: NFSv4.2 clients can read `user.*` xattrs; SMB cannot. Local: `os.getxattr(path, "user.sha256")`.
- Writing is the script's job only; xattr writes change ctime, not mtime, and do not affect dedupe.

## CLI
```
./dev.py file-hash install            # copy script to the box, enable both timers
./dev.py file-hash uninstall          # disable timers, remove units and box copy
./dev.py file-hash status             # heartbeat summary per filesystem
./dev.py file-hash get PATH           # one file; PATH is a path on the box
```
- `get` prints JSON, exit code: 0 fresh, 1 unhashed, 2 stale.

```json
{"path": "/srv/mirror/a.flac", "sha256": "ab12…", "state": "fresh", "hash_age_s": 5120, "mtime_ns": 1763400000000000000}
```
- `state` is `fresh`, `stale` or `unhashed`; `sha256` is `null` when unhashed.

## Heartbeat (lag and stopped-daemon detection)
- File `/var/lib/awto-filehash/<name>.json`, rewritten atomically at the end of every pass.

| Field | Meaning |
|---|---|
| `started`, `finished` | epoch seconds of the pass |
| `complete` | `false` = budget hit, backlog remains |
| `hashed`, `fresh`, `settling`, `errors` | files per outcome this pass |
| `backlog` | stale files not reached this pass |
| `oldest_backlog_age_s` | age (since mtime) of the oldest unhashed or stale file seen |
| `interval_s` | expected pass-to-pass time, used for the stall check |

- `status` output: `mirror: last pass ended 212s ago (budget hit): hashed 41, fresh 0, settling 0, errors 0, backlog 90211 (oldest 604800s)`.
- Prefix `STALLED` = no pass ended in 2 × `interval_s` (service stopped, failed or starved).
- `no heartbeat: never ran` = timer not installed or first pass unfinished.
- Backlog and oldest age are the load-lag signal: they grow while hashing falls behind new writes.

## Not built
- HTTP API for clients that cannot read xattrs (SMB, other hosts without NFS).
- Reverse lookup (`sha256` → paths). Needs an index; the xattrs only answer path → hash.
- fs-verity for sealed archives: needs `CONFIG_FS_VERITY` (not enabled) and a test against bees.

## Alternatives checked
- shatag/cshatag: same xattr idea; not packaged for Fedora, maintenance unverified.
- hashdeep/rhash: write hash files, not xattrs.
- git-annex, restic, Kopia: content-addressed stores that own the layout.
- Btrfs checksums: per block, integrity only.
