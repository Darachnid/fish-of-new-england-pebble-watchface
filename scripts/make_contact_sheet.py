#!/usr/bin/env python3
"""Build a white-backed contact sheet of all color fish plates."""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
COLOR_DIR = ROOT / "resources" / "images" / "color"
OUT = ROOT / "screenshots" / "contact_sheet_color.png"

COLS = 8
PAD = 8
LABEL_H = 14
CELL_W = 200
THUMB_W = 180


def main() -> None:
    files = sorted(COLOR_DIR.glob("*.png"))
    if len(files) != 44:
        raise SystemExit(f"expected 44 color PNGs, found {len(files)}")
    rows = math.ceil(len(files) / COLS)
    cell_h = THUMB_W // 3 + LABEL_H + PAD
    w = COLS * CELL_W + PAD
    h = rows * cell_h + PAD
    sheet = Image.new("RGB", (w, h), (255, 255, 255))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("/usr/share/fonts/TTF/DejaVuSans.ttf", 10)
    except OSError:
        font = ImageFont.load_default()

    for i, path in enumerate(files):
        col, row = i % COLS, i // COLS
        x0 = PAD + col * CELL_W
        y0 = PAD + row * cell_h
        im = Image.open(path).convert("RGB")
        im.thumbnail((THUMB_W, THUMB_W // 2), Image.Resampling.LANCZOS)
        ox = x0 + (CELL_W - im.size[0]) // 2
        oy = y0 + 2
        sheet.paste(im, (ox, oy))
        label = path.stem.replace("_", " ")
        draw.text((x0 + 4, y0 + THUMB_W // 2 + 4), label, fill=(0, 0, 0), font=font)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(OUT, format="PNG", optimize=True)
    print(f"Wrote {OUT} ({w}x{h})")


if __name__ == "__main__":
    main()