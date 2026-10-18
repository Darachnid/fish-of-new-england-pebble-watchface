# Fish Time — Pebble Time 2

A black-and-white daily fish watchface using five supplied historical-style fish engravings. The watchface displays a fish image, common name, scientific name, current time, and date. A deterministic pseudorandom fish is selected from the other four fish each local calendar day, so the fish changes at midnight and remains fixed throughout that day even after a restart. Time follows your Pebble 12/24-hour preference.

## Build and install

Requires the Pebble SDK with Emery support (Pebble Time 2).

```sh
pebble build
pebble install --emulator emery
```

To install on a paired watch, use your normal Pebble SDK device-install workflow, such as `pebble install --phone <PHONE_IP>` if configured.

## Fish included

- Labrador whitefish — *Coregonus labradorius*
- Rainbow smelt — *Osmerus mordax*
- Brook trout — *Salvelinus fontinalis*
- Lake trout — *Salvelinus namaycush*
- Atlantic salmon — *Salmo salar*

The source images were supplied by the project owner. **Copyright and attribution of the original illustrations have not been independently verified.** Do not represent the engravings as public domain without checking their sources.

## Scope

- Pebble Time 2 (Emery), 200×228 layout, monochrome visuals
- No companion, network, configuration, or permissions required
- Resource images are thresholded 1-bit PNGs optimized for watch display
- Built with Pebble C SDK 3 watchface API

## Publishing

Review rights/attribution for the five source illustrations, test text layout on the real watch and emulator, and capture store screenshots before publishing.
