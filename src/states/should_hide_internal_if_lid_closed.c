#include <gtk/gtk.h>
#include "config.h"
#include "../constants/main.c"

#ifndef SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_STATE
#define SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_STATE

void set_should_hide_internal_if_lid_closed(gboolean value) {
  GSettings *settings = g_settings_new(APPLICATION_ID);
  g_settings_set_boolean(settings, SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_GSETTINGS_KEY, value);
  g_object_unref(settings);
}

gboolean get_should_hide_internal_if_lid_closed() {
  GSettings *settings = g_settings_new(APPLICATION_ID);
  gboolean value = g_settings_get_boolean(settings, SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_GSETTINGS_KEY);
  g_object_unref(settings);

  return value;
}

#endif
