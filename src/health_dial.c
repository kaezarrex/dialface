#include <pebble.h>
#include "dial_layer.h"

#define STEPS_KEY 0

#define STEP_GOAL 10000

static Layer *s_health_layer;
static int current_steps;
static bool dark_theme = false;

static int todays_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if(mask & HealthServiceAccessibilityMaskAvailable) {
    int steps = (int)health_service_sum_today(metric);
    APP_LOG(APP_LOG_LEVEL_INFO, "Steps today: %d", steps);
    return steps;
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
    return 0;
  }
}

static void health_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int radius = bounds.size.w / 2;
  float dial_angle = TRIG_MAX_ANGLE * 6 / 12;
  GColor color = current_steps < STEP_GOAL ? GColorBrightGreen : GColorBrilliantRose;
  
  GPoint dial_center = (GPoint) {
    .x = (int16_t)(sin_lookup(dial_angle) * (int32_t)(radius * 0.5) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(dial_angle) * (int32_t)(radius * 0.5) / TRIG_MAX_RATIO) + center.y,
  };
  draw_dial(layer, ctx, dark_theme, color, dial_center, radius/4, 100, current_steps / 100.0);
}

void health_dial_update(struct tm *tick_time, bool dark) {
  if(tick_time->tm_min % 5 == 0) {
    current_steps = todays_steps();
    layer_mark_dirty(s_health_layer);
  }
  if (dark_theme != dark) {
    dark_theme = dark;
    layer_mark_dirty(s_health_layer);
  }
}

void health_dial_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  if (persist_exists(STEPS_KEY)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Reading persistent step data");
    current_steps = persist_read_int(STEPS_KEY);
  } else {
    current_steps = todays_steps();
  }

  s_health_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_health_layer, health_update_proc);
  layer_add_child(window_get_root_layer(window), s_health_layer);

  layer_mark_dirty(s_health_layer);
}

void health_dial_unload() {
  layer_destroy(s_health_layer);
  persist_write_int(STEPS_KEY, current_steps);
}