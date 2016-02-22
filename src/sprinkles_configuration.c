#include "sprinkles_configuration.h"

// Might switch away from using enamel in the future, hence the need for this wrapper file
#include "enamel.h"

static SprinklesConfigurationChangedCallback s_sprinkles_configuration_changed_callback;
static void *s_sprinkles_configuration_callback_context;
static SprinklesConfiguration s_current_sprinkles_configuration;

static void prv_update_sprinkles_configuration(SprinklesConfiguration *configuration) {
  if (!configuration) {
    return;
  }

  *configuration = (SprinklesConfiguration) {
    .background_color = get_background_color(),
    .hour_hand_color = get_hour_hand_color(),
    .minute_hand_color = get_minute_hand_color(),
    .center_dot_color = get_center_dot_color(),
    .seconds_hand_enabled = get_seconds_hand_enabled(),
    .seconds_hand_color = get_seconds_hand_color(),
    .date_enabled = get_date_enabled(),
    .date_background_color = get_date_background_color(),
    .date_text_color = get_date_text_color(),
  };
}

static void prv_app_message_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Update the cached configuration
  prv_update_sprinkles_configuration(&s_current_sprinkles_configuration);

  // Call the user-provided callback, if one was provided
  if (s_sprinkles_configuration_changed_callback) {
    s_sprinkles_configuration_changed_callback(&s_current_sprinkles_configuration,
                                               s_sprinkles_configuration_callback_context);
  }
}

void sprinkles_configuration_init(void) {
  enamel_init(0, 0);
  enamel_register_custom_inbox_received(prv_app_message_inbox_received_handler);

  prv_update_sprinkles_configuration(&s_current_sprinkles_configuration);
}

void sprinkles_configuration_set_callback(SprinklesConfigurationChangedCallback callback,
                                          void *context) {
  s_sprinkles_configuration_changed_callback = callback;
  s_sprinkles_configuration_callback_context = context;
}

SprinklesConfiguration *sprinkles_configuration_get_configuration(void) {
  return &s_current_sprinkles_configuration;
}

void sprinkles_configuration_deinit(void) {
  enamel_deinit();
}
