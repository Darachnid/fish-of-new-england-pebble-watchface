# Fish Time

Pebble Time 2 black-and-white fish watchface using the final 44 unique images from Fish (1).zip.

- New fish at midnight, in a 44-day shuffled-looking cycle with no duplicates within one cycle.
- Triple *physical* tap on the watch glass: accelerometer-based fish advancement, with haptic confirmation. Stock watchface firmware does not deliver touchscreen TouchService events.
- Manual selection resets to the scheduled fish at midnight.
- Common and scientific names below the image (catalog spellings preserved where historical).

Build: `pebble clean && pebble build` then `pebble install --cloudpebble`.

## Phone settings (Clay)

In the Pebble app, open **Fish Time → Settings**:

- **Color illustrations** — full-color vs. original B&W plates (white background always).
- **Preview all species (5 sec)** — cycles through all 44 fish every 5 seconds for review.

Original plates: `resources/images/reference/`. Build: `color/`, `mono/`. Install color art with `python3 scripts/install_color_assets.py`, then `python3 scripts/gen_fish_resources.py`.

```bash
python3 scripts/gen_fish_resources.py
```
