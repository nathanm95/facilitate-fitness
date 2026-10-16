#pragma once
#include <pebble.h>

// Only the Time 2 has a heart-rate sensor; the Round 2 has none, so its
// layout never shows heart rate.
#if defined(PBL_PLATFORM_EMERY)
#define FF_HAS_HRM 1
#else
#define FF_HAS_HRM 0
#endif

typedef struct {
  int32_t steps;
  int32_t goal;        // never 0
  int32_t usual_now;   // steps you'd usually have by now; 0 if Health has no history yet
  int32_t distance_m;
  int32_t active_kcal;
  int32_t active_min;
  int32_t sleep_min;   // last night (and any nap today)
  int32_t hr_bpm;      // 0 = no reading in the last hour
  bool hr_recent;      // the reading is under 15 minutes old
  bool imperial;       // show distance in miles
} FitData;

extern FitData g_fit;

// Re-reads everything from Pebble Health. Cheap enough to call every minute.
void fit_refresh(void);
