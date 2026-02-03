#include "config.h"
#include "com-sidevesh-Luminance.h"

static LuminanceService *skeleton = NULL;

// Forward declarations from states/displays.c
guint displays_count(void);
char* get_display_name(guint index);
gdouble get_display_brightness_percentage(guint index);
void set_display_brightness_percentage(guint index, gdouble brightness_percentage, gboolean emit_osd_signal);
void load_displays(void (*)(), void (*)());

static gchar *
get_dbus_object_path (void)
{
  gchar *object_path_str = g_strdup(APP_INFO_PACKAGE_NAME);
  g_strdelimit(object_path_str, ".", '/');
  gchar *full_path = g_strdup_printf("/%s/Service", object_path_str);
  g_free(object_path_str);
  return full_path;
}

static gboolean
on_handle_get_monitors (LuminanceService *interface,
                        GDBusMethodInvocation *invocation,
                        gpointer user_data)
{
  load_displays(NULL, NULL);

  GString *json_builder = g_string_new("[");
  guint count = displays_count();

  for (guint i = 0; i < count; i++) {
    if (i > 0) g_string_append(json_builder, ",");

    char *name = get_display_name(i);
    // Escape the name for JSON
    gchar *escaped_name = g_strescape(name, "");
    gdouble brightness = get_display_brightness_percentage(i);

    g_string_append_printf(json_builder,
                           "{\"id\": \"%u\", \"name\": \"%s\", \"brightness\": %.2f}",
                           i, escaped_name ? escaped_name : "Unknown", brightness);

    if (escaped_name) g_free(escaped_name);
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
  } else {
      g_warning("Monitor ID out of range: %s", monitor_id);
  }

  luminance_service_complete_set_brightness (interface, invocation);
  return TRUE;
}

static gboolean
on_handle_quit (LuminanceService *interface,
                GDBusMethodInvocation *invocation,
                gpointer user_data)
{
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

void emit_osd_signal_dbus(gdouble percentage, const char* monitor) {
    if (skeleton != NULL) {
        luminance_service_emit_show_osd(skeleton, percentage, monitor ? monitor : "");
    } else {
        // CLI mode: emit signal directly
        GError *error = NULL;
        GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
        if (conn) {
             gchar *object_path = get_dbus_object_path();
             g_dbus_connection_emit_signal(conn,
                                           NULL, 
                                           object_path,
                                           "com.sidevesh.Luminance",
                                           "ShowOSD",
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
}
