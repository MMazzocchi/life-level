#include <pebble.h>

static Window *window;
static TextLayer *time_layer;
static TextLayer *date_layer;
static Layer *canvas;
static GRect bounds;

static GBitmap* empty;
static GBitmap* quarter;
static GBitmap* half;
static GBitmap* three_quarter;
static GBitmap* full;

static Animation *charge_animation;
static AnimationImplementation ami;

int sectors;
bool charging;
int minutes = -1;

int chargeToSectors(int charge) {
  int sectors = (charge * 12)/100;

  if(sectors > 12) {
  return 12;
  }

  return sectors;
}

void drawHearts(Layer *layer, GContext *ctx) {
  int y = (bounds.size.h/2)-12;
  int x = 9;

  int sectors_left = sectors;

  for(int i=0; i<3; i++) {
    if(sectors_left > 3) {
      graphics_draw_bitmap_in_rect(ctx, full, GRect(x, y, 36, 32));
    } else if(sectors_left > 2) {
      graphics_draw_bitmap_in_rect(ctx, three_quarter, GRect(x, y, 36, 32));
    } else if(sectors_left > 1) {
      graphics_draw_bitmap_in_rect(ctx, half, GRect(x, y, 36, 32));
    } else if(sectors_left > 0) {
      graphics_draw_bitmap_in_rect(ctx, quarter, GRect(x, y, 36, 32));
    } else {
      graphics_draw_bitmap_in_rect(ctx, empty, GRect(x, y, 36, 32));
    }
    x+=45;
    sectors_left -= 4;
  }
}

void drawTime() {
  time_t now = time(NULL);
  struct tm *time = localtime(&now);

  static char time_text[] = "XX:XX";

  strftime(time_text, sizeof(time_text), "%I:%M", time);
  int hour;
  if(clock_is_24h_style()) {
    strftime(time_text, sizeof(time_text), "%H:%M", time);
    text_layer_set_text(time_layer, time_text);
  } else {
    strftime(time_text, sizeof(time_text), "%I:%M", time);
    hour = time->tm_hour % 12;
    if(hour == 0 || hour > 9) {
      text_layer_set_text(time_layer, time_text);
    } else {
      text_layer_set_text(time_layer, time_text+1);
    }
  }

  static char date_text[] = "Saturday, January 31 2014";
  strftime(date_text, sizeof(date_text), "%a, %b %d", time);

  text_layer_set_text(date_layer, date_text);
}

void tick(struct tm *time, TimeUnits changed) {
  BatteryChargeState state = battery_state_service_peek();

  bool redraw = false;

  if(!charging) {
    if(state.is_charging) {
      charging = true;
      animation_schedule(charge_animation);

    } else {
      int charge = state.charge_percent;
      int new_sectors = chargeToSectors(charge);
      if(sectors != new_sectors) {
        sectors = new_sectors;
        redraw = true;
      }
    }
  } else {
    if(!state.is_charging) {
      animation_unschedule(charge_animation);
      charging = false;
    }
  }

  if(minutes != time->tm_min) {
    minutes = time->tm_min;
    redraw = true;
  }

  if(redraw) {
    drawTime();
    layer_mark_dirty(canvas);
  }
}

void loadBitmaps() {
  empty = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EMPTY);
  quarter = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_QUARTER);
  half = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HALF);
  three_quarter = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THREE_QUARTER);
  full = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FULL);
}

void chargeStart(Animation* animation, void *context) {
  sectors = 0;
}

void chargeStop(Animation *animation, void *context) {}
void chargeSetup(Animation *animation) {}
void chargeTeardown(Animation *animation) {}

void chargeUpdate(Animation *animation, int t) {
  sectors += 1;

  if(sectors > 13) {
    sectors = 0;
  }

  layer_mark_dirty(canvas);
}

void initAnimation() {
  charge_animation = animation_create();
  ami = (struct AnimationImplementation) {
    .setup = (AnimationSetupImplementation)chargeSetup,
    .update = (AnimationUpdateImplementation)chargeUpdate,
    .teardown = (AnimationTeardownImplementation)chargeTeardown
  };
  animation_set_implementation(charge_animation, &ami);

  AnimationHandlers ah = (struct AnimationHandlers) {
    .started = (AnimationStartedHandler)chargeStart,
    .stopped = (AnimationStoppedHandler)chargeStop
  };

  animation_set_handlers(charge_animation, ah, NULL);

  animation_set_curve(charge_animation, AnimationCurveLinear);
  animation_set_duration(charge_animation, ANIMATION_DURATION_INFINITE);
}

static void init(void) {
  sectors = 0;
  charging = false;

  loadBitmaps();
  initAnimation();

  window = window_create();
  window_stack_push(window, true);

  Layer *window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  GRect t_box = GRect(0, (bounds.size.h/2)-75, bounds.size.w, 45);

  time_layer = text_layer_create(t_box);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer,
    fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));

  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);

  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  GRect d_box = GRect(0, (bounds.size.h/2)+31, bounds.size.w, 45);

  date_layer = text_layer_create(d_box);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_font(date_layer,
    fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);

  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  GRect box = GRect(0, 0, bounds.size.w, bounds.size.h);

  canvas = layer_create(box);
  layer_set_update_proc(canvas, &drawHearts);
  layer_add_child(window_layer, canvas);

  tick_timer_service_subscribe(SECOND_UNIT, &tick);
}

static void deinit(void) {
  gbitmap_destroy(empty);
  gbitmap_destroy(quarter);
  gbitmap_destroy(half);
  gbitmap_destroy(three_quarter);
  gbitmap_destroy(full);

  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  layer_destroy(canvas);

  animation_destroy(charge_animation);

  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
