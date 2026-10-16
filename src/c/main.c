// Facilitate Fitness 2: the 2017 step-counter face, redrawn for the
// Pebble Time 2 (emery) and Pebble Round 2 (gabbro).
//
// Everything is drawn in one canvas update proc from g_fit (fit.c) and the
// clock. It redraws once a minute and on Health, battery and settings events.

#include <pebble.h>
#include "art.h"
#include "fit.h"
#include "settings.h"
#include "theme.h"

// A system font plus where its digits actually sit inside the text box, so
// mixed sizes can share a baseline and centre on icons. Measured with
// FF_CALIB=1 (see tools/calib.py).
typedef struct {
  GFont font;
  int8_t top;  // box top to digit top
  int8_t cap;  // digit height
} Face;

static Window *s_window;
static Layer *s_canvas;
static Face s_time, s_big, s_val, s_small, s_unit, s_label;
static char s_time_buf[8];
static char s_date_buf[24];
static int s_battery = 100;

static void redraw(void) {
  // Handlers can run before the window exists; layer_mark_dirty doesn't check
  // for NULL and only the emulator forgives it.
  if (s_canvas) layer_mark_dirty(s_canvas);
}

// ---------------------------------------------------------------- text ----

static int text_w(const char *s, const Face *f) {
  return graphics_text_layout_get_content_size(s, f->font, GRect(0, 0, 400, 100),
                                               GTextOverflowModeFill, GTextAlignmentLeft).w;
}

// Draws `s` with its digits' top edge at `cap_y`.
static void text_at(GContext *ctx, const char *s, const Face *f, int x, int cap_y, int w,
                    GTextAlignment align, GColor colour) {
  graphics_context_set_text_color(ctx, colour);
  graphics_draw_text(ctx, s, f->font, GRect(x, cap_y - f->top, w, f->top + f->cap + 12),
                     GTextOverflowModeFill, align, NULL);
}

static void thousands(char *buf, size_t n, int32_t v) {
  if (v >= 1000) {
    snprintf(buf, n, "%d,%03d", (int)(v / 1000), (int)(v % 1000));
  } else {
    snprintf(buf, n, "%d", (int)v);
  }
}

typedef struct {
  char value[12];
  char unit[6];
} Reading;

static Reading fmt_distance(void) {
  Reading r;
  int32_t tenths = g_fit.imperial ? (g_fit.distance_m * 10 + 804) / 1609 : (g_fit.distance_m + 50) / 100;
  if (tenths >= 1000) {
    snprintf(r.value, sizeof(r.value), "%d", (int)(tenths / 10));
  } else {
    snprintf(r.value, sizeof(r.value), "%d.%d", (int)(tenths / 10), (int)(tenths % 10));
  }
  snprintf(r.unit, sizeof(r.unit), "%s", g_fit.imperial ? "mi" : "km");
  return r;
}

static Reading fmt_sleep(void) {
  Reading r;
  if (g_fit.sleep_min <= 0) {
    snprintf(r.value, sizeof(r.value), "--");
  } else {
    snprintf(r.value, sizeof(r.value), "%dh %02dm", (int)(g_fit.sleep_min / 60 % 100), (int)(g_fit.sleep_min % 60));
  }
  r.unit[0] = '\0';
  return r;
}

static Reading fmt_plain(int32_t v, const char *unit) {
  Reading r;
  snprintf(r.value, sizeof(r.value), "%d", (int)v);
  snprintf(r.unit, sizeof(r.unit), "%s", unit);
  return r;
}

static Reading __attribute__((unused)) fmt_heart(void) {
  Reading r;
  if (g_fit.hr_bpm > 0) {
    snprintf(r.value, sizeof(r.value), "%d", (int)g_fit.hr_bpm);
  } else {
    snprintf(r.value, sizeof(r.value), "--");
  }
  snprintf(r.unit, sizeof(r.unit), "bpm");
  return r;
}

