#include <gtk/gtk.h>

#ifndef BRIGHTNESS_CODE
#include "../ddcbc-api/ddcbc-api.c"
#endif

#include "./types/display_section.c"
#include "./states/display_list.c"
#include "./states/is_brightness_linked.c"
#include "./ui/constants/main.c"
#include "./ui/components/display_brightness_scale.c"
#include "./ui/components/display_icon.c"
#include "./ui/components/display_label.c"
#include "./ui/components/link_brightness_check_button.c"
#include "./ui/components/separator.c"
#include "./ui/screens/displays.c"
#include "./ui/screens/no_displays.c"
#include "./ui/screens/refreshing_displays.c"
#include "./window.c"

display_section **display_sections;
guint display_sections_count = 0;

gboolean set_brightness(GtkWidget *widget, GdkEvent *event, guint data) {
	guint index_of_display_section = GPOINTER_TO_UINT(data);

	display_section *display_section = display_sections[index_of_display_section];
	ddcbc_display *display = display_section->display;

	guint16 new_val = gtk_range_get_value(GTK_RANGE(widget));
	DDCBC_Status rc = ddcbc_display_set_brightness(display, new_val);

	// If the link checkbox is checked, set all other displays to the same value:
	// TODO: It might be the case that the new value will exceed the max of other
	// displays.
	if (get_is_brightness_linked()) {
		for (guint index = 0; index < display_sections_count; index++) {
			GtkWidget *scale = display_sections[index]->scale;
			gtk_range_set_value(GTK_RANGE(scale), new_val);

			ddcbc_display *display = display_sections[index]->display;
			ddcbc_display_set_brightness(display, new_val);
		}
	}

	if (rc == 1) {
		g_printerr(
			"Partial sucess in setting the brightness of display no"
			" %d to %u. Code: %d\n",
			display->info.dispno, new_val, rc
		);
	} else if (rc != 0) {
		g_printerr(
			"An error occured when setting the brightness of display no"
			" %d to %u. Code: %d\n",
			display->info.dispno, new_val, rc
		);
	}

	return FALSE;
}

void update_display_brightness_scales(GtkRange *range, guint data) {
	guint index_of_display_section = GPOINTER_TO_UINT(data);

	guint new_value = gtk_range_get_value(range);
	for (guint index = 0; index < display_sections_count; index++) {
		if (get_is_brightness_linked() || index == index_of_display_section) {
			gtk_range_set_value(GTK_RANGE(display_sections[index]->scale), new_value);
		}
	}
}

void link_brightness(GtkToggleButton *link_brightness_check_button) {
	gboolean is_brightness_linked = gtk_toggle_button_get_active(link_brightness_check_button);
	set_is_brightness_linked(is_brightness_linked);

	if (!is_brightness_linked) {
		return;
	}

	guint max_scale = 0;
	for (guint index = 0; index < display_sections_count; index++) {
		guint val = gtk_range_get_value(GTK_RANGE(display_sections[index]->scale));
		max_scale = val > max_scale ? val : max_scale;
	}

	for (guint index = 0; index < display_sections_count; index++) {
		gtk_range_set_value(GTK_RANGE(display_sections[index]->scale), max_scale);
		ddcbc_display *display = display_sections[index]->display;
		DDCBC_Status rc = ddcbc_display_set_brightness(display, max_scale);
		
		if (rc == 1) {
			g_printerr(
				"Partial success in setting the brightness of display no"
				" %d to %u while linking displays. Code: %d\n",
				display->info.dispno, max_scale, rc
			);	
		} else if (rc != 0) {
			g_printerr(
				"An error occured when setting the brightness of display no"
				" %d to %u while linking displays. Code: %d\n",
				display->info.dispno, max_scale, rc
			);	
		}
	}
}

void update_window_contents() {
	GtkWidget *current_screen;

	if (is_display_list_loading() == TRUE) {
		current_screen = get_refreshing_displays_screen();
	} else if (display_list_count() == 0) {
		current_screen = get_no_displays_screen();
	} else {
		current_screen = get_displays_screen();
	}

	update_window_content_screen(current_screen);
}

static void activate(GtkApplication *app) {
	initialize_application_window(app);
	update_window_contents();
}

int main(int argc, char **argv) {
	GtkApplication *app;
	int status;

	initialize_display_list();

	app = gtk_application_new("com.sidevesh.Luminance", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	free_display_list();

	return status;
}
