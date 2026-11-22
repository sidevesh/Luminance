#include "config.h"
#include "com-sidevesh-Luminance.h"

static LuminanceService *skeleton = NULL;

// Forward declarations from states/displays.c
guint displays_count(void);
char* get_display_name(guint index);
gdouble get_display_brightness_percentage(guint index);
void set_display_brightness_percentage(guint index, gdouble brightness_percentage, gboolean emit_osd_signal);
gboolean get_display_has_contrast(guint index);
gboolean get_display_is_internal(guint index);
gboolean get_display_edid_identity(guint index, const char **vendor, const char **model, const char **serial);
gdouble get_display_contrast_percentage(guint index);
void set_display_contrast_percentage(guint index, gdouble contrast_percentage, gboolean emit_osd_signal);
void load_displays(void (*)(), void (*)());
void refresh_display_values(void);
void notify_display_values_changed(void);
void rescan_displays(void);

static gchar *
get_dbus_object_path (void)
{
  gchar *object_path_str = g_strdup(APP_INFO_PACKAGE_NAME);
  g_strdelimit(object_path_str, ".", '/');
  gchar *full_path = g_strdup_printf("/%s/Service", object_path_str);
  g_free(object_path_str);
  return full_path;
}

// Restart the GApplication inactivity timer. D-Bus method calls on our own
// skeleton do not count as application use, so without this a D-Bus activated
// service quits on schedule even while a client keeps calling it.
static void
mark_service_busy (void)
{
  GApplication *app = g_application_get_default();
  if (app == NULL) return;
  g_application_hold(app);
  g_application_release(app);
}

// Append `value` as a JSON string literal (quoted, escaped). UTF-8 passes
// through unchanged; only quotes, backslashes and control characters are escaped.
static void
append_json_string (GString *out, const char *value)
{
  g_string_append_c(out, '"');
  for (const char *p = value ? value : ""; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (c == '"' || c == '\\')
      g_string_append_printf(out, "\\%c", c);
    else if (c < 0x20)
      g_string_append_printf(out, "\\u%04x", c);
    else
      g_string_append_c(out, c);
  }
  g_string_append_c(out, '"');
}

static void
append_json_string_field (GString *out, const char *key, const char *value)
{
  g_string_append_printf(out, ", \"%s\": ", key);
  append_json_string(out, value);
}

static gboolean
on_handle_get_monitors (LuminanceService *interface,
                        GDBusMethodInvocation *invocation,
                        gpointer user_data)
{
  (void)user_data;
  mark_service_busy();
  // Enumerate only once; afterwards just re-read values. Re-enumerating on
  // every call is slow and disturbs the open window's display list.
  if (displays_count() == 0)
    load_displays(NULL, NULL);
  else
    refresh_display_values();

  GString *json_builder = g_string_new("[");
  guint count = displays_count();

  for (guint i = 0; i < count; i++) {
    if (i > 0) g_string_append(json_builder, ",");

    gdouble brightness = get_display_brightness_percentage(i);
    gboolean has_contrast = get_display_has_contrast(i);

    g_string_append_printf(json_builder, "{\"id\": \"%u\"", i);
    append_json_string_field(json_builder, "name", get_display_name(i));
    g_string_append_printf(json_builder, ", \"brightness\": %.2f", brightness);
    append_json_string_field(json_builder, "type", get_display_is_internal(i) ? "internal" : "ddc");

    if (has_contrast)
      g_string_append_printf(json_builder, ", \"has_contrast\": true, \"contrast\": %.2f",
                             get_display_contrast_percentage(i));
    else
      g_string_append(json_builder, ", \"has_contrast\": false");

    // EDID identity lets the GNOME extension match this display to a Mutter
    // monitor (connector), e.g. to act on the monitor under the pointer.
    const char *vendor, *model, *serial;
    if (get_display_edid_identity(i, &vendor, &model, &serial)) {
      append_json_string_field(json_builder, "vendor", vendor);
      append_json_string_field(json_builder, "model", model);
      append_json_string_field(json_builder, "serial", serial);
    }

    g_string_append(json_builder, "}");
  }
  g_string_append(json_builder, "]");

  luminance_service_complete_get_monitors (interface, invocation, json_builder->str);

  g_string_free(json_builder, TRUE);
  return TRUE;
}

static gboolean
on_handle_set_brightness (LuminanceService *interface,
                          GDBusMethodInvocation *invocation,
                          const gchar *monitor_id,
                          gdouble value,
                          gpointer user_data)
{
  (void)user_data;
  mark_service_busy();
  gchar *endptr;
  guint64 index_val = g_ascii_strtoull(monitor_id, &endptr, 10);

  if (*endptr != '\0') {
      g_warning("Invalid monitor ID format: %s", monitor_id);
      luminance_service_complete_set_brightness (interface, invocation);
      return TRUE;
  }

  if (displays_count() == 0) {
      load_displays(NULL, NULL);
  }

  if (index_val < displays_count()) {
      g_print("DBus Request: Set brightness for %s (index %lu) to %f\n", monitor_id, index_val, value);
      // Emit signal is set to TRUE as requested
      set_display_brightness_percentage((guint)index_val, value, TRUE);
      notify_display_values_changed();
  } else {
      g_warning("Monitor ID out of range: %s", monitor_id);
  }

  luminance_service_complete_set_brightness (interface, invocation);
  return TRUE;
}

