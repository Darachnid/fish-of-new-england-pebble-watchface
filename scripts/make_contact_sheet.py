#!/usr/bin/env python3
"""Build a contact sheet of color fish plates for release review."""

from __future__ import annotations

import csv
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "fish_catalog.csv"
COLOR_DIR = ROOT / "resources" / "images" / "color"
OUT = ROOT / "screenshots" / "color-contact-sheet.png"

COLS = 4
THUMB_W = 200
LABEL_H = 28
PAD = 8


def main() -> None:
    rows = list(csv.DictReader(CATALOG.open(encoding="utf-8")))
    n = len(rows)
    rows_n = (n + COLS - 1) // COLS
    cell_h = THUMB_W // 2 + LABEL_H
    sheet = Image.new(
        "RGB",
        (COLS * (THUMB_W + PAD) + PAD, rows_n * (cell_h + PAD) + PAD),
        (255, 255, 255),
    )
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()

    for i, row in enumerate(rows):
        col, row_i = i % COLS, i // COLS
        x = PAD + col * (THUMB_W + PAD)
        y = PAD + row_i * (cell_h + PAD)
        src = COLOR_DIR / row["filename"]
        thumb = Image.open(src).convert("RGB")
        thumb.thumbnail((THUMB_W, THUMB_W // 2), Image.Resampling.LANCZOS)
        tx = x + (THUMB_W - thumb.width) // 2
        sheet.paste(thumb, (tx, y))
        label = row["common"]
        draw.text((x, y + thumb.height + 2), label, fill=(0, 0, 0), font=font)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(OUT, format="PNG", optimize=True)
    print(f"Wrote {OUT} ({sheet.size[0]}x{sheet.size[1]})")


if __name__ == "__main__":
    main()