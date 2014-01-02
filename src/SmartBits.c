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

// App-specific data
Window *window; // All apps must have at least one window

// TODO change to a binary indicator
TextLayer *battery_layer;

// Layer containing all the circles (LEDs)
Layer *led_layer;

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

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, battery_text);
}

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  layer_mark_dirty(led_layer);
}

static void connection_layer_update_callback(Layer *me, GContext* ctx) {
//  graphics_context_set_fill_color(ctx, GColorWhite);
//  GPoint center = GPoint(0,0);
//  graphics_fill_circle(ctx)
}

static void handle_bluetooth(bool connected) {
//  static bool previous_state = connected;
//  if (previous_state != connected) {
//    set_dirty
//  }
  // check if bluetooth is on
//  text_layer_set_text(connection_layer, connected ? "connected" : "disconnected");
//  previous_state = connected;
}

// Handle the start-up of the app
static void do_init(void) {
  // Create our app's base window
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *root_layer = window_get_root_layer(window);
  GRect root_frame = layer_get_frame(root_layer);

  led_layer = layer_create(root_frame);
  layer_set_update_proc(led_layer, &led_layer_update_callback);
  layer_add_child(root_layer, led_layer);

  battery_layer = text_layer_create(GRect(SPACE, 114, /* width */ root_frame.size.w, 24 /* height */));
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentLeft);
  text_layer_set_text(battery_layer, "100%");

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  handle_second_tick(NULL /* unnecessary */ , SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
//  bluetooth_connection_service_subscribe(&handle_bluetooth);

  // Before the battery changes, lets see what its value is:
  handle_battery(battery_state_service_peek());

  layer_add_child(root_layer, led_layer);
  layer_add_child(root_layer, text_layer_get_layer(battery_layer));
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
//  bluetooth_connection_service_unsubscribe();
  layer_destroy(led_layer);
  text_layer_destroy(battery_layer);
  window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
