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

static void _set_brightness(GtkGesture *gesture, gdouble offset_x, gdouble offset_y, gpointer user_data) {
	guint index_of_display_section = GPOINTER_TO_UINT(user_data);
	display_section *display_section = _display_sections[index_of_display_section];
	GtkWidget *scale_widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
	GtkRange *range = GTK_RANGE(scale_widget);
	gdouble current_value = gtk_range_get_value(range);
	gdouble new_value = current_value;

	if (GTK_IS_GESTURE_DRAG(gesture)) {
		// For drag, you might want to adjust based on the drag distance
		// This is a simple example, you may want to fine-tune it
		new_value += offset_x * 0.1;  // Adjust 0.1 to control sensitivity
	} else if (GTK_IS_EVENT_CONTROLLER_SCROLL(gesture)) {
		// For scroll, adjust up or down
		new_value += (offset_y < 0) ? 1.0 : -1.0;  // Scroll up increases, down decreases
	}

	// Clamp the new value between 0 and 100
	new_value = CLAMP(new_value, 0.0, 100.0);

	guint16 new_brightness_percentage = (guint16)new_value;
	set_display_brightness_percentage(display_section->display_index, new_brightness_percentage);

	// Update the range value (this will trigger _update_display_brightness_scales)
	gtk_range_set_value(range, new_value);

	if (get_is_brightness_linked()) {
		for (guint index = 0; index < _display_sections_count; index++) {
			if (_display_sections[index]->display_index == display_section->display_index) {
				continue;  // Skip the current display
			}
			GtkRange *linked_range = GTK_RANGE(_display_sections[index]->scale);
			gtk_range_set_value(linked_range, new_value);
			set_display_brightness_percentage(_display_sections[index]->display_index, new_brightness_percentage);
		}
	}
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

void _link_brightness(GtkCheckButton *link_brightness_checkbox) {
	gboolean is_brightness_linked = gtk_check_button_get_active(link_brightness_checkbox);
	gdouble max_scale_percentage = 0;

	set_is_brightness_linked(is_brightness_linked);
	if (!is_brightness_linked) {
		return;
	}

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
	display_section **sections = malloc(displays_count());
	display_section *sibling = NULL;

	grid = gtk_grid_new();
	_display_sections = sections;
	_display_sections_count = displays_count();

	for (guint index = 0; index < displays_count(); index++) {
		display_section *display_section_instance = malloc(sizeof(display_section));
		display_section_instance->display_index = index;
		display_section_instance->label = get_display_label(get_display_name(index));
		display_section_instance->scale = get_display_brightness_scale(get_display_brightness_percentage(index), 100.0);

		g_signal_connect(display_section_instance->scale, "value-changed", G_CALLBACK(_update_display_brightness_scales), GUINT_TO_POINTER(index));
		GtkGesture *drag_gesture = gtk_gesture_drag_new();
		gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag_gesture), 0);
		g_signal_connect(drag_gesture, "drag-end", G_CALLBACK(_set_brightness), GUINT_TO_POINTER(index));
		gtk_widget_add_controller(display_section_instance->scale, GTK_EVENT_CONTROLLER(drag_gesture));
		GtkEventController *scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
		g_signal_connect(scroll_controller, "scroll", G_CALLBACK(_set_brightness), GUINT_TO_POINTER(index));
		gtk_widget_add_controller(display_section_instance->scale, GTK_EVENT_CONTROLLER(scroll_controller));

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
	gtk_grid_attach_next_to(GTK_GRID(grid), link_brightness_checkbox, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);
	g_signal_connect(link_brightness_checkbox, "toggled", G_CALLBACK(_link_brightness), NULL);

	return grid;
}
