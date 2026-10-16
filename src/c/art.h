#pragma once
#include <pebble.h>

typedef enum {
  ICON_HEART,
  ICON_FLAME,
  ICON_PIN,
  ICON_WATCH,
  ICON_MOON,
  ICON_COUNT
} IconId;

// The 2017 badge, redrawn so it scales: a pink circle holding a two-tone
// diamond with a runner in the middle.
void art_logo(GContext *ctx, GPoint centre, int radius, GColor badge);

GSize art_icon_size(IconId id);
void art_icon(GContext *ctx, IconId id, GPoint origin, GColor colour);

// A battery outline with a fill; the fill turns `low` at 20% and under.
void art_battery(GContext *ctx, GRect box, int pct, GColor fg, GColor low);
