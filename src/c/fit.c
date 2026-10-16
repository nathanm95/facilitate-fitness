#include "fit.h"
#include "settings.h"

#define GOAL_FALLBACK 10000
// Sleep is counted from 6pm the day before, so last night's sleep shows all
// day even when you went to bed before midnight.
#define SLEEP_LOOKBACK (6 * SECONDS_PER_HOUR)
#define HR_LOOKBACK_MIN 60

FitData g_fit;

#if FF_HAS_HRM
static HealthMinuteData s_minutes[HR_LOOKBACK_MIN];
#endif

static bool available(HealthMetric m, time_t start, time_t end) {
  return health_service_metric_accessible(m, start, end) & HealthServiceAccessibilityMaskAvailable;
}

static int32_t sum(HealthMetric m, time_t start, time_t end) {
  return available(m, start, end) ? health_service_sum(m, start, end) : 0;
}

static int32_t usual(HealthMetric m, time_t start, time_t end) {
  if (!(health_service_metric_averaged_accessible(m, start, end, HealthServiceTimeScopeDailyWeekdayOrWeekend) &
        HealthServiceAccessibilityMaskAvailable)) {
    return 0;
  }
  return health_service_sum_averaged(m, start, end, HealthServiceTimeScopeDailyWeekdayOrWeekend);
}

#if FF_HAS_HRM
// The filtered reading is only kept for 15 minutes and the watch samples about
// every 10, so between readings fall back to the newest one in the last hour
// rather than flashing "--".
static void read_heart_rate(time_t now) {
  g_fit.hr_bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
  g_fit.hr_recent = g_fit.hr_bpm > 0;
  if (g_fit.hr_bpm > 0) return;

  time_t start = now - HR_LOOKBACK_MIN * SECONDS_PER_MINUTE, end = now;
  uint32_t n = health_service_get_minute_history(s_minutes, HR_LOOKBACK_MIN, &start, &end);
  for (int i = (int)n - 1; i >= 0; i--) {
    if (!s_minutes[i].is_invalid && s_minutes[i].heart_rate_bpm > 0) {
      g_fit.hr_bpm = s_minutes[i].heart_rate_bpm;
      return;
    }
  }
}
#endif

void fit_refresh(void) {
  time_t now = time(NULL);
  time_t today = time_start_of_today();

  g_fit.steps = sum(HealthMetricStepCount, today, now);
  g_fit.distance_m = sum(HealthMetricWalkedDistanceMeters, today, now);
  g_fit.active_kcal = sum(HealthMetricActiveKCalories, today, now);
  g_fit.active_min = sum(HealthMetricActiveSeconds, today, now) / SECONDS_PER_MINUTE;
  g_fit.sleep_min = sum(HealthMetricSleepSeconds, today - SLEEP_LOOKBACK, now) / SECONDS_PER_MINUTE;

  int32_t usual_day = usual(HealthMetricStepCount, today, today + SECONDS_PER_DAY);
  g_fit.goal = g_settings.goal ? g_settings.goal : usual_day;
  if (g_fit.goal <= 0) g_fit.goal = GOAL_FALLBACK;
  g_fit.usual_now = usual(HealthMetricStepCount, today, now);

#if FF_HAS_HRM
  read_heart_rate(now);
#else
  g_fit.hr_bpm = 0;
  g_fit.hr_recent = false;
#endif

  if (g_settings.units == UNITS_AUTO) {
    g_fit.imperial = health_service_get_measurement_system_for_display(HealthMetricWalkedDistanceMeters) ==
                     MeasurementSystemImperial;
  } else {
    g_fit.imperial = g_settings.units == UNITS_MILES;
  }

#if defined(FF_FAKE)
  // A believable mid-afternoon, for store shots: the emulator's Health is empty.
#if defined(FF_PCT)
  int pct = FF_PCT;
#else
  int pct = 73;
#endif
#if defined(FF_USUAL)
  int usual_steps = FF_USUAL;
#else
  int usual_steps = 6100;
#endif
#if defined(FF_BPM)
  int bpm = FF_BPM;
#else
  int bpm = 72;
#endif
  g_fit.goal = 10000;
  g_fit.steps = g_fit.goal * pct / 100;
  g_fit.usual_now = usual_steps;
  g_fit.distance_m = g_fit.steps * 71 / 100;
  g_fit.active_kcal = g_fit.steps * 43 / 1000;
  g_fit.active_min = g_fit.steps / 160;
  g_fit.sleep_min = 7 * 60 + 12;
  g_fit.hr_bpm = FF_HAS_HRM ? bpm : 0;
  g_fit.hr_recent = FF_HAS_HRM;
  g_fit.imperial = false;
#endif
}
