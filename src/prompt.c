#include <pebble.h>

static Window *window;
static TextLayer *appt_text_layer;
static TextLayer *appt_time_layer;
static TextLayer *travel_time_layer;
static TextLayer *travel_method_layer;
static TextLayer *route_name_layer;
static TextLayer *departure_time_layer;
static TextLayer *leave_in_layer;
static TextLayer *time_layer; // The clock

static AppSync sync;
static uint8_t sync_buffer[200];

enum MessageKey {
    NEXT_APPT_NAME = 0x0,  // TUPLE_CSTRING < 24
    NEXT_APPT_TIME = 0x1,  // TUPLE_INT 
    TRAVEL_TIME    = 0x2,  // TUPLE_INT
    TRAVEL_METHOD  = 0x3,  // TUPLE_CSTRING
    ROUTE_NAME     = 0x4,  // TUPLE_CSTRING
    DEPARTURE_TIME = 0x5,  // TUPLE_INT
    LEAVE_IN       = 0x6,  // TUPLE_INT
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Compare to: APP_MSG_BUFFER_OVERFLOW: %d", APP_MSG_BUFFER_OVERFLOW);
}

static void print_tm(struct tm* tm) {
    char thestring[20];
    snprintf(thestring, sizeof(thestring), "%d-%02d-%02d %02d:%02d:%02d",
            tm->tm_year + 1900,
            tm->tm_mon + 1,
            tm->tm_mday,
            tm->tm_hour,
            tm->tm_min,
            tm->tm_sec);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", thestring);
}

static void describe_time(int seconds, char buffer[], int buffer_size) {
    if (seconds < 60) {
        snprintf(buffer, buffer_size, "%d seconds", seconds);
    } else {
        seconds = seconds / 60; // now minutes 
        if (seconds < 60) {
            if (seconds == 1) {
                snprintf(buffer, buffer_size, "%d minute", seconds);
            } else {
                snprintf(buffer, buffer_size, "%d minutes", seconds);
            }
        } else {
            seconds = seconds / 60; // now hours
            if (seconds == 1) {
                snprintf(buffer, buffer_size, "%d hour", seconds);
            } else {
                snprintf(buffer, buffer_size, "%d hours", seconds);
            }
        }
    }
}

static void describe_relative_time(int seconds, char buffer[], int buffer_size) {
    bool ago = false;
    char tbuffer[22];
    if (seconds < 0) {
        seconds = -seconds;
        ago = true;
    }

    if (seconds < 60) {
        snprintf(buffer, buffer_size, "now");
        return;
    }

    describe_time(seconds, tbuffer, sizeof(tbuffer));

    if (ago) {
        snprintf(buffer, buffer_size, "%s ago", tbuffer);
    } else {
        snprintf(buffer, buffer_size, "in %s", tbuffer);
    }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  //static char text_time[22];
  //static char departure_time[22];
  static char text_travel_by[19];
  static char text_leave_in[15];
  //static char text_travel_time[15];
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received tuple changed callback %d type: %d length: %d", (int)key, new_tuple->type, new_tuple->length);
  switch (key) {
    case NEXT_APPT_NAME:
      text_layer_set_text(appt_text_layer, new_tuple->value->cstring);
      break;
    case NEXT_APPT_TIME: ;
                         /*
      time_t then = (time_t) (new_tuple->value->int32);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "next_appt_time: %d which is: ", (int) new_tuple->value->int32);
      struct tm *appt_time = localtime(&then);
      print_tm(appt_time);
      strftime(text_time, sizeof(text_time), "%F %T", appt_time);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "got here %s", text_time);
      //snprintf(text_time, sizeof(text_time), "-%d-", (int)new_tuple->value->int32);
      text_layer_set_text(appt_time_layer, text_time);
      */
      break;
    case TRAVEL_TIME:
      /*describe_time(new_tuple->value->int32, text_travel_time, sizeof(text_travel_time));
      APP_LOG(APP_LOG_LEVEL_DEBUG, "travel_time: %d which is: %s", (int) new_tuple->value->int32, text_travel_time);
      text_layer_set_text(travel_time_layer, text_travel_time);*/
      break;
    case TRAVEL_METHOD:
      snprintf(text_travel_by, sizeof(text_travel_by), "Leave by %s", new_tuple->value->cstring);
      text_layer_set_text(travel_method_layer, text_travel_by);
      break;
    case ROUTE_NAME:
      text_layer_set_text(route_name_layer, new_tuple->value->cstring);
      break;
    case DEPARTURE_TIME: ;
                         /*
      time_t d_time = (time_t) (new_tuple->value->int32);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "departure_time: %d which is: ", (int) new_tuple->value->int32);
      struct tm *dept_time = localtime(&d_time);
      print_tm(dept_time);
      strftime(departure_time, sizeof(departure_time), "%F %T", dept_time);
      text_layer_set_text(departure_time_layer, departure_time);
      */
      break;
    case LEAVE_IN: ;
      describe_relative_time(new_tuple->value->int32, text_leave_in, sizeof(text_leave_in));
      APP_LOG(APP_LOG_LEVEL_DEBUG, "leave_in: %d which is: %s", (int) new_tuple->value->int32, text_leave_in);
      text_layer_set_text(leave_in_layer, text_leave_in);
      break;
  }
}

