#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include <gtk/gtk.h>

GtkWidget * get_no_displays_screen ();
GtkWidget * get_refreshing_displays_screen ();
GtkWidget * get_show_displays_screen ();
void _link_brightness (GtkCheckButton * link_brightness_checkbox);
void _on_no_displays_screen_refresh_button_clicked (GtkWidget * widget,gpointer data);
void _update_display_brightness (GtkRange * range,guint data);

#endif // UI_SCREENS_H
