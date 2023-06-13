#include <gtk/gtk.h>

#ifndef BRIGHTNESS_CODE
#include "../../../ddcbc-api/ddcbc-api.c"
#endif

#include "../../states/displays.c"
#include "../../types/display_section.c"

extern display_section **display_sections;
extern guint display_sections_count;

extern gboolean set_brightness(GtkWidget *widget, GdkEvent *event, guint data);
extern void update_display_brightness_scales(GtkRange *range, guint data);
extern void link_brightness(GtkToggleButton *link_brightness_check_button);

GtkWidget* get_show_displays_screen() {
	GtkWidget *grid, *link_brightness_check_button;

	grid = gtk_grid_new();

	display_section **sections = malloc(displays_count());
	display_sections = sections;
	display_sections_count = displays_count();

	display_section *sibling = NULL;

	for (guint index = 0; index < displays_count(); index++) {
    ddcbc_display *display = get_display(index);
		display_section *display_section_instance = malloc(sizeof(display_section));
		display_section_instance->display = display;
		display_section_instance->label = get_display_label(display->info.model_name);
		display_section_instance->scale = get_display_brightness_scale(display->last_val, display->max_val);
		g_signal_connect(display_section_instance->scale, "button-release-event", G_CALLBACK(set_brightness), GUINT_TO_POINTER(index));
		g_signal_connect(display_section_instance->scale, "scroll-event", G_CALLBACK(set_brightness), GUINT_TO_POINTER(index));
		g_signal_connect(display_section_instance->scale, "value-changed", G_CALLBACK(update_display_brightness_scales), GUINT_TO_POINTER(index));
		display_section_instance->separator_left_column = get_separator();
		display_section_instance->separator_right_column = get_separator();
		display_section_instance->icon = get_display_icon();
		sections[index] = display_section_instance;

		if (sibling == NULL) {
			gtk_grid_attach(GTK_GRID(grid), sections[index]->label, 1, 0, 1, 1);
		} else {
			gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->label, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);
		}
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->scale, sections[index]->label, GTK_POS_BOTTOM, 1, 1);
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->icon, sections[index]->label, GTK_POS_LEFT, 1, 2);
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->separator_left_column, sections[index]->icon, GTK_POS_BOTTOM, 1, 1);
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->separator_right_column, sections[index]->scale, GTK_POS_BOTTOM, 1, 1);
		sibling = sections[index];
	}

	link_brightness_check_button = get_link_brightness_check_button(get_is_brightness_linked());
	g_signal_connect(link_brightness_check_button, "toggled", G_CALLBACK(link_brightness), NULL);
	gtk_grid_attach_next_to(GTK_GRID(grid), link_brightness_check_button, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);

	return grid;
}
