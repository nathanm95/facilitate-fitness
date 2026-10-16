#!/usr/bin/env python3
"""Regenerates everything in store/: screenshots, preview GIFs, icons, banner.

    python3 tools/store_assets.py            # everything (needs both emulators)
    python3 tools/store_assets.py --art      # icons and banner only, from store/*.png

Each variant is a debug build (FF_FAKE / FF_SHOW_TIME / FF_THEME / FF_PCT ...,
see wscript) installed on emery and gabbro and captured through the QEMU
monitor (tools/shot.py). Finishes with a plain `pebble build`, so build/ never
holds a debug bundle afterwards.
"""
import os
import re
import subprocess
import sys
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
STORE = ROOT / 'store'
PLATFORMS = ['emery', 'gabbro']

LAVENDER = (170, 85, 255)
PINK = (255, 170, 255)
INDIGO = (85, 0, 170)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

# Stills: always 10:08, the store convention.
STILLS = [
    ('2_time', dict(FF_THEME=0, FF_PCT=73)),
    ('3_goal', dict(FF_THEME=0, FF_PCT=118, FF_USUAL=8200, FF_BPM=96)),
    ('4_midnight', dict(FF_THEME=1, FF_PCT=88, FF_USUAL=7400, FF_BPM=64)),
    ('5_blush', dict(FF_THEME=2, FF_PCT=46, FF_USUAL=5200, FF_BPM=78)),
]
STILL_BG = {0: LAVENDER, 1: BLACK, 2: PINK}

# The preview GIF: one day, the ring filling from breakfast to bedtime.
DAY = [
    (730, 6, 500, 64), (915, 21, 1900, 88), (1140, 38, 3600, 76),
    (1305, 52, 4900, 81), (1520, 66, 6200, 74), (1745, 84, 7600, 112),
    (1930, 100, 8600, 95), (2145, 118, 9400, 68),
]
FRAME_MS, LAST_MS = 900, 2200


def run(cmd, env=None):
    return subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True)


def build(flags):
    env = {k: v for k, v in os.environ.items() if not k.startswith('FF_')}
    env.update({k: str(v) for k, v in flags.items()})
    r = run(['pebble', 'build'], env)
    if r.returncode:
        sys.exit(r.stdout + r.stderr)


def looks_right(path, bg):
    """The face is on screen (not another app or a 'not responding' modal):
    most of the screen is the theme's background."""
    im = Image.open(path).convert('RGB')
    counts = {c: n for n, c in im.getcolors(1 << 16)}
    return counts.get(bg, 0) > im.width * im.height * 0.25


def capture(platform, out, bg):
    for attempt in range(3):
        r = run(['pebble', 'install', '--emulator', platform])
        if 'succeeded' not in r.stdout + r.stderr:
            print('  install failed, retrying', platform)
        time.sleep(3)
        run(['python3', 'tools/shot.py', platform, str(out)])
        if out.exists() and looks_right(out, bg):
            return
        # Another face in front or a modal: Back, then try again.
        run(['pebble', 'emu-button', '--emulator', platform, 'push', 'back'])
        time.sleep(0.3)
        run(['pebble', 'emu-button', '--emulator', platform, 'release', 'back'])
        time.sleep(1)
    sys.exit(f'{out.name}: the face never came up on {platform}')


def shots():
    STORE.mkdir(exist_ok=True)
    for name, flags in STILLS:
        build(dict(FF_FAKE=1, FF_SHOW_TIME=1008, **flags))
        for p in PLATFORMS:
            out = STORE / f'{p}_{name}.png'
            capture(p, out, STILL_BG[flags['FF_THEME']])
            print(out.name)

    frames = {p: [] for p in PLATFORMS}
    tmp = ROOT / 'build' / 'gif'
    tmp.mkdir(parents=True, exist_ok=True)
    for i, (hhmm, pct, usual, bpm) in enumerate(DAY):
        build(dict(FF_FAKE=1, FF_SHOW_TIME=hhmm, FF_THEME=0, FF_PCT=pct, FF_USUAL=usual, FF_BPM=bpm))
        for p in PLATFORMS:
            out = tmp / f'{p}_{i}.png'
            capture(p, out, LAVENDER)
            frames[p].append(Image.open(out).convert('RGB'))
        print('frame', i)
    for p in PLATFORMS:
        save_gif(frames[p], STORE / f'{p}_1_preview.gif')
        print(f'{p}_1_preview.gif')

    build({})


def save_gif(frames, path):
    # One shared palette of the watch's own colours: no dithering, no drift.
    colours = sorted({c for f in frames for _, c in f.getcolors(1 << 16)})
    assert len(colours) <= 256, len(colours)
    pal = Image.new('P', (1, 1))
    flat = [v for c in colours for v in c]
    pal.putpalette(flat + [0] * (768 - len(flat)))
    idx = [f.quantize(palette=pal, dither=Image.Dither.NONE) for f in frames]
    durations = [FRAME_MS] * (len(idx) - 1) + [LAST_MS]
    idx[0].save(path, save_all=True, append_images=idx[1:], duration=durations, loop=0, optimize=False)


