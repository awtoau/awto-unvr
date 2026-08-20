#!/usr/bin/env python3
"""Crop regions of an RPS board photo to read IC markings / count pins.
Usage: crop_rps.py <src.jpg> <outdir> [name x0 y0 x1 y1 (in ORIGINAL px)]...
Default crops target docs/photos/RPS.jpg (4000x1848)."""

import os
import sys

from PIL import Image

src = sys.argv[1] if len(sys.argv) > 1 else "/mnt/2tb/git/awto-unvr/docs/photos/RPS.jpg"
outdir = sys.argv[2] if len(sys.argv) > 2 else "/mnt/2tb/git/awto-unvr/tmp/rps-crops"
os.makedirs(outdir, exist_ok=True)
im = Image.open(src)
W, H = im.size

# default regions in original px (from RPS.jpg display*2 estimates)
regions = [
    ("u122", 2180, 980, 2560, 1280),
    ("u48_u1", 2260, 1220, 2720, 1560),
    ("d500", 1900, 480, 2280, 820),
    ("connector", 1560, 320, 1980, 1560),
    ("jb5_dcout", 3040, 300, 3520, 660),
    ("vr5_area", 2600, 900, 3100, 1400),
]
# allow override groups of 5 args
extra = sys.argv[3:]
for i in range(0, len(extra), 5):
    name = extra[i]
    x0, y0, x1, y1 = map(int, extra[i + 1 : i + 5])
    regions.append((name, x0, y0, x1, y1))

for name, x0, y0, x1, y1 in regions:
    x0 = max(0, x0)
    y0 = max(0, y0)
    x1 = min(W, x1)
    y1 = min(H, y1)
    crop = im.crop((x0, y0, x1, y1))
    # upscale small crops 2x for legibility
    if crop.width < 900:
        crop = crop.resize((crop.width * 2, crop.height * 2), Image.LANCZOS)
    p = os.path.join(outdir, name + ".png")
    crop.save(p)
    print(p, crop.size)
