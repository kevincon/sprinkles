#pragma once

#include <pebble.h>

typedef struct {
  GColor background_color;
  GColor hour_hand_color;
  GColor minute_hand_color;
  GColor center_dot_color;
  bool seconds_hand_enabled;
  GColor seconds_hand_color;
  bool date_enabled;
  GColor date_background_color;
  GColor date_text_color;
} SprinklesConfiguration;

typedef void (*SprinklesConfigurationChangedCallback)(SprinklesConfiguration *updated_configuration,
                                                      void *context);

void sprinkles_configuration_init(void);

void sprinkles_configuration_set_callback(SprinklesConfigurationChangedCallback callback,
                                          void *context);

SprinklesConfiguration *sprinkles_configuration_get_configuration(void);

void sprinkles_configuration_deinit(void);
