#include <gtk/gtk.h>
#include "../constants/main.c"
#include "../../states/displays.c"

extern void update_window_contents_in_ui();

void _on_no_displays_screen_refresh_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
	reload_displays(update_window_contents_in_ui, update_window_contents_in_ui);
}

GtkWidget* get_no_displays_screen() {
  GtkWidget *box, *image, *title, *subtitle, *button;
	GtkCssProvider *button_css_provider;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_top(box, MARGIN_UNIT * 2.4);
	gtk_widget_set_margin_start(box, MARGIN_UNIT);
	gtk_widget_set_margin_end(box, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(box, MARGIN_UNIT);

	image = gtk_image_new_from_icon_name("video-display-symbolic");
  gtk_widget_add_css_class(image, "dim-label");
	gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
	gtk_widget_set_size_request(image, 128, 128);
	gtk_widget_set_margin_bottom(image, MARGIN_UNIT * 2);
	gtk_box_append(GTK_BOX(box), image);

	title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='heavy'>No compatible displays found</span>");
  gtk_box_append(GTK_BOX(box), title);

  subtitle = gtk_label_new("Try refreshing if you have just connected the displays");
	gtk_widget_set_margin_bottom(subtitle, MARGIN_UNIT);
  gtk_box_append(GTK_BOX(box), subtitle);

	button = gtk_button_new_with_label("Refresh");
  gtk_widget_add_css_class(button, "circular");
	button_css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(button_css_provider, "button {padding-left: 12px;padding-right: 12px;padding-top: 4px;padding-bottom: 4px;}");
  gtk_style_context_add_provider(gtk_widget_get_style_context(button), GTK_STYLE_PROVIDER(button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
  g_signal_connect(button, "clicked", G_CALLBACK(_on_no_displays_screen_refresh_button_clicked), NULL);
  gtk_box_append(GTK_BOX(box), button);

	return box;
}
