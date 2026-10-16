#include "art.h"

// Pixel icons, one character per pixel, row by row. Drawn a pixel at a time:
// they're tiny and only redrawn once a minute.
typedef struct {
  uint8_t w, h;
  const char *px;
} Icon;

static const char HEART[] =
  ".XXX...XXX."
  "XXXXX.XXXXX"
  "XXXXXXXXXXX"
  "XXXXXXXXXXX"
  "XXXXXXXXXXX"
  ".XXXXXXXXX."
  "..XXXXXXX.."
  "...XXXXX..."
  "....XXX...."
  ".....X.....";

static const char FLAME[] =
  "....X...."
  "....XX..."
  "...XXX..."
  "...XXXX.."
  "..XXXXX.X"
  ".XXXXXXXX"
  ".XXXX.XXX"
  "XXXX..XXX"
  "XXX....XX"
  "XXX....XX"
  ".XXX..XX."
  "..XXXXX..";

static const char PIN[] =
  "..XXXXX.."
  ".XXXXXXX."
  "XXX...XXX"
  "XX.....XX"
  "XX.....XX"
  "XXX...XXX"
  ".XXXXXXX."
  ".XXXXXXX."
  "..XXXXX.."
  "...XXX..."
  "...XXX..."
  "....X....";

static const char WATCH[] =
  "....XXX...."
  ".....X....."
  "..XXXXXXX.."
  ".XX.....XX."
  "XX...X...XX"
  "X....X....X"
  "X....X....X"
  "X....XXX..X"
  "X.........X"
  "XX.......XX"
  ".XX.....XX."
  "..XXXXXXX..";

static const char MOON[] =
  "...XXX....."
  ".XXXX......"
  ".XXX......."
  "XXX........"
  "XXX........"
  "XXX........"
  "XXXX......."
  ".XXXX....X."
  ".XXXXXXXXX."
  "..XXXXXXX.."
  "....XXX....";

static const char RUNNER[] =
  "......XX."
  "......XX."
  "...XXX..."
  "..XXXXX.."
  ".X.XXX.X."
  "...XXX..."
  "..XX.X..."
  "XXX...X.."
  "X.....X.."
  "......XX.";

static const Icon ICONS[ICON_COUNT] = {
  [ICON_HEART] = { 11, 10, HEART },
  [ICON_FLAME] = { 9, 12, FLAME },
  [ICON_PIN] = { 9, 12, PIN },
  [ICON_WATCH] = { 11, 12, WATCH },
  [ICON_MOON] = { 11, 11, MOON },
};

static const Icon RUNNER_ICON = { 9, 10, RUNNER };

static void blit(GContext *ctx, const Icon *ic, GPoint o, GColor c) {
  graphics_context_set_stroke_color(ctx, c);
  for (int y = 0; y < ic->h; y++) {
    for (int x = 0; x < ic->w; x++) {
      if (ic->px[y * ic->w + x] == 'X') {
        graphics_draw_pixel(ctx, GPoint(o.x + x, o.y + y));
      }
    }
  }
}

GSize art_icon_size(IconId id) {
  return GSize(ICONS[id].w, ICONS[id].h);
}

void art_icon(GContext *ctx, IconId id, GPoint origin, GColor colour) {
  blit(ctx, &ICONS[id], origin, colour);
}

static void band(GContext *ctx, GPoint a, GPoint b, GPoint c, GPoint d, GColor colour) {
  GPoint pts[4] = { a, b, c, d };
  GPathInfo info = { 4, pts };
  GPath *path = gpath_create(&info);
  if (!path) return;
  graphics_context_set_fill_color(ctx, colour);
  gpath_draw_filled(ctx, path);
  gpath_destroy(path);
}

void art_logo(GContext *ctx, GPoint c, int r, GColor badge) {
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, badge);
  graphics_fill_circle(ctx, c, r);

  // The 2017 badge had its name under the diamond; without it the diamond
  // grows to fill the circle.
  int hw = r * 58 / 100, hh = r * 84 / 100;
  int t = r * 25 / 100;
  if (t < 3) t = 3;
  int ihw = hw - t, ihh = hh - t * hh / hw;

  GPoint ot = GPoint(c.x, c.y - hh), orr = GPoint(c.x + hw, c.y);
  GPoint ob = GPoint(c.x, c.y + hh), ol = GPoint(c.x - hw, c.y);
  GPoint it = GPoint(c.x, c.y - ihh), ir = GPoint(c.x + ihw, c.y);
  GPoint ib = GPoint(c.x, c.y + ihh), il = GPoint(c.x - ihw, c.y);

  // A ribbon: the bands alternate indigo and black round the diamond.
  band(ctx, ot, orr, ir, it, GColorBlack);
  band(ctx, ob, ol, il, ib, GColorBlack);
  band(ctx, ot, ol, il, it, GColorIndigo);
  band(ctx, ob, orr, ir, ib, GColorIndigo);

  blit(ctx, &RUNNER_ICON, GPoint(c.x - RUNNER_ICON.w / 2, c.y - RUNNER_ICON.h / 2), GColorBlack);
}

void art_battery(GContext *ctx, GRect box, int pct, GColor fg, GColor low) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  int bw = box.size.w - 2;
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, fg);
  graphics_draw_rect(ctx, GRect(box.origin.x, box.origin.y, bw, box.size.h));
  graphics_context_set_fill_color(ctx, fg);
  graphics_fill_rect(ctx, GRect(box.origin.x + bw, box.origin.y + box.size.h / 2 - 2, 2, 4), 0, GCornerNone);
  int inner = bw - 4;
  int fill = inner * pct / 100;
  if (pct > 0 && fill < 1) fill = 1;
  graphics_context_set_fill_color(ctx, pct <= 20 ? low : fg);
  graphics_fill_rect(ctx, GRect(box.origin.x + 2, box.origin.y + 2, fill, box.size.h - 4), 0, GCornerNone);
  graphics_context_set_antialiased(ctx, true);
}
