#include <pebble.h>

//=====================================
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
  KEY_ICON = 2
};

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_weather_layer;
static int s_battery_level;
static Layer * s_battery_layer, *s_root_layer, *s_timewarp_layer;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;
static BitmapLayer *s_weather_icon_layer;
static GBitmap *s_weather_icon_bitmap;
static BitmapLayer *s_charge_icon_layer;
static GBitmap *s_charge_icon_bitmap;
static GFont s_time_font_48;
static GRect s_bounds;
static int battery_radius = 4;
static int red, green, blue = 0;

//=====================================
// update routines
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

  red = (int)((tick_time->tm_hour / 23.0f) * 255.0f);
  green = (int)((tick_time->tm_min / 59.0f) * 255.0f);
  blue = (int)((tick_time->tm_sec / 61.0f) * 255.0f);

  //display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);

  // copy date into buffer from tm structure
  static char date_buffer[] = "yyyy-mm-dd day";
  strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d %a", tick_time);

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
  graphics_fill_rect(ctx, bounds, battery_radius, GCornersAll);

  //draw the bar
  graphics_context_set_fill_color(ctx, colour);
  graphics_fill_rect(ctx, GRect(2,1, width, bounds.size.h - 2), battery_radius, GCornersAll);
}


static void timewarp_update_proc(Layer *layer, GContext *ctx){
  // draw timewarp
  //APP_LOG(APP_LOG_LEVEL_INFO, "timewarp: GColorFromRGB(%d, %d, %d)", red, green, blue);

  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorFromRGB(red, green, blue));
  graphics_fill_rect(ctx, bounds, battery_radius, GCornersAll);
}

static void bluetooth_callback(bool connected){
  // show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), !connected);

  if(!connected){
    // issue a vribrate alert
    vibes_double_pulse();
  }
}

//=====================================
// build gui

static void buildTimeDisplay(Window *window){
  // create time textLayer
  s_time_layer = text_layer_create(GRect(0, 18, 144, 50));

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);

  s_time_font_48 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_DISPLAY_48));

  // improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, s_time_font_48);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // add it as a child layer to the Window's root layer.
  layer_add_child(s_root_layer, text_layer_get_layer(s_time_layer));

  s_timewarp_layer = layer_create(GRect(5, 100, s_bounds.size.w - 10, 12));
  layer_set_update_proc(s_timewarp_layer, timewarp_update_proc);

  // Add to window
  layer_add_child(s_root_layer, s_timewarp_layer);
}

static void buildBatteryDisplay(Window *window){
  // create battery meter layer;
  s_battery_layer = layer_create(GRect(25, 7, s_bounds.size.w - 50, 12));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add to window
  layer_add_child(s_root_layer, s_battery_layer);
}

static void buildWeatherDisplay(Window *window){
  // create weather textLayer
  s_weather_layer = text_layer_create(GRect(s_bounds.size.w-70, s_bounds.size.h-45, 50, 50));

  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);

  // improve the layout to be more like a watchface
  //text_layer_set_font(s_weather_layer, s_time_font_48);//fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // add it as a child layer to the Window's root layer.
  layer_add_child(s_root_layer, text_layer_get_layer(s_weather_layer));
}

static void buildDateDisplay(Window *window){
  // create date textLayer
  s_date_layer = text_layer_create(GRect(0, 65, 144, 30));

  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);

  // improve the layout to be more like a watchface
  //text_layer_set_font(s_date_layer, s_time_font_48);//fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // add it as a child layer to the Window's root layer.
  layer_add_child(s_root_layer, text_layer_get_layer(s_date_layer));
}

static void buildIconsDisplay(Window *window){
  //create bluetooth icon gbitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  // create the bitmaplayer to display the bitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(0, 0, 24, 24));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);
  layer_add_child(s_root_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  //create bluetooth icon gbitmap
  s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_50DN);

  // create the bitmaplayer to display the bitmap
  s_weather_icon_layer = bitmap_layer_create(GRect(20, s_bounds.size.h - 50, 50, 50));
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_weather_icon_layer, GCompOpSet);
  layer_add_child(s_root_layer, bitmap_layer_get_layer(s_weather_icon_layer));


  //create charge icon gbitmap
  s_charge_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE);
  GRect image_frame = GRect(s_bounds.size.w-24, 1, 24, 24);

  // create the bitmaplayer to display the bitmap
  s_charge_icon_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_bitmap(s_charge_icon_layer, s_charge_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_charge_icon_layer, GCompOpSet);
  layer_add_child(s_root_layer, bitmap_layer_get_layer(s_charge_icon_layer));
}

//=====================================
// window load/unload
static void main_window_load(Window* window) {
  s_root_layer = window_get_root_layer(window);
  s_bounds = layer_get_bounds(s_root_layer);
  buildTimeDisplay(window);
  buildBatteryDisplay(window);
  buildWeatherDisplay(window);
  buildDateDisplay(window);
  buildIconsDisplay(window);
}

static void main_window_unload(Window* window) {
  // destroy textlayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_timewarp_layer);
  bitmap_layer_destroy(s_bt_icon_layer);
  bitmap_layer_destroy(s_charge_icon_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  gbitmap_destroy(s_charge_icon_bitmap);
}

//=====================================
// service handlers
static void tick_handler(struct tm* tick_time, TimeUnits units_changed){
  update_time();
  layer_mark_dirty(s_timewarp_layer);
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

  if(state.is_charging){
      layer_set_hidden(bitmap_layer_get_layer(s_charge_icon_layer), false);
  }else{
      layer_set_hidden(bitmap_layer_get_layer(s_charge_icon_layer), true);
  }

  layer_mark_dirty(bitmap_layer_get_layer(s_charge_icon_layer));
}

//=====================================
// messaging system
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
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%d\u00B0C", (int)t->value->int32);
        break;
      case KEY_CONDITIONS:
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
        break;
      case KEY_ICON:
        if(strcmp(t->value->cstring, "01d") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_01D);
        } else if(strcmp(t->value->cstring, "02d") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_02D);
        } else if(strcmp(t->value->cstring, "03d") == 0 || strcmp(t->value->cstring, "03n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_03DN);
        } else if(strcmp(t->value->cstring, "04d") == 0 || strcmp(t->value->cstring, "04n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_04DN);
        } else if(strcmp(t->value->cstring, "09d") == 0 ||strcmp(t->value->cstring, "09n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_09DN);
        } else if(strcmp(t->value->cstring, "10d") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_10D);
        } else if(strcmp(t->value->cstring, "11d") == 0 ||strcmp(t->value->cstring, "11n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_11DN);
        } else if(strcmp(t->value->cstring, "13d") == 0 || strcmp(t->value->cstring, "13n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_13DN);
        } else if(strcmp(t->value->cstring, "50d") == 0 || strcmp(t->value->cstring, "50n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_50DN);
        } else if(strcmp(t->value->cstring, "01n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_01N);
        } else if(strcmp(t->value->cstring, "02n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_02N);
        } else if(strcmp(t->value->cstring, "10n") == 0){
            s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_10N);
        } else {
                APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown icon %s", t->value->cstring);
        }
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognised", (int)t->key);
    }

    // get next item
    t = dict_read_next(iterator);
  }


  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon_bitmap);
  layer_mark_dirty(bitmap_layer_get_layer(s_weather_icon_layer));

  // assemble weather for display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);//, conditions_buffer);
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

//=====================================
// initiliase/tear down
static void init() {
  // register with the time service
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

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
  window_set_background_color(s_main_window, GColorCadetBlue);

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
