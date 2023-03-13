#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_display_icon() {
  GtkWidget *icon = gtk_image_new_from_icon_name("video-display-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(icon, MARGIN_UNIT);

  return icon;
}
