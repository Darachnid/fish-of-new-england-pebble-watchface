#!/usr/bin/env python3
"""Install user color art for Fish Time (white-backed PNGs for Pebble)."""

from __future__ import annotations

import csv
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "fish_catalog.csv"
ASSETS = Path("/home/manjo/.cursor/projects/home-manjo-Pebble/assets")
COLOR_OUT = ROOT / "resources" / "images" / "color"

MAX_W = 190
MAX_H = 102


def trim(im: Image.Image) -> Image.Image:
    if im.mode != "RGBA":
        im = im.convert("RGBA")
    bg = Image.new("RGBA", im.size, (255, 255, 255, 255))
    diff = ImageChops.difference(im, bg)
    bbox = diff.getbbox()
    if bbox:
        im = im.crop(bbox)
    return im


def fit(im: Image.Image) -> Image.Image:
    im.thumbnail((MAX_W, MAX_H), Image.Resampling.LANCZOS)
    return im


def key_paper_background(im: Image.Image) -> Image.Image:
    """Remove white / JPEG halos; leave fish opaque."""
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            spread = max(r, g, b) - min(r, g, b)
            if r >= 238 and g >= 238 and b >= 238:
                px[x, y] = (255, 255, 255, 0)
            elif r >= 215 and g >= 215 and b >= 215 and spread < 30:
                px[x, y] = (255, 255, 255, 0)
    return im


def defringe_matte(im: Image.Image) -> Image.Image:
    """Drop dark JPEG halos between paper and fish; keep ink outlines (very dark)."""
    im = im.convert("RGB")
    px = im.load()
    w, h = im.size

    def is_white(r: int, g: int, b: int) -> bool:
        return r >= 250 and g >= 250 and b >= 250

    def is_halo(r: int, g: int, b: int) -> bool:
        return 35 < max(r, g, b) < 100 and max(r, g, b) - min(r, g, b) < 35

    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            if not is_halo(r, g, b):
                continue
            whites = 0
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h:
                        nr, ng, nb = px[nx, ny]
                        if is_white(nr, ng, nb):
                            whites += 1
            if whites >= 4:
                px[x, y] = (255, 255, 255)
    return im


def flatten_on_white(im: Image.Image) -> Image.Image:
    """
    Pebble GCompOpAssign paints transparent pixels black. Always ship RGB on white.
    """
    im = im.convert("RGBA")
    out = Image.new("RGB", im.size, (255, 255, 255))
    out.paste(im, mask=im.split()[3])
    return out


def find_source(basename: str) -> Path:
    stem = basename.replace(".png", "")
    matches = sorted(ASSETS.glob(f"{stem}-*.png"))
    if not matches:
        matches = sorted(ASSETS.glob(f"{stem}.png"))
    if not matches:
        raise FileNotFoundError(stem)
    return matches[-1]


def process_color(src: Path, dest: Path) -> None:
    im = Image.open(src)
    im = trim(im)
    im = fit(im)
    im = key_paper_background(im)
    im = flatten_on_white(im)
    im = defringe_matte(im)
    dest.parent.mkdir(parents=True, exist_ok=True)
    im.save(dest, format="PNG", optimize=True)


def main() -> None:
    rows = list(csv.DictReader(CATALOG.open(encoding="utf-8")))
    COLOR_OUT.mkdir(parents=True, exist_ok=True)
    for row in rows:
        filename = row["filename"]
        src = find_source(filename)
        process_color(src, COLOR_OUT / filename)

    print(f"Installed {len(rows)} white-backed color images.")


if __name__ == "__main__":
    main()