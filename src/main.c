#include <pebble.h>

#define NUM_FISH 5

typedef struct {
  uint32_t resource_id;
  const char *common_name;
  const char *scientific_name;
} Fish;

static const Fish FISH[NUM_FISH] = {
  {RESOURCE_ID_FISH_LABRADOR, "Labrador whitefish", "Coregonus labradorius"},
  {RESOURCE_ID_FISH_SMELT, "Rainbow smelt", "Osmerus mordax"},
  {RESOURCE_ID_FISH_BROOK, "Brook trout", "Salvelinus fontinalis"},
  {RESOURCE_ID_FISH_LAKE, "Lake trout", "Salvelinus namaycush"},
  {RESOURCE_ID_FISH_SALMON, "Atlantic salmon", "Salmo salar"}
};

static Window *s_window;
static BitmapLayer *s_fish_layer;
static TextLayer *s_time_layer, *s_date_layer, *s_common_layer, *s_scientific_layer;
static GBitmap *s_fish_bitmap;
static int32_t s_selected_day = -1;
static char s_time_buffer[16], s_date_buffer[24];

// Integer hash with a fixed seed: varied order, reproducible across watch restarts.
static uint32_t mix(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dUL;
  x ^= x >> 15;
  x *= 0x846ca68bUL;
  x ^= x >> 16;
  return x;
}

static int32_t absolute_day(const struct tm *t) {
  int32_t year = t->tm_year + 1900;
  int32_t y = year - 1;
  return 365 * y + y / 4 - y / 100 + y / 400 + t->tm_yday;
}

// Choose a new, apparently random fish each day while excluding yesterday's.
// The same calendar day always selects the same fish, including after reboot.
static int daily_fish(int32_t day) {
  int yesterday = (int)(mix((uint32_t)(day - 1) ^ 0x46a1e5d3UL) % NUM_FISH);
  int choice = (int)(mix((uint32_t)day ^ 0xb53c29e1UL) % (NUM_FISH - 1));
  return choice >= yesterday ? choice + 1 : choice;
}

static void update_display(struct tm *tick) {
  int32_t day = absolute_day(tick);
  if (day != s_selected_day) {
    s_selected_day = day;
    int i = daily_fish(day);
    if (s_fish_bitmap) { bitmap_layer_set_bitmap(s_fish_layer, NULL); gbitmap_destroy(s_fish_bitmap); }
    s_fish_bitmap = gbitmap_create_with_resource(FISH[i].resource_id);
    bitmap_layer_set_bitmap(s_fish_layer, s_fish_bitmap);
    text_layer_set_text(s_common_layer, FISH[i].common_name);
    text_layer_set_text(s_scientific_layer, FISH[i].scientific_name);
  }
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick);
  if (!clock_is_24h_style() && s_time_buffer[0] == '0') memmove(s_time_buffer, s_time_buffer + 1, strlen(s_time_buffer));
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %b %e", tick);
  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick, TimeUnits units_changed) { update_display(tick); }

static TextLayer *make_label(GRect frame, const char *font_key, GTextAlignment align) {
  TextLayer *layer = text_layer_create(frame);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorBlack);
  text_layer_set_font(layer, fonts_get_system_font(font_key));
  text_layer_set_text_alignment(layer, align);
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(layer));
  return layer;
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_stack_push(s_window, true);
  Layer *root = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(root);
  int w = bounds.size.w;
  s_fish_layer = bitmap_layer_create(GRect(6, 15, w - 12, 106));
  bitmap_layer_set_alignment(s_fish_layer, GAlignCenter);
  bitmap_layer_set_background_color(s_fish_layer, GColorWhite);
  bitmap_layer_set_compositing_mode(s_fish_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_fish_layer));
  s_common_layer = make_label(GRect(4, 125, w - 8, 22), FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentCenter);
  s_scientific_layer = make_label(GRect(4, 148, w - 8, 20), FONT_KEY_GOTHIC_14, GTextAlignmentCenter);
  s_time_layer = make_label(GRect(2, 171, w - 4, 45), FONT_KEY_BITHAM_42_BOLD, GTextAlignmentCenter);
  s_date_layer = make_label(GRect(4, 210, w - 8, 18), FONT_KEY_GOTHIC_14, GTextAlignmentCenter);
  time_t now = time(NULL);
  update_display(localtime(&now));
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  if (s_fish_bitmap) gbitmap_destroy(s_fish_bitmap);
  bitmap_layer_destroy(s_fish_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_common_layer);
  text_layer_destroy(s_scientific_layer);
  window_destroy(s_window);
}

int main(void) { init(); app_event_loop(); deinit(); }
