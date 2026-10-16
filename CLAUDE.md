# Facilitate Fitness 2: Pebble watchface

Rebuild of the 2017 Facilitate Fitness face (store id 58c17c050dfc32a52a00077a,
UUID 75d8f90d…, under an old unclaimed developer account) for emery + gabbro.
This is a new app with its own UUID. README.md explains the face; this file
holds the rules.

## Commands

```sh
pebble build
FF_FAKE=1 FF_SHOW_TIME=1008 tools/snap.sh /tmp/ff emery gabbro   # build, install, screenshot
python3 tools/shot.py emery out.png            # QEMU monitor screendump
python3 tools/store_assets.py                  # regenerate store/
```

## Rules of the house

- Keep the 2017 look: lavender `AA55FF`, badge pink `FFAAFF`, diamond indigo
  `5500AA` + black, Bitham 42 Bold clock. Colours must be legal GColor8
  values (each channel 00/55/AA/FF).
- The Round 2 has no heart-rate sensor (`FF_HAS_HRM` in fit.h). Never show
  heart rate on gabbro.
- Don't change heart-rate sampling in the shipped face; `FF_HR_PERIOD` is for
  emulator tests only.
- Redraw through `redraw()` in main.c, never `layer_mark_dirty(s_canvas)` directly:
  handlers can run before the window exists and the emulator hides a NULL layer.
- Subscribe to services only after `window_stack_push`.
- Theme order is saved in settings (ThemeId enum, `src/pkjs/config.js`). Never
  reorder, only append.
- Text placement uses the measured Face table in main.c. If you change a font,
  re-measure with `FF_CALIB=1` + `tools/calib.py`.
- Never ship a bundle built with any `FF_*` flag.
- Emulators are shared with other sessions. If the face doesn't come up after
  an install, press and release Back (`pebble emu-button --emulator <p> push back`,
  then `release back`) and install again.
