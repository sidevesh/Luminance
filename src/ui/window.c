#include <gtk/gtk.h>
#include "../constants/main.c"
#include "./constants/main.c"
#include "../states/displays.c"
#include "../states/laptop_lid.c"
#include "../states/should_hide_internal_if_lid_closed.c"

GtkWidget *_window;
GtkWidget *_window_header;

GtkWidget* _last_window_content_screen = NULL;

GtkWidget *_refresh_displays_button = NULL;

extern GtkApplication *app;

extern void update_window_contents_in_ui();

void _on_window_refresh_button_clicked(GtkWidget *widget, gpointer data) {
	reload_displays(update_window_contents_in_ui, update_window_contents_in_ui);
}

void _on_should_hide_internal_if_lid_closed_checkbox_toggled(GtkCheckButton *widget, gpointer data) {
	gboolean should_hide_internal_if_lid_closed = gtk_check_button_get_active(widget);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), should_hide_internal_if_lid_closed);
	set_should_hide_internal_if_lid_closed(should_hide_internal_if_lid_closed);
}

void _open_about_dialog() {
	GtkWidget *about_dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_DISPLAY_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_VERSION_NUMBER);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_DESCRIPTION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_SOURCE_REPOSITORY_TITLE);
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_SOURCE_REPOSITORY_LINK);
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_LICENSE);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), APP_INFO_ICON_NAME);
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), (const gchar *[]){
		APP_INFO_PROJECT_AUTHORS,
		NULL
	});
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about_dialog), (const gchar *[]){
		APP_INFO_PROJECT_ARTISTS,
		NULL
	});
	gtk_widget_show(about_dialog);
	g_signal_connect(about_dialog, "response", G_CALLBACK(gtk_window_destroy), about_dialog);
}

