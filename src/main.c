#include <pebble.h>
#include <stdio.h>

static Window *main_window;
static TextLayer *utime_layer;
static TextLayer *time_layer;
static TextLayer *date_layer;
static TextLayer *status_layer;
static GFont time_font;

static char *bt_status = "Unk";
static char batt_status[] = "+100%";

#define TIMEVLEN    20
#define TIMEFMT_12H "%I:%M %p"
#define TIMEFMT_24H "%H:%M"

static void update_time(void)
{
    static char utime_buffer[TIMEVLEN];
    static char time_buffer[TIMEVLEN];
    static char date_buffer[TIMEVLEN];
    time_t utime;
    struct tm *tm;

    utime = time(NULL);
    tm = localtime(&utime);

    snprintf(utime_buffer, TIMEVLEN, "%x", (int)utime);

    if(clock_is_24h_style() == true) {
        strftime(time_buffer, TIMEVLEN, TIMEFMT_24H, tm);
    } else {
        strftime(time_buffer, TIMEVLEN, TIMEFMT_12H, tm);
    }

    strftime(date_buffer, TIMEVLEN, "%a %b %e", tm);

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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    handle_bluetooth(bluetooth_connection_service_peek());
    handle_battery(battery_state_service_peek());

    update_time();
    update_status();
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
    utime_layer = text_layer_create(GRect(0, 5, bounds.size.w, 30));
    text_layer_set_background_color(utime_layer, GColorClear);
    text_layer_set_text_color(utime_layer, GColorBlack);
    text_layer_set_text(utime_layer, "");
    text_layer_set_font(utime_layer, time_font);
    text_layer_set_text_alignment(utime_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(utime_layer));

    /*
     * Time.
     */
    time_layer = text_layer_create(GRect(0, 35, bounds.size.w, 30));
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_color(time_layer, GColorBlack);
    text_layer_set_text(time_layer, "00:00");
    text_layer_set_font(time_layer, time_font);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

    /*
     * Date.
     */
    date_layer = text_layer_create(GRect(0, 100, bounds.size.w, 30));
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, GColorBlack);
    text_layer_set_text(date_layer, "");
    text_layer_set_font(date_layer, time_font);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

    /*
     * Status.
     */
    status_layer = text_layer_create(GRect(0, bounds.size.h - 34, bounds.size.w, 34));
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
    text_layer_destroy(date_layer);

    fonts_unload_custom_font(time_font);
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
