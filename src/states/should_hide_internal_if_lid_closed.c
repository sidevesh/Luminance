#include "states/lid.h"

#include <gtk/gtk.h>

#include "constants.h"

void set_should_hide_internal_if_lid_closed(gboolean value) {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  g_settings_set_boolean(settings, SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_GSETTINGS_KEY, value);
  g_object_unref(settings);
}

gboolean get_should_hide_internal_if_lid_closed() {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  gboolean value = g_settings_get_boolean(settings, SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_GSETTINGS_KEY);
  g_object_unref(settings);

  return value;
}