void initialize_application_window(GtkApplication *app) {
	_window = gtk_application_window_new(app);

	gtk_window_set_resizable(GTK_WINDOW(_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(_window), MINIMUM_WINDOW_WIDTH, -1);  // Set the minimum width

	_window_header = gtk_header_bar_new();
	gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(_window_header), TRUE);
	gtk_header_bar_set_title_widget(GTK_HEADER_BAR(_window_header), gtk_label_new(""));

	GtkWidget *menu_button = gtk_menu_button_new();
	GtkWidget *menu_image = gtk_image_new_from_icon_name("open-menu-symbolic");
	gtk_button_set_child(GTK_BUTTON(menu_button), menu_image);

	GtkWidget *menu_popover = gtk_popover_new();
	GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkCssProvider *menu_box_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(menu_box_css_provider, "box {padding: 5px;}");
	gtk_style_context_add_provider(gtk_widget_get_style_context(menu_box), GTK_STYLE_PROVIDER(menu_box_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	GtkCssProvider *menu_button_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(menu_button_css_provider, "button.flat {min-width: 150px;font-weight: normal;}");
	gtk_style_context_add_provider(gtk_widget_get_style_context(menu_box), GTK_STYLE_PROVIDER(menu_button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	if (get_has_lid()) {
		GtkWidget *should_hide_internal_if_lid_closed_checkbox = gtk_check_button_new_with_label("Hide built-in displays when lid is closed");
		gtk_check_button_set_active(GTK_CHECK_BUTTON(should_hide_internal_if_lid_closed_checkbox), get_should_hide_internal_if_lid_closed());

		GtkStyleContext *should_hide_internal_if_lid_closed_checkbox_style_context = gtk_widget_get_style_context(should_hide_internal_if_lid_closed_checkbox);
		gtk_style_context_add_class(should_hide_internal_if_lid_closed_checkbox_style_context, "flat");
		gtk_style_context_add_provider(gtk_widget_get_style_context(should_hide_internal_if_lid_closed_checkbox), GTK_STYLE_PROVIDER(menu_button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

		GtkCssProvider *should_hide_internal_if_lid_closed_checkbox_css_provider = gtk_css_provider_new();
		gtk_css_provider_load_from_string(should_hide_internal_if_lid_closed_checkbox_css_provider, "checkbutton.flat {padding: 5px;}");
		gtk_style_context_add_provider(gtk_widget_get_style_context(should_hide_internal_if_lid_closed_checkbox), GTK_STYLE_PROVIDER(should_hide_internal_if_lid_closed_checkbox_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

		GtkWidget *should_hide_internal_if_lid_closed_checkbox_label = gtk_label_new(NULL);
		gtk_label_set_xalign(GTK_LABEL(should_hide_internal_if_lid_closed_checkbox_label), 0.0);
		gtk_widget_set_parent(should_hide_internal_if_lid_closed_checkbox_label, should_hide_internal_if_lid_closed_checkbox);
		g_signal_connect(should_hide_internal_if_lid_closed_checkbox, "toggled", G_CALLBACK(_on_should_hide_internal_if_lid_closed_checkbox_toggled), NULL);
		gtk_box_append(GTK_BOX(menu_box), should_hide_internal_if_lid_closed_checkbox);

		GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_box_append(GTK_BOX(menu_box), separator);
	}

	gchar about_button_label_text[100];
	sprintf(about_button_label_text, "About %s", APP_INFO_DISPLAY_NAME);
	GtkWidget *about_button = gtk_button_new_with_label(about_button_label_text);

	GtkStyleContext *about_button_style_context = gtk_widget_get_style_context(about_button);
	gtk_style_context_add_class(about_button_style_context, "flat");
	gtk_style_context_add_provider(gtk_widget_get_style_context(about_button), GTK_STYLE_PROVIDER(menu_button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	GtkWidget *about_button_label = gtk_label_new(NULL);
	gtk_label_set_xalign(GTK_LABEL(about_button_label), 0.0);
	gtk_widget_set_parent(about_button_label, about_button);
	g_signal_connect(about_button, "clicked", G_CALLBACK(_open_about_dialog), NULL);
	gtk_box_append(GTK_BOX(menu_box), about_button);

	gtk_popover_set_child(GTK_POPOVER(menu_popover), menu_box);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), menu_popover);
	gtk_widget_show(menu_box);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(_window_header), menu_button);

	if (is_displays_loading() == FALSE) {
		_refresh_displays_button = gtk_button_new_from_icon_name("view-refresh-symbolic");
		gtk_header_bar_pack_start(GTK_HEADER_BAR(_window_header), _refresh_displays_button);
		g_signal_connect(_refresh_displays_button, "clicked", G_CALLBACK(_on_window_refresh_button_clicked), NULL);
	}

	GtkCssProvider *header_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(header_css_provider, "headerbar {border-bottom: 0px;background-image: none;background-color: @theme_bg_color;box-shadow: none;}");
	gtk_style_context_add_provider(gtk_widget_get_style_context(_window_header), GTK_STYLE_PROVIDER(header_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_window_set_titlebar(GTK_WINDOW(_window), _window_header);
}

void update_window_content_screen(GtkWidget *new_window_content_screen) {
	if (_last_window_content_screen != NULL) {
		gtk_widget_unparent(_last_window_content_screen);
		// gtk_widget_destroy(_last_window_content_screen);
		g_object_unref(_last_window_content_screen);
	}
	gtk_window_set_child(GTK_WINDOW(_window), new_window_content_screen);
	// gtk_window_resize(GTK_WINDOW(_window), gtk_widget_get_allocated_width(_window), gtk_widget_get_allocated_height(new_window_content_screen));
	gtk_window_set_default_size(GTK_WINDOW(_window), gtk_widget_get_allocated_width(_window), gtk_widget_get_allocated_height(new_window_content_screen));
	_last_window_content_screen = new_window_content_screen;

	if (is_displays_loading() == FALSE) {
		if (_refresh_displays_button == NULL) {
			_refresh_displays_button = gtk_button_new_from_icon_name("view-refresh-symbolic");
			gtk_header_bar_pack_start(GTK_HEADER_BAR(_window_header), _refresh_displays_button);
			g_signal_connect(_refresh_displays_button, "clicked", G_CALLBACK(_on_window_refresh_button_clicked), NULL);
		}
	} else {
		if (_refresh_displays_button != NULL) {
			gtk_header_bar_remove(GTK_HEADER_BAR(_window_header), _refresh_displays_button);
			// gtk_widget_unparent(_refresh_displays_button);
			// gtk_widget_destroy(_refresh_displays_button);
			g_object_unref(_refresh_displays_button);
			_refresh_displays_button = NULL;
		}
	}

	gtk_widget_show(_window);
}
