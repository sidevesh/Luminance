#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_link_brightness_check_button() {
	GtkWidget *link_brightness_check_button = gtk_check_button_new_with_label("Sync brightness of all displays");
	gtk_widget_set_margin_start(link_brightness_check_button, MARGIN_UNIT);
	gtk_widget_set_margin_end(link_brightness_check_button, MARGIN_UNIT);
	gtk_widget_set_margin_top(link_brightness_check_button, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(link_brightness_check_button, MARGIN_UNIT);

	return link_brightness_check_button;
}
