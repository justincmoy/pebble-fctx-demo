#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include "settings.h"

// --------------------------------------------------------------------------
// Types and global variables.
// --------------------------------------------------------------------------

Window* g_window;
Layer* g_layer;
FFont* g_font;

static char text_buffer[10];

#define TEXT_SIZE 23
#define HAND_SIZE 10

#if defined(PBL_ROUND)
#define BEZEL_INSET 6
#else
#define BEZEL_INSET 2
#endif

// --------------------------------------------------------------------------
// Utility functions.
// --------------------------------------------------------------------------

static inline FPoint clockToCartesian(FPoint center, fixed_t radius, int32_t angle) {
    FPoint pt;
    int32_t c = cos_lookup(angle);
    int32_t s = sin_lookup(angle);
    pt.x = center.x + s * radius / TRIG_MAX_RATIO;
    pt.y = center.y - c * radius / TRIG_MAX_RATIO;
    return pt;
}

static const char date_lookup[] = 
    "JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC\0"
    "SUN\0MON\0TUE\0WED\0THU\0FRI\0SAT";

static const char* get_month_string(int month) {
    return &date_lookup[month * 4];
}

static const char* get_weekday_string(int weekday) {
    return &date_lookup[48 + weekday * 4];
}

static inline void format_minute_text(char* buffer, const struct tm* time) {
  snprintf(buffer, 10, "%s %d", get_month_string(time->tm_mon), time->tm_mday);
}

static inline void format_hour_text(char* buffer, const struct tm* time) {
  snprintf(buffer, 4, "%s", get_weekday_string(time->tm_wday));
}

static void draw_hand(FContext* fctx, GColor color, FPoint center, fixed_t radius, int32_t angle, fixed_t hand_size, fixed_t ctrl) {
    fctx_begin_fill(fctx);
    fctx_set_fill_color(fctx, color);
    fctx_set_offset(fctx, center);
    fctx_set_scale(fctx, FPointOne, FPointOne);
    fctx_set_rotation(fctx, angle);

    // FPoints are created with arguments (X, Y).
    // X=0, Y=0 is the very middle of the face.

    // This creates the outermost tip of the hand, with counter-clockwise curve.
    fctx_move_to (fctx, FPoint(0, - radius));
    fctx_curve_to(fctx, FPoint(0, - radius),
                        FPoint(hand_size, 1 * hand_size - radius),
                        FPoint(hand_size, 3 * hand_size - radius));

    // If we're drawing a hand at the 12 o'clock position, this draws the left-most line.
    fctx_line_to (fctx, FPoint(hand_size, 0));

    // Counter-clockwise curve around the middle of the face.
    fctx_curve_to(fctx, FPoint(hand_size, ctrl),
                        FPoint(ctrl, hand_size),
                        FPoint(0, hand_size));
    fctx_curve_to(fctx, FPoint(-ctrl, hand_size),
                        FPoint(-hand_size, ctrl),
                        FPoint(-hand_size, 0));

    // This draws the right-most line, and completes the curve at the tip.
    fctx_line_to (fctx, FPoint(-hand_size, 3 * hand_size - radius));
    fctx_curve_to(fctx, FPoint(-hand_size, 1 * hand_size - radius),
                        FPoint(0, - radius),
                        FPoint(0, - radius));

    fctx_end_fill(fctx);
}

static inline int32_t angle_diff(int32_t a, int32_t b) {
    int32_t diff = abs(a - b) % TRIG_MAX_ANGLE;
    return diff > TRIG_MAX_ANGLE / 2 ? TRIG_MAX_ANGLE - diff : diff;
}

// --------------------------------------------------------------------------
// The main drawing function.
// --------------------------------------------------------------------------

