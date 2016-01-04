#include <pebble.h>

#include "sprinkles_configuration.h"

typedef struct {
  Window *window;
  GBitmap *homer_bitmap;
  BitmapLayer *homer_layer;
  Layer *homer_eyes_layer;
  GBitmap *donut_bitmap;
  Layer *hands_layer;
  AnimationProgress intro_animation_progress;
  int current_seconds;
  int current_minutes;
  int current_hours;
} SprinklesAppData;

static SprinklesAppData *s_app_data;

static int64_t prv_interpolate_int64_linear(int64_t from, int64_t to, AnimationProgress progress) {
  return from + ((progress * (to - from)) / ANIMATION_NORMALIZED_MAX);
}

static GRect prv_get_seconds_forward_rect(const GRect *layer_bounds) {
  return grect_inset(*layer_bounds, GEdgeInsets(2));
}

static GRect prv_get_donut_orbit_rect(const GRect *layer_bounds) {
  const GRect seconds_forward_rect = prv_get_seconds_forward_rect(layer_bounds);
  const SprinklesConfiguration *configuration = sprinkles_configuration_get_configuration();
  const unsigned int inset_factor_of_width = configuration->seconds_hand_enabled ? 10 : 5;
  return grect_inset(seconds_forward_rect,
                     GEdgeInsets(seconds_forward_rect.size.w / inset_factor_of_width));
}

static GRect prv_get_donut_rect(const GRect *layer_bounds, int32_t donut_angle) {
  const GRect donut_orbit_rect = prv_get_donut_orbit_rect(layer_bounds);
  GRect donut_rect = grect_centered_from_polar(donut_orbit_rect, GOvalScaleModeFitCircle, donut_angle,
                                               gbitmap_get_bounds(s_app_data->donut_bitmap).size);
  donut_rect.origin.y = (int16_t)prv_interpolate_int64_linear(0, donut_rect.origin.y,
                                                              s_app_data->intro_animation_progress);
  return donut_rect;
}

static int32_t prv_get_donut_angle(void) {
  // We can draw the donut on either the seconds hand or the minutes hand, depending on the configuration
  const SprinklesConfiguration *configuration = sprinkles_configuration_get_configuration();
  const int time_unit_of_interest = configuration->seconds_hand_enabled ? s_app_data->current_seconds :
                                                                          s_app_data->current_minutes;
  const int32_t angle_per_time_unit_ratio = TRIG_MAX_ANGLE / 60;
  return time_unit_of_interest * angle_per_time_unit_ratio;
}

static GPoint prv_get_donut_orbit_perimeter_point(const GRect *layer_bounds) {
  const GRect donut_rect = prv_get_donut_rect(layer_bounds, prv_get_donut_angle());
  return grect_center_point(&donut_rect);
}

static void prv_draw_major_hands(GContext *ctx, const GPoint *center, int32_t hand_length, int32_t angle,
                                 GColor color) {
  GPoint points[] = {
    {-3, 0},
    {3, 0},
    {6, (int16_t)-(hand_length - 8)},
    {0, (int16_t)-hand_length},
    {-6, (int16_t)-(hand_length - 8)},
  };

  const GPathInfo path_info = (GPathInfo) {
    .num_points = ARRAY_LENGTH(points),
    .points = points,
  };

  GPath *path = gpath_create(&path_info);
  gpath_move_to(path, *center);
  gpath_rotate_to(path, angle);
  graphics_context_set_fill_color(ctx, color);
  gpath_draw_filled(ctx, path);

  gpath_destroy(path);
}