// Icon, value and unit as one centred group, the icon centred on the digits
// and the unit sharing the value's baseline.
static void metric(GContext *ctx, int cx, int cap_y, IconId icon, const Reading *r,
                   const Face *vf, const Face *uf, GColor icon_c, GColor val_c, GColor unit_c) {
  GSize is = art_icon_size(icon);
  int vw = text_w(r->value, vf);
  int uw = r->unit[0] ? text_w(r->unit, uf) : 0;
  int gap = 4, ugap = uw ? 2 : 0;
  int total = is.w + gap + vw + ugap + uw;
  int x = cx - total / 2;
  art_icon(ctx, icon, GPoint(x, cap_y + vf->cap / 2 - is.h / 2), icon_c);
  x += is.w + gap;
  text_at(ctx, r->value, vf, x - 1, cap_y, vw + 4, GTextAlignmentLeft, val_c);
  x += vw + ugap;
  if (uw) {
    text_at(ctx, r->unit, uf, x, cap_y + vf->cap - uf->cap, uw + 4, GTextAlignmentLeft, unit_c);
  }
}

// ---------------------------------------------------------------- ring ----

static GPoint on_ring(GRect box, int thick, int32_t angle) {
  GRect mid = grect_inset(box, GEdgeInsets(thick / 2));
  return gpoint_from_polar(mid, GOvalScaleModeFitCircle, angle);
}

// Step progress: a full lap is the goal; past it, a second lap in the marker
// colour. A short tick shows where your usual day is at this time.
static void step_ring(GContext *ctx, GRect box, int thick, const Theme *t) {
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, t->track);
  graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thick, 0, TRIG_MAX_ANGLE);

  int32_t permille = g_fit.steps * 1000 / g_fit.goal;
  int32_t lap1 = permille > 1000 ? 1000 : permille;
  if (lap1 > 0) {
    int32_t end = TRIG_MAX_ANGLE * lap1 / 1000;
    graphics_context_set_fill_color(ctx, t->ring);
    graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thick, 0, end);
    // Round both ends of the arc.
    graphics_fill_circle(ctx, on_ring(box, thick, 0), thick / 2);
    if (lap1 < 1000) graphics_fill_circle(ctx, on_ring(box, thick, end), thick / 2);
  }
  if (permille > 1000) {
    int32_t lap2 = permille - 1000 > 1000 ? 1000 : permille - 1000;
    int32_t end = TRIG_MAX_ANGLE * lap2 / 1000;
    graphics_context_set_fill_color(ctx, t->marker);
    graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thick, 0, end);
    graphics_fill_circle(ctx, on_ring(box, thick, 0), thick / 2);
    graphics_fill_circle(ctx, on_ring(box, thick, end), thick / 2);
  }

  if (g_fit.usual_now > 0 && g_fit.usual_now < g_fit.goal && permille <= 1000) {
    int32_t a = TRIG_MAX_ANGLE * g_fit.usual_now / g_fit.goal;
    GPoint outer = gpoint_from_polar(grect_inset(box, GEdgeInsets(-1)), GOvalScaleModeFitCircle, a);
    GPoint inner = gpoint_from_polar(grect_inset(box, GEdgeInsets(thick + 1)), GOvalScaleModeFitCircle, a);
    graphics_context_set_stroke_color(ctx, t->marker);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_line(ctx, outer, inner);
    graphics_context_set_stroke_width(ctx, 1);
  }
}

static int __attribute__((unused)) step_pct(void) {
  return (int)(g_fit.steps * 100 / g_fit.goal);
}

// ------------------------------------------------------------- layouts ----

#if defined(PBL_ROUND)

