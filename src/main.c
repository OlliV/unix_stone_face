#include <pebble.h>
#include <stdio.h>

static Window *main_window;
static TextLayer *utime_layer;
static TextLayer *time_layer[2];
static TextLayer *date_layer;
static GFont time_font;

#define TIMEVLEN    20
#define TIMEFMT_12H "%I:%M %p"
#define TIMEFMT_24H "%H:%M"

static void update_time(void)
{
    static char utime_buffer[TIMEVLEN];
    static char ltime_buffer[TIMEVLEN];
    static char ptime_buffer[TIMEVLEN];
    static char date_buffer[TIMEVLEN];
    time_t utime, ptime;
    struct tm *tm;

    utime = time(NULL);
    ptime = utime - 10 * 60 * 60;
    
    snprintf(utime_buffer, TIMEVLEN, "%x", (int)utime);
    
    tm = localtime(&utime);
    strftime(date_buffer, TIMEVLEN, "%a %b %e", tm);
    
    if(clock_is_24h_style() == true) {
        strftime(ltime_buffer, TIMEVLEN, TIMEFMT_24H, tm);
        
        tm = localtime(&ptime);
        strftime(ptime_buffer, TIMEVLEN, TIMEFMT_24H, tm);
    } else {
        strftime(ltime_buffer, TIMEVLEN, TIMEFMT_12H, tm);
        
        tm = localtime(&ptime);
        strftime(ptime_buffer, TIMEVLEN, TIMEFMT_12H, tm);
    }

    text_layer_set_text(utime_layer, utime_buffer);
    text_layer_set_text(time_layer[0], ltime_buffer);
    text_layer_set_text(time_layer[1], ptime_buffer);
    text_layer_set_text(date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    update_time();
}

static void main_window_load(Window *window)
{
    size_t i;

    time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_23));

    /*
     * Unix timestamp.
     */
    
    utime_layer = text_layer_create(GRect(5, 5, 139, 50));
    text_layer_set_background_color(utime_layer, GColorClear);
    text_layer_set_text_color(utime_layer, GColorBlack);
    text_layer_set_text(utime_layer, "");
    
    text_layer_set_font(utime_layer, time_font);
    text_layer_set_text_alignment(utime_layer, GTextAlignmentCenter);

    layer_add_child(window_get_root_layer(window), text_layer_get_layer(utime_layer));
    
    /*
     * Time.
     */
    for (i = 0; i < sizeof(time_layer) / sizeof(time_layer[0]); i++) {
        static TextLayer *tl;

        time_layer[i] = text_layer_create(GRect(5, 35 + i * 23, 139, 90 + i * 23));
        tl = time_layer[i];

        text_layer_set_background_color(tl, GColorClear);
        text_layer_set_text_color(tl, GColorBlack);
        text_layer_set_text(tl, "00:00");
        
        text_layer_set_font(tl, time_font);
        text_layer_set_text_alignment(tl, GTextAlignmentCenter);
        
        layer_add_child(window_get_root_layer(window), text_layer_get_layer(tl));
    }
    
    /*
     * Date.
     */
    time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_23));

    date_layer = text_layer_create(GRect(5, 35 + 3 * 23, 139, 3 * 23));
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, GColorBlack);
    text_layer_set_text(date_layer, "");
    
    text_layer_set_font(date_layer, time_font);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

    update_time();
}

static void main_window_unload(Window *window)
{
    size_t i;
    
    text_layer_destroy(utime_layer);
    
    for (i = 0; i < sizeof(time_layer) / sizeof(time_layer[0]); i++) {
        text_layer_destroy(time_layer[i]);
    }
    
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
