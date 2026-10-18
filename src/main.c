#include <pebble.h>
#include <string.h>
#define NUM_FISH 44
#define TRIPLE_TAP_WINDOW_MS 1050

typedef struct {uint32_t resource_id; const char *common; const char *scientific;} Fish;
static const Fish FISH[NUM_FISH] = {
  {RESOURCE_ID_FISH_COREGONUS_LABRADORIUS, "Labrador whitefish", "Coregonus labradorius"},
  {RESOURCE_ID_FISH_OSMERUS_MORDAX, "Rainbow smelt", "Osmerus mordax"},
  {RESOURCE_ID_FISH_SALVELINUS_FONTINALIS, "Brook trout", "Salvelinus fontinalis"},
  {RESOURCE_ID_FISH_SALVELINUS_NAMAYCUSH, "Lake trout", "Salvelinus namaycush"},
  {RESOURCE_ID_FISH_SALMO_SALAR, "Atlantic salmon", "Salmo salar"},
  {RESOURCE_ID_FISH_ESOX_MASQUINONGY, "Muskellunge", "Esox masquinongy"},
  {RESOURCE_ID_FISH_ESOX_RETICULATUS, "Chain pickerel", "Esox reticulatus"},
  {RESOURCE_ID_FISH_ESOX_AMERICANUS, "Redfin pickerel", "Esox americanus"},
  {RESOURCE_ID_FISH_ESOX_INDUS, "Pike (ID uncertain)", "Esox Indus"},
  {RESOURCE_ID_FISH_COREGONUS_QUADRILATERALIS, "Whitefish (ID uncertain)", "Coregonus quadrilateralis"},
  {RESOURCE_ID_FISH_LEPOMIS_AURITUS, "Redbreast sunfish", "Lepomis auritus"},
  {RESOURCE_ID_FISH_LEPOMIS_GIBBOSUS, "Pumpkinseed", "Lepomis gibbosus"},
  {RESOURCE_ID_FISH_MENIDIA_NOTATA, "Atlantic silverside", "Menidia notata"},
  {RESOURCE_ID_FISH_ETHEOSTOMA_FUSIFORME, "Swamp darter", "Etheostoma fusiforme"},
  {RESOURCE_ID_FISH_ETHEOSTOMA_CAPRODES, "Logperch", "Etheostoma caprodes"},
  {RESOURCE_ID_FISH_ETHEOSTOMA_OLMSTEDI, "Tessellated darter", "Etheostoma olmstedi"},
  {RESOURCE_ID_FISH_ROCCUS_LINEATUS, "Striped bass", "Roccus lineatus"},
  {RESOURCE_ID_FISH_MORONE_AMERICANA, "White perch", "Morone americana"},
  {RESOURCE_ID_FISH_STIZOSTEDION_CANADENSE, "Sauger", "Stizostedion canadense"},
  {RESOURCE_ID_FISH_PERCA_FLAVESCENS_AMERICANA_FLUVIATILIS, "Yellow perch", "Perca (flavescens) (americana) fluviatilis"},
  {RESOURCE_ID_FISH_AMIURUS_NEBULOSUS, "Brown bullhead", "Amiurus nebulosus"},
  {RESOURCE_ID_FISH_GASTEROSTEUS_ACULEATUS, "Threespine stickleback", "Gasterosteus aculeatus"},
  {RESOURCE_ID_FISH_COTTUS_GRACILIS, "Slimy sculpin", "Cottus gracilis"},
  {RESOURCE_ID_FISH_LOTA_LOTA_MACULOSA, "Burbot", "Lota lota (maculosa)"},
  {RESOURCE_ID_FISH_PETROMYZON_MARINUS, "Sea lamprey", "Petromyzon marinus"},
  {RESOURCE_ID_FISH_ANQUILLA_ANQUILLA, "American eel", "Anguilla rostrata"},
  {RESOURCE_ID_FISH_UMBRA_LIMI, "Central mudminnow", "Umbra limi"},
  {RESOURCE_ID_FISH_FUNDULUS_MAJALIS, "Striped killifish", "Fundulus majalis"},
  {RESOURCE_ID_FISH_FUNDULUS_HETEROCLITUS, "Mummichog", "Fundulus heteroclitus"},
  {RESOURCE_ID_FISH_FUNDULUS_DIAPHANUS, "Banded killifish", "Fundulus diaphanus"},
  {RESOURCE_ID_FISH_CYPRINODON_VARIEGATUS, "Sheepshead minnow", "Cyprinodon variegatus"},
  {RESOURCE_ID_FISH_CATOSTOMUS_TERES, "White sucker", "Catostomus teres"},
  {RESOURCE_ID_FISH_ERIMYSON_SUCETTA, "Lake chubsucker", "Erimyson sucetta"},
  {RESOURCE_ID_FISH_SEMOTILUS_BULLARIS, "Fallfish", "Semotilus bullaris"},
  {RESOURCE_ID_FISH_SEMOTILUS_ATROMACULATUS, "Creek chub", "Semotilus atromaculatus"},
  {RESOURCE_ID_FISH_RHINICHTYS_ATRONASUS, "Blacknose dace", "Rhinichtys atronasus"},
  {RESOURCE_ID_FISH_RHINICHTYS_CATARACT, "Longnose dace", "Rhinichtys cataract\u00e6"},
  {RESOURCE_ID_FISH_PIMEPHALES_NOTATUS, "Bluntnose minnow", "Pimephales notatus"},
  {RESOURCE_ID_FISH_PIMEPHALES_PROMELAS, "Fathead minnow", "Pimephales promelas"},
  {RESOURCE_ID_FISH_NOTROPIS_BIFRENATUS, "Bridle shiner", "Notropis bifrenatus"},
  {RESOURCE_ID_FISH_NOTROPIS_CORNUTUS, "Common shiner", "Notropis cornutus"},
  {RESOURCE_ID_FISH_NOTEMIGONUS_CHRYSOLEUCUS, "Golden shiner", "Notemigonus chrysoleucus"},
  {RESOURCE_ID_FISH_CLUPEA_PSEUDOHARENGUS, "Alewife", "Clupea pseudoharengus"},
  {RESOURCE_ID_FISH_CLUPEA_SAPIDISSIMA, "American shad", "Clupea sapidissima"}
};
static Window *s_window;
static BitmapLayer *s_fish_layer;
static TextLayer *s_time_layer,*s_date_layer,*s_common_layer,*s_scientific_layer;
static GBitmap *s_fish_bitmap;
static int32_t s_day=-1;
static int s_fish_index=-1;
static char s_time_buffer[16],s_date_buffer[24];
static uint8_t s_tap_count=0;
static AppTimer *s_tap_timeout=NULL;

