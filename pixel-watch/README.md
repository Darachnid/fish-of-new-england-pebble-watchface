# Fish Time — Pixel Watch edition

This directory adds a Wear OS Watch Face Format (WFF) version of Fish Time while leaving the Pebble watchface unchanged.

## What changes

- 44 fish, matching the existing Fish Time catalog and daily permutation.
- Species-appropriate color photographs from Wikimedia Commons when a suitable freely licensed image exists.
- Transparent fish cutouts normalized for the 450×450 WFF canvas.
- Original Fish Time artwork is retained as a fallback and colorized when Commons has no suitable photograph.
- Large glanceable time with a subdued date line.
- Dark water-like background.
- Fish and labels disappear in ambient mode; the clock remains dim.

WFF 1 is used deliberately for Wear OS 4 / API 33+ compatibility.

## Generate assets

From the repository root:

    python -m pip install "Pillow>=11" requests "rembg[cpu]"
    python pixel-watch/tools/generate_pixel_fish.py

Without background removal:

    python pixel-watch/tools/generate_pixel_fish.py --no-rembg

The generator writes the 44 fish PNGs, WFF XML, preview image, and ATTRIBUTIONS.csv.

## Build

Open pixel-watch/ in Android Studio or use Gradle 9.1:

    cd pixel-watch
    gradle :watchface:assembleDebug

The debug APK is under watchface/build/outputs/apk/debug/.

The GitHub Actions workflow also generates the assets, builds the APK, and uploads it as an artifact.

## Licensing

Every downloaded photo is recorded in ATTRIBUTIONS.csv with its Commons page, author, and license. The generator only accepts public-domain, CC0, CC BY, or CC BY-SA candidates. The repository's existing GPLv3 license remains the license for the project code.