# ------------------------------------------------------------------ art ----

def runner_pixels():
    """The runner, read from src/c/art.c so the icon can't drift from the watch."""
    src = (ROOT / 'src/c/art.c').read_text()
    block = re.search(r'static const char RUNNER\[\] =(.*?);', src, re.S).group(1)
    return re.findall(r'"([.X]+)"', block)


def badge(draw, cx, cy, r, scale, badge_colour=PINK):
    """Same geometry as art_logo() in src/c/art.c."""
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=badge_colour)
    hw, hh, t = r * 0.58, r * 0.84, r * 0.25
    ihw, ihh = hw - t, hh - t * hh / hw
    ot, orr, ob, ol = (cx, cy - hh), (cx + hw, cy), (cx, cy + hh), (cx - hw, cy)
    it, ir, ib, il = (cx, cy - ihh), (cx + ihw, cy), (cx, cy + ihh), (cx - ihw, cy)
    draw.polygon([ot, orr, ir, it], fill=BLACK)
    draw.polygon([ob, ol, il, ib], fill=BLACK)
    draw.polygon([ot, ol, il, it], fill=INDIGO)
    draw.polygon([ob, orr, ir, ib], fill=INDIGO)
    rows = runner_pixels()
    w, h = len(rows[0]), len(rows)
    x0, y0 = cx - w * scale / 2, cy - h * scale / 2
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c == 'X':
                draw.rectangle([x0 + x * scale, y0 + y * scale,
                                x0 + (x + 1) * scale - 1, y0 + (y + 1) * scale - 1], fill=BLACK)


def runner_scale(r):
    """On the watch the 9px runner sits in a radius-27 badge; keep that ratio."""
    return max(1, round(r * 9 / 27 / 9))


def ring(draw, box, width, frac):
    draw.ellipse(box, outline=INDIGO, width=width)
    draw.arc(box, -90, -90 + 360 * frac, fill=PINK, width=width)


def icon(size):
    ss = 4  # supersample, then shrink: store icons aren't limited to 64 colours
    S = size * ss
    im = Image.new('RGB', (S, S), LAVENDER)
    d = ImageDraw.Draw(im)
    m = S * 0.07
    ring(d, [m, m, S - m, S - m], int(S * 0.085), 0.73)
    r = S * 0.30
    badge(d, S / 2, S / 2, r, scale=runner_scale(r))
    return im.resize((size, size), Image.LANCZOS)


def font(size, weight='Bold'):
    names = {'Bold': 1, 'Heavy': 8, 'Medium': 5, 'Regular': 7, 'Demi Bold': 2}
    path = '/System/Library/Fonts/Avenir Next.ttc'
    for i in range(12):
        f = ImageFont.truetype(path, size, index=i)
        if f.getname()[1] == weight:
            return f
    return ImageFont.truetype(path, size, index=names.get(weight, 0))


def banner():
    W, H = 720, 320
    im = Image.new('RGB', (W, H), LAVENDER)
    d = ImageDraw.Draw(im)

    em = Image.open(STORE / 'emery_2_time.png').convert('RGB')
    ga = Image.open(STORE / 'gabbro_2_time.png').convert('RGB')
    gx, gy = W - 16 - ga.width, (H - ga.height) // 2
    ex, ey = gx - 18 - em.width, (H - em.height) // 2

    d.rounded_rectangle([ex - 6, ey - 6, ex + em.width + 5, ey + em.height + 5], 14, fill=INDIGO)
    im.paste(em, (ex, ey))
    d.ellipse([gx - 6, gy - 6, gx + ga.width + 5, gy + ga.height + 5], fill=INDIGO)
    mask = Image.new('L', ga.size, 0)
    ImageDraw.Draw(mask).ellipse([0, 0, ga.width - 1, ga.height - 1], fill=255)
    im.paste(ga, (gx, gy), mask)

    x = 22
    badge(d, x + 26, 58, 26, scale=runner_scale(26))
    title = font(34, 'Heavy')
    d.text((x, 94), 'Facilitate', font=title, fill=WHITE)
    d.text((x, 132), 'Fitness 2', font=title, fill=WHITE)
    small = font(16, 'Demi Bold')
    for i, line in enumerate(['Steps, heart rate,', 'calories, distance', 'and sleep']):
        d.text((x, 184 + i * 21), line, font=small, fill=INDIGO)
    d.text((x, 262), 'Pebble Time 2 · Round 2', font=font(13, 'Demi Bold'), fill=PINK)
    right = max(d.textbbox((x, 0), s, font=f)[2] for s, f in
                [('Facilitate', title), ('calories, distance', small),
                 ('Pebble Time 2 · Round 2', font(13, 'Demi Bold'))])
    assert right < ex - 10, f'banner text runs into the watch ({right} >= {ex - 10})'
    im.save(STORE / 'banner_720x320.png')
    print('banner_720x320.png')


def art():
    for size in (80, 144):
        icon(size).save(STORE / f'icon_{size}x{size}.png')
        print(f'icon_{size}x{size}.png')
    banner()


if __name__ == '__main__':
    if '--art' not in sys.argv:
        shots()
    art()
