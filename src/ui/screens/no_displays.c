#include <gtk/gtk.h>
#include "../constants/main.c"
#include "../../states/displays.c"

extern void update_window_contents_in_ui();

void _on_no_displays_screen_refresh_button_clicked(GtkWidget *widget, gpointer data) {
	reload_displays(update_window_contents_in_ui, update_window_contents_in_ui);
}

GtkWidget* get_no_displays_screen() {
  GtkWidget *box, *image, *title, *subtitle, *button;
	GtkStyleContext *image_style_context, *button_style_context;
	GtkCssProvider *button_css_provider;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_set_spacing(GTK_BOX(box), 0);
  gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top(box, MARGIN_UNIT * 2.4);
	gtk_widget_set_margin_start(box, MARGIN_UNIT);
	gtk_widget_set_margin_end(box, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(box, MARGIN_UNIT);

  image = gtk_image_new_from_icon_name("video-display-symbolic", GTK_ICON_SIZE_DIALOG);
	image_style_context = gtk_widget_get_style_context(image);
	gtk_style_context_add_class(image_style_context, "dim-label");
	gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
  gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(image, MARGIN_UNIT * 2);
  gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

	title = gtk_label_new(NULL);
	gtk_label_set_markup (GTK_LABEL(title), "<span size='xx-large' weight='heavy'>No compatible displays found</span>");
  gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(title, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 5);

  subtitle = gtk_label_new("Try refreshing if you have just connected the displays");
  gtk_widget_set_halign(subtitle, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(subtitle, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(subtitle, MARGIN_UNIT);
  gtk_box_pack_start(GTK_BOX(box), subtitle, FALSE, FALSE, 5);

	button = gtk_button_new_with_label("Refresh");
	button_style_context = gtk_widget_get_style_context(button);
	gtk_style_context_add_class(button_style_context, "circular");
	button_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(button_css_provider, "button {padding: 10px;}", -1, NULL);
	gtk_style_context_add_provider(button_style_context, GTK_STYLE_PROVIDER(button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 5);

	g_signal_connect(button, "clicked", G_CALLBACK(_on_no_displays_screen_refresh_button_clicked), NULL);

	return box;
}
