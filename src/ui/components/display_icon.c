#include <gtk/gtk.h>

#include "ui/constants.h"
#include "ui/components.h"

GtkWidget* get_display_icon() {
	GtkWidget *icon = gtk_image_new_from_icon_name("video-display-symbolic");

	gtk_image_set_pixel_size(GTK_IMAGE(icon), 56);
	gtk_widget_set_margin_start(icon, MARGIN_UNIT);

	return icon;
}
