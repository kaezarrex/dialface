#include <pebble.h>

void draw_dial(Layer *layer, GContext *ctx, GPoint center, int radius, int max, float value) {
  
  for(int i = 0; i < max; i+=2) {
    int r = radius * 0.95;
    
    if (i % 10 == 0) {
      graphics_context_set_stroke_color(ctx, GColorLightGray);
      graphics_context_set_stroke_width(ctx, 3);
      r = radius * 0.9;
    } else if (i % 10 == 2) {
      graphics_context_set_stroke_color(ctx, GColorDarkGray);
      graphics_context_set_stroke_width(ctx, 1);
    }
    
    float angle = TRIG_MAX_ANGLE * i / max;
    GPoint outer_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(radius) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(radius) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint inner_point = (GPoint) {
      .x = (int16_t)(sin_lookup(angle) * (int32_t)(r) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * (int32_t)(r) / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, outer_point, inner_point);
  }
  
  graphics_context_set_stroke_width(ctx, 3);
  
  float hand_angle = TRIG_MAX_ANGLE * value / max;

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