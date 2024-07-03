#ifndef OSD_OSD_H
#define OSD_OSD_H

#include <gtk/gtk.h>

#define GNOME_OSD 'g'

gboolean is_osd_provider_supported(gchar provider);
void set_osd_brightness_percentage_to_show(gdouble brightness_percentage);
void show_osd_after_brightness_change(gchar provider);

#endif // OSD_OSD_H
