#include <gtk/gtk.h>

#ifndef LAPTOP_LID_STATE
#define LAPTOP_LID_STATE

static GDBusProxy *luminance_upower_proxy = NULL;

static void ensure_upower_proxy() {
  if (luminance_upower_proxy) return;

  GError *error = NULL;
  luminance_upower_proxy = g_dbus_proxy_new_for_bus_sync(
      G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      "org.freedesktop.UPower",
      "/org/freedesktop/UPower",
      "org.freedesktop.UPower",
      NULL,
      &error
  );

  if (error) {
    g_warning("Failed to connect to UPower: %s", error->message);
    g_error_free(error);
    luminance_upower_proxy = NULL;
  }
}

gboolean get_has_lid() {
  ensure_upower_proxy();
  if (luminance_upower_proxy == NULL) {
    return FALSE;
  }
  
  GVariant *v = g_dbus_proxy_get_cached_property(luminance_upower_proxy, "LidIsPresent");
  if (v == NULL) {
    // If we can't get the property, we might assume no lid or try later.
    // However, if proxied, it should be there. 
    return FALSE;
  }
  
  gboolean present = g_variant_get_boolean(v);
  g_variant_unref(v);
  return present;
}

gboolean is_lid_open() {
  ensure_upower_proxy();
  if (luminance_upower_proxy == NULL) {
    return TRUE;
  }

  GVariant *v = g_dbus_proxy_get_cached_property(luminance_upower_proxy, "LidIsClosed");
  if (v == NULL) {
      return TRUE;
  }

  gboolean is_closed = g_variant_get_boolean(v);
  g_variant_unref(v);
  
  return !is_closed;
}

static void (*on_lid_state_changed_cb)() = NULL;

static void on_upower_properties_changed (GDBusProxy *proxy,
                                          GVariant   *changed_properties,
                                          const gchar *const *invalidated_properties,
                                          gpointer    user_data)
{
    (void)proxy;
    (void)invalidated_properties;
    (void)user_data;
    if (g_variant_lookup (changed_properties, "LidIsClosed", "b", NULL)) {
        g_print("UPower signal received: LidIsClosed changed.\n");
        if (on_lid_state_changed_cb) {
            on_lid_state_changed_cb();
        }
    }
}

void register_lid_state_observer(void (*callback)()) {
    ensure_upower_proxy();
    on_lid_state_changed_cb = callback;
    if (luminance_upower_proxy) {
         g_signal_connect (luminance_upower_proxy,
                           "g-properties-changed",
                           G_CALLBACK (on_upower_properties_changed),
                           NULL);
    }
}

#endif
