#include <gtk/gtk.h>

#include "../constants/main.c"

#ifndef DISPLAY_NAMES_STATE
#define DISPLAY_NAMES_STATE

gchar* get_custom_display_name(const gchar *display_name) {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  GVariant *dict = g_settings_get_value(settings, DISPLAY_NAMES_GSETTINGS_KEY);
  g_object_unref(settings);

  gchar *custom_name = NULL;
  if (g_variant_lookup(dict, display_name, "s", &custom_name)) {
    if (strlen(custom_name) == 0) {
      g_free(custom_name);
      custom_name = NULL;
    }
  }

  g_variant_unref(dict);
  return custom_name; // Caller must free with g_free, or NULL
}

void set_custom_display_name(const gchar *display_name, const gchar *custom_name) {
  GSettings *settings = g_settings_new(APP_INFO_PACKAGE_NAME);
  GVariant *dict = g_settings_get_value(settings, DISPLAY_NAMES_GSETTINGS_KEY);

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));

  GVariantIter iter;
  gchar *key;
  gchar *val;
  g_variant_iter_init(&iter, dict);
  while (g_variant_iter_loop(&iter, "{&ss}", &key, &val)) {
    if (g_strcmp0(key, display_name) != 0) {
      g_variant_builder_add(&builder, "{&ss}", key, val);
    }
  }

  // Only add if custom_name is non-empty
  if (custom_name != NULL && strlen(custom_name) > 0) {
    g_variant_builder_add(&builder, "{&ss}", display_name, custom_name);
  }

  GVariant *new_dict = g_variant_builder_end(&builder);
  g_settings_set_value(settings, DISPLAY_NAMES_GSETTINGS_KEY, new_dict);

  g_variant_unref(dict);
  g_object_unref(settings);
}

#endif
