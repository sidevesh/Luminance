#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_display_label(gchar *text) {
	GtkWidget *label = gtk_label_new(text);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_set_hexpand(label, FALSE);
	gtk_widget_set_valign(label, GTK_ALIGN_START);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_margin_start(label, MARGIN_UNIT);
	gtk_widget_set_margin_end(label, MARGIN_UNIT);
	gtk_widget_set_margin_top(label, MARGIN_UNIT);

	return label;
}
