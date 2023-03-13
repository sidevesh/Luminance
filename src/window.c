#include <gtk/gtk.h>

#ifndef BRIGHTNESS_CODE
#include "../ddcbc-api/ddcbc-api.c"
#endif

#include "./states/display_list.c"

#define MINIMUM_WINDOW_WIDTH 454
#define MINIMUM_WINDOW_HEIGHT 474

GtkWidget *_window;
GtkWidget *_window_header;

GtkWidget* _last_window_content_screen = NULL;

void initialize_application_window(GtkApplication *app) {
	_window = gtk_application_window_new(app);

	gtk_window_set_resizable(GTK_WINDOW(_window), FALSE);
	gtk_window_set_geometry_hints(GTK_WINDOW(_window), NULL, &(GdkGeometry){.min_width = MINIMUM_WINDOW_WIDTH, .min_height = MINIMUM_WINDOW_HEIGHT}, GDK_HINT_MIN_SIZE);
	gtk_window_set_keep_above(GTK_WINDOW(_window), TRUE);
	g_signal_connect(_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  _window_header = gtk_header_bar_new();
	if (is_display_list_loading == FALSE) {
		GtkWidget *refresh_displays_button = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_header_bar_pack_start(GTK_HEADER_BAR(_window_header), refresh_displays_button);
		g_signal_connect(refresh_displays_button, "clicked", G_CALLBACK(refresh_displays), NULL);
	}
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(_window_header), TRUE);
	GtkCssProvider *css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css_provider, "headerbar {border-bottom: 0px;background-color: @theme_bg_color;}", -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(_window_header), GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_window_set_titlebar(GTK_WINDOW(_window), _window_header);
}

void update_window_content_screen(GtkWidget *new_window_content_screen) {
  if (_last_window_content_screen != NULL) {
    gtk_container_remove(GTK_CONTAINER(_window), _last_window_content_screen);
    gtk_widget_destroy(_last_window_content_screen);
  }
  gtk_container_add(GTK_CONTAINER(_window), new_window_content_screen);
  _last_window_content_screen = new_window_content_screen;

	GList *children = gtk_container_get_children(GTK_CONTAINER(_window_header));
	GtkWidget *refresh_displays_button = g_list_nth_data(children, 0);
	if (is_display_list_loading == FALSE) {
		if (refresh_displays_button == NULL) {
			refresh_displays_button = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
			gtk_header_bar_pack_start(GTK_HEADER_BAR(_window_header), refresh_displays_button);
			g_signal_connect(refresh_displays_button, "clicked", G_CALLBACK(refresh_displays), NULL);
		}
	} else {
		if (refresh_displays_button != NULL) {
			gtk_container_remove(GTK_CONTAINER(_window_header), refresh_displays_button);
			gtk_widget_destroy(refresh_displays_button);
		}
	}

	gtk_widget_show_all(_window);
}
