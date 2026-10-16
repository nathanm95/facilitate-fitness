#pragma once
#include <pebble.h>

typedef enum { UNITS_AUTO = 0, UNITS_KM = 1, UNITS_MILES = 2 } Units;

typedef struct {
  uint8_t theme;   // ThemeId, see theme.h
  uint8_t units;   // Units
  uint16_t goal;   // daily step goal; 0 = "your usual day" from Pebble Health
} Settings;

extern Settings g_settings;

void settings_load(void);
// Applies a Clay settings message; true if anything changed.
bool settings_handle_inbox(DictionaryIterator *iter);
