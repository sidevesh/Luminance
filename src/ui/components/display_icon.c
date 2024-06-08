#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_display_icon() {
	GtkWidget *icon = gtk_image_new_from_icon_name("video-display-symbolic");
	gtk_image_set_pixel_size(GTK_IMAGE(icon), 48);
	gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_start(icon, MARGIN_UNIT);

	return icon;
}