static int32_t absolute_day(const struct tm *t) {
  int32_t year=t->tm_year+1900,y=year-1;
  return 365*y+y/4-y/100+y/400+t->tm_yday;
}
// A seeded full-cycle permutation: no repeat until all 44 have appeared.
static int daily_fish(int32_t day) {return (int)(((uint32_t)day*17u+31u)%NUM_FISH);}
static void display_fish(int index) {
  if (index<0 || index>=NUM_FISH) return;
  GBitmap *replacement=gbitmap_create_with_resource(FISH[index].resource_id);
  if (!replacement) return;
  bitmap_layer_set_bitmap(s_fish_layer,NULL);
  if(s_fish_bitmap)gbitmap_destroy(s_fish_bitmap);
  s_fish_bitmap=replacement;
  s_fish_index=index;
  bitmap_layer_set_bitmap(s_fish_layer,s_fish_bitmap);
  text_layer_set_text(s_common_layer,FISH[index].common);
  text_layer_set_text(s_scientific_layer,FISH[index].scientific);
}
static void update_display(struct tm *tick) {
  int32_t day=absolute_day(tick);
  if(day!=s_day) {s_day=day;display_fish(daily_fish(day));}
  strftime(s_time_buffer,sizeof(s_time_buffer),clock_is_24h_style()?"%H:%M":"%I:%M",tick);
  if(!clock_is_24h_style() && s_time_buffer[0]=='0')memmove(s_time_buffer,s_time_buffer+1,strlen(s_time_buffer));
  strftime(s_date_buffer,sizeof(s_date_buffer),"%a, %b %e",tick);
  text_layer_set_text(s_time_layer,s_time_buffer);
  text_layer_set_text(s_date_layer,s_date_buffer);
}
static void tick_handler(struct tm *tick,TimeUnits changed){update_display(tick);}
static void reset_taps(void *context) {s_tap_count=0;s_tap_timeout=NULL;}
// Hardware acceleration tap, not touchscreen TouchService: stock firmware
// deliberately prohibits TouchService in watchfaces.
static void accel_tap_handler(AccelAxisType axis,int32_t direction) {
  if(s_tap_timeout) {app_timer_cancel(s_tap_timeout);s_tap_timeout=NULL;}
  s_tap_count++;
  if(s_tap_count>=3) {
    s_tap_count=0;
    display_fish((s_fish_index+1)%NUM_FISH);
    vibes_short_pulse();
  } else {
    s_tap_timeout=app_timer_register(TRIPLE_TAP_WINDOW_MS,reset_taps,NULL);
  }
}
static TextLayer *make_label(GRect frame,const char *font_key,GTextAlignment align) {
  TextLayer *layer=text_layer_create(frame);
  text_layer_set_background_color(layer,GColorClear);
  text_layer_set_text_color(layer,GColorBlack);
  text_layer_set_font(layer,fonts_get_system_font(font_key));
  text_layer_set_text_alignment(layer,align);
  layer_add_child(window_get_root_layer(s_window),text_layer_get_layer(layer));
  return layer;
}
static void init(void) {
  s_window=window_create();
  window_set_background_color(s_window,GColorWhite);
  window_stack_push(s_window,true);
  Layer *root=window_get_root_layer(s_window);GRect bounds=layer_get_bounds(root);int w=bounds.size.w;
  s_fish_layer=bitmap_layer_create(GRect(6,15,w-12,106));
  bitmap_layer_set_alignment(s_fish_layer,GAlignCenter);
  bitmap_layer_set_background_color(s_fish_layer,GColorWhite);
  bitmap_layer_set_compositing_mode(s_fish_layer,GCompOpAssign);
  layer_add_child(root,bitmap_layer_get_layer(s_fish_layer));
  s_common_layer=make_label(GRect(4,125,w-8,22),FONT_KEY_GOTHIC_18_BOLD,GTextAlignmentCenter);
  s_scientific_layer=make_label(GRect(4,147,w-8,22),FONT_KEY_GOTHIC_14,GTextAlignmentCenter);
  s_time_layer=make_label(GRect(2,171,w-4,45),FONT_KEY_BITHAM_42_BOLD,GTextAlignmentCenter);
  s_date_layer=make_label(GRect(4,210,w-8,18),FONT_KEY_GOTHIC_14,GTextAlignmentCenter);
  time_t now=time(NULL);update_display(localtime(&now));
  tick_timer_service_subscribe(MINUTE_UNIT,tick_handler);
  accel_tap_service_subscribe(accel_tap_handler);
}
static void deinit(void) {
  accel_tap_service_unsubscribe();tick_timer_service_unsubscribe();
  if(s_tap_timeout)app_timer_cancel(s_tap_timeout);
  if(s_fish_bitmap)gbitmap_destroy(s_fish_bitmap);
  bitmap_layer_destroy(s_fish_layer);
  text_layer_destroy(s_time_layer);text_layer_destroy(s_date_layer);
  text_layer_destroy(s_common_layer);text_layer_destroy(s_scientific_layer);
  window_destroy(s_window);
}
int main(void) {init();app_event_loop();deinit();}
