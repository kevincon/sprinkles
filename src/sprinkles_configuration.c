#include "sprinkles_configuration.h"

// Might switch away from using autoconfig in the future, hence the need for this wrapper file
#include "autoconfig.h"

static SprinklesConfigurationChangedCallback s_sprinkles_configuration_changed_callback;
static void *s_sprinkles_configuration_callback_context;
static SprinklesConfiguration s_current_sprinkles_configuration;

static void prv_update_sprinkles_configuration(SprinklesConfiguration *configuration) {
  if (!configuration) {
    return;
  }

  *configuration = (SprinklesConfiguration) {
    .background_color = getBackground_color(),
    .hour_hand_color = getHour_hand_color(),
    .minute_hand_color = getMinute_hand_color(),
    .center_dot_color = getCenter_dot_color(),
    .seconds_hand_enabled = getSeconds_hand_enabled(),
    .seconds_hand_color = getSeconds_hand_color(),
  };
}

static void prv_app_message_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Let autoconfig handle the received message
  autoconfig_in_received_handler(iter, context);

  // Update the cached configuration
  prv_update_sprinkles_configuration(&s_current_sprinkles_configuration);

  // Call the user-provided callback, if one was provided
  if (s_sprinkles_configuration_changed_callback) {
    s_sprinkles_configuration_changed_callback(&s_current_sprinkles_configuration,
                                               s_sprinkles_configuration_callback_context);
  }
}

void sprinkles_configuration_init(void) {
  const uint32_t size_inbound = 300;
  const uint32_t size_outbound = 0;
  autoconfig_init(size_inbound, size_outbound);

  prv_update_sprinkles_configuration(&s_current_sprinkles_configuration);

  // Register a custom received handler which in turn will call autoconfig's received handler
  app_message_register_inbox_received(prv_app_message_inbox_received_handler);
}

void sprinkles_configuration_set_callback(SprinklesConfigurationChangedCallback callback, void *context) {
  s_sprinkles_configuration_changed_callback = callback;
  s_sprinkles_configuration_callback_context = context;
}

SprinklesConfiguration *sprinkles_configuration_get_configuration(void) {
  return &s_current_sprinkles_configuration;
}

void sprinkles_configuration_deinit(void) {
  autoconfig_deinit();
}