static void prv_draw_seconds_hand(GContext *ctx, const GRect *layer_bounds, const GPoint *center) {
  const SprinklesConfiguration *configuration = sprinkles_configuration_get_configuration();

  const int32_t seconds_angle = s_app_data->current_seconds * TRIG_MAX_ANGLE / 60;

  graphics_context_set_stroke_color(ctx, configuration->seconds_hand_color);

  // Draw the thinner front part of the seconds hand
  const uint8_t seconds_forward_stroke_width = 1;
  graphics_context_set_stroke_width(ctx, seconds_forward_stroke_width);

  const GRect seconds_forward_rect = prv_get_seconds_forward_rect(layer_bounds);
  // Interpolate the seconds forward rect so that the seconds hand grows out to pierce the donut
  const int16_t inset = (int16_t)prv_interpolate_int64_linear(seconds_forward_rect.size.w / 2, 0,
                                                              s_app_data->intro_animation_progress);
  const GRect interpolated_seconds_forward_rect = grect_inset(seconds_forward_rect, GEdgeInsets(inset));

  const GPoint seconds_forward_point = gpoint_from_polar(interpolated_seconds_forward_rect, GOvalScaleModeFitCircle,
                                                         seconds_angle);
  graphics_draw_line(ctx, (*center), seconds_forward_point);

  // Draw the thicker back part of the seconds hand
  const uint8_t seconds_backward_stroke_width = 2;
  graphics_context_set_stroke_width(ctx, seconds_backward_stroke_width);
  const GRect seconds_backward_rect = grect_inset((*layer_bounds), GEdgeInsets(seconds_forward_rect.size.w * 43   / 100));
  const GPoint seconds_backward_point = gpoint_from_polar(seconds_backward_rect, GOvalScaleModeFitCircle,
                                                          seconds_angle - (TRIG_MAX_ANGLE / 2));
  graphics_draw_line(ctx, (*center), seconds_backward_point);
}

static void prv_hands_layer_update_proc(Layer *layer, GContext *ctx) {
  const SprinklesConfiguration *configuration = sprinkles_configuration_get_configuration();
  const GRect layer_bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&layer_bounds);

  // Minutes
  const int32_t minutes_angle = s_app_data->current_minutes * TRIG_MAX_ANGLE / 60;
  const int32_t minutes_hand_length = layer_bounds.size.w * 4 / 9;
  prv_draw_major_hands(ctx, &center, minutes_hand_length, minutes_angle, configuration->minute_hand_color);

  // Hours
  int32_t hours_angle = ((s_app_data->current_hours * TRIG_MAX_ANGLE) + minutes_angle) / 12;
  const int32_t hours_hand_length = layer_bounds.size.w * 2 / 7;
  prv_draw_major_hands(ctx, &center, hours_hand_length, hours_angle, configuration->hour_hand_color);

  // Seconds (if enabled in the configuration)
  if (configuration->seconds_hand_enabled) {
    prv_draw_seconds_hand(ctx, &layer_bounds, &center);
  }

  // Draw the black dot in the center of the watchface
  const int16_t center_circle_radius = 5;
  graphics_fill_circle(ctx, center, center_circle_radius);

  // Draw the donut
  const int32_t donut_angle = prv_get_donut_angle();
  const GRect donut_rect = prv_get_donut_rect(&layer_bounds, donut_angle);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_app_data->donut_bitmap, donut_rect);
}

void prv_draw_pupil(GContext *ctx, const GRect *eye_rect, const GPoint *perimeter_point) {
  const int16_t pupil_radius = 3;
  const GSize pupil_size = GSize(pupil_radius * 2, pupil_radius * 2);
  const GRect pupil_container_rect = grect_inset((*eye_rect), GEdgeInsets(2 * pupil_radius));
  const GPoint pupil_center = grect_center_point(&pupil_container_rect);
  const int32_t pupil_angle = atan2_lookup(perimeter_point->y - pupil_center.y,
                                           perimeter_point->x - pupil_center.x) + DEG_TO_TRIGANGLE(90);
  const GRect pupil_rect = grect_centered_from_polar(pupil_container_rect, GOvalScaleModeFitCircle,
                                                     pupil_angle, pupil_size);
  graphics_fill_radial(ctx, pupil_rect, GOvalScaleModeFitCircle, pupil_radius, 0, TRIG_MAX_ANGLE);
}

static void prv_homer_eyes_layer_update_proc(Layer *layer, GContext *ctx) {
  const GRect layer_bounds = layer_get_bounds(layer);

  const GPoint donut_orbit_perimeter_point = prv_get_donut_orbit_perimeter_point(&layer_bounds);

  const GEdgeInsets eye_rect_insets = GEdgeInsets(1);

  const GRect right_eye_rect = grect_inset(PBL_IF_ROUND_ELSE(GRect(53, 49, 33, 33),
                                                             GRect(52, 45, 35, 32)), eye_rect_insets);
  prv_draw_pupil(ctx, &right_eye_rect, &donut_orbit_perimeter_point);

  const GRect left_eye_rect = grect_inset(PBL_IF_ROUND_ELSE(GRect(22, 48, 32, 31),
                                                            GRect(22, 45, 32, 29)), eye_rect_insets);
  prv_draw_pupil(ctx, &left_eye_rect, &donut_orbit_perimeter_point);
}

