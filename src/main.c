#include <pebble.h>
#include <stdio.h>

static Window *main_window;
static TextLayer *utime_layer;
static TextLayer *time_layer;
static TextLayer *weather_layer;
static TextLayer *date_layer;
static TextLayer *status_layer;
static GFont time_font;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static char *bt_status = "Unk";
static char batt_status[] = "+100%";

#define TIMEVLEN            9
#define TIMEFMT_12H         "%I:%M %p"
#define TIMEFMT_24H         "%H:%M"
#define DATEVLEN            11
#define DATEFMT             "%a %b %e"

#define KEY_TEMPERATURE     0
#define KEY_CONDITIONS      1
#define WEATHER_ICON_KEY    2

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_SUN,
  RESOURCE_ID_IMAGE_CLOUD,
  RESOURCE_ID_IMAGE_RAIN,
  RESOURCE_ID_IMAGE_SNOW
};

static void update_time(void)
{
    static char utime_buffer[TIMEVLEN];
    static char time_buffer[TIMEVLEN];
    static char date_buffer[DATEVLEN];
    time_t utime;
    struct tm *tm;

    utime = time(NULL);
    tm = localtime(&utime);

    snprintf(utime_buffer, sizeof(utime_buffer), "%x", (unsigned)utime);

    if (clock_is_24h_style() == true) {
        strftime(time_buffer, sizeof(time_buffer), TIMEFMT_24H, tm);
    } else {
        strftime(time_buffer, sizeof(time_buffer), TIMEFMT_12H, tm);
    }

    strftime(date_buffer, sizeof(date_buffer), DATEFMT, tm);

    text_layer_set_text(utime_layer, utime_buffer);
    text_layer_set_text(time_layer, time_buffer);
    text_layer_set_text(date_layer, date_buffer);
}

static void update_status(void)
{
    static char status_text[] = "Down +100%";

    snprintf(status_text, sizeof(status_text), "%s %s",
             bt_status,
             batt_status);

    text_layer_set_text(status_layer, status_text);
}

static void handle_bluetooth(bool connected)
{
    bt_status = connected ? "Up" : "Down";

    update_status();
}

static void handle_battery(BatteryChargeState charge_state)
{
    snprintf(batt_status, sizeof(batt_status), "%c%d%%",
             charge_state.is_charging ? '+' : ' ',
             charge_state.charge_percent);

    update_status();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{
    /* Store incoming information */
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[32];
  
    Tuple *t = dict_read_first(iterator); 
    while(t != NULL) {
        // Which key was received?
        switch (t->key) {
        case KEY_TEMPERATURE:
            snprintf(temperature_buffer, sizeof(temperature_buffer),
                     "%dC", (int)t->value->int32);
            break;
        case KEY_CONDITIONS:
            snprintf(conditions_buffer, sizeof(conditions_buffer),
                     "%s", t->value->cstring);
            break;
        case WEATHER_ICON_KEY:
            if (icon_bitmap) {
                gbitmap_destroy(icon_bitmap);
            }

            icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[t->value->uint8]);

#ifdef PBL_SDK_3
            bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
#endif
            bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
            break;
        default:
            APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }

        /* Look for next item */
        t = dict_read_next(iterator);
    }
  
    /* Assemble full string and display */
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s",
             temperature_buffer, conditions_buffer);
    text_layer_set_text(weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    handle_bluetooth(bluetooth_connection_service_peek());
    handle_battery(battery_state_service_peek());

    update_time();
    update_status();

    if (tick_time->tm_min % 60 == 0) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, 0, 0);
        app_message_outbox_send();
    }
}

static void main_window_load(Window *window)
{
    Layer *window_layer;
    GRect bounds;
    
    window_layer = window_get_root_layer(window);
    bounds = layer_get_frame(window_layer);
    time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_23));

    /*
     * Unix timestamp.
     */
    utime_layer = text_layer_create(GRect(0, 2, bounds.size.w, 30));
    text_layer_set_background_color(utime_layer, GColorClear);
    text_layer_set_text_color(utime_layer, GColorBlack);
    text_layer_set_font(utime_layer, time_font);
    text_layer_set_text_alignment(utime_layer, GTextAlignmentCenter);
    text_layer_set_text(utime_layer, "");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(utime_layer));

    /*
     * Time.
     */
    time_layer = text_layer_create(GRect(0, 30, bounds.size.w, 30));
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_color(time_layer, GColorBlack);
    text_layer_set_font(time_layer, time_font);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_text(time_layer, "00:00");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

    /*
     * Weather.
     */
    weather_layer = text_layer_create(GRect(0, 95, bounds.size.w, 30));
    text_layer_set_background_color(weather_layer, GColorClear);
    text_layer_set_text_color(weather_layer, GColorBlack);
    text_layer_set_font(weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(weather_layer, GTextAlignmentCenter);
    text_layer_set_text(weather_layer, "?");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));
    
    icon_layer = bitmap_layer_create(GRect(32, 45, 64, 64));
    layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

    /*
     * Date.
     */
    date_layer = text_layer_create(GRect(0, bounds.size.h - 50, bounds.size.w, 24));
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, GColorBlack);
    text_layer_set_font(date_layer, time_font);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_text(date_layer, "");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

    /*
     * Status.
     */
    status_layer = text_layer_create(GRect(0, bounds.size.h - 24, bounds.size.w, 24));
    text_layer_set_background_color(status_layer, GColorBlack);
    text_layer_set_text_color(status_layer, GColorClear);
    text_layer_set_font(status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(status_layer, GTextAlignmentCenter);
    text_layer_set_text(status_layer, "Down +100%");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(status_layer));

    update_time();
    update_status();

    bluetooth_connection_service_subscribe(handle_bluetooth);
    battery_state_service_subscribe(handle_battery);
}

static void main_window_unload(Window *window)
{
    text_layer_destroy(utime_layer);
    text_layer_destroy(time_layer);
    text_layer_destroy(weather_layer);
    if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
    }
    text_layer_destroy(date_layer);
    text_layer_destroy(status_layer);

    fonts_unload_custom_font(time_font);
    bitmap_layer_destroy(icon_layer);
}

static void init(void)
{
    main_window = window_create();

    window_set_window_handlers(main_window, (WindowHandlers){
        .load = main_window_load,
        .unload = main_window_unload
    });

    window_stack_push(main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    
    /*
     * Register message callbacks.
     */
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    app_message_open(app_message_inbox_size_maximum(),
                     app_message_outbox_size_maximum());
}

static void deinit(void)
{
    window_destroy(main_window);
}

int main(void)
{
    init();
    app_event_loop();
    deinit();
}
