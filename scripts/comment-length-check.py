#!/usr/bin/env python3
"""Report C comment blocks longer than the repo rule (3 default, 6 hard limit).

Skips the file header (the first block, which carries SPDX/copyright/scope and
is exempt). Logs to tmp/logs/comment-length-check.log as well as stdout.

Usage: scripts/comment-length-check.py <file.c> [<file.c> ...]
Exit 0 always - this is a report, not a gate.
"""

import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG = os.path.join(REPO, "tmp", "logs", "comment-length-check.log")
HARD_LIMIT = 6


def is_comment(line: str) -> bool:
    s = line.lstrip()
    return s.startswith("/*") or s.startswith("*") or s.startswith("//")


def blocks(path: str):
    """Yield (start_line, length) for each run of comment lines."""
    with open(path, encoding="utf-8", errors="replace") as fh:
        lines = fh.readlines()

    start = None
    for i, line in enumerate(lines, 1):
        if is_comment(line):
            if start is None:
                start = i
        elif start is not None:
            yield start, i - start
            start = None
    if start is not None:
        yield start, len(lines) + 1 - start


def main() -> int:
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    findings = []

    for path in sys.argv[1:]:
        for n, (start, length) in enumerate(blocks(path)):
            if n == 0:
                continue  # file header is exempt
            if length > HARD_LIMIT:
                findings.append(f"{path}:{start} - {length} lines (limit {HARD_LIMIT})")

    with open(LOG, "w") as fh:
        for f in findings:
            print(f)
            fh.write(f + "\n")
        summary = f"{len(findings)} block(s) over the {HARD_LIMIT}-line hard limit"
        print(summary)
        fh.write(summary + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
