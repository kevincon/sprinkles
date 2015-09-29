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

static GRect prv_get_seconds_forward_rect(const GRect *layer_bounds) {
  return grect_inset(*layer_bounds, GEdgeInsets(2));
}

static GRect prv_get_donut_orbit_rect(const GRect *layer_bounds) {
  const GRect seconds_forward_rect = prv_get_seconds_forward_rect(layer_bounds);
  return grect_inset(seconds_forward_rect, GEdgeInsets(seconds_forward_rect.size.w / 10));
}

static GPoint prv_get_donut_orbit_perimeter_point(const GRect *layer_bounds, int32_t seconds_angle) {
  const GRect donut_perimeter_rect = prv_get_donut_orbit_rect(layer_bounds);
  return gpoint_from_polar(donut_perimeter_rect, GOvalScaleModeFitCircle, seconds_angle);
}

static void prv_draw_major_hands(GContext *ctx, const GPoint *center, const GRect *hand_perimeter_rect, int32_t angle) {
  // Draw the thin part of the hand
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  const GPoint perimeter_point = gpoint_from_polar(*hand_perimeter_rect, GOvalScaleModeFitCircle, angle);
  graphics_draw_line(ctx, *center, perimeter_point);

  // Draw the elongated part of the hand
  graphics_context_set_stroke_color(ctx, GColorDarkCandyAppleRed);
  const uint8_t elongation_stroke_width = 9;
  graphics_context_set_stroke_width(ctx, elongation_stroke_width);
  const GRect elongation_perimeter_rect = grect_inset(*hand_perimeter_rect,
                                                      GEdgeInsets(hand_perimeter_rect->size.w / 5));
  const GPoint elongation_point = gpoint_from_polar(elongation_perimeter_rect, GOvalScaleModeFitCircle, angle);
  graphics_draw_line(ctx, perimeter_point, elongation_point);
}

void prv_draw_seconds_hand(GContext *ctx, const GRect *layer_bounds, const GPoint *center) {
  const int32_t seconds_angle = s_app_data->current_seconds * TRIG_MAX_ANGLE / 60;

  graphics_context_set_stroke_color(ctx, GColorBlack);

  // Draw the thinner front part of the seconds hand
  const uint8_t seconds_forward_stroke_width = 1;
  graphics_context_set_stroke_width(ctx, seconds_forward_stroke_width);

  const GRect seconds_forward_rect = prv_get_seconds_forward_rect(layer_bounds);
  const GPoint seconds_forward_point = gpoint_from_polar(seconds_forward_rect, GOvalScaleModeFitCircle, seconds_angle);
  graphics_draw_line(ctx, (*center), seconds_forward_point);

  // Draw the thicker back part of the seconds hand
  const uint8_t seconds_backward_stroke_width = 2;
  graphics_context_set_stroke_width(ctx, seconds_backward_stroke_width);
  const GRect seconds_backward_rect = grect_inset((*layer_bounds), GEdgeInsets(seconds_forward_rect.size.w / 2));
  const GPoint seconds_backward_point = gpoint_from_polar(seconds_backward_rect, GOvalScaleModeFitCircle,
                                                          seconds_angle - (TRIG_MAX_ANGLE / 2));
  graphics_draw_line(ctx, (*center), seconds_backward_point);

  // Draw the black dot in the center of the watchface
  const int16_t center_circle_radius = 5;
  graphics_fill_circle(ctx, (*center), center_circle_radius);

  // Draw the donut near the end of the seconds hand
  const GRect donut_orbit_rect = prv_get_donut_orbit_rect(layer_bounds);
  const GRect donut_rect = prv_shim_grect_centered_from_polar(donut_orbit_rect, GOvalScaleModeFitCircle, seconds_angle,
                                                              gbitmap_get_bounds(s_app_data->donut_bitmap).size);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_app_data->donut_bitmap, donut_rect);
}

static void prv_hands_layer_update_proc(Layer *layer, GContext *ctx) {
  const GRect layer_bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&layer_bounds);

  // Minutes and hours
  const int32_t minutes_angle = s_app_data->current_minutes * TRIG_MAX_ANGLE / 60;
  const GRect minutes_rect = grect_inset(layer_bounds, GEdgeInsets(layer_bounds.size.w / 18));
  prv_draw_major_hands(ctx, &center, &minutes_rect, minutes_angle);

  const int32_t hours_angle = ((s_app_data->current_hours * TRIG_MAX_ANGLE) + minutes_angle) / 12;
  const GRect hours_rect = grect_inset(layer_bounds, GEdgeInsets(layer_bounds.size.w / 6));
  prv_draw_major_hands(ctx, &center, &hours_rect, hours_angle);

  // Seconds
  prv_draw_seconds_hand(ctx, &layer_bounds, &center);
}

void prv_draw_pupil(GContext *ctx, const int32_t seconds_angle, const GRect *eye_rect,
                    const GPoint *seconds_perimeter_point) {
  const int16_t pupil_radius = 3;
  const GSize pupil_size = GSize(pupil_radius * 2, pupil_radius * 2);
  const GRect pupil_container_rect = grect_inset((*eye_rect), GEdgeInsets(2 * pupil_radius));
  const GPoint pupil_center = grect_center_point(&pupil_container_rect);
  const int32_t pupil_angle = atan2_lookup(seconds_perimeter_point->y - pupil_center.y,
                                           seconds_perimeter_point->x - pupil_center.x) + DEG_TO_TRIGANGLE(90);
  const GRect pupil_rect = prv_shim_grect_centered_from_polar(pupil_container_rect, GOvalScaleModeFitCircle,
                                                              pupil_angle, pupil_size);
  graphics_fill_radial(ctx, pupil_rect, GOvalScaleModeFitCircle, pupil_radius, 0, TRIG_MAX_ANGLE);
}

static void prv_homer_eyes_layer_update_proc(Layer *layer, GContext *ctx) {
  const GRect layer_bounds = layer_get_bounds(layer);
  const int32_t seconds_angle = s_app_data->current_seconds * TRIG_MAX_ANGLE / 60;

  const GPoint donut_orbit_perimeter_point = prv_get_donut_orbit_perimeter_point(&layer_bounds, seconds_angle);

  const GRect right_eye_rect = GRect(PBL_IF_ROUND_ELSE(72, 58), 51, 32, 32);
  prv_draw_pupil(ctx, seconds_angle, &right_eye_rect, &donut_orbit_perimeter_point);

  const GRect left_eye_rect = GRect(PBL_IF_ROUND_ELSE(42, 24), 50, 28, 28);
  prv_draw_pupil(ctx, seconds_angle, &left_eye_rect, &donut_orbit_perimeter_point);
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
  window_set_background_color(s_app_data->window, GColorPictonBlue);

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
