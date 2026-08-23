#!/usr/bin/env python3
"""Trace prior-art repos: metadata, forks ahead of parent, issues, PRs.

Answers "is there newer work than the parent repo?" - forks are where abandoned
projects continue, and issues/PRs carry the unresolved gotchas.

Output: tmp/logs/prior-art.json (raw) + tmp/logs/prior-art.md (readable).
Run: ./scripts/research-prior-art.py
"""

import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOGS = REPO / "tmp" / "logs"

REPOS = [
    "bcyangkmluohmars/linux-alpine-v2",
    "UrNVR/unvr-kernel",
    "UrNVR/urnvr",
    "UrNVR/urnvr-init",
    "delroth/al_eth-standalone",
    "delroth/alpine_hal",
    "delroth/linux-qnap-tsx32x",
    "riptidewave93/UNVR-NAS",
    "NeccoNeko/UNVR-diy-os",
    "NeccoNeko/UBNT-source-code",
    "mornepousse/al_thermal-standalone",
    "rjmotion/unifi-unvr-emu",
    "snowsnoot/unifi-unvr-arm64",
    "markdegrootnl/unifi-protect-arm64",
    "dciancu/unifi-protect-unvr-docker-arm64",
]


def gh(path, jq=None, paginate=False):
    cmd = ["gh", "api", path]
    if paginate:
        cmd.append("--paginate")
    if jq:
        cmd += ["--jq", jq]
    p = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if p.returncode != 0:
        return None
    out = p.stdout.strip()
    if not out:
        return None
    try:
        return json.loads(out) if not jq else out
    except json.JSONDecodeError:
        return out


def meta(name):
    raw = gh(f"repos/{name}")
    if raw is None:
        return None
    return {
        "full_name": raw.get("full_name"),
        "description": raw.get("description"),
        "archived": raw.get("archived"),
        "fork": raw.get("fork"),
        "parent": (raw.get("parent") or {}).get("full_name"),
        "created_at": raw.get("created_at"),
        "pushed_at": raw.get("pushed_at"),
        "size_kb": raw.get("size"),
        "stars": raw.get("stargazers_count"),
        "forks": raw.get("forks_count"),
        "open_issues": raw.get("open_issues_count"),
        "license": (raw.get("license") or {}).get("spdx_id"),
        "default_branch": raw.get("default_branch"),
    }


def forks(name, parent_pushed):
    raw = gh(f"repos/{name}/forks?sort=newest&per_page=100")
    if not raw:
        return []
    out = []
    for f in raw:
        ahead = parent_pushed and f.get("pushed_at", "") > parent_pushed
        out.append(
            {
                "full_name": f.get("full_name"),
                "pushed_at": f.get("pushed_at"),
                "stars": f.get("stargazers_count"),
                "ahead_of_parent": bool(ahead),
            }
        )
    out.sort(key=lambda x: x.get("pushed_at") or "", reverse=True)
    return out


def issues_and_prs(name):
    raw = gh(f"repos/{name}/issues?state=all&per_page=100")
    if not raw:
        return [], []
    iss, prs = [], []
    for i in raw:
        rec = {
            "number": i.get("number"),
            "title": i.get("title"),
            "state": i.get("state"),
            "created_at": i.get("created_at"),
            "updated_at": i.get("updated_at"),
            "comments": i.get("comments"),
            "user": (i.get("user") or {}).get("login"),
        }
        (prs if "pull_request" in i else iss).append(rec)
    return iss, prs


if __name__ == "__main__":
    LOGS.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")
    report = {"generated": stamp, "repos": {}}

    for name in REPOS:
        print(f"... {name}", flush=True)
        m = meta(name)
        if m is None:
            report["repos"][name] = {"error": "not found or no access"}
            continue
        f = forks(name, m["pushed_at"])
        iss, prs = issues_and_prs(name)
        report["repos"][name] = {"meta": m, "forks": f, "issues": iss, "prs": prs}

    (LOGS / "prior-art.json").write_text(json.dumps(report, indent=2))

    # Readable summary
    lines = [f"# Prior-art trace\n\nGenerated {stamp}\n"]
    for name, d in report["repos"].items():
        lines.append(f"\n## {name}\n")
        if "error" in d:
            lines.append(f"- **{d['error']}**\n")
            continue
        m = d["meta"]
        lines.append(f"- {m['description'] or '(no description)'}")
        flags = []
        if m["archived"]:
            flags.append("**ARCHIVED**")
        if m["fork"]:
            flags.append(f"fork of `{m['parent']}`")
        if flags:
            lines.append(f"- {', '.join(flags)}")
        lines.append(
            f"- pushed `{m['pushed_at']}` | created `{m['created_at']}` | "
            f"{m['size_kb']} KB | {m['stars']}★ | {m['forks']} forks | "
            f"{m['open_issues']} open issues | licence `{m['license']}`"
        )
        ahead = [x for x in d["forks"] if x["ahead_of_parent"]]
        if ahead:
            lines.append(f"- **{len(ahead)} fork(s) pushed AFTER parent:**")
            for x in ahead[:10]:
                lines.append(
                    f"  - `{x['full_name']}` pushed {x['pushed_at']} ({x['stars']}★)"
                )
        elif d["forks"]:
            lines.append(f"- {len(d['forks'])} fork(s), none newer than parent")
        if d["issues"]:
            op = [i for i in d["issues"] if i["state"] == "open"]
            lines.append(f"- issues: {len(d['issues'])} total, {len(op)} open")
            for i in d["issues"][:15]:
                lines.append(
                    f"  - #{i['number']} [{i['state']}] {i['title']} "
                    f"({i['comments']} comments, updated {i['updated_at'][:10]})"
                )
        if d["prs"]:
            op = [i for i in d["prs"] if i["state"] == "open"]
            lines.append(f"- PRs: {len(d['prs'])} total, {len(op)} open")
            for i in d["prs"][:15]:
                lines.append(
                    f"  - #{i['number']} [{i['state']}] {i['title']} "
                    f"(updated {i['updated_at'][:10]})"
                )
    (LOGS / "prior-art.md").write_text("\n".join(lines) + "\n")
    print(f"\nwrote {LOGS / 'prior-art.md'} and prior-art.json")
