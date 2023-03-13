#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_display_brightness_scale(guint16 last_value, guint16 max_value) {
	GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, max_value, 1);
	gtk_range_set_value(GTK_RANGE(scale), last_value);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
	gtk_widget_set_hexpand(scale, FALSE);
	gtk_widget_set_valign(scale, GTK_ALIGN_START);
	gtk_widget_set_halign(scale, 0.0);
	gtk_widget_set_margin_start(scale, MARGIN_UNIT / 4);
	gtk_widget_set_margin_end(scale, MARGIN_UNIT / 2);
	gtk_widget_set_margin_bottom(scale, MARGIN_UNIT);

	return scale;
}
