#pragma once
#include <pebble.h>

// Saved settings are indexes into this enum: never reorder, only append.
typedef enum {
  THEME_CLASSIC,   // the 2017 face: lavender, pink badge, indigo diamond
  THEME_MIDNIGHT,  // black, for battery and the dark
  THEME_BLUSH,     // the badge's pink as the background
  THEME_COUNT
} ThemeId;

typedef struct {
  GColor bg;          // screen
  GColor text;        // time, steps, date
  GColor soft;        // labels on the background
  GColor panel;       // the metrics card
  GColor panel_text;  // values on the card
  GColor panel_soft;  // units on the card
  GColor icon;        // icons on the card
  GColor ring;        // step progress
  GColor track;       // the unfilled ring
  GColor marker;      // "usual by now" tick and the second lap past 100%
  GColor badge;       // the logo's circle
} Theme;

const Theme *theme_get(void);
