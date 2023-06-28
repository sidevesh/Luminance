#include <gtk/gtk.h>

#include "../../states/displays.c"

typedef struct display_section {
  GtkWidget *icon;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *separator_left_column;
  GtkWidget *separator_right_column;
  guint display_index;
} display_section;

display_section **_display_sections;
guint _display_sections_count = 0;

gboolean _set_brightness(GtkWidget *widget, GdkEvent *event, guint data) {
	guint index_of_display_section = GPOINTER_TO_UINT(data);

	display_section *display_section = _display_sections[index_of_display_section];

	guint16 new_brightness_percentage = gtk_range_get_value(GTK_RANGE(widget));
	set_display_brightness_percentage(display_section->display_index, new_brightness_percentage);

	if (get_is_brightness_linked()) {
		for (guint index = 0; index < _display_sections_count; index++) {
			GtkWidget *scale = _display_sections[index]->scale;

			if (_display_sections[index]->display_index == display_section->display_index) {
				continue;
			}
			gtk_range_set_value(GTK_RANGE(scale), new_brightness_percentage);
			set_display_brightness_percentage(_display_sections[index]->display_index, new_brightness_percentage);
		}
	}

	return FALSE;
}

void _update_display_brightness_scales(GtkRange *range, guint data) {
	guint index_of_display_section = GPOINTER_TO_UINT(data);

	guint new_value = gtk_range_get_value(range);
	for (guint index = 0; index < _display_sections_count; index++) {
		if (get_is_brightness_linked() || index == index_of_display_section) {
			gtk_range_set_value(GTK_RANGE(_display_sections[index]->scale), new_value);
		}
	}
}

void _link_brightness(GtkToggleButton *link_brightness_checkbox) {
	gboolean is_brightness_linked = gtk_toggle_button_get_active(link_brightness_checkbox);
	set_is_brightness_linked(is_brightness_linked);

	if (!is_brightness_linked) {
		return;
	}

	gdouble max_scale_percentage = 0;
	for (guint index = 0; index < _display_sections_count; index++) {
		guint percentage_value = gtk_range_get_value(GTK_RANGE(_display_sections[index]->scale));
		max_scale_percentage = percentage_value > max_scale_percentage ? percentage_value : max_scale_percentage;
	}

	for (guint index = 0; index < _display_sections_count; index++) {
		gtk_range_set_value(GTK_RANGE(_display_sections[index]->scale), max_scale_percentage);
		set_display_brightness_percentage(_display_sections[index]->display_index, max_scale_percentage);
	}
}

GtkWidget* get_show_displays_screen() {
	GtkWidget *grid, *link_brightness_checkbox;

	grid = gtk_grid_new();

	display_section **sections = malloc(displays_count());
	_display_sections = sections;
	_display_sections_count = displays_count();

	display_section *sibling = NULL;

	for (guint index = 0; index < displays_count(); index++) {
		display_section *display_section_instance = malloc(sizeof(display_section));
		display_section_instance->display_index = index;
		display_section_instance->label = get_display_label(get_display_name(index));
		display_section_instance->scale = get_display_brightness_scale(get_display_brightness_percentage(index), 100.0);
		g_signal_connect(display_section_instance->scale, "button-release-event", G_CALLBACK(_set_brightness), GUINT_TO_POINTER(index));
		g_signal_connect(display_section_instance->scale, "scroll-event", G_CALLBACK(_set_brightness), GUINT_TO_POINTER(index));
		g_signal_connect(display_section_instance->scale, "value-changed", G_CALLBACK(_update_display_brightness_scales), GUINT_TO_POINTER(index));
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

	link_brightness_checkbox = get_link_brightness_checkbox(get_is_brightness_linked());
	g_signal_connect(link_brightness_checkbox, "toggled", G_CALLBACK(_link_brightness), NULL);
	gtk_grid_attach_next_to(GTK_GRID(grid), link_brightness_checkbox, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);

	return grid;
}
