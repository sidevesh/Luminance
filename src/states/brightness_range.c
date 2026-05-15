#include <gtk/gtk.h>

#include "../constants/main.c"

#ifndef BRIGHTNESS_RANGE_STATE
#define BRIGHTNESS_RANGE_STATE

// --- Generic helper for per-display brightness limits (min or max) ---

static GVariant* _get_brightness_dict(const gchar *gsettings_key) {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  GVariant *dict = g_settings_get_value(settings, gsettings_key);
  g_object_unref(settings);
  return dict;
}

static void _set_brightness_dict(const gchar *gsettings_key, const gchar *display_name, gint percentage) {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  GVariant *dict = g_settings_get_value(settings, gsettings_key);

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{si}"));

  GVariantIter iter;
  gchar *key;
  gint32 val;
  g_variant_iter_init(&iter, dict);
  while (g_variant_iter_loop(&iter, "{&si}", &key, &val)) {
    if (g_strcmp0(key, display_name) != 0) {
      g_variant_builder_add(&builder, "{&si}", key, val);
    }
  }

  g_variant_builder_add(&builder, "{&si}", display_name, percentage);

  GVariant *new_dict = g_variant_builder_end(&builder);
  g_settings_set_value(settings, gsettings_key, new_dict);

  g_variant_unref(dict);
  g_object_unref(settings);
}

static gdouble _get_brightness_from_dict(GVariant *dict, const gchar *display_name, gdouble default_value) {
  gint32 int_val = 0;
  if (g_variant_lookup(dict, display_name, "i", &int_val)) {
    if (int_val >= 0 && int_val <= 100) {
      return (gdouble)int_val;
    }
  }
  return default_value;
}

// --- Max brightness ---

GVariant *_max_brightness_cache = NULL;

gdouble get_max_brightness_percentage(const gchar *display_name) {
  if (_max_brightness_cache == NULL) {
    _max_brightness_cache = _get_brightness_dict(MAX_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY);
  }
  return _get_brightness_from_dict(_max_brightness_cache, display_name, 100.0);
}

void set_max_brightness_percentage(const gchar *display_name, gint percentage) {
  if (_max_brightness_cache != NULL) {
    g_variant_unref(_max_brightness_cache);
    _max_brightness_cache = NULL;
  }
  _set_brightness_dict(MAX_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY, display_name, percentage);
  _max_brightness_cache = _get_brightness_dict(MAX_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY);
}

// --- Min brightness ---

GVariant *_min_brightness_cache = NULL;

gdouble get_min_brightness_percentage(const gchar *display_name) {
  if (_min_brightness_cache == NULL) {
    _min_brightness_cache = _get_brightness_dict(MIN_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY);
  }
  return _get_brightness_from_dict(_min_brightness_cache, display_name, 0.0);
}

void set_min_brightness_percentage(const gchar *display_name, gint percentage) {
  if (_min_brightness_cache != NULL) {
    g_variant_unref(_min_brightness_cache);
    _min_brightness_cache = NULL;
  }
  _set_brightness_dict(MIN_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY, display_name, percentage);
  _min_brightness_cache = _get_brightness_dict(MIN_BRIGHTNESS_PER_DISPLAY_GSETTINGS_KEY);
}

#endif
