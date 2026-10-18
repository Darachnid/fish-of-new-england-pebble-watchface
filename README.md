# Fish Time

Pebble Time 2 black-and-white fish watchface using the final 44 unique images from Fish (1).zip.

- New fish at midnight, in a 44-day shuffled-looking cycle with no duplicates within one cycle.
- Triple *physical* tap on the watch glass: accelerometer-based fish advancement, with haptic confirmation. Stock watchface firmware does not deliver touchscreen TouchService events.
- Manual selection resets to the scheduled fish at midnight.
- Common and source-provided scientific labels below the image. Historical/uncertain taxonomic names are not silently corrected.

Build: `pebble clean && pebble build` then `pebble install --cloudpebble`.
