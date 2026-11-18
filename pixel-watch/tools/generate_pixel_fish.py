#!/usr/bin/env python3
"""
Generate the Pixel Watch edition of Fish Time.

Uses freely licensed color photographs from Wikimedia Commons when a suitable
image exists, removes the background, normalizes the fish for a 450x450 WFF
canvas, and falls back to colorized original line art when necessary.
"""
from __future__ import annotations
import argparse, csv, io, re, sys
from pathlib import Path
from xml.sax.saxutils import escape
import requests
from PIL import Image, ImageEnhance, ImageFilter, ImageOps, ImageDraw

try:
    from rembc import remove as remove_background
except Exception:
    remove_background = None

ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "fish_catalog.csv"
SOURCE_DIR = ROOT / "resources" / "images"
OUT_DIR = ROOT / "pixel-watch" / "watchface" / "src" / "main" / "res" / "drawable-nodpi"
RAW_DIR = ROOT / "pixel-watch" / "watchface" / "src" / "main" / "res" / "raw"
PREVIEW = ROOT / "pixel-watch" / "watchface" / "src" / "main" / "res" / "drawable" / "preview.png"
ATTRIBUTIONS = ROOT / "pixel-watch" / "ATTRIBUTIONS.csv"
WATCHFACE = RAW_DIR / "watchface.xml"

COMMONS_API = "https://commons.wikimedia.org/w/api.php"
UA = "FishTimePixel/1.0 (Fish Time watch face; GitHub repo)"
ALLOWED_LICENSES = ("public domain", "cc0", "cc by", "cc by-sa")
BAD_WORDS = re.compile(
    r"illustration|drawing|diagram|plate|map|scan|skeleton|fossil|egg|embryo|"
    r"juvenile|museum specimen|painting|engraving|woodcut", re.I)

PALETTES = {
    "trout": ((55, 91, 86), (191, 105, 69)),
    "salmon": ((68, 100, 93), (214, 111, 72)),
    "pike": ((75, 112, 58), (178, 148, 61)),
    "sunfish": ((62, 86, 51), (218, 120, 48)),
    "perch": ((54, 91, 68), (224, 176, 67)),
    "bass": ((55, 83, 71), (188, 176, 94)),
    "minnow": ((76, 104, 96), (181, 159, 111)),
    "eel": ((48, 66, 54), (137, 112, 74)),
    "lamprey": ((89, 67, 55), (167, 101, 75)),
    "default": ((65, 92, 89), (184, 150, 91)),
}

def read_catalog():
    with CATALOG.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))

def metadata_value(meta, key):
    value = meta.get(key, {})
    return value.get("value", "") if isinstance(value, dict) else (value or "")

def search_commons(common: str, scientific: str):
    queries = [
        f'filetype:bitmap "{scientific}" -illustration -drawing -diagram -plate -map -scan',
        f'filetype:bitmap "{common}" fish -illustration -drawing -diagram -plate -map -scan',
        f'filetype:bitmap {scientific} fish -illustration -drawing -diagram',
    ]
    candidates = []
    for query in queries:
        params = {
            "action": "query", "generator": "search", "gsrnamespace": 6,
            "gsrlimit": 20, "gsrsearch": query, "prop": "imageinfo",
            "iiprop": "url|size|mime|extmetadata", "iiurlwidth": 1600,
            "format": "json",
        }
        r = requests.get(COMMONS_API, params=params, headers={"User-Agent": UA}, timeout=30)
        r.raise_for_status()
        pages = r.json().get("query", {}).get("pages", {}).values()
        for page in pages:
            info = (page.get("imageinfo") or [{}])[0]
            meta = info.get("extmetadata") or {}
            license_name = metadata_value(meta, "LicenseShortName")
            artist = metadata_value(meta, "Artist")
            desc = metadata_value(meta, "ImageDescription")
            title = page.get("title", "")
            blob = f"{title} {desc}".lower()
            if not (info.get("thumburl") or info.get("url")):
                continue
            if not any(x in license_name.lower() for x in ALLOWED_LICENSES):
                continue
            if BAD_WORDS.search(blob):
                continue
            width = int(info.get("thumbwidth") or info.get("width") or 0)
            height = int(info.get("thumbheight") or info.get("height") or 0)
            if width < 400 or height < 200:
                continue
            score = 0
            title_l = title.lower()
            if scientific.lower() in title_l: score += 100
            if common.lower() in title_l: score += 90
            if scientific.lower() in blob: score += 50
            if common.lower() in blob: score += 40
            if "fish" in blob: score += 10
            if "public domain" in license_name.lower() or "cc0" in license_name.lower(): score += 25
            if any(org in (artist + " " + desc).lower() for org in ("fish and wildlife", "noaa", "smithsonian")): score += 20
            ratio = width / max(height, 1)
            if 1.25 <= ratio <= 5.0: score += 15
            candidates.append((score, page, info, meta, license_name, artist, desc))
    if not candidates:
        return None
    candidates.sort(key=lambda x: x[0], reverse=True)
    return candidates[0]

