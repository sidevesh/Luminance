#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_separator() {
	GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_halign(separator, 0);
	gtk_widget_set_margin_start(separator, 0);

	return separator;
}
