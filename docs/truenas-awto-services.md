# awto services for TrueNAS 27: truesearchd, awto-webshare (#357)

- Real implementations, not stand-ins: `truenas/awto/<name>/` (stand-ins stay in `truenas/stubs/`).
- Clean-room from public GPL source only: Samba `mdssvc_truesearch.c` + `truenas_truesearch.x`, middleware 27 plugins/etc_files. No closed TrueNAS binary read.
- Wired like the stubs: `scripts/truenas-build.py` `stub_dirs()` snapshots each dir with `debian/control` into `LOCAL/<name>` → manifest `sources`, middlewared explicit deps, `base-packages` (binary names `awto-truesearchd`, `awto-webshare`).
- Tests: `tests/test_truesearchd.py`, `tests/test_awto_webshare.py` (part of `./dev.py test` / gate).

## awto-truesearchd
- Python stdlib only. Binary `truesearchd serve|index`; units `truesearchd.service`, `truesearch-indexer.service` (middleware `services/truesearch.py` starts both; both have ExecReload).
- Socket `/run/truesearchd/mdssvc.sock` mode 0666, rix v1: 4-byte BE length + XDR. Codec is generated from the shipped `truesearchd/proto/truenas_truesearch.x` (verbatim copy of Samba's), strict like the Rust side.
- Index: SQLite `/var/lib/truesearchd/index.db`, one row per file per (dataset, share). Targets register on first OPEN (dataset → mountpoint via `/proc/self/mountinfo`, share = root within dataset); first walk waits ≤5 s then `RIX_NO_INDEX`. Indexer rescans every 600 s (no inotify yet). No FTS5: GLOB over folded name columns.
- Query subset: name (`*`, kMDItemFSName, kMDItemDisplayName), kMDItemPath, size, owner/group id, dates (`$time.iso`, InRange), content type folder/extension groups; modifiers `c d w`. Unknown attribute terms drop out (Finder's content term), no usable term = `RIX_BAD_QUERY`.
- Permissions: with `TRIM_BY_IDENTITY`, candidates are lstat'ed in a child dropped to the asking uid/gid/groups (`truesearchd.trim`), `HIDE_UNREADABLE` adds `access(R_OK)`. Identity is taken from the request only when `SO_PEERCRED` uid is 0, else the peer's own.
- Samba side: `spotlight backend = truesearch` is already rendered by middleware (`util_smbconf.py`); share `search` needs `truesearch.unavailable_reasons == []` (patch 0006).
- Known gaps (see #357): `gen` is 0 so `file_id` = ino (Samba must not use zfs_core file ids; `STATX_GEN` unchecked on 6.18); `volume_id` = statvfs `f_fsid`; btime = min(mtime, ctime); no Finder tags/comments, no content search; middleware `truesearch_client` is still the stand-in, so `truesearch.query`/`status` API reports unavailable.

## awto-webshare
- Python, Debian `python3-aiohttp`; libpam via ctypes. Binary `awto-webshare`, unit `truenas-webshare-auth.service` (what middleware's `webshare` service starts), `/etc/pam.d/webshare` = common-auth + common-account + truenas-session.
- Reads middleware's `/etc/webshare-auth/config.json` (port, bind_addrs, allowed_groups, pam_service_name) and `/etc/webshare/config.json` (shares; `is_home_base` → `<path>/<username>`). Re-read when either file changes.
- HTTPS on 755: TrueNAS Connect cert if present under `proxy.cert_path`, else self-signed in `/var/lib/awto-webshare` (openssl, 10 y).
- Login: PAM service, then group in `allowed_groups`; HttpOnly SameSite=Strict cookie, 1 h idle; `X-WebShare: 1` required on non-GET; 10 failures/min/IP.
- File ops: one child per operation (`awto_webshare.worker`), dropped to the user when root; paths walked with `O_NOFOLLOW` from the share root fd (symlinks and `..` cannot escape).
- API: `GET /api/list|dl/<share>/<path>` (Range), `PUT /api/ul/...[?overwrite=1]`, `POST /api/mkdir|rename/...`, `DELETE /api/rm/...`, SPA at `/`.
- Not in v1: share links (`webshare-link`, port 8443), MCP, passkeys, URL downloads, zip, search via truesearchd, webui TNC gates.
- Middleware patch `0006-awto-webshare-truesearch-community.patch`: `webshare.start_blocker` → None, WEBSHARE and TRUESEARCH entitlement checks and the index-dataset gate removed.
