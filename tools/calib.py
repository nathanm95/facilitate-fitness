#!/usr/bin/env python3
"""Measures where each font's digits sit inside its text box.

Build with FF_CALIB=1, install, screenshot with tools/shot.py, then run
`calib.py shot.png`. main.c draws "0" in each Face, one per 32px column, with
the box top at y=20; this prints each column's digit top offset and height,
the numbers the Face table in main.c wants.
"""
import sys
from PIL import Image

NAMES = ['time', 'big', 'val', 'small', 'unit', 'label']
im = Image.open(sys.argv[1]).convert('RGB')
w, h = im.size
for i, name in enumerate(NAMES):
    x0 = 4 + i * 32
    ys = [y for y in range(h) for x in range(x0, min(x0 + 32, w)) if sum(im.getpixel((x, y))) > 384]
    if not ys:
        print(name, 'nothing drawn')
        continue
    print(f'{name:6s} top={min(ys) - 20:3d} cap={max(ys) - min(ys) + 1:3d}')