def download_candidate(candidate):
    _, page, info, meta, license_name, artist, desc = candidate
    url = info.get("thumburl") or info.get("url")
    r = requests.get(url, headers={"User-Agent": UA}, timeout=60)
    r.raise_for_status()
    return Image.open(io.BytesIO(r.content)).convert("RGBA"), {
        "title": page.get("title", ""),
        "source_url": "https://commons.wikimedia.org/wiki/" + page.get("title", "").replace(" ", "_"),
        "media_url": url,
        "license": license_name,
        "artist": artist,
        "description": desc,
    }

def palette_for(common, scientific):
    s = f"{common} {scientific}".lower()
    for key in ("trout", "salmon", "pike", "sunfish", "perch", "bass", "minnow", "eel", "lamprey"):
        if key in s:
            return PALETTES[key]
    return PALETTES["default"]

def colorize_line_art(src: Path, common: str, scientific: str) -> Image.Image:
    im = Image.open(src).convert("RGBA")
    gray = ImageOps.grayscale(im)
    alpha = im.getchannel("A")
    if alpha.getextrema()[1] == 0:
        alpha = ImageOps.invert(gray)
    alpha = alpha.point(lambda p: 0 if p > 245 else p).filter(ImageFilter.MaxFilter(5))
    dark, light = palette_for(common, scientific)
    out = Image.new("RGBA", im.size)
    px, ap = out.load(), alpha.load()
    for y in range(im.height):
        t = y / max(1, im.height - 1)
        c = tuple(round(dark[k] * (1 - t) + light[k] * t) for k in range(3))
        for x in range(im.width):
            px[x, y] = (*c, ap[x, y])
    return ImageEnhance.Sharpness(ImageEnhance.Contrast(out).enhance(1.15)).enhance(1.25)

