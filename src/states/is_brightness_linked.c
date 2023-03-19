#include <gtk/gtk.h>


#ifndef IS_BRIGHTNESS_LINKED_STATE
#define IS_BRIGHTNESS_LINKED_STATE

#define GSETTINGS_SCHEMA_ID "com.sidevesh.Luminance"
#define GSETTINGS_KEY "is-brightness-linked"

void set_is_brightness_linked(gboolean value) {
  GSettings *settings = g_settings_new(GSETTINGS_SCHEMA_ID);
  g_settings_set_boolean(settings, GSETTINGS_KEY, value);
  g_object_unref(settings);
}

gboolean get_is_brightness_linked() {
  GSettings *settings = g_settings_new(GSETTINGS_SCHEMA_ID);
  gboolean value = g_settings_get_boolean(settings, GSETTINGS_KEY);
  g_object_unref(settings);

  return value;
}

#endif
