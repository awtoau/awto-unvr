#!/usr/bin/env python3
"""File docs/issues/NN-*.md drafts as GitHub issues on awtoau/awto-unvr.

Draft format: line1 `# <title>`, then a `labels: a, b` line, a `---`, then body.
Idempotent: skips a draft whose exact title already exists as an issue.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import REPO  # noqa: E402

REPOSLUG = "awtoau/awto-unvr"
ISSUES = REPO / "docs" / "issues"
LOG = REPO / "tmp" / "logs" / "file-issues.log"


def sh(*args, check=True):
    return subprocess.run(args, capture_output=True, text=True, check=check)


def existing_titles() -> set[str]:
    r = sh("gh", "issue", "list", "--repo", REPOSLUG, "--state", "all",
           "--limit", "200", "--json", "title", check=False)
    if r.returncode != 0:
        return set()
    import json
    return {i["title"] for i in json.loads(r.stdout or "[]")}


def parse(md: str):
    lines = md.splitlines()
    title = lines[0].lstrip("# ").strip()
    labels, body_start = [], 1
    for i, ln in enumerate(lines[1:], 1):
        if ln.lower().startswith("labels:"):
            labels = [x.strip() for x in ln.split(":", 1)[1].split(",") if x.strip()]
        if ln.strip() == "---":
            body_start = i + 1
            break
    body = "\n".join(lines[body_start:]).strip() or title
    return title, labels, body


def main():
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logf = LOG.open("a")

    def log(m):
        print(m, flush=True)
        logf.write(m + "\n")

    drafts = sorted(p for p in ISSUES.glob("*.md") if p.name != "README.md")
    have = existing_titles()

    # Ensure all labels exist (create is a no-op error if present).
    all_labels = set()
    for p in drafts:
        all_labels |= set(parse(p.read_text())[1])
    for lab in sorted(all_labels):
        sh("gh", "label", "create", lab, "--repo", REPOSLUG, "--force", check=False)

    created = 0
    for p in drafts:
        title, labels, body = parse(p.read_text())
        if title in have:
            log(f"skip (exists): {p.name} -> {title!r}")
            continue
        args = ["gh", "issue", "create", "--repo", REPOSLUG,
                "--title", title, "--body", body]
        for lab in labels:
            args += ["--label", lab]
        r = sh(*args, check=False)
        if r.returncode == 0:
            url = r.stdout.strip().splitlines()[-1]
            log(f"filed: {p.name} -> {url}")
            created += 1
        else:
            log(f"FAIL: {p.name}: {r.stderr.strip()[:200]}")
    log(f"done. {created} created, {len(drafts)-created} skipped/failed of {len(drafts)}")


if __name__ == "__main__":
    main()