static void send_cmd(void) {
  Tuplet value = TupletCString(9, "Updateme");

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  appt_time_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(appt_time_layer, "Next Appointment");
  text_layer_set_text_alignment(appt_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(appt_time_layer));

  appt_text_layer = text_layer_create((GRect) { .origin = { 0, 20 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(appt_text_layer, "Initializing");
  text_layer_set_text_alignment(appt_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(appt_text_layer));

  route_name_layer = text_layer_create((GRect) { .origin = { 0, 40 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(route_name_layer, ".");
  text_layer_set_text_alignment(route_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(route_name_layer));

  travel_time_layer = text_layer_create((GRect) { .origin = { 0, 60 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(travel_time_layer, "");
  text_layer_set_text_alignment(travel_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(travel_time_layer));

  travel_method_layer = text_layer_create((GRect) { .origin = { 0, 80 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(travel_method_layer, "Travel by");
  text_layer_set_text_alignment(travel_method_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(travel_method_layer));

  leave_in_layer = text_layer_create((GRect) { .origin = { 0, 100 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(leave_in_layer, "...");
  text_layer_set_text_alignment(leave_in_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(leave_in_layer));

  departure_time_layer = text_layer_create((GRect) { .origin = { 0, 120 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(departure_time_layer, "");
  text_layer_set_text_alignment(departure_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(departure_time_layer));


  Tuplet initial_values[] = {
    TupletCString(NEXT_APPT_NAME, "(Appointment Name)"),
    TupletInteger(NEXT_APPT_TIME, 0),
    TupletInteger(TRAVEL_TIME, 0),
    TupletCString(TRAVEL_METHOD, "(method)"),
    TupletCString(ROUTE_NAME, "(route)"),
    TupletInteger(DEPARTURE_TIME, 0),
    TupletInteger(LEAVE_IN, 0)
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync buffer size: %d", sizeof(sync_buffer));

  send_cmd();
}

static void window_unload(Window *window) {
  text_layer_destroy(appt_text_layer);
  text_layer_destroy(appt_time_layer);
  text_layer_destroy(travel_time_layer);
  text_layer_destroy(travel_method_layer);
  text_layer_destroy(route_name_layer);
  text_layer_destroy(departure_time_layer);
  text_layer_destroy(leave_in_layer);
  text_layer_destroy(time_layer);
}

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {

  static char time_text[] = "00:00:00"; // Needs to be static because it's used by the system later.

  strftime(time_text, sizeof(time_text), "%T", tick_time);
  text_layer_set_text(time_layer, time_text);

  if (tick_time->tm_sec == 0) {
      send_cmd();
  }
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const int inbound_size = 200;
  const int outbound_size = 80;
  app_message_open(inbound_size, outbound_size);

  const bool animated = true;
  window_stack_push(window, animated);

  // Init the text layer used to show the time
  time_layer = text_layer_create(GRect(29, 134, 144-40 /* width */, 168-54 /* height */));
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_second_tick(current_time, SECOND_UNIT);
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Prompt done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
