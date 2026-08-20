#!/usr/bin/env python3
"""Local in-depth board-photo analysis via ollama vision models (offline).

Runs every board photo through a local ollama vision model to extract IC
silkscreen refs + verbatim top-markings, switches (SWx), connectors and test
points, building a component list — no cloud. Results per-photo + a combined
report under tmp/logs/chip-vision/.

Usage:
  ./scripts/chip-vision-ollama.py [--model qwen2.5vl:7b] [--dir tmp/photos-extract]
                                  [--only SW]        # only photos likely to help a topic
Models present locally (pick per need): qwen2.5vl:7b (OCR), minicpm-v:8b (OCR),
llama3.2-vision:11b / :90b (strongest, slow), gemma3:12b.

Timeout: a 7-11B vision model on a ~4000px photo takes ~20-90 s; per-image cap
is 300 s (~3x worst case). On expiry: log the photo + skip it (never hang).
"""

from __future__ import annotations

import argparse
import base64
import json
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, REPO

OLLAMA = "http://localhost:11434/api/generate"
OUT = LOGS / "chip-vision"
PER_IMAGE_TIMEOUT = 300  # failsafe: ~3x the ~90s worst case for an 11B model

PROMPT = (
    "This is a close-up photo of a green PCB (a network appliance mainboard). "
    "Read it like an electronics engineer doing component identification. List, "
    "as a table, EVERY integrated circuit, switch, button, connector, crystal, "
    "and test point you can see. For each row give: (1) the white silkscreen "
    "reference designator next to it (e.g. U8, UB1, U122, SW1, Y4, TP20, J3), "
    "(2) the EXACT top-marking text on the part, VERBATIM, each line separated "
    "by ' / ' (do not guess or expand abbreviations; copy what is printed), "
    "(3) your best guess of the part/function. Pay special attention to any "
    "TACTILE SWITCHES or BUTTONS labelled SW1, SW2, etc. — describe their exact "
    "location, silkscreen, nearby components, and any nearby text hinting at "
    "their purpose (reset, boot, recovery, mode). If text is unreadable, write "
    "'illegible' rather than guessing. Be exhaustive and precise."
)


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    OUT.mkdir(parents=True, exist_ok=True)
    (LOGS / "chip-vision-ollama.log").open("a").write(line + "\n")


def analyse(model: str, img: Path) -> str | None:
    b64 = base64.b64encode(img.read_bytes()).decode()
    body = json.dumps(
        {
            "model": model,
            "prompt": PROMPT,
            "images": [b64],
            "stream": False,
            "options": {"temperature": 0.0},
        }
    ).encode()
    req = urllib.request.Request(
        OLLAMA, data=body, headers={"Content-Type": "application/json"}
    )
    t0 = time.monotonic()
    try:
        with urllib.request.urlopen(req, timeout=PER_IMAGE_TIMEOUT) as r:
            resp = json.loads(r.read())
        dt = time.monotonic() - t0
        log(f"  OK {img.name} ({dt:.0f}s)")
        return resp.get("response", "")
    except (urllib.error.URLError, TimeoutError, OSError) as e:
        log(f"  FAIL {img.name} after {time.monotonic() - t0:.0f}s: {e}")
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="qwen2.5vl:7b")
    ap.add_argument("--dir", default="tmp/photos-extract")
    ap.add_argument("--only", default="", help="substring filter on filenames")
    a = ap.parse_args()

    imgs = sorted((REPO / a.dir).glob("*.jpg"))
    if a.only:
        imgs = [p for p in imgs if a.only in p.name]
    if not imgs:
        sys.exit(f"no .jpg under {a.dir}")
    OUT.mkdir(parents=True, exist_ok=True)
    log(f"=== chip-vision: {len(imgs)} photos, model={a.model} ===")

    combined = OUT / f"ALL-{a.model.replace(':', '_').replace('/', '_')}.md"
    with combined.open("w") as agg:
        agg.write(f"# Board chip vision pass — model {a.model}\n\n")
        for i, img in enumerate(imgs, 1):
            log(f"[{i}/{len(imgs)}] {img.name}")
            out = analyse(a.model, img)
            per = OUT / f"{img.stem}.txt"
            per.write_text(out or "(failed)")
            agg.write(f"\n## {img.name}\n\n{out or '(failed)'}\n")
            agg.flush()
    log(f"DONE. Combined report: {combined}")


if __name__ == "__main__":
    sys.exit(main())
