---
name: no-redact-macs-ips
description: "awto-unvr — do NOT redact this unit's MACs, LAN IPs, PARTUUIDs from docs/logs (even the public repo)"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-17T04:41:57.808Z
---

For the **awto-unvr** project, do **not** redact the device's MAC addresses,
RFC1918 LAN IPs (192.168.25.x), or PARTUUIDs from committed docs/logs — the owner
stated they are "not a risk for us," even though awto-unvr is a **public** repo.

**Why:** owner explicitly asked twice ("i dont care about macs and ip - leave them
in"; "please dont redact this stuff it isnot a risk for us") after I over-redacted
a U-Boot `printenv` dump.

**How to apply:** the public-repo content-scrub still holds for actual secrets —
private filesystem paths (`/mnt`, `/home`), credentials/keys, and the sensitive
V07/Hantek RE work. MACs/IPs/PARTUUIDs are NOT in that set here. Don't strip them.

**Also:** board photos / large media are **docs** in this project — track them in
`docs/photos/`, do NOT gitignore them (owner: "photos are docs - must not be
gitignored"), even though the repo is public and they're large (~250 MB). Don't
route them to gitignored `sources/`. Relates to [[granular-commits-per-fix]].