static void prv_tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_app_data->current_hours = tick_time->tm_hour;
  s_app_data->current_minutes = tick_time->tm_min;
  s_app_data->current_seconds = tick_time->tm_sec;
  layer_mark_dirty(s_app_data->homer_eyes_layer);
  layer_mark_dirty(s_app_data->hands_layer);
}

static void prv_intro_animation_update(Animation *animation,
                                       const AnimationProgress progress) {
  if (s_app_data) {
    s_app_data->intro_animation_progress = progress;
    layer_mark_dirty(window_get_root_layer(s_app_data->window));
  }
}

static const AnimationImplementation s_intro_animation_implementation = {
  .update = prv_intro_animation_update,
};

static Animation *prv_create_intro_animation(void) {
  Animation *intro_animation = animation_create();
  animation_set_implementation(intro_animation, &s_intro_animation_implementation);
  animation_set_duration(intro_animation, 500);
  animation_set_curve(intro_animation, AnimationCurveEaseIn);
  return intro_animation;
}

static void prv_start_intro_animation(void) {
  SprinklesAppData *data = s_app_data;
  if (!data) {
    return;
  }

  Animation *intro_animation = prv_create_intro_animation();
  animation_schedule(intro_animation);
}

static void window_load(Window *window) {
  SprinklesAppData *data = window_get_user_data(window);

  Layer *root_layer = window_get_root_layer(window);
  const GRect root_layer_bounds = layer_get_bounds(root_layer);

  data->homer_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HOMER);

  GRect homer_bitmap_layer_frame = root_layer_bounds;
  homer_bitmap_layer_frame.size = gbitmap_get_bounds(data->homer_bitmap).size;
  grect_align(&homer_bitmap_layer_frame, &root_layer_bounds, GAlignBottom, true /* clip */);

  data->homer_layer = bitmap_layer_create(homer_bitmap_layer_frame);
  bitmap_layer_set_bitmap(data->homer_layer, data->homer_bitmap);
  bitmap_layer_set_compositing_mode(data->homer_layer, GCompOpSet);
  layer_add_child(root_layer, bitmap_layer_get_layer(data->homer_layer));


  data->homer_eyes_layer = layer_create(homer_bitmap_layer_frame);
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
  SprinklesAppData *data = window_get_user_data(window);

  layer_destroy(data->hands_layer);
  layer_destroy(data->homer_eyes_layer);

  gbitmap_destroy(data->donut_bitmap);

  bitmap_layer_destroy(data->homer_layer);
  gbitmap_destroy(data->homer_bitmap);

  window_destroy(window);
}

static void prv_configuration_changed_callback(SprinklesConfiguration *updated_configuration,
                                               void *context) {
  SprinklesAppData *data = context;
  if (data) {
    window_set_background_color(data->window, updated_configuration->background_color);
    layer_mark_dirty(window_get_root_layer(data->window));
  }
}

static void prv_app_did_focus(bool did_focus) {
  if (!did_focus) {
    return;
  }
  app_focus_service_unsubscribe();
  prv_start_intro_animation();
}

static void init(void) {
  s_app_data = calloc(1, sizeof(SprinklesAppData));

  s_app_data->window = window_create();
  window_set_user_data(s_app_data->window, s_app_data);
  window_set_window_handlers(s_app_data->window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  sprinkles_configuration_init();
  sprinkles_configuration_set_callback(prv_configuration_changed_callback, s_app_data);

  const SprinklesConfiguration *configuration = sprinkles_configuration_get_configuration();
  window_set_background_color(s_app_data->window, configuration->background_color);

  tick_timer_service_subscribe(SECOND_UNIT, prv_tick_timer_service_handler);

  window_stack_push(s_app_data->window, true /* animated */);

  // Subscribe to app focus events so we can schedule an intro animation
  app_focus_service_subscribe_handlers((AppFocusHandlers) {
    .did_focus = prv_app_did_focus,
  });
}

static void deinit(void) {
  sprinkles_configuration_deinit();
  free(s_app_data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
