#include <pebble.h>
#include "dial_layer.h"

#define TEMPERATURE_KEY 1

static Layer *s_temperature_layer;
static int current_temperature;
static bool dark_theme = false;

static void temperature_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int radius = bounds.size.w / 2;
  float dial_angle = TRIG_MAX_ANGLE * 9 / 12;
  GColor color = current_temperature < 20 ? GColorVividCerulean : GColorChromeYellow;
  
  GPoint dial_center = (GPoint) {
    .x = (int16_t)(sin_lookup(dial_angle) * (int32_t)(radius * 0.5) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(dial_angle) * (int32_t)(radius * 0.5) / TRIG_MAX_RATIO) + center.y,
  };
  draw_dial(layer, ctx, dark_theme, color, dial_center, radius/4, 40, current_temperature);
}

void temperature_dial_update(int temperature, bool dark) {
  current_temperature = temperature;
  dark_theme = dark;
  layer_mark_dirty(s_temperature_layer);
  APP_LOG(APP_LOG_LEVEL_INFO, "Temperature: %d", current_temperature);
}

void temperature_dial_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  if (persist_exists(TEMPERATURE_KEY)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Reading persistent temperature data");
    current_temperature = persist_read_int(TEMPERATURE_KEY);
  } else {
    current_temperature = 0;
  }

  s_temperature_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_temperature_layer, temperature_update_proc);
  layer_add_child(window_get_root_layer(window), s_temperature_layer);

  layer_mark_dirty(s_temperature_layer);
}

void temperature_dial_unload() {
  layer_destroy(s_temperature_layer);
  persist_write_int(TEMPERATURE_KEY, current_temperature);
}