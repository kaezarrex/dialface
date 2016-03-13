#include <pebble.h>

#define COLORS       PBL_IF_COLOR_ELSE(true, false)
#define ANTIALIASING true

#define HAND_MARGIN  20

typedef struct {
  int hours;
  int minutes;
} Time;

static Window *s_main_window;
static Layer *s_canvas_layer;

static GPoint s_center;
static Time s_last_time;
static int s_radius = 90, s_color_channels[3];

/************************************ UI **************************************/

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  // Store time
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;

  for(int i = 0; i < 3; i++) {
    s_color_channels[i] = rand() % 256;
  }

  // Redraw
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void draw_dial(Layer *layer, GContext *ctx, GPoint center, int radius) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 3);
  
  for(int i = 0; i < 10; i++) {
    float angle = TRIG_MAX_ANGLE * i / 10;
    GPoint outer_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(radius) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(radius) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint inner_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(radius * 0.9) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(radius * 0.9) / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, outer_point, inner_point);
  }
  
  graphics_context_set_stroke_width(ctx, 3);
  
  float hand_angle = TRIG_MAX_ANGLE * s_last_time.minutes / 60;

  // Plot hands
  GPoint hand_point = (GPoint) {
    .x = (int16_t)(sin_lookup(hand_angle) * (int32_t)(radius * 0.7) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hand_angle) * (int32_t)(radius * 0.7) / TRIG_MAX_RATIO) + center.y,
  };
  
  graphics_context_set_stroke_color(ctx, GColorBrightGreen);
  graphics_draw_line(ctx, center, hand_point);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 3);
}

static void update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_antialiased(ctx, ANTIALIASING);

  // White clockface
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_center, s_radius);

  // Draw ticks
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 5);
  
  for(int i = 0; i < 12; i++) {
    float angle = TRIG_MAX_ANGLE * i / 12;
    GPoint outer_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(s_radius) / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(s_radius) / TRIG_MAX_RATIO) + s_center.y,
    };
    GPoint inner_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(s_radius * 0.92) / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(s_radius * 0.92) / TRIG_MAX_RATIO) + s_center.y,
    };
    graphics_draw_line(ctx, outer_point, inner_point);
  }
  
  // Draw dial
  float dial_angle = TRIG_MAX_ANGLE * 6 / 12;
  GPoint dial_center = (GPoint) {
    .x = (int16_t)(sin_lookup(dial_angle) * (int32_t)(s_radius * 0.5) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(dial_angle) * (int32_t)(s_radius * 0.5) / TRIG_MAX_RATIO) + s_center.y,
  };
  draw_dial(layer, ctx, dial_center, 25);

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
  
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_draw_line(ctx, s_center, minute_hand_short);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, minute_hand_short, minute_hand);
  
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_draw_line(ctx, s_center, hour_hand_short);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, hour_hand_short, hour_hand);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_center, 5);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&window_bounds);

  s_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

/*********************************** App **************************************/

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
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
