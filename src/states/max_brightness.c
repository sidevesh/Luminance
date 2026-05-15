#include <gtk/gtk.h>

#include "../constants/main.c"

#ifndef MAX_BRIGHTNESS_STATE
#define MAX_BRIGHTNESS_STATE

// Cache the GVariant dict so we don't re-read on every slider movement
GVariant *_max_brightness_cache = NULL;

static GVariant* _get_max_brightness_dict() {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  GVariant *dict = g_settings_get_value(settings, MAX_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY);
  g_object_unref(settings);
  return dict;
}

static void _invalidate_cache() {
  if (_max_brightness_cache != NULL) {
    g_variant_unref(_max_brightness_cache);
    _max_brightness_cache = NULL;
  }
}

gdouble get_max_brightness_percentage(const gchar *display_name) {
  if (_max_brightness_cache == NULL) {
    _max_brightness_cache = _get_max_brightness_dict();
  }

  gdouble value = 100.0;

  gint32 int_val = 0;
  if (g_variant_lookup(_max_brightness_cache, display_name, "i", &int_val)) {
    if (int_val > 0 && int_val <= 100) {
      value = (gdouble)int_val;
    }
  }

  return value;
}

void set_max_brightness_percentage(const gchar *display_name, gint percentage) {
  _invalidate_cache();

  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  GVariant *dict = g_settings_get_value(settings, MAX_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY);

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{si}"));

  // Copy existing entries, skipping the one we're updating
  GVariantIter iter;
  gchar *key;
  gint32 val;
  g_variant_iter_init(&iter, dict);
  while (g_variant_iter_loop(&iter, "{&si}", &key, &val)) {
    if (g_strcmp0(key, display_name) != 0) {
      g_variant_builder_add(&builder, "{&si}", key, val);
    }
  }

  // Add the new/updated entry
  g_variant_builder_add(&builder, "{&si}", display_name, percentage);

  GVariant *new_dict = g_variant_builder_end(&builder);
  g_settings_set_value(settings, MAX_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY, new_dict);

  g_variant_unref(dict);
  g_object_unref(settings);

  _max_brightness_cache = _get_max_brightness_dict();
}

#endif
