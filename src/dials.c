#include <pebble.h>
#include "health_dial.h"
#include "temperature_dial.h"

#define COLORS       PBL_IF_COLOR_ELSE(true, false)
#define ANTIALIASING true

#define HAND_MARGIN  20
#define SHADOW_OFFSET_X  2
#define SHADOW_OFFSET_Y  2

typedef struct {
  int hours;
  int minutes;
} Time;

typedef enum {
  AppKeyTemperature = 0,
  AppKeySunrise,
  AppKeySunset,
} AppKeys;

static Window *s_main_window;
static Layer *s_canvas_layer;
static Layer *s_shadow_layer;
static Layer *s_time_layer;

static GPoint s_center;
static Time s_last_time;
static int s_radius = 90;
static bool s_dark = false;

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *temperature_tuple = dict_find(iter, AppKeyTemperature);
  Tuple *sunrise_tuple = dict_find(iter, AppKeySunrise);
  Tuple *sunset_tuple = dict_find(iter, AppKeySunset);
  if(temperature_tuple) {
    int32_t temperature = temperature_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature Received: %d", (int)temperature);
    temperature_dial_update(temperature, s_dark);
  }
  if(sunrise_tuple) {
    int32_t sunrise = sunrise_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sunrise Received: %d", (int)sunrise);
  }
  if(sunset_tuple) {
    int32_t sunset = sunset_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sunset Received: %d", (int)sunset);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  // Store time
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;

  // Redraw
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
    layer_mark_dirty(s_shadow_layer);
    health_dial_update(tick_time, s_dark);
  }
  
}

static void update_canvas_proc(Layer *layer, GContext *ctx) {
  if (s_dark) {
    graphics_context_set_fill_color(ctx, GColorBlack);
  } else {
    graphics_context_set_fill_color(ctx, GColorWhite);
  }
  graphics_fill_circle(ctx, s_center, s_radius);
}

static void update_time_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_antialiased(ctx, ANTIALIASING);
  GColor hand_main_color = GColorBlack;
  GColor hand_accent_color = GColorDarkGray;
  GColor tick_color = GColorLightGray;
  
  if (s_dark){
    hand_main_color = GColorWhite;
    hand_accent_color = GColorLightGray;
    tick_color = GColorDarkGray;
  }

  // Draw ticks
  graphics_context_set_stroke_color(ctx, hand_main_color);
  graphics_context_set_stroke_width(ctx, 3);
  
  for(int i = 0; i < 60; i++) {
    int r = s_radius * 0.96;
    
    if (i % 5 == 0) {
      graphics_context_set_stroke_color(ctx, hand_main_color);
      graphics_context_set_stroke_width(ctx, 3);
      r = s_radius * 0.92;
    } else if (i % 5 == 1) {
      graphics_context_set_stroke_color(ctx, tick_color);
      graphics_context_set_stroke_width(ctx, 3);
    }
    
    float angle = TRIG_MAX_ANGLE * i / 60;
    GPoint outer_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(s_radius) / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(s_radius) / TRIG_MAX_RATIO) + s_center.y,
    };
    GPoint inner_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(r) / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(r) / TRIG_MAX_RATIO) + s_center.y,
    };
    graphics_draw_line(ctx, outer_point, inner_point);
  }

  // Adjust for minutes through the hour
  float minute_angle = TRIG_MAX_ANGLE * s_last_time.minutes / 60;
  float hour_angle = TRIG_MAX_ANGLE * s_last_time.hours / 12;
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);

  // Plot hands
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.y,
  };
  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(s_radius - (2.2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - (2.2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + s_center.y,
  };
  GPoint minute_hand_short = (GPoint) {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)(s_radius - HAND_MARGIN * 1.5) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)(s_radius - HAND_MARGIN * 1.5) / TRIG_MAX_RATIO) + s_center.y,
  };
  GPoint hour_hand_short = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(s_radius - (2.7 * HAND_MARGIN)) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - (2.7 * HAND_MARGIN)) / TRIG_MAX_RATIO) + s_center.y,
  };
  
  graphics_context_set_stroke_width(ctx, 5);
  
  graphics_context_set_stroke_color(ctx, hand_accent_color);
  graphics_draw_line(ctx, s_center, minute_hand_short);
  graphics_context_set_stroke_color(ctx, hand_main_color);
  graphics_draw_line(ctx, minute_hand_short, minute_hand);
  
  graphics_context_set_stroke_color(ctx, hand_accent_color);
  graphics_draw_line(ctx, s_center, hour_hand_short);
  graphics_context_set_stroke_color(ctx, hand_main_color);
  graphics_draw_line(ctx, hour_hand_short, hour_hand);
  
  graphics_context_set_fill_color(ctx, hand_main_color);
  graphics_fill_circle(ctx, s_center, 5);
}

static void update_shadow_proc(Layer *layer, GContext *ctx) {
  if (s_dark){ return; }
  
  graphics_context_set_antialiased(ctx, ANTIALIASING);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  
  float minute_angle = TRIG_MAX_ANGLE * s_last_time.minutes / 60;
  float hour_angle = TRIG_MAX_ANGLE * s_last_time.hours / 12;
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);
  GPoint center = (GPoint) {
    .x = s_center.x + SHADOW_OFFSET_X,
    .y = s_center.y + SHADOW_OFFSET_Y,
  };
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(s_radius - (2.2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - (2.2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + center.y,
  };
  
  graphics_draw_line(ctx, center, minute_hand);
  graphics_draw_line(ctx, center, hour_hand);
  graphics_fill_circle(ctx, center, 5);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&window_bounds);

  s_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_canvas_layer, update_canvas_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  s_shadow_layer = layer_create(window_bounds);
  layer_set_update_proc(s_shadow_layer, update_shadow_proc);
  layer_add_child(window_layer, s_shadow_layer);
  
  health_dial_load(window);
  temperature_dial_load(window);
  
  s_time_layer = layer_create(window_bounds);
  layer_set_update_proc(s_time_layer, update_time_proc);
  layer_add_child(window_layer, s_time_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  layer_destroy(s_shadow_layer);
  health_dial_unload();
  temperature_dial_unload();
}

static void init() {
  srand(time(NULL));

  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  tick_handler(time_now, MINUTE_UNIT);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register app_message callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  
  // Largest expected inbox and outbox message sizes
  const uint32_t inbox_size = 256;
  const uint32_t outbox_size = 256;

  // Open AppMessage
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
