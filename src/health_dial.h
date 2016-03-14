#include <pebble.h>

void health_dial_load(Window *window);

void health_dial_unload();

void health_dial_update(struct tm *tick_time, bool dark);