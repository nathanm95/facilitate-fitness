#include "settings.h"
#include "theme.h"

#define SETTINGS_KEY 1
#define SETTINGS_VERSION 1
#define GOAL_DEFAULT 10000
#define GOAL_MAX 50000

// Stored with a version byte so a later layout change can't misread old bytes.
typedef struct {
  uint8_t version;
  Settings s;
} StoredSettings;

Settings g_settings;

static void set_defaults(void) {
  g_settings = (Settings){
    .theme = THEME_CLASSIC,
    .units = UNITS_AUTO,
    .goal = GOAL_DEFAULT,
  };
}

static void save(void) {
  StoredSettings st = { SETTINGS_VERSION, g_settings };
  persist_write_data(SETTINGS_KEY, &st, sizeof(st));
}

void settings_load(void) {
  set_defaults();
  StoredSettings st;
  if (persist_read_data(SETTINGS_KEY, &st, sizeof(st)) != sizeof(st) || st.version != SETTINGS_VERSION) {
    return;
  }
  g_settings = st.s;
  if (g_settings.theme >= THEME_COUNT) g_settings.theme = THEME_CLASSIC;
  if (g_settings.units > UNITS_MILES) g_settings.units = UNITS_AUTO;
  if (g_settings.goal > GOAL_MAX) g_settings.goal = GOAL_DEFAULT;
}

// Clay sends select values as strings and toggles as integers; accept either
// so a Clay version change can't silently break a setting.
static bool read_int(DictionaryIterator *it, uint32_t key, int32_t *out) {
  Tuple *t = dict_find(it, key);
  if (!t) return false;
  if (t->type == TUPLE_CSTRING) {
    *out = atoi(t->value->cstring);
  } else if (t->length == 1) {
    *out = t->type == TUPLE_INT ? t->value->int8 : t->value->uint8;
  } else if (t->length == 2) {
    *out = t->type == TUPLE_INT ? t->value->int16 : t->value->uint16;
  } else {
    *out = t->value->int32;
  }
  return true;
}

bool settings_handle_inbox(DictionaryIterator *iter) {
  bool any = false;
  int32_t v;
  if (read_int(iter, MESSAGE_KEY_Theme, &v)) {
    g_settings.theme = (v >= 0 && v < THEME_COUNT) ? v : THEME_CLASSIC;
    any = true;
  }
  if (read_int(iter, MESSAGE_KEY_Units, &v)) {
    g_settings.units = (v >= UNITS_AUTO && v <= UNITS_MILES) ? v : UNITS_AUTO;
    any = true;
  }
  if (read_int(iter, MESSAGE_KEY_Goal, &v)) {
    g_settings.goal = (v >= 0 && v <= GOAL_MAX) ? v : GOAL_DEFAULT;
    any = true;
  }
  if (any) save();
  return any;
}
