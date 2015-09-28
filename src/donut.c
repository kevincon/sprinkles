#include <pebble.h>

typedef struct {
  Window *window;
  GBitmap *homer_bitmap;
  BitmapLayer *homer_layer;
  Layer *homer_eyes_layer;
  Layer *hands_layer;
  int current_seconds;
  int current_minutes;
  int current_hours;
} DonutAppData;

static DonutAppData *s_app_data;

static GRect prv_shim_grect_centered_from_polar(GRect rect, GOvalScaleMode scale_mode, int32_t angle, GSize size) {
  const GPoint point_centered_from_polar = gpoint_from_polar(rect, scale_mode, angle);
  return GRect(point_centered_from_polar.x - (size.w / 2),
               point_centered_from_polar.y - (size.h / 2), size.w, size.h);
}

static void prv_hands_layer_update_proc(Layer *layer, GContext *ctx) {

}

void prv_draw_pupil(GContext *ctx, const int32_t seconds_angle, const GRect *eye_rect) {
  const int16_t pupil_radius = 3;
  const GSize pupil_size = GSize(pupil_radius * 2, pupil_radius * 2);
  const GRect pupil_container_rect = grect_inset((*eye_rect), GEdgeInsets(2 * pupil_radius));
  const GRect pupil_rect = prv_shim_grect_centered_from_polar(pupil_container_rect, GOvalScaleModeFitCircle,
                                                              seconds_angle, pupil_size);
  graphics_fill_radial(ctx, pupil_rect, GOvalScaleModeFitCircle, pupil_radius, 0, TRIG_MAX_ANGLE);
}

static void prv_homer_eyes_layer_update_proc(Layer *layer, GContext *ctx) {
  const int32_t seconds_angle = s_app_data->current_seconds * TRIG_MAX_ANGLE / 60;

  const GRect right_eye_rect = GRect(72, 51, 32, 32);
  prv_draw_pupil(ctx, seconds_angle, &right_eye_rect);

  const GRect left_eye_rect = GRect(42, 50, 28, 28);
  prv_draw_pupil(ctx, seconds_angle, &left_eye_rect);
}

static void prv_tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_app_data->current_hours = tick_time->tm_hour;
  s_app_data->current_minutes = tick_time->tm_min;
  s_app_data->current_seconds = tick_time->tm_sec;
  layer_mark_dirty(s_app_data->homer_eyes_layer);
  layer_mark_dirty(s_app_data->hands_layer);
}

static void window_load(Window *window) {
  DonutAppData *data = window_get_user_data(window);

  Layer *root_layer = window_get_root_layer(window);
  const GRect root_layer_bounds = layer_get_bounds(root_layer);

  data->homer_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HOMER);

  data->homer_layer = bitmap_layer_create(root_layer_bounds);
  bitmap_layer_set_bitmap(data->homer_layer, data->homer_bitmap);
  bitmap_layer_set_alignment(data->homer_layer, GAlignBottom);
  bitmap_layer_set_compositing_mode(data->homer_layer, GCompOpSet);
  layer_add_child(root_layer, bitmap_layer_get_layer(data->homer_layer));

  data->homer_eyes_layer = layer_create(root_layer_bounds);
  layer_set_update_proc(data->homer_eyes_layer, prv_homer_eyes_layer_update_proc);
  layer_add_child(root_layer, data->homer_eyes_layer);

  data->hands_layer = layer_create(root_layer_bounds);
  layer_set_update_proc(data->hands_layer, prv_hands_layer_update_proc);
  layer_add_child(root_layer, data->hands_layer);

  const time_t current_time = time(NULL);
  struct tm *current_time_tm = localtime(&current_time);
  prv_tick_timer_service_handler(current_time_tm, SECOND_UNIT);
}

static void window_unload(Window *window) {
  DonutAppData *data = window_get_user_data(window);

  layer_destroy(data->hands_layer);
  layer_destroy(data->homer_eyes_layer);

  bitmap_layer_destroy(data->homer_layer);
  gbitmap_destroy(data->homer_bitmap);

  window_destroy(window);
}

static void init(void) {
  s_app_data = malloc(sizeof(DonutAppData));
  memset(s_app_data, 0, sizeof(DonutAppData));

  s_app_data->window = window_create();
  window_set_user_data(s_app_data->window, s_app_data);
  window_set_window_handlers(s_app_data->window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  tick_timer_service_subscribe(SECOND_UNIT, prv_tick_timer_service_handler);

  window_stack_push(s_app_data->window, true /* animated */);
}

static void deinit(void) {
  free(s_app_data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
