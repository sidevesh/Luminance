#include "com-sidevesh-Luminance.h"

static LuminanceService *skeleton = NULL;

static gboolean
on_handle_get_monitors (LuminanceService *interface,
                        GDBusMethodInvocation *invocation,
                        gpointer user_data)
{
  // Placeholder: In real implementation, query displays.c
  // For now return empty list to demonstrate connectivity
  gchar *json = "[]"; 
  luminance_service_complete_get_monitors (interface, invocation, json);
  return TRUE;
}

static gboolean
on_handle_set_brightness (LuminanceService *interface,
                          GDBusMethodInvocation *invocation,
                          const gchar *monitor_id,
                          gdouble value,
                          gpointer user_data)
{
  // Placeholder: Call logic to set brightness
  g_print("DBus Request: Set brightness for %s to %f\n", monitor_id, value);
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

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
                                         connection,
                                         "/com/sidevesh/Luminance",
                                         &error))
    {
      g_warning ("Error exporting DBus interface: %s", error->message);
      g_error_free (error);
    } else {
      g_print("DBus interface exported at /com/sidevesh/Luminance\n");
    }
}

void emit_osd_signal_dbus(gdouble percentage, const char* monitor) {
    if (skeleton != NULL) {
        luminance_service_emit_show_osd(skeleton, percentage, monitor ? monitor : "");
    } else {
        // CLI mode: emit signal directly
        GError *error = NULL;
        GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
        if (conn) {
             g_dbus_connection_emit_signal(conn,
                                           NULL, 
                                           "/com/sidevesh/Luminance",
                                           "com.sidevesh.Luminance",
                                           "ShowOSD",
                                           g_variant_new("(ds)", percentage, monitor ? monitor : ""),
                                           &error);
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
