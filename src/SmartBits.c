#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xE2, 0xE6, 0x9A, 0x43, 0xE3, 0xDC, 0x4F, 0x27, 0xBE, 0xB0, 0x8B, 0xAA, 0xAB, 0x18, 0x54, 0x1D }

PBL_APP_INFO(MY_UUID,
             "Binary Calendar", "Gus Monod",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

// --------- Defining global constants

#define SCREEN_WIDTH  144
#define SCREEN_HEIGHT 168

#define NB_COLS    5
#define NB_ROWS    6
#define SPACE      4
#define DOT_RADIUS 12

enum Column { C_MONTH, C_MONTH_DAY, C_HOUR, C_MINUTE, C_SECOND };
// 6th line
#define L_WEEK_DAY 5

// --------- End of definition of global constants

Window window;

// Layer containing all the circles (LEDs)
Layer led_layer;

// Gets the center of a specific LED
GPoint getCenter(int col, int row) {
  return GPoint(DOT_RADIUS * (1 + 2 * col) + SPACE * col + SPACE,
                DOT_RADIUS * (1 + 2 * row) + SPACE * row + SPACE / 2);
}

// Toggles the state of a given LED
void toggle_led(GContext *ctx, int col, int row, bool on) {
  graphics_context_set_fill_color(ctx, on ? GColorWhite : GColorBlack);
  graphics_fill_circle(ctx, getCenter(col, row), DOT_RADIUS);
}

void led_layer_update_callback(Layer *me, GContext *ctx) {
  // Gets the time
  PblTm t;
  get_time(&t);

  bool           is_24h_style = clock_is_24h_style();
  unsigned short hour         = is_24h_style ? t.tm_hour : t.tm_hour % 12;
                 hour         = !is_24h_style && 0 == hour ? 12 : hour;
  bool           pm           = t.tm_hour / 12;
  unsigned short month        = t.tm_mon + 1;
  unsigned short week_day     = 0 != t.tm_wday ? t.tm_wday : 7;// 0 as 7

  toggle_led(ctx, C_SECOND, 0, t.tm_sec & 1);
  toggle_led(ctx, C_SECOND, 1, t.tm_sec & 2);
  toggle_led(ctx, C_SECOND, 2, t.tm_sec & 4);
  toggle_led(ctx, C_SECOND, 3, t.tm_sec & 8);
  toggle_led(ctx, C_SECOND, 4, t.tm_sec & 16);
  toggle_led(ctx, C_SECOND, 5, t.tm_sec & 32);

  toggle_led(ctx, C_MINUTE, 0, t.tm_min & 1);
  toggle_led(ctx, C_MINUTE, 1, t.tm_min & 2);
  toggle_led(ctx, C_MINUTE, 2, t.tm_min & 4);
  toggle_led(ctx, C_MINUTE, 3, t.tm_min & 8);
  toggle_led(ctx, C_MINUTE, 4, t.tm_min & 16);
  toggle_led(ctx, C_MINUTE, 5, t.tm_min & 32);

  toggle_led(ctx, C_HOUR, 0, hour & 1);
  toggle_led(ctx, C_HOUR, 1, hour & 2);
  toggle_led(ctx, C_HOUR, 2, hour & 4);
  toggle_led(ctx, C_HOUR, 3, hour & 8);
  toggle_led(ctx, C_HOUR, 4, is_24h_style ? hour & 16 : pm);
  // last bit of hour is week_day lowest bit
  toggle_led(ctx, C_HOUR, L_WEEK_DAY, week_day & 1);

  toggle_led(ctx, C_MONTH_DAY, 0, t.tm_mday & 1);
  toggle_led(ctx, C_MONTH_DAY, 1, t.tm_mday & 2);
  toggle_led(ctx, C_MONTH_DAY, 2, t.tm_mday & 4);
  toggle_led(ctx, C_MONTH_DAY, 3, t.tm_mday & 8);
  toggle_led(ctx, C_MONTH_DAY, 4, t.tm_mday & 16);
  // last bit of hour is week_day middle bit
  toggle_led(ctx, C_MONTH_DAY, L_WEEK_DAY, week_day & 2);

  toggle_led(ctx, C_MONTH, 0, month & 1);
  toggle_led(ctx, C_MONTH, 1, month & 2);
  toggle_led(ctx, C_MONTH, 2, month & 4);
  toggle_led(ctx, C_MONTH, 3, month & 8);
  // last bit of month is week_day highest bit
  toggle_led(ctx, C_MONTH, L_WEEK_DAY, week_day & 4);

}

void handle_init(AppContextRef ctx) {

  window_init(&window, "Binary dots");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  // where to display the layer
  layer_init(&led_layer, window.layer.frame);

  // what to do when the layer is dirty
  led_layer.update_proc = &led_layer_update_callback;

  // displays the layer
  layer_add_child(&window.layer, &led_layer);
}

// Whenever there is a tick, trigger the update event for the LEDs
void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  layer_mark_dirty(&led_layer);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    // What to do at initialisation
    .init_handler = &handle_init,
    .tick_info = {
      // What to do when tick is triggered
      .tick_handler = &handle_tick,
      // When to trigger the tick event
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