// Round 2, 260x260: the bezel is the step ring, the badge sits at the top.
static void draw_face(GContext *ctx, GRect b, const Theme *t) {
  int cx = b.size.w / 2;
  step_ring(ctx, grect_inset(b, GEdgeInsets(3)), 10, t);

  art_logo(ctx, GPoint(cx, 44), 22, t->badge);
  text_at(ctx, s_date_buf, &s_label, 0, 72, b.size.w, GTextAlignmentCenter, t->text);
  text_at(ctx, s_time_buf, &s_time, 0, 92, b.size.w, GTextAlignmentCenter, t->text);

  // "7,342 steps" on one baseline.
  char steps[12];
  thousands(steps, sizeof(steps), g_fit.steps);
  const char *word = " steps";
  int sw = text_w(steps, &s_big), ww = text_w(word, &s_small);
  int x = cx - (sw + ww) / 2;
  int cap_y = 134;
  text_at(ctx, steps, &s_big, x, cap_y, sw + 4, GTextAlignmentLeft, t->text);
  text_at(ctx, word, &s_small, x + sw, cap_y + s_big.cap - s_small.cap, ww + 4, GTextAlignmentLeft, t->soft);

  GRect panel = GRect(cx - 84, 160, 168, 52);
  graphics_context_set_fill_color(ctx, t->panel);
  graphics_fill_rect(ctx, panel, 12, GCornersAll);
  Reading kcal = fmt_plain(g_fit.active_kcal, "kcal");
  Reading dist = fmt_distance();
  Reading act = fmt_plain(g_fit.active_min, "min");
  Reading sleep = fmt_sleep();
  int lx = cx - 42, rx = cx + 42;
  int r1 = panel.origin.y + 7, r2 = panel.origin.y + 7 + 22;
  metric(ctx, lx, r1, ICON_FLAME, &kcal, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
  metric(ctx, rx, r1, ICON_PIN, &dist, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
  metric(ctx, lx, r2, ICON_WATCH, &act, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
  metric(ctx, rx, r2, ICON_MOON, &sleep, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);

  art_battery(ctx, GRect(cx - 11, 220, 22, 11), s_battery, t->text, GColorFolly);
}

#else

// Time 2, 200x228: badge inside the step ring, heart rate beside it, and a
// card of the rest underneath.
static void draw_face(GContext *ctx, GRect b, const Theme *t) {
  int w = b.size.w;

  text_at(ctx, s_date_buf, &s_label, 8, 6, 120, GTextAlignmentLeft, t->text);
  char pct[6];
  snprintf(pct, sizeof(pct), "%d%%", s_battery);
  int pw = text_w(pct, &s_unit);
  art_battery(ctx, GRect(w - 8 - 22, 7, 22, 11), s_battery, t->text, GColorFolly);
  text_at(ctx, pct, &s_unit, w - 8 - 22 - 4 - pw, 8, pw + 4, GTextAlignmentLeft, t->text);

  text_at(ctx, s_time_buf, &s_time, 0, 30, w, GTextAlignmentCenter, t->text);

  // A gap of background between badge and ring keeps the pink badge from
  // reading as part of the pink progress.
  GRect ring = GRect(8, 72, 78, 78);
  step_ring(ctx, ring, 8, t);
  art_logo(ctx, grect_center_point(&ring), 27, t->badge);

  int x0 = 96, cw = w - x0 - 4;
  char steps[12];
  thousands(steps, sizeof(steps), g_fit.steps);
  text_at(ctx, steps, &s_big, x0, 80, cw, GTextAlignmentLeft, t->text);
  char label[24];
  snprintf(label, sizeof(label), "STEPS  %d%%", step_pct());
  text_at(ctx, label, &s_label, x0, 104, cw, GTextAlignmentLeft, t->soft);

  Reading hr = fmt_heart();
  GColor heart = g_fit.hr_recent ? t->text : t->soft;
  GSize hs = art_icon_size(ICON_HEART);
  int hr_y = 126;
  art_icon(ctx, ICON_HEART, GPoint(x0, hr_y + s_val.cap / 2 - hs.h / 2), GColorFolly);
  int vw = text_w(hr.value, &s_val);
  text_at(ctx, hr.value, &s_val, x0 + hs.w + 5, hr_y, vw + 4, GTextAlignmentLeft, heart);
  text_at(ctx, hr.unit, &s_unit, x0 + hs.w + 5 + vw + 3, hr_y + s_val.cap - s_unit.cap, 40,
          GTextAlignmentLeft, t->soft);

  GRect panel = GRect(4, 156, w - 8, 68);
  graphics_context_set_fill_color(ctx, t->panel);
  graphics_fill_rect(ctx, panel, 10, GCornersAll);
  Reading kcal = fmt_plain(g_fit.active_kcal, "kcal");
  Reading dist = fmt_distance();
  Reading act = fmt_plain(g_fit.active_min, "min");
  Reading sleep = fmt_sleep();
  int lx = panel.origin.x + panel.size.w / 4, rx = panel.origin.x + panel.size.w * 3 / 4;
  int r1 = panel.origin.y + 9, r2 = panel.origin.y + 9 + 33;
  metric(ctx, lx, r1, ICON_FLAME, &kcal, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
  metric(ctx, rx, r1, ICON_PIN, &dist, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
  metric(ctx, lx, r2, ICON_WATCH, &act, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
  metric(ctx, rx, r2, ICON_MOON, &sleep, &s_val, &s_unit, t->icon, t->panel_text, t->panel_soft);
}

#endif

#if defined(FF_CALIB)
// Each face's "0" in its own column, box top at y=20, for tools/calib.py.
static void draw_calib(GContext *ctx, GRect b) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  const Face *faces[] = { &s_time, &s_big, &s_val, &s_small, &s_unit, &s_label };
  int x = 4;
  for (unsigned i = 0; i < ARRAY_LENGTH(faces); i++) {
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "0", faces[i]->font, GRect(x, 20, 40, 80), GTextOverflowModeFill,
                       GTextAlignmentLeft, NULL);
    x += 32;
  }
}
#endif

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
#if defined(FF_CALIB)
  draw_calib(ctx, b);
  return;
#endif
  const Theme *t = theme_get();
  graphics_context_set_fill_color(ctx, t->bg);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  draw_face(ctx, b, t);
}

// -------------------------------------------------------------- events ----

static void update_clock(struct tm *tm) {
  int hour = tm->tm_hour, min = tm->tm_min;
#if defined(FF_SHOW_TIME)
  hour = FF_SHOW_TIME / 100;
  min = FF_SHOW_TIME % 100;
#endif
  if (!clock_is_24h_style()) {
    hour %= 12;
    if (hour == 0) hour = 12;
    snprintf(s_time_buf, sizeof(s_time_buf), "%d:%02d", hour, min);
  } else {
    snprintf(s_time_buf, sizeof(s_time_buf), "%02d:%02d", hour, min);
  }

  char day[8], month[8];
  strftime(day, sizeof(day), "%a", tm);
  strftime(month, sizeof(month), "%b", tm);
  snprintf(s_date_buf, sizeof(s_date_buf), "%s %d %s", day, tm->tm_mday, month);
  for (char *p = s_date_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') *p -= 'a' - 'A';
  }
}

static void tick_handler(struct tm *tm, TimeUnits changed) {
  update_clock(tm);
  fit_refresh();
  redraw();
}

static void health_handler(HealthEventType event, void *context) {
  fit_refresh();
  redraw();
}

static void battery_handler(BatteryChargeState state) {
  s_battery = state.charge_percent;
#if defined(FF_FAKE)
  s_battery = 80;
#endif
  redraw();
}

static void inbox_handler(DictionaryIterator *iter, void *context) {
  if (settings_handle_inbox(iter)) {
    fit_refresh();
    redraw();
  }
}

// ---------------------------------------------------------- lifecycle ----

static Face face(const char *key, int top, int cap) {
  return (Face){ fonts_get_system_font(key), top, cap };
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

static void init(void) {
  settings_load();
  s_time = face(FONT_KEY_BITHAM_42_BOLD, 12, 30);
  s_big = face(FONT_KEY_GOTHIC_28_BOLD, 10, 18);
  s_val = face(FONT_KEY_GOTHIC_24_BOLD, 10, 14);
  s_small = face(FONT_KEY_GOTHIC_18_BOLD, 7, 11);
  s_unit = face(FONT_KEY_GOTHIC_14_BOLD, 5, 9);
  s_label = face(FONT_KEY_GOTHIC_18_BOLD, 7, 11);

  time_t now = time(NULL);
  update_clock(localtime(&now));
  fit_refresh();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // Services only once the window (and so the canvas) exists.
  battery_handler(battery_state_service_peek());
  battery_state_service_subscribe(battery_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  health_service_events_subscribe(health_handler, NULL);
#if defined(FF_HR_PERIOD) && FF_HAS_HRM
  // The emulator only takes `pebble emu-heart-rate` while the sensor is on.
  // The shipped face leaves sampling to the watch's own Health setting.
  health_service_set_heart_rate_sample_period(FF_HR_PERIOD);
#endif
  app_message_register_inbox_received(inbox_handler);
  app_message_open(128, 32);
}

static void deinit(void) {
  health_service_events_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
