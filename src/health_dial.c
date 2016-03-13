#include <pebble.h>
#include "dial_layer.h"

#define STEPS_KEY 0

#define STEP_GOAL 10000

static Layer *s_health_layer;
int current_steps;

static int todays_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if(mask & HealthServiceAccessibilityMaskAvailable) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Steps today: %d", (int)health_service_sum_today(metric));
    return (int)health_service_sum_today(metric);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
    return 0;
  }
}

static float todays_progress() {
  return todays_steps() / (float)STEP_GOAL;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(tick_time->tm_min % 10 == 0) {
    current_steps = todays_progress();
    layer_mark_dirty(s_health_layer);
  }
}

static void health_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int radius = bounds.size.w / 2;
  float dial_angle = TRIG_MAX_ANGLE * 6 / 12;
  
  GPoint dial_center = (GPoint) {
    .x = (int16_t)(sin_lookup(dial_angle) * (int32_t)(radius * 0.5) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(dial_angle) * (int32_t)(radius * 0.5) / TRIG_MAX_RATIO) + center.y,
  };
  draw_dial(layer, ctx, dial_center, 25, 100, current_steps / 100);
}

void health_dial_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  if (persist_exists(STEPS_KEY)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Reading persistent step data");
    current_steps = persist_read_int(STEPS_KEY);
  } else {
    current_steps = todays_progress();
  }

  s_health_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_health_layer, health_update_proc);
  layer_add_child(window_get_root_layer(window), s_health_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  layer_mark_dirty(s_health_layer);
}

void health_dial_unload() {
  layer_destroy(s_health_layer);
  persist_write_int(STEPS_KEY, current_steps);
}