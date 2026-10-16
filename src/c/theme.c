#include "theme.h"
#include "settings.h"

// Every colour is one of the watch's 64 (each channel 00/55/AA/FF). The
// Classic set is taken from the 2017 face's own logo and background.
static Theme s_themes[THEME_COUNT];
static bool s_ready;

static void build(void) {
  s_themes[THEME_CLASSIC] = (Theme){
    .bg = GColorLavenderIndigo,          // AA55FF
    .text = GColorWhite,
    .soft = GColorIndigo,                // 5500AA
    .panel = GColorIndigo,
    .panel_text = GColorWhite,
    .panel_soft = GColorRichBrilliantLavender,  // FFAAFF
    .icon = GColorRichBrilliantLavender,
    .ring = GColorRichBrilliantLavender,
    .track = GColorIndigo,
    .marker = GColorWhite,
    .badge = GColorRichBrilliantLavender,
  };
  s_themes[THEME_MIDNIGHT] = (Theme){
    .bg = GColorBlack,
    .text = GColorWhite,
    .soft = GColorLavenderIndigo,
    .panel = GColorIndigo,
    .panel_text = GColorWhite,
    .panel_soft = GColorRichBrilliantLavender,
    .icon = GColorRichBrilliantLavender,
    .ring = GColorRichBrilliantLavender,
    .track = GColorImperialPurple,       // 550055
    .marker = GColorWhite,
    .badge = GColorRichBrilliantLavender,
  };
  s_themes[THEME_BLUSH] = (Theme){
    .bg = GColorRichBrilliantLavender,
    .text = GColorIndigo,
    .soft = GColorPurple,                // AA00AA
    .panel = GColorIndigo,
    .panel_text = GColorWhite,
    .panel_soft = GColorRichBrilliantLavender,
    .icon = GColorRichBrilliantLavender,
    .ring = GColorLavenderIndigo,
    .track = GColorWhite,
    .marker = GColorIndigo,
    .badge = GColorWhite,
  };
  s_ready = true;
}

const Theme *theme_get(void) {
  if (!s_ready) build();
#if defined(FF_THEME)
  return &s_themes[FF_THEME % THEME_COUNT];
#else
  return &s_themes[g_settings.theme < THEME_COUNT ? g_settings.theme : THEME_CLASSIC];
#endif
}