void on_layer_update(Layer* layer, GContext* ctx) {
    time_t now = time(NULL);
    struct tm local_time = *localtime(&now);

    GRect bounds = layer_get_bounds(layer);
    FPoint center = FPointI(bounds.size.w / 2, bounds.size.h / 2);
    fixed_t safe_radius = INT_TO_FIXED(bounds.size.w / 2 - BEZEL_INSET);

    fixed_t minute_hand_radius = safe_radius;
    fixed_t hour_hand_radius = safe_radius - INT_TO_FIXED(22);

    int32_t minute_angle = local_time.tm_min * TRIG_MAX_ANGLE / 60.0;
    int32_t hour_angle = ((local_time.tm_hour % 12) + (local_time.tm_min / 60.0)) * TRIG_MAX_ANGLE / 12.0;

    FContext fctx;
    fctx_init_context(&fctx, ctx);
    fctx_set_color_bias(&fctx, 0);

    /* Draw the hour hand. */

    fixed_t hand_size = INT_TO_FIXED(HAND_SIZE);
    fixed_t ctrl = hand_size * 3 / 4;
    draw_hand(&fctx, settings.MinuteHandColor, center, hour_hand_radius, hour_angle, hand_size, ctrl);

    /* Draw the minute hand. */
    draw_hand(&fctx, settings.MinuteHandColor, center, minute_hand_radius, minute_angle, hand_size, ctrl);

    /* Draw the string onto the minute hand. */

    FPoint anchor_point = clockToCartesian(center, minute_hand_radius - (2 * hand_size), minute_angle);
    int32_t text_rotation;
    GTextAlignment text_align;
    if (local_time.tm_min < 30) {
        text_rotation = minute_angle - TRIG_MAX_ANGLE / 4;
        text_align = GTextAlignmentRight;
    } else {
        text_rotation = minute_angle + TRIG_MAX_ANGLE / 4;
        text_align = GTextAlignmentLeft;
    }

    format_minute_text(text_buffer, &local_time);
    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, settings.MinuteTextColor);
    fctx_set_offset(&fctx, anchor_point);
    fctx_set_rotation(&fctx, text_rotation);
    fctx_set_text_em_height(&fctx, g_font, TEXT_SIZE);
    fctx_draw_string(&fctx, text_buffer, g_font, text_align, FTextAnchorMiddle);
    fctx_end_fill(&fctx);

    /* Draw the string onto the hour hand. */
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "hour: %d, minute: %d, diff: %d, diff check: %d", hour_angle, minute_angle, angle_diff(hour_angle, minute_angle), DEG_TO_TRIGANGLE(40));

    fctx_deinit_context(&fctx);

    if (angle_diff(hour_angle, minute_angle) > DEG_TO_TRIGANGLE(40) ) {
        anchor_point = clockToCartesian(center, hour_hand_radius - (2 * hand_size), hour_angle);
        if ((local_time.tm_hour % 12) < 6) {
            text_rotation = hour_angle - TRIG_MAX_ANGLE / 4;
            text_align = GTextAlignmentRight;
        } else {
            text_rotation = hour_angle + TRIG_MAX_ANGLE / 4;
            text_align = GTextAlignmentLeft;
        }

        format_hour_text(text_buffer, &local_time);
        fctx_begin_fill(&fctx);
        fctx_set_fill_color(&fctx, settings.MinuteTextColor);
        fctx_set_offset(&fctx, anchor_point);
        fctx_set_rotation(&fctx, text_rotation);
        fctx_set_text_em_height(&fctx, g_font, TEXT_SIZE);
        fctx_draw_string(&fctx, text_buffer, g_font, text_align, FTextAnchorMiddle);
        fctx_end_fill(&fctx);
    }
}

// --------------------------------------------------------------------------
// System event handlers.
// --------------------------------------------------------------------------

void on_tick_timer(struct tm* tick_time, TimeUnits units_changed) {
    layer_mark_dirty(g_layer);
}

// --------------------------------------------------------------------------
// Initialization and teardown.
// --------------------------------------------------------------------------

static void init() {
    setlocale(LC_ALL, "");

    g_font = ffont_create_from_resource(RESOURCE_ID_DIN_CONDENSED_FFONT);
    // ffont_debug_log(g_font, APP_LOG_LEVEL_DEBUG);

    init_settings();

    g_window = window_create();
    window_set_background_color(g_window, settings.FaceColor);
    window_stack_push(g_window, true);
    Layer* window_layer = window_get_root_layer(g_window);
    GRect window_frame = layer_get_frame(window_layer);

    g_layer = layer_create(window_frame);
    layer_set_update_proc(g_layer, &on_layer_update);
    layer_add_child(window_layer, g_layer);


    tick_timer_service_subscribe(MINUTE_UNIT, &on_tick_timer);
}

static void deinit() {
    tick_timer_service_unsubscribe();
    window_destroy(g_window);
    layer_destroy(g_layer);
    ffont_destroy(g_font);
}

// --------------------------------------------------------------------------
// The main event loop.
// --------------------------------------------------------------------------

int main() {
    init();
    app_event_loop();
    deinit();
}
