#include <gtk/gtk.h>
#include <stdlib.h>
#include "../../states/displays.c"
#include "../components/flatpak_setup_dialog.c"

#ifndef BRIGHTNESS_DEBOUNCE_DELAY_MS
#define BRIGHTNESS_DEBOUNCE_DELAY_MS 300
#endif

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

static guint *_pending_brightness_timeout_ids = NULL;
static gdouble *_pending_brightness_values = NULL;

static gboolean _apply_debounced_brightness(gpointer user_data) {
    guint index = GPOINTER_TO_UINT(user_data);
    if (index >= _display_sections_count || _pending_brightness_values == NULL) {
        return G_SOURCE_REMOVE;
    }
    gdouble brightness = _pending_brightness_values[index];

    set_display_brightness_percentage(index, brightness, FALSE);

    if (get_is_brightness_linked()) {
        for (guint i = 0; i < _display_sections_count; i++) {
            if (_display_sections[i]->display_index == index) continue;
            set_display_brightness_percentage(_display_sections[i]->display_index, brightness, FALSE);
        }
    }

    _pending_brightness_timeout_ids[index] = 0;
    return G_SOURCE_REMOVE;
}

void _update_display_brightness(GtkRange *range, guint data) {
	if (_pending_brightness_timeout_ids == NULL || _pending_brightness_values == NULL) {
		return;
	}
	guint index_of_display_section = GPOINTER_TO_UINT(data);
	gdouble new_value = gtk_range_get_value(range);

	new_value = CLAMP(new_value, 0.0, 100.0);

	if (_pending_brightness_timeout_ids[index_of_display_section] != 0) {
		g_source_remove(_pending_brightness_timeout_ids[index_of_display_section]);
	}

	_pending_brightness_values[index_of_display_section] = new_value;

	guint new_timeout_id = g_timeout_add(
		BRIGHTNESS_DEBOUNCE_DELAY_MS,
		_apply_debounced_brightness,
		GUINT_TO_POINTER(index_of_display_section)
	);
	if (new_timeout_id != 0) {
		_pending_brightness_timeout_ids[index_of_display_section] = new_timeout_id;
	}

	if (get_is_brightness_linked()) {
		for (guint index = 0; index < _display_sections_count; index++) {
			if (_display_sections[index]->display_index == index_of_display_section) {
				continue;
			}
			GtkRange *linked_range = GTK_RANGE(_display_sections[index]->scale);
			// Block signal handler to prevent recursive calls and redundant debounce timers
			// when updating linked sliders programmatically.
			g_signal_handlers_block_by_func(linked_range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
			gtk_range_set_value(linked_range, new_value);
			g_signal_handlers_unblock_by_func(linked_range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
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
		GtkRange *range = GTK_RANGE(_display_sections[index]->scale);
		// Block signal handler to verify we simply update the UI and apply brightness immediately
		// without triggering the debounce mechanism.
		g_signal_handlers_block_by_func(range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		gtk_range_set_value(range, max_scale_percentage);
		g_signal_handlers_unblock_by_func(range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		set_display_brightness_percentage(_display_sections[index]->display_index, max_scale_percentage, FALSE);
	}
}


static void _cleanup_display_resources(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    if (_pending_brightness_timeout_ids != NULL) {
        for (guint i = 0; i < _display_sections_count; i++) {
            if (_pending_brightness_timeout_ids[i] > 0) {
                g_source_remove(_pending_brightness_timeout_ids[i]);
            }
        }
        free(_pending_brightness_timeout_ids);
        _pending_brightness_timeout_ids = NULL;
    }

    if (_pending_brightness_values != NULL) {
        free(_pending_brightness_values);
        _pending_brightness_values = NULL;
    }

    if (_display_sections != NULL) {
        for (guint i = 0; i < _display_sections_count; i++) {
            if (_display_sections[i] != NULL) {
                free(_display_sections[i]);
            }
        }
        free(_display_sections);
        _display_sections = NULL;
    }

    _display_sections_count = 0;
}

void _on_setup_permissions_banner_button_clicked(GtkWidget *widget, gpointer data) {
    (void)data;
    show_flatpak_setup_dialog(GTK_WINDOW(gtk_widget_get_root(widget)));
}

GtkWidget* get_show_displays_screen() {
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Check for permissions
    gboolean show_banner = have_internal_displays_without_permission_in_flatpak();
    
    if (show_banner) {
        GtkWidget *banner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_add_css_class(banner, "banner");
        gtk_widget_add_css_class(banner, "warning");
        
        GtkWidget *label = gtk_label_new("Missing permissions for built-in displays.");
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        
        GtkWidget *button = gtk_button_new_with_label("Setup");
        g_signal_connect(button, "clicked", G_CALLBACK(_on_setup_permissions_banner_button_clicked), NULL);
        
        gtk_box_append(GTK_BOX(banner), label);
        gtk_box_append(GTK_BOX(banner), button);
        
        // Add styling for banner
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_string(provider, 
            ".banner { padding: 12px; background-color: alpha(@warning_bg_color, 0.2); border-bottom: 1px solid alpha(@warning_fg_color, 0.2); }");
        gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
        gtk_box_append(GTK_BOX(main_box), banner);
    }

	GtkWidget *grid, *link_brightness_checkbox;
	display_section **sections = malloc(displays_count() * sizeof(display_section *));
	display_section *sibling = NULL;

	grid = gtk_grid_new();
    gtk_box_append(GTK_BOX(main_box), grid);
    
    g_signal_connect(grid, "destroy", G_CALLBACK(_cleanup_display_resources), NULL);

	_display_sections = sections;
	_display_sections_count = displays_count();

	_pending_brightness_timeout_ids = calloc(_display_sections_count, sizeof(guint));
	_pending_brightness_values = calloc(_display_sections_count, sizeof(gdouble));

	if (_pending_brightness_timeout_ids == NULL || _pending_brightness_values == NULL) {
		fprintf(stderr, "Failed to allocate debounce tracking arrays\n");
	}

	for (guint index = 0; index < displays_count(); index++) {
		display_section *display_section_instance = malloc(sizeof(display_section));
		display_section_instance->display_index = index;
		display_section_instance->label = get_display_label(get_display_name(index));
		display_section_instance->scale = get_display_brightness_scale(get_display_brightness_percentage(index), 100.0);
		g_signal_connect(display_section_instance->scale, "value-changed", G_CALLBACK(_update_display_brightness), GUINT_TO_POINTER(index));

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

	return main_box;
}