def normalize_photo(im: Image.Image) -> Image.Image:
    if remove_background is not None:
        try:
            im = remove_background(im)
            im = im if isinstance(im, Image.Image) else Image.open(io.BytesIO(im))
        except Except as exc:
            print(f"  background removal failed: {exc}", file=sys.stderr)
    im = im.convert("RGBA")
    bbox = im.getchannel("A").getbbox()
    if bbox:
        im = im.crop(bbox)
    im.thumbnail((390, 238), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (400, 238), (0, 0, 0, 0))
    canvas.alpha_composite(im, ((400 - im.width)//2, (238 - im.height)//2))
    rgb = ImageEnhance.Sharpness(
        ImageEnhance.Contrast(
            ImageEnhance.Color(canvas.convert("RGB")).enhance(1.08)
        ).enhance(1.07)
    ).enhance(1.18)
    result = rgb.convert("RGBA")
    result.putalpha(canvas.getchannel("A"))
    return result

def preview_from_first_asset(asset: Path):
    bg = Image.new("RGB", (450, 450), (5, 15, 18))
    draw = ImageDraw.Draw(bg)
    for r in range(450, 0, -15):
        f = r / 450
        c = (round(5 + 18*f), round(15 + 38*f), round(18 + 43*f))
        draw.ellipse((225-r/2, 165-r/2, 225+r/2, 165+r/2), fill=c)
    fish = Image.open(asset).convert("RGBA")
    fish.thumbnail((400, 238), Image.Resampling.LANCZOS)
    bg_rgba = bg.convert("RGBA")
    bg_rgba.alpha_composite(fish, ((450-fish.width)//2, 20))
    draw = ImageDraw.Draw(bg_rgba)
    draw.text((225, 300), "Fish Time", fill=(242, 250, 250), align="mm")
    draw.text(225, 340), "10:08", fill=(242, 250, 250), anchor="mm")
    draw.text(225, 382), "Tue, Nov 18", fill=(180, 205, 208), anchor="mm")
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    bg_rgba.convert("RGB").save(PREVIEW, "PNG", optimize=True)

def build_watchface_xml(rows):
    day_expr = (
        "((365 * ([YEAR] - 1) + floor(([YEAR] - 1) / 4) "
        "- floor(([YEAR] - 1) / 100) + floor(([YEAR] - 1) / 400) "
        "+ [DAY_OF_YEAR] - 1) * 17 + 31) % 44"
    )
    expressions = "\n".join(
        f'        <Expression name="fish_{i}"><![CDATA[{day_expr} == {i}]]></Expression>'
        for i, _ in enumerate(rows)
    )
    compares = []
    for i, row in enumerate(rows):
        common = escape(row["common"])
        scientific = escape(row["scientific"])
        stem = Path(row["filename"]).stem
        compares.append(f"""        <Compare expression="fish_{i}">
          <Group x="0" y="0" width="450" height="320">
            <Variant mode="AMBIENT" target="alpha" value="0" />
            <PartImage x="25" y="12" width="400" height="238">
              <Image resource="fish_{stem}" />
            </PartImage>
            <PartText x="20" y="245" width="410" height="30">
              <Text align="CENTER" isAutoSize="TRUE" maxLines="1" verticalAlign="CENTER">
                <Font family="SYNC_TO_DEVICE" size="25" weight="SEMI_BOLD" color="#F2F7F7"><![CDATA[{common}]]></Font>
              </Text>
            </PartText>
            <PartText x="20" y="275" width="410" height="22">
              <Text align="CENTER" isAutoSize="TRUE" maxLines="1" verticalAlign="CENTER">
                <Font family="SYNC_TO_DEVICE" size="15" slant="ITALIC" color="#A9C3C5"><![CDATA[{scientific}]]></Font>
              </Text>
            </PartText>
          </Group>
        </Compare>""")
    xml = f"""<?xml version="1.0" encoding="utf-8"?>
<!-- Generated by tools/generate_pixel_fish.py. Do not hand-edit. -->
<WatchFace width="450" height="450" clipShape="CIRCLE">
  <Scene>
    <PartDraw x="0" y="0" width="450" height="450">
      <Variant mode="AMBIENT" target="alpha" value="0" />
      <Rectangle x="0" y="0" width="450" height="450">
        <Fill color="#061317">
          <RadialGradient centerX="225" centerY="175" radius="360"
              colors="#173B43 #061317 #020607" positions="0.0 0.55 1.0" />
        </Fill>
      </Rectangle>
    </PartDraw>

    <Condition>
      <Expressions>
{expressions}
      </Expressions>
{chr(10).join(compares)}
      <Default>
        <Group x="0" y="0" width="450" height="320">
          <PartImage x="25" y="12" width="400" height="238">
            <Image resource="fish_{Path(rows[0]["filename"]).stem}" />
          </PartImage>
        </Group>
      </Default>
    </Condition>

    <DigitalClock x="20" y="313" width="410" height="76">
      <Variant mode="AMBIENT" target="alpha" value="170" />
      <TimeText x="0" y="0" width="410" height="76" format="h:mm" hourFormat="SYNC_TO_DEVICE" align="CENTER">
        <Font family="SYNC_TO_DEVICE" size="72" weight="BLACK" color="#F4FBFB">
          <Shadow color="#000000" offsetX="2" offsetY="3" radius="3" />
        </Font>
      </TimeText>
    </DigitalClock>

    <PartText x="25" y="392" width="400" height="34">
      <Variant mode="AMBIENT" target="alpha" value="120" />
      <Text align="CENTER" verticalAlign="CENTER">
        <Font family="SYNC_TO_DEVICE" size="19" weight="MEDIUM" color="#B8D0D2">
          <Template><![CDATA[%s, %s %d]]>
            <Parameter expression="[DAY_OF_WEEK_S]" />
            <Parameter expression="[MONTH_S]" />
            <Parameter expression="[DAY]" />
          </Template>
        </Font>
      </Text>
    </PartText>
  </Scene>
</WatchFace>
"""
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    WATCHFACE.write_text(xml, encoding="utf-8")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-rembg", action="store_true")
    parser.add_argument("--refresh", action="store_true")
    args = parser.parse_args()
    global remove_background
    if args.no_rembg:
        remove_background = None

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rows = read_catalog()
    attributions = []

    for n, row in enumerate(rows, 1):
        stem = Path(row["filename"]).stem
        out = OUT_DIR / f"fish_{stem}.png"
        print(f"[{n:02d}/{len(rows)}] {row['common']}")
        if out.exists() and not args.refresh:
            attributions.append({
                "common": row["common"], "scientific": row["scientific"],
                "asset": out.name, "source_type": "existing generated asset",
                "file_title": "", "author": "", "license": "",
                "source_url": "", "media_url": "",
            })
            continue

        candidate = None
        try:
            candidate = search_commons(row["common"], row["scientific"])
        except Exception as exc:
            print(f"  Commons search failed: {exc}", file=sys.stderr)

        source_type = "Wikimedia Commons photograph"
        meta = {"title":"","source_url":"","media_url":"","license":"","artist":""}
        try:
            if not candidate:
                raise RuntimeError("no suitable freely licensed photo")
            image, meta = download_candidate(candidate)
            image = normalize_photo(image)
        except Exception as exc:
            print(f"  using line-art fallback: {exc}")
            source_type = "colorized original line art"
            image = colorize_line_art(SOURCE_DIR / row["filename"], row["common"], row["scientific"])
            image.thumbnail((390, 238), Image.Resampling.LANCZOS)
            canvas = Image.new("RGBA", (400, 238), (0, 0, 0, 0))
            canvas.alpha_composite(image, ((400-image.width)//2, (238-image.height)//2))
            image = canvas

        image.save(out, "PNG", optimize=True)
        attributions.append({
            "common": row["common"], "scientific": row["scientific"],
            "asset": out.name, "source_type": source_type,
            "file_title": meta.get("title",""), "author": meta.get("artist",""),
            "license": meta.get("license",""),
            "source_url": meta.get("source_url",""), "media_url": meta.get("media_url",""),
        })

    build_watchface_xml(rows)
    preview_from_first_asset(OUT_DIR / f"fish_{Path(rows[0]['filename']).stem}.png")
    with ATTRIBUTIONS.open("w", newline="", encoding="utf-8") as f:
        fields = ["common","scientific","asset","source_type","file_title","author","license","source_url","media_url"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerws(attributions)
    print(f"Wrote {len(rows)} fish assets, watchface.xml, preview.png, and ATTRIBUTIONS.csv")

if __name__ == "__main__":
    main()
