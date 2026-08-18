#!/usr/bin/env python3
"""Replace the ambiguous term "vendor" across docs/scripts/dts with unambiguous terms.

"vendor" carried three unrelated meanings. Split them:
  * as-shipped Ubiquiti firmware        -> "stock" (stock U-Boot/kernel/NAND/DTB/...)
  * Ubiquiti's published GPL source     -> "Ubiquiti GPL U-Boot"
  * Annapurna proprietary preboot/HAL   -> "Annapurna" (S2/algorithm/code/wrappers)
  * copied into this repo (vendoring)   -> "imported" / import-oot-modules.py

KEEP untouched (real names, not shorthand):
  * sysfs attribute files `.../vendor`, USB `idVendor`  (device data, not prose)
  * PCI "Vendor Specific Information"                    (only in hw-reference dumps)
  * DTS partition labels                                 (none contain "vendor")
hw-reference/ captured dumps are excluded entirely (evidence, not authored text).

Ordered longest-first so specific phrases win. Case of the match's first letter is
preserved. Idempotent. Residual bare "vendor" is left for manual review + reported.

Run: python scripts/disambiguate-vendor-term.py   (logs tmp/logs/disambiguate-vendor-term.log)
"""
import os
import re
import subprocess
import sys

REPO = "/mnt/2tb/git/awto-unvr"
LOG = os.path.join(REPO, "tmp/logs/disambiguate-vendor-term.log")

# (regex, canonical replacement). Ordered: most specific first.
RULES = [
    (r"vendor[- ]gpl[- ]u[- ]?boot", "Ubiquiti GPL U-Boot"),
    (r"vendor[- ]u[- ]?boot",        "stock U-Boot"),
    (r"vendor[- ]4\.19",             "stock 4.19"),
    (r"vendor[- ]kernel",            "stock kernel"),
    (r"vendor[- ]nand",              "stock NAND"),
    (r"vendor[- ]dtb",               "stock DTB"),
    (r"vendor[- ]dt\b",              "stock DT"),
    (r"vendor[- ]initramfs",         "stock initramfs"),
    (r"vendor[- ]rootfs",            "stock rootfs"),
    (r"vendor reserved region",      "stock reserved region"),
    (r"vendor build path",           "stock build path"),
    (r"vendor path",                 "stock path"),
    (r"vendor labels",               "stock partition labels"),
    (r"vendor checksum",             "stock checksum"),
    (r"vendor[- ]s2",                "Annapurna S2"),
    (r"vendor algorithm",            "Annapurna S2 algorithm"),
    (r"vendor wrappers",             "Annapurna U-Boot wrappers"),
    (r"vendor code",                 "Annapurna code"),
    (r"\bvendor-modules\b",          "import-oot-modules"),
    (r"vendor the",                  "import the"),
    (r"vendored",                    "imported"),
    (r"vendoring",                   "importing"),
    (r"vendor's",                    "stock"),
]

# Lines matching these are left completely alone (real device attributes).
KEEP_LINE = re.compile(r"idVendor|\$d/vendor|cat \$d/vendor|/vendor\b.*\bdevice\b")

COMPILED = [(re.compile(p, re.IGNORECASE), r) for p, r in RULES]


def match_case(matched: str, repl: str) -> str:
    """Preserve the case of the matched text's first letter."""
    if matched[:1].isupper():
        return repl[:1].upper() + repl[1:]
    return repl


def process(text: str):
    n = 0
    out_lines = []
    for line in text.splitlines(keepends=True):
        if KEEP_LINE.search(line):
            out_lines.append(line)
            continue
        for rx, repl in COMPILED:
            def _sub(m):
                nonlocal n
                n += 1
                return match_case(m.group(0), repl)
            line = rx.sub(_sub, line)
        out_lines.append(line)
    return "".join(out_lines), n


def main():
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    files = subprocess.run(
        ["git", "grep", "-il", "vendor", "--",
         "docs", "scripts", "dts", "modules", "kernel-patches", "sources", "*.md"],
        cwd=REPO, capture_output=True, text=True,
    ).stdout.split()
    files = [f for f in files if "hw-reference/" not in f]

    log = open(LOG, "w")
    total = 0
    changed_files = 0
    for rel in files:
        path = os.path.join(REPO, rel)
        with open(path, encoding="utf-8", errors="surrogateescape") as fh:
            orig = fh.read()
        new, n = process(orig)
        if n and new != orig:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
            changed_files += 1
            total += n
            print(f"{n:4d}  {rel}", file=log)
            print(f"{n:4d}  {rel}")
    print(f"\n{total} replacements across {changed_files} files", file=log)
    print(f"\n{total} replacements across {changed_files} files")

    # Report residual bare "vendor" (case-insensitive) for manual review.
    resid = subprocess.run(
        ["git", "grep", "-in", "vendor", "--",
         "docs", "scripts", "dts", "modules", "kernel-patches", "sources", "*.md"],
        cwd=REPO, capture_output=True, text=True,
    ).stdout.splitlines()
    resid = [l for l in resid
             if "hw-reference/" not in l
             and not re.search(r"idVendor|\$d/vendor|Vendor Specific", l)]
    print(f"\n--- {len(resid)} residual 'vendor' lines for manual review ---", file=log)
    for l in resid:
        print(l, file=log)
    print(f"\n{len(resid)} residual 'vendor' lines (see log) for manual review")
    log.close()


if __name__ == "__main__":
    main()
