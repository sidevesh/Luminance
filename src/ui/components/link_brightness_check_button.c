#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_link_brightness_checkbox(gboolean initial_value) {
	GtkWidget *link_brightness_checkbox = gtk_check_button_new_with_label("Sync brightness of all displays");

	gtk_widget_set_margin_start(link_brightness_checkbox, MARGIN_UNIT);
	gtk_widget_set_margin_end(link_brightness_checkbox, MARGIN_UNIT);
	gtk_widget_set_margin_top(link_brightness_checkbox, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(link_brightness_checkbox, MARGIN_UNIT);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(link_brightness_checkbox), initial_value);

	return link_brightness_checkbox;
}
