// Standard includes
#include "pebble.h"

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

Window *window;

// Layer containing all the circles (LEDs)
Layer *led_layer;

// Layer showing the battery status
Layer *battery_layer;

// Updates the layer containing all LEDs when the time changes
static void time_update(Layer *me, GContext *ctx);

// Updates the battery layer when the battery changes
static void battery_update(BatteryChargeState charge_state);

// Updates the connection status
static void connection_status_update(bool connected);

// Gets the center of a specific LED
static GPoint getCenter(int col, int row);

// Changes the state of a given LED to on (on=true) or off (on=false)
static void toggle_led(GContext *ctx, int col, int row, bool on);

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  layer_mark_dirty(led_layer);
}

// Handle the start-up of the app
static void init(void) {
  // Create our app's base window
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *root_layer = window_get_root_layer(window);
  GRect root_frame = layer_get_frame(root_layer);

  led_layer = layer_create(root_frame);
  layer_set_update_proc(led_layer, &time_update);
  layer_add_child(root_layer, led_layer);

  battery_layer = layer_create(root_frame);

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  handle_second_tick(NULL /* unnecessary */ , SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&battery_update);
  bluetooth_connection_service_subscribe(&connection_status_update);

  // Before the battery changes, lets see what its value is:
  battery_update(battery_state_service_peek());

  layer_add_child(root_layer, led_layer);
  layer_add_child(root_layer, battery_layer);
}

// Handle the destruction of the app
static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
//  bluetooth_connection_service_unsubscribe();
  layer_destroy(led_layer);
  layer_destroy(battery_layer);
  window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
  init();
  app_event_loop();
  deinit();
}

// See top of this file
static void time_update(Layer *me, GContext *ctx) {
  // Gets the time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  bool           is_24h_style = clock_is_24h_style();
  unsigned short hour         = is_24h_style ? t->tm_hour : t->tm_hour % 12;
                 hour         = !is_24h_style && 0 == hour ? 12 : hour;
  bool           pm           = t->tm_hour / 12;
  unsigned short month        = t->tm_mon + 1;
  unsigned short week_day     = 0 != t->tm_wday ? t->tm_wday : 7;// 0 as 7

  toggle_led(ctx, C_SECOND, 0, t->tm_sec & 1);
  toggle_led(ctx, C_SECOND, 1, t->tm_sec & 2);
  toggle_led(ctx, C_SECOND, 2, t->tm_sec & 4);
  toggle_led(ctx, C_SECOND, 3, t->tm_sec & 8);
  toggle_led(ctx, C_SECOND, 4, t->tm_sec & 16);
  toggle_led(ctx, C_SECOND, 5, t->tm_sec & 32);

  toggle_led(ctx, C_MINUTE, 0, t->tm_min & 1);
  toggle_led(ctx, C_MINUTE, 1, t->tm_min & 2);
  toggle_led(ctx, C_MINUTE, 2, t->tm_min & 4);
  toggle_led(ctx, C_MINUTE, 3, t->tm_min & 8);
  toggle_led(ctx, C_MINUTE, 4, t->tm_min & 16);
  toggle_led(ctx, C_MINUTE, 5, t->tm_min & 32);

  toggle_led(ctx, C_HOUR, 0, hour & 1);
  toggle_led(ctx, C_HOUR, 1, hour & 2);
  toggle_led(ctx, C_HOUR, 2, hour & 4);
  toggle_led(ctx, C_HOUR, 3, hour & 8);
  toggle_led(ctx, C_HOUR, 4, is_24h_style ? hour & 16 : pm);
  // last bit of hour is week_day lowest bit
  toggle_led(ctx, C_HOUR, L_WEEK_DAY, week_day & 1);

  toggle_led(ctx, C_MONTH_DAY, 0, t->tm_mday & 1);
  toggle_led(ctx, C_MONTH_DAY, 1, t->tm_mday & 2);
  toggle_led(ctx, C_MONTH_DAY, 2, t->tm_mday & 4);
  toggle_led(ctx, C_MONTH_DAY, 3, t->tm_mday & 8);
  toggle_led(ctx, C_MONTH_DAY, 4, t->tm_mday & 16);
  // last bit of hour is week_day middle bit
  toggle_led(ctx, C_MONTH_DAY, L_WEEK_DAY, week_day & 2);

  toggle_led(ctx, C_MONTH, 0, month & 1);
  toggle_led(ctx, C_MONTH, 1, month & 2);
  toggle_led(ctx, C_MONTH, 2, month & 4);
  toggle_led(ctx, C_MONTH, 3, month & 8);
  // last bit of month is week_day highest bit
  toggle_led(ctx, C_MONTH, L_WEEK_DAY, week_day & 4);

}

// See top of this file
static void battery_update(BatteryChargeState charge_state) {
  // charge_state.charge_percent
  // charge_state.is_charging
}

// See top of this file
static void connection_status_update(bool connected) {
//  graphics_context_set_fill_color(ctx, GColorWhite);
//  GPoint center = GPoint(0,0);
//  graphics_fill_circle(ctx)
}

// See top of this file
static GPoint getCenter(int col, int row) {
  return GPoint(DOT_RADIUS * (1 + 2 * col) + SPACE * col + SPACE,
                DOT_RADIUS * (1 + 2 * row) + SPACE * row + SPACE / 2);
}

// See top of this file
static void toggle_led(GContext *ctx, int col, int row, bool on) {
  graphics_context_set_fill_color(ctx, on ? GColorWhite : GColorBlack);
  graphics_fill_circle(ctx, getCenter(col, row), DOT_RADIUS);
}
