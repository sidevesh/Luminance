#include <gtk/gtk.h>

#include "ui/constants.h"
#include "ui/components.h"


static char* format_brightness_value(GtkScale *scale, gdouble value, gpointer data) {
	return g_strdup_printf(" %d%%", (int)value);
}

GtkWidget* get_display_brightness_scale(gdouble last_value, gdouble max_value) {
	GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, max_value, 1);

	gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
	gtk_scale_set_format_value_func(GTK_SCALE(scale), format_brightness_value, NULL, NULL);
	gtk_widget_set_halign(scale, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(scale, TRUE);
	gtk_widget_set_margin_start(scale, MARGIN_UNIT / 4);
	gtk_widget_set_margin_end(scale, MARGIN_UNIT / 2);
	gtk_widget_set_margin_bottom(scale, MARGIN_UNIT);
	gtk_range_set_value(GTK_RANGE(scale), last_value);

	return scale;
}
