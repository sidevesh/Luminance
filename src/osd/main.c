#include <gtk/gtk.h>
#include "./providers/experimental/gnome.c"

#ifndef OSD
#define OSD

#define GNOME_OSD 'g'

gdouble _osd_brightness_percentage_to_show = -1;

gboolean is_osd_provider_supported(gchar provider) {
  if (provider == GNOME_OSD) {
    return TRUE;
  }

  return FALSE;
}

void set_osd_brightness_percentage_to_show(gdouble brightness_percentage) {
  _osd_brightness_percentage_to_show = brightness_percentage;
}

void show_osd_after_brightness_change(gchar provider) {
	if (_osd_brightness_percentage_to_show == -1) {
		return;
	}

  if (provider == GNOME_OSD) {
    show_gnome_osd(_osd_brightness_percentage_to_show);
  }

  _osd_brightness_percentage_to_show = -1;
}

#endif
