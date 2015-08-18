#include <pebble.h>

// bind to JS keys
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1
};

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_weather_layer;
static int s_battery_level;
static Layer * s_battery_layer;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static void update_time(){
  // get a tm struct
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  //create a long-lived buffer
  static char buffer[] = "00:00";

  // write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true){
    // use 24 hour style
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // use 12 hour style
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  //display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);

  // copy date into buffer from tm structure
  static char date_buffer[] = "yyyy/mm/dd";
  strftime(date_buffer, sizeof(date_buffer), "%Y/%m/%d", tick_time);

  //show the date
  text_layer_set_text(s_date_layer, date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx){
 GRect bounds = layer_get_bounds(layer);

  // find width of bar - nb 114.0 is the width we defined for the layer
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 111.0F);

  GColor8 colour = GColorGreen;

  // draw the background
  if (s_battery_level < 50){
    if (s_battery_level > 25){
      colour =  GColorYellow;
    } else{
      colour =  GColorRed;
    }
  }
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 4, GCornersAll);

  //draw the bar
  graphics_context_set_fill_color(ctx, colour);
  graphics_fill_rect(ctx, GRect(2,1, width, bounds.size.h - 2), 2, GCornersAll);
}

static void bluetooth_callback(bool connected){
  // show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected){
    // issue a vribrate alert
    vibes_double_pulse();
  }
}

static void main_window_load(Window* window) {
  // create time textLayer
  s_time_layer = text_layer_create(GRect(0, 55, 144, 50));

  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  //text_layer_set_text(s_time_layer, "00:00");


  // improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // add it as a child layer to the Window's root layer.
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // create weather textLayer
  s_weather_layer = text_layer_create(GRect(0, 100, 144, 30));

  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);

  // improve the layout to be more like a watchface
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);

  // add it as a child layer to the Window's root layer.
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  // create date textLayer
  s_date_layer = text_layer_create(GRect(0, 125, 144, 30));

  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);

  // improve the layout to be more like a watchface
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // add it as a child layer to the Window's root layer.
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  // create battery meter layer;
  s_battery_layer = layer_create(GRect(12, 54, 115, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add to window
  layer_add_child(window_get_root_layer(window), s_battery_layer);

  //create bluetooth icon gbitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  // create the bitmaplayer to display the bitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(59, 12, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
}

static void main_window_unload(Window* window) {
  // destroy textlayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_battery_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}

// service handlers
static void tick_handler(struct tm* tick_time, TimeUnits units_changed){
  update_time();

  //update weather
  if(tick_time->tm_min % 30 == 0){
    // begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // add key-value pair
    dict_write_uint8(iter, 0, 0);

    //send message
    app_message_outbox_send();
  }
}

static void battery_callback(BatteryChargeState state){
  // record the new battery level
  s_battery_level = state.charge_percent;

  //update meter
  layer_mark_dirty(s_battery_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context){
  // store incoming data
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];


  // read first item
  Tuple *t = dict_read_first(iterator);

  // for all items
  while(t != NULL){
    // which key did we get?
    switch(t->key){
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
        break;
      case KEY_CONDITIONS:
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognised", (int)t->key);
    }

    // get next item
    t = dict_read_next(iterator);
  }

  // assemble weather for display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success");
}

// initiliase/tear down
static void init() {
  // register with the time service
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  //register for battery info updates
  battery_state_service_subscribe(battery_callback);

  // register for bluetooth events
  bluetooth_connection_service_subscribe(bluetooth_callback);

  // register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  //open app message
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  //create main window element and assign to pointer
  s_main_window = window_create();

  //set handlers to manage elements inside Window
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // make sure the time is displayed from the start
  update_time();

  // ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // ensure bluetooth is correct at boot
  bluetooth_callback(bluetooth_connection_service_peek());
}

static void deinit() {
  // destroy window
  window_destroy(s_main_window);
}


int main(void){
  init();
  app_event_loop();
  deinit();
}
