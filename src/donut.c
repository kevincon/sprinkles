#include <pebble.h>

typedef struct {
  Window *window;
  GBitmap *homer_bitmap;
  BitmapLayer *homer_layer;
  Layer *homer_eyes_layer;
  GBitmap *donut_bitmap;
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
  const GRect layer_bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&layer_bounds);

  // Minutes and hours

  const int32_t minutes_angle = s_app_data->current_minutes * TRIG_MAX_ANGLE / 60;
  const int32_t hours_angle = ((s_app_data->current_hours * TRIG_MAX_ANGLE) + minutes_angle) / 12;

  graphics_context_set_stroke_color(ctx, GColorFolly);
  const uint8_t minutes_and_hours_stroke_width = 6;
  graphics_context_set_stroke_width(ctx, minutes_and_hours_stroke_width);

  const GRect minutes_rect = grect_inset(layer_bounds, GEdgeInsets(layer_bounds.size.w / 12));
  const GPoint minutes_point = gpoint_from_polar(minutes_rect, GOvalScaleModeFitCircle, minutes_angle);
  graphics_draw_line(ctx, center, minutes_point);

  const GRect hours_rect = grect_inset(layer_bounds, GEdgeInsets(layer_bounds.size.w / 4));
  const GPoint hours_point = gpoint_from_polar(hours_rect, GOvalScaleModeFitCircle, hours_angle);
  graphics_draw_line(ctx, center, hours_point);

  // Seconds

  const int32_t seconds_angle = s_app_data->current_seconds * TRIG_MAX_ANGLE / 60;

  graphics_context_set_stroke_color(ctx, GColorBlack);
  const uint8_t seconds_forward_stroke_width = 1;
  graphics_context_set_stroke_width(ctx, seconds_forward_stroke_width);

  const GRect seconds_forward_rect = grect_inset(layer_bounds, GEdgeInsets(layer_bounds.size.w / 12));
  const GPoint seconds_forward_point = gpoint_from_polar(seconds_forward_rect, GOvalScaleModeFitCircle, seconds_angle);
  graphics_draw_line(ctx, center, seconds_forward_point);

  const uint8_t seconds_backward_stroke_width = 2;
  graphics_context_set_stroke_width(ctx, seconds_backward_stroke_width);
  const GRect seconds_backward_rect = grect_inset(layer_bounds, GEdgeInsets(seconds_forward_rect.size.w / 2));
  const GPoint seconds_backward_point = gpoint_from_polar(seconds_backward_rect, GOvalScaleModeFitCircle,
                                                          seconds_angle - (TRIG_MAX_ANGLE / 2));
  graphics_draw_line(ctx, center, seconds_backward_point);

  const int16_t center_circle_radius = 5;
  const int16_t center_circle_diameter = center_circle_radius * 2;
  graphics_fill_circle(ctx, center, center_circle_radius);
  const GRect donut_orbit_rect = grect_inset(layer_bounds, GEdgeInsets(layer_bounds.size.w / 6));
  const GRect donut_rect = prv_shim_grect_centered_from_polar(donut_orbit_rect, GOvalScaleModeFitCircle, seconds_angle,
                                                              gbitmap_get_bounds(s_app_data->donut_bitmap).size);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_app_data->donut_bitmap, donut_rect);
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

  const GRect right_eye_rect = GRect(PBL_IF_ROUND_ELSE(72, 58), 51, 32, 32);
  prv_draw_pupil(ctx, seconds_angle, &right_eye_rect);

  const GRect left_eye_rect = GRect(PBL_IF_ROUND_ELSE(42, 24), 50, 28, 28);
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

  data->donut_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DONUT);

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

  gbitmap_destroy(data->donut_bitmap);

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
