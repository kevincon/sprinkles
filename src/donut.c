#include <pebble.h>

typedef struct {
  Window *window;
  GBitmap *homer_bitmap;
  BitmapLayer *homer_layer;
  Layer *homer_eyes_layer;
  Layer *hands_layer;
} DonutAppData;

static DonutAppData *s_app_data;

static GRect prv_shim_grect_centered_from_polar(GRect rect, GOvalScaleMode scale_mode, int32_t angle, GSize size) {
  const GPoint point_centered_from_polar = gpoint_from_polar(rect, scale_mode, angle);
  return GRect(point_centered_from_polar.x - (size.w / 2),
               point_centered_from_polar.y - (size.h / 2), size.w, size.h);
}

static GRect prv_rect_from_polar(const GPoint *center, uint16_t radius) {
  const int16_t diameter = radius * 2;
  return GRect(center->x - radius, center->y - radius, diameter, diameter);
}

static void prv_hands_layer_update_proc(Layer *layer, GContext *ctx) {

}

static void prv_homer_eyes_layer_update_proc(Layer *layer, GContext *ctx) {
  const int16_t pupil_radius = 3;

  const GRect right_eye_rect = GRect(72, 51, 32, 32);
  const GPoint right_eye_center = grect_center_point(&right_eye_rect);
  const GRect right_eye_pupil_rect = prv_rect_from_polar(&right_eye_center, pupil_radius);
  graphics_fill_radial(ctx, right_eye_pupil_rect, GOvalScaleModeFitCircle, pupil_radius, 0, TRIG_MAX_ANGLE);

  const GRect left_eye_rect = GRect(41, 50, 30, 30);
  const GPoint left_eye_center = grect_center_point(&left_eye_rect);
  const GRect left_eye_pupil_rect = prv_rect_from_polar(&left_eye_center, pupil_radius);
  graphics_fill_radial(ctx, left_eye_pupil_rect, GOvalScaleModeFitCircle, pupil_radius, 0, TRIG_MAX_ANGLE);
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
