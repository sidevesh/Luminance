#include <gtk/gtk.h>

#include "../constants/main.c"
#include "./constants/main.c"

#include "../states/displays.c"

GtkWidget *_window;
GtkWidget *_window_header;

GtkWidget* _last_window_content_screen = NULL;

GtkWidget *_refresh_displays_button = NULL;

extern GtkApplication *app;

extern void update_window_contents_in_ui();

void _on_window_refresh_button_clicked(GtkWidget *widget, gpointer data) {
	reload_displays(update_window_contents_in_ui, update_window_contents_in_ui);
}

void _open_about_dialog() {
	GtkWidget *about_dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_DISPLAY_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_VERSION_NUMBER);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_DESCRIPTION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_SOURCE_REPOSITORY_TITLE);
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_SOURCE_REPOSITORY_LINK);
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_LICENSE);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), "video-display");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), (const gchar *[]){
		APP_INFO_PROJECT_AUTHORS,
		NULL
	});
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about_dialog), (const gchar *[]){
		APP_INFO_PROJECT_ARTISTS,
		NULL
	});
	gtk_dialog_run(GTK_DIALOG(about_dialog));
	gtk_widget_destroy(about_dialog);
}

void initialize_application_window(GtkApplication *app) {
	_window = gtk_application_window_new(app);

	gtk_window_set_resizable(GTK_WINDOW(_window), FALSE);
	gtk_window_set_geometry_hints(GTK_WINDOW(_window), NULL, &(GdkGeometry){.min_width = MINIMUM_WINDOW_WIDTH}, GDK_HINT_MIN_SIZE);
	gtk_window_set_keep_above(GTK_WINDOW(_window), TRUE);

  _window_header = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(_window_header), TRUE);

	GtkWidget *menu_button = gtk_menu_button_new();
	GtkWidget *menu_image = gtk_image_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(menu_button), menu_image);

	GtkWidget *menu_popover = gtk_popover_new(menu_button);
	GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkCssProvider *menu_box_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(menu_box_css_provider, "box {padding: 5px;}", -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(menu_box), GTK_STYLE_PROVIDER(menu_box_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	GtkCssProvider *menu_button_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(menu_button_css_provider, "button.flat {min-width: 150px;font-weight: normal;}", -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(menu_box), GTK_STYLE_PROVIDER(menu_button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	GtkWidget *about_button = gtk_button_new_with_label("About Luminance");
	GtkStyleContext *about_button_style_context = gtk_widget_get_style_context(about_button);
	gtk_style_context_add_class(about_button_style_context, "flat");
	gtk_style_context_add_provider(gtk_widget_get_style_context(about_button), GTK_STYLE_PROVIDER(menu_button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	GtkWidget *about_button_label = gtk_bin_get_child(GTK_BIN(about_button));
	gtk_label_set_xalign(GTK_LABEL(about_button_label), 0.0);
	g_signal_connect(about_button, "clicked", G_CALLBACK(_open_about_dialog), NULL);
	gtk_box_pack_start(GTK_BOX(menu_box), about_button, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(menu_popover), menu_box);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), menu_popover);
	gtk_widget_show_all(menu_box);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(_window_header), menu_button);

	if (is_displays_loading() == FALSE) {
		_refresh_displays_button = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_header_bar_pack_start(GTK_HEADER_BAR(_window_header), _refresh_displays_button);
		g_signal_connect(_refresh_displays_button, "clicked", G_CALLBACK(_on_window_refresh_button_clicked), NULL);
	}

	GtkCssProvider *header_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(header_css_provider, "headerbar {border-bottom: 0px;background-color: @theme_bg_color;}", -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(_window_header), GTK_STYLE_PROVIDER(header_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_window_set_titlebar(GTK_WINDOW(_window), _window_header);
}

void update_window_content_screen(GtkWidget *new_window_content_screen) {
  if (_last_window_content_screen != NULL) {
    gtk_container_remove(GTK_CONTAINER(_window), _last_window_content_screen);
    gtk_widget_destroy(_last_window_content_screen);
  }
  gtk_container_add(GTK_CONTAINER(_window), new_window_content_screen);
	gtk_window_resize(GTK_WINDOW(_window), gtk_widget_get_allocated_width(_window), gtk_widget_get_allocated_height(new_window_content_screen));
  _last_window_content_screen = new_window_content_screen;

	GtkWidget *_refresh_displays_button = NULL;
	GList *children = gtk_container_get_children(GTK_CONTAINER(_window_header));
	if (g_list_length(children) > 1) {
		_refresh_displays_button = g_list_nth_data(children, 0);
	}
	if (is_displays_loading() == FALSE) {
		if (_refresh_displays_button == NULL) {
			_refresh_displays_button = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
			gtk_header_bar_pack_start(GTK_HEADER_BAR(_window_header), _refresh_displays_button);
			g_signal_connect(_refresh_displays_button, "clicked", G_CALLBACK(_on_window_refresh_button_clicked), NULL);
		}
	} else {
		if (_refresh_displays_button != NULL) {
			gtk_container_remove(GTK_CONTAINER(_window_header), _refresh_displays_button);
			gtk_widget_destroy(_refresh_displays_button);
		}
	}

	gtk_widget_show_all(_window);
}