static gboolean
on_handle_set_brightness_quiet (LuminanceService *interface,
                                GDBusMethodInvocation *invocation,
                                const gchar *monitor_id,
                                gdouble value,
                                gpointer user_data)
{
  (void)user_data;
  mark_service_busy();
  gchar *endptr;
  guint64 index_val = g_ascii_strtoull(monitor_id, &endptr, 10);

  if (*endptr != '\0') {
      g_warning("Invalid monitor ID format: %s", monitor_id);
      luminance_service_complete_set_brightness_quiet (interface, invocation);
      return TRUE;
  }

  if (displays_count() == 0) {
      load_displays(NULL, NULL);
  }

  if (index_val < displays_count()) {
      g_print("DBus Request: Set brightness (quiet) for %s (index %lu) to %f\n", monitor_id, index_val, value);
      set_display_brightness_percentage((guint)index_val, value, FALSE);
      notify_display_values_changed();
  } else {
      g_warning("Monitor ID out of range: %s", monitor_id);
  }

  luminance_service_complete_set_brightness_quiet (interface, invocation);
  return TRUE;
}

static gboolean
on_handle_set_contrast (LuminanceService *interface,
                        GDBusMethodInvocation *invocation,
                        const gchar *monitor_id,
                        gdouble value,
                        gpointer user_data)
{
  (void)user_data;
  mark_service_busy();
  gchar *endptr;
  guint64 index_val = g_ascii_strtoull(monitor_id, &endptr, 10);

  if (*endptr != '\0') {
      g_warning("Invalid monitor ID format: %s", monitor_id);
      luminance_service_complete_set_contrast (interface, invocation);
      return TRUE;
  }

  if (displays_count() == 0) {
      load_displays(NULL, NULL);
  }

  if (index_val < displays_count()) {
      if (get_display_has_contrast((guint)index_val)) {
          g_print("DBus Request: Set contrast for %s (index %lu) to %f\n", monitor_id, index_val, value);
          set_display_contrast_percentage((guint)index_val, value, TRUE);
          notify_display_values_changed();
      } else {
          g_warning("Monitor %s does not support contrast control", monitor_id);
      }
  } else {
      g_warning("Monitor ID out of range: %s", monitor_id);
  }

  luminance_service_complete_set_contrast (interface, invocation);
  return TRUE;
}

static gboolean
on_handle_rescan (LuminanceService *interface,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
  (void)user_data;
  mark_service_busy();
  g_print("DBus Request: Rescan displays\n");
  rescan_displays();
  luminance_service_complete_rescan (interface, invocation);
  return TRUE;
}

static gboolean
on_handle_quit (LuminanceService *interface,
                GDBusMethodInvocation *invocation,
                gpointer user_data)
{
  (void)user_data;
  luminance_service_complete_quit (interface, invocation);
  GApplication *app = g_application_get_default();
  if (app) {
    g_application_quit(app);
  }
  return TRUE;
}

void setup_dbus_service (GDBusConnection *connection)
{
  GError *error = NULL;

  if (skeleton != NULL) return; // Already setup

  skeleton = luminance_service_skeleton_new ();

  g_signal_connect (skeleton,
                    "handle-get-monitors",
                    G_CALLBACK (on_handle_get_monitors),
                    NULL);
  g_signal_connect (skeleton,
                    "handle-set-brightness",
                    G_CALLBACK (on_handle_set_brightness),
                    NULL);
  g_signal_connect (skeleton,
                    "handle-set-brightness-quiet",
                    G_CALLBACK (on_handle_set_brightness_quiet),
                    NULL);
  g_signal_connect (skeleton,
                    "handle-set-contrast",
                    G_CALLBACK (on_handle_set_contrast),
                    NULL);
  g_signal_connect (skeleton,
                    "handle-rescan",
                    G_CALLBACK (on_handle_rescan),
                    NULL);
  g_signal_connect (skeleton,
                    "handle-quit",
                    G_CALLBACK (on_handle_quit),
                    NULL);

  gchar *object_path = get_dbus_object_path();
  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
                                         connection,
                                         object_path,
                                         &error))
    {
      g_warning ("Error exporting DBus interface: %s", error->message);
      g_error_free (error);
    } else {
      g_print("DBus interface exported at %s\n", object_path);
    }
  g_free(object_path);
}

static void _emit_named_signal_dbus(const char* signal_name, gdouble percentage, const char* monitor) {
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (conn) {
         gchar *object_path = get_dbus_object_path();
         g_dbus_connection_emit_signal(conn,
                                       NULL,
                                       object_path,
                                       APP_INFO_PACKAGE_NAME,
                                       signal_name,
                                       g_variant_new("(ds)", percentage, monitor ? monitor : ""),
                                       &error);
         g_free(object_path);
         if (error) {
             g_warning("Failed to emit signal: %s", error->message);
             g_error_free(error);
         } else {
             g_dbus_connection_flush_sync(conn, NULL, NULL);
         }
         g_object_unref(conn);
    } else {
         g_warning("Could not connect to session bus: %s", error ? error->message : "Unknown error");
         if (error) g_error_free(error);
    }
}

void emit_osd_signal_dbus(gdouble percentage, const char* monitor) {
    if (skeleton != NULL) {
        luminance_service_emit_show_osd(skeleton, percentage, monitor ? monitor : "");
    } else {
        _emit_named_signal_dbus("ShowOSD", percentage, monitor);
    }
}

void emit_contrast_osd_signal_dbus(gdouble percentage, const char* monitor) {
    if (skeleton != NULL) {
        luminance_service_emit_show_contrast_osd(skeleton, percentage, monitor ? monitor : "");
    } else {
        _emit_named_signal_dbus("ShowContrastOSD", percentage, monitor);
    }
}
