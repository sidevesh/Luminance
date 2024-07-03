#include <gtk/gtk.h>
#include <adwaita.h>

#include "constants.h"
#include "ui/constants.h"
#include "ui/window.h"

#include "states/displays.h"
#include "states/lid.h"

GtkWidget *_window;
GtkWidget *_window_header;
GtkWidget *_menu_popover;
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

void _open_about_dialog_and_close_popover() {
	AdwDialog *about_dialog = adw_about_dialog_new();

	gtk_popover_popdown(GTK_POPOVER(_menu_popover));
	adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_DISPLAY_NAME);
	adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_VERSION_NUMBER);
	adw_about_dialog_set_comments(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_DESCRIPTION);
	adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_SOURCE_REPOSITORY_TITLE);
	adw_about_dialog_set_issue_url(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_SOURCE_REPOSITORY_LINK);
	adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_LICENSE);
	adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about_dialog), APP_INFO_ICON_NAME);
	const char *developers[] = { APP_INFO_PROJECT_AUTHORS, NULL };
	adw_about_dialog_set_developers(ADW_ABOUT_DIALOG(about_dialog), developers);
	const char *artists[] = { APP_INFO_PROJECT_ARTISTS, NULL };
	adw_about_dialog_set_artists(ADW_ABOUT_DIALOG(about_dialog), artists);
	adw_dialog_present(ADW_DIALOG(about_dialog), _window);
}

void initialize_application_window(GtkApplication *app) {
	_window = gtk_application_window_new(app);

	gtk_window_set_resizable(GTK_WINDOW(_window), FALSE);
	gtk_widget_set_size_request(_window, MINIMUM_WINDOW_WIDTH, -1);

	_window_header = gtk_header_bar_new();
	gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(_window_header), TRUE);
	gtk_header_bar_set_title_widget(GTK_HEADER_BAR(_window_header), gtk_label_new(""));

	_menu_popover = gtk_popover_new();
	GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *menu_button = gtk_menu_button_new();
	gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");

	if (get_has_lid()) {
		GtkWidget *should_hide_internal_if_lid_closed_checkbox = gtk_check_button_new_with_label("Hide built-in displays when lid is closed");
		gtk_check_button_set_active(GTK_CHECK_BUTTON(should_hide_internal_if_lid_closed_checkbox), get_should_hide_internal_if_lid_closed());

		GtkStyleContext *should_hide_internal_if_lid_closed_checkbox_style_context = gtk_widget_get_style_context(should_hide_internal_if_lid_closed_checkbox);
		GtkCssProvider *should_hide_internal_if_lid_closed_checkbox_css_provider = gtk_css_provider_new();
		gtk_css_provider_load_from_string(should_hide_internal_if_lid_closed_checkbox_css_provider, g_strdup_printf("checkbutton.flat {padding: 4px;margin-top: 4px;margin-bottom: 8px;}"));
		gtk_style_context_add_provider(should_hide_internal_if_lid_closed_checkbox_style_context, GTK_STYLE_PROVIDER(should_hide_internal_if_lid_closed_checkbox_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_class(should_hide_internal_if_lid_closed_checkbox_style_context, "flat");

		gtk_box_append(GTK_BOX(menu_box), should_hide_internal_if_lid_closed_checkbox);
		g_signal_connect(should_hide_internal_if_lid_closed_checkbox, "toggled", G_CALLBACK(_on_should_hide_internal_if_lid_closed_checkbox_toggled), NULL);

		GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		GtkStyleContext *separator_style_context = gtk_widget_get_style_context(separator);
		GtkCssProvider *separator_css_provider = gtk_css_provider_new();
		gtk_css_provider_load_from_string(separator_css_provider, "separator {margin-bottom: 8px;}");
		gtk_style_context_add_provider(separator_style_context, GTK_STYLE_PROVIDER(separator_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_box_append(GTK_BOX(menu_box), separator);
	}

	gchar about_button_label_text[100];
	sprintf(about_button_label_text, "About %s", APP_INFO_DISPLAY_NAME);
	GtkWidget *about_button = gtk_button_new_with_label(about_button_label_text);

	GtkStyleContext *about_button_style_context = gtk_widget_get_style_context(about_button);
	GtkCssProvider *about_button_css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(about_button_css_provider, "button.flat {font-weight: normal;}");
	gtk_style_context_add_provider(about_button_style_context, GTK_STYLE_PROVIDER(about_button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_class(about_button_style_context, "flat");

	gtk_box_append(GTK_BOX(menu_box), about_button);
	g_signal_connect(about_button, "clicked", G_CALLBACK(_open_about_dialog_and_close_popover), NULL);

	gtk_popover_set_child(GTK_POPOVER(_menu_popover), menu_box);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), _menu_popover);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(_window_header), menu_button);
	gtk_widget_set_visible(menu_box, true);

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
		g_object_unref(_last_window_content_screen);
	}
	gtk_window_set_child(GTK_WINDOW(_window), new_window_content_screen);
	gtk_window_set_default_size(GTK_WINDOW(_window), gtk_widget_get_width(_window), gtk_widget_get_height(new_window_content_screen));
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
			gtk_widget_unparent(_refresh_displays_button);
			g_object_unref(_refresh_displays_button);
			_refresh_displays_button = NULL;
		}
	}

	gtk_widget_set_visible(_window, true);
}
