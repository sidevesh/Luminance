#include <gtk/gtk.h>
#include "../constants/main.c"

GtkWidget* get_refreshing_displays_screen() {
  GtkWidget *box, *image, *title, *spinner;
	GtkStyleContext *image_style_context;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_set_spacing(GTK_BOX(box), 0);
  gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top(box, MARGIN_UNIT * 2.4);
	gtk_widget_set_margin_start(box, MARGIN_UNIT);
	gtk_widget_set_margin_end(box, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(box, MARGIN_UNIT);

  // image = gtk_image_new_from_icon_name("system-search-symbolic", GTK_ICON_SIZE_DIALOG);
	image = gtk_image_new_from_icon_name("system-search-symbolic");

	image_style_context = gtk_widget_get_style_context(image);
	gtk_style_context_add_class(image_style_context, "dim-label");
	gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
  gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(image, MARGIN_UNIT * 2);
  // gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
	gtk_box_append(GTK_BOX(box), image);

	title = gtk_label_new(NULL);
	gtk_label_set_markup (GTK_LABEL(title), "<span size='xx-large' weight='heavy'>Refreshing displays...</span>");
  gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(title, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(title, MARGIN_UNIT);
	// gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 5);
	gtk_box_append(GTK_BOX(box), title);

	spinner = gtk_spinner_new();
	gtk_widget_set_size_request(spinner, 32, 32);
	gtk_widget_set_halign(spinner, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(spinner, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(spinner, MARGIN_UNIT * 2.7);
	// gtk_box_pack_start(GTK_BOX(box), spinner, FALSE, FALSE, 5);
	gtk_box_append(GTK_BOX(box), spinner);
	gtk_spinner_start(GTK_SPINNER(spinner));

	return box;
}
