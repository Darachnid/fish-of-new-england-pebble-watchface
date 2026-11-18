#include <pebble.h>
#include <string.h>

#define NUM_FISH 44

#define PERSIST_KEY_COLOR_FISH 1
#define PERSIST_KEY_DISPLAY_STYLE 3
#define PERSIST_KEY_PREVIEW_SPECIES 4
#define PREVIEW_INTERVAL_MS 5000

typedef struct {
  uint32_t color_resource_id;
  uint32_t mono_resource_id;
  const char *common;
  const char *scientific;
} Fish;

#include "fish_table.inc"

static Window *s_window;
static BitmapLayer *s_fish_layer;
static TextLayer *s_time_layer, *s_date_layer, *s_common_layer, *s_scientific_layer;
static GBitmap *s_fish_bitmap;
static int32_t s_day = -1;
static int s_fish_index = -1;
static bool s_color_fish = true;
static bool s_preview_species = false;
static AppTimer *s_preview_timer;
static char s_time_buffer[16], s_date_buffer[24];

static void display_fish(int index);
static int daily_fish(int32_t day);

static uint32_t fish_resource_id(int index) {
  if (index < 0 || index >= NUM_FISH) {
    return 0;
  }
  return s_color_fish ? FISH[index].color_resource_id : FISH[index].mono_resource_id;
}

static void apply_art_style(void) {
  bitmap_layer_set_compositing_mode(s_fish_layer, GCompOpAssign);
  if (s_fish_index >= 0) {
    display_fish(s_fish_index);
  }
}

static void set_color_fish(bool color) {
  if (color == s_color_fish) {
    return;
  }
  s_color_fish = color;
  persist_write_bool(PERSIST_KEY_COLOR_FISH, s_color_fish);
  apply_art_style();
}

static void preview_timer_callback(void *context) {
  if (!s_preview_species) {
    s_preview_timer = NULL;
    return;
  }
  int next = s_fish_index < 0 ? 0 : (s_fish_index + 1) % NUM_FISH;
  display_fish(next);
  s_preview_timer = app_timer_register(PREVIEW_INTERVAL_MS, preview_timer_callback, NULL);
}

static void update_preview_timer(void) {
  if (s_preview_timer) {
    app_timer_cancel(s_preview_timer);
    s_preview_timer = NULL;
  }
  if (!s_preview_species) {
    if (s_day >= 0) {
      display_fish(daily_fish(s_day));
    }
    return;
  }
  s_preview_timer = app_timer_register(PREVIEW_INTERVAL_MS, preview_timer_callback, NULL);
}

static void set_preview_species(bool enabled) {
  if (enabled == s_preview_species) {
    return;
  }
  s_preview_species = enabled;
  persist_write_bool(PERSIST_KEY_PREVIEW_SPECIES, s_preview_species);
  update_preview_timer();
}

static int32_t absolute_day(const struct tm *t) {
  int32_t year = t->tm_year + 1900;
  int32_t y = year - 1;
  return 365 * y + y / 4 - y / 100 + y / 400 + t->tm_yday;
}

static int daily_fish(int32_t day) {
  return (int)(((uint32_t)day * 17u + 31u) % NUM_FISH);
}

static void display_fish(int index) {
  if (index < 0 || index >= NUM_FISH) {
    return;
  }
  uint32_t resource_id = fish_resource_id(index);
  GBitmap *replacement = gbitmap_create_with_resource(resource_id);
  if (!replacement) {
    return;
  }
  bitmap_layer_set_bitmap(s_fish_layer, NULL);
  if (s_fish_bitmap) {
    gbitmap_destroy(s_fish_bitmap);
  }
  s_fish_bitmap = replacement;
  s_fish_index = index;
  bitmap_layer_set_bitmap(s_fish_layer, s_fish_bitmap);
  text_layer_set_text(s_common_layer, FISH[index].common);
  text_layer_set_text(s_scientific_layer, FISH[index].scientific);
}

static void update_display(struct tm *tick) {
  int32_t day = absolute_day(tick);
  if (day != s_day) {
    s_day = day;
    if (!s_preview_species) {
      display_fish(daily_fish(day));
    }
  }
  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick);
  if (!clock_is_24h_style() && s_time_buffer[0] == '0') {
    memmove(s_time_buffer, s_time_buffer + 1, strlen(s_time_buffer));
  }
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %b %e", tick);
  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick, TimeUnits changed) {
  update_display(tick);
}

static void load_persisted_settings(void) {
  if (persist_exists(PERSIST_KEY_COLOR_FISH)) {
    s_color_fish = persist_read_bool(PERSIST_KEY_COLOR_FISH);
    return;
  }
  if (persist_exists(PERSIST_KEY_DISPLAY_STYLE)) {
    int style = persist_read_int(PERSIST_KEY_DISPLAY_STYLE);
    s_color_fish = (style != 2 && style != 3) ? true : false;
    if (style == 3) {
      s_color_fish = false;
    }
    return;
  }
  s_color_fish = true;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *color_tuple = dict_find(iterator, MESSAGE_KEY_ColorFish);
  if (color_tuple) {
    set_color_fish(color_tuple->value->uint8 != 0);
  }

  Tuple *preview_tuple = dict_find(iterator, MESSAGE_KEY_PreviewCycle);
  if (preview_tuple) {
    set_preview_species(preview_tuple->value->uint8 != 0);
  }

  Tuple *style_tuple = dict_find(iterator, MESSAGE_KEY_DisplayStyle);
  if (style_tuple) {
    if (style_tuple->type == TUPLE_CSTRING) {
      const char *s = style_tuple->value->cstring;
      if (strcmp(s, "2") == 0 || strcmp(s, "3") == 0) {
        set_color_fish(false);
      } else {
        set_color_fish(true);
      }
    } else {
      set_color_fish(style_tuple->value->uint8 != 2 && style_tuple->value->uint8 != 3);
    }
  }
}

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
  load_persisted_settings();

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
  s_scientific_layer = make_label(GRect(4, 147, w - 8, 22), FONT_KEY_GOTHIC_14, GTextAlignmentCenter);
  s_time_layer = make_label(GRect(2, 171, w - 4, 45), FONT_KEY_BITHAM_42_BOLD, GTextAlignmentCenter);
  s_date_layer = make_label(GRect(4, 210, w - 8, 18), FONT_KEY_GOTHIC_14, GTextAlignmentCenter);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(64, 64);

  if (persist_exists(PERSIST_KEY_PREVIEW_SPECIES)) {
    s_preview_species = persist_read_bool(PERSIST_KEY_PREVIEW_SPECIES);
  }

  time_t now = time(NULL);
  struct tm *tick = localtime(&now);
  update_display(tick);
  if (s_preview_species) {
    update_preview_timer();
  }
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  if (s_preview_timer) {
    app_timer_cancel(s_preview_timer);
    s_preview_timer = NULL;
  }
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  if (s_fish_bitmap) {
    gbitmap_destroy(s_fish_bitmap);
  }
  bitmap_layer_destroy(s_fish_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_common_layer);
  text_layer_destroy(s_scientific_layer);
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}