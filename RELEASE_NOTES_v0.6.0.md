# Fish Time v0.6.0

## Highlights

- **44 full-color field-guide fish plates** for Pebble Time 2 (emery), alongside original B&W plates.
- **White background only** — color art is flattened to opaque white-backed PNGs (no black matte on the watch).
- **Phone settings** (Clay): color vs. B&W, and **preview all species** (cycles every 5 seconds).
- Daily midnight species rotation unchanged; labels use catalog common names (no “ID uncertain” text).

## Install

Download `pebble-fish-time-v0.6.0.pbw` and install with the Pebble/Rebble app on your phone, or build from source:

```bash
npm install
python3 scripts/install_color_assets.py   # if refreshing color art from Cursor assets
python3 scripts/gen_fish_resources.py
pebble build
```

## Assets in this release

| File | Description |
| --- | --- |
| `pebble-fish-time-v0.6.0.pbw` | Watchface bundle for emery |
| `contact_sheet_color.png` | All 44 color illustrations on white |
| `emulator_smelt.png` | Emulator screenshot (color, white face) |

## Repository layout

- `resources/images/color/` — color build plates  
- `resources/images/mono/` — monochrome build plates  
- `resources/images/reference/` — archived original B&W plates  
- `scripts/install_color_assets.py` — trim, key, flatten color art for Pebble  
- `scripts/gen_fish_resources.py` — regenerate `package.json` + `fish_table.inc`