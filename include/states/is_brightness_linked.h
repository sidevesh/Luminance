#include <gtk/gtk.h>

#include "constants.h"

#ifndef IS_BRIGHTNESS_LINKED_STATE_H
#define IS_BRIGHTNESS_LINKED_STATE_H

static void set_is_brightness_linked(gboolean value) {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  g_settings_set_boolean(settings, IS_BRIGHTNESS_LINKED_GSETTINGS_KEY, value);
  g_object_unref(settings);
}

static gboolean get_is_brightness_linked() {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  gboolean value = g_settings_get_boolean(settings, IS_BRIGHTNESS_LINKED_GSETTINGS_KEY);
  g_object_unref(settings);

  return value;
}

#endif
