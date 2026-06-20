#include <gtk/gtk.h>
#include <stdlib.h>
#include "../../states/displays.c"
#include "../../states/is_contrast_hidden.c"
#include "../components/flatpak_setup_dialog.c"

#ifndef BRIGHTNESS_DEBOUNCE_DELAY_MS
#define BRIGHTNESS_DEBOUNCE_DELAY_MS 300
#endif

typedef struct display_section {
  GtkWidget *icon;
  GtkWidget *brightness_icon; // small secondary icon, NULL if display has no contrast
  GtkWidget *label;
  GtkWidget *scale;          // brightness GtkScale
  GtkWidget *contrast_scale; // contrast GtkScale, NULL if not supported
  GtkWidget *contrast_row;   // HBox with contrast icon+scale, NULL if not supported
  GtkWidget *separator_left_column;
  GtkWidget *separator_right_column;
  guint display_index;
} display_section;

display_section **_display_sections;
guint _display_sections_count = 0;

static guint *_pending_brightness_timeout_ids = NULL;
static gdouble *_pending_brightness_values = NULL;

static guint *_pending_contrast_timeout_ids = NULL;
static gdouble *_pending_contrast_values = NULL;

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
			g_signal_handlers_block_by_func(linked_range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
			gtk_range_set_value(linked_range, new_value);
			g_signal_handlers_unblock_by_func(linked_range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		}
	}
}

static gboolean _apply_debounced_contrast(gpointer user_data) {
    guint index = GPOINTER_TO_UINT(user_data);
    if (index >= _display_sections_count || _pending_contrast_values == NULL) {
        return G_SOURCE_REMOVE;
    }
    gdouble contrast = _pending_contrast_values[index];
    set_display_contrast_percentage(_display_sections[index]->display_index, contrast, FALSE);
    _pending_contrast_timeout_ids[index] = 0;
    return G_SOURCE_REMOVE;
}

void _update_display_contrast(GtkRange *range, guint data) {
    if (_pending_contrast_timeout_ids == NULL || _pending_contrast_values == NULL) {
        return;
    }
    guint index_of_display_section = GPOINTER_TO_UINT(data);
    gdouble new_value = gtk_range_get_value(range);
    new_value = CLAMP(new_value, 0.0, 100.0);

    if (_pending_contrast_timeout_ids[index_of_display_section] != 0) {
        g_source_remove(_pending_contrast_timeout_ids[index_of_display_section]);
    }

    _pending_contrast_values[index_of_display_section] = new_value;

    guint new_timeout_id = g_timeout_add(
        BRIGHTNESS_DEBOUNCE_DELAY_MS,
        _apply_debounced_contrast,
        GUINT_TO_POINTER(index_of_display_section)
    );
    if (new_timeout_id != 0) {
        _pending_contrast_timeout_ids[index_of_display_section] = new_timeout_id;
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

    if (_pending_contrast_timeout_ids != NULL) {
        for (guint i = 0; i < _display_sections_count; i++) {
            if (_pending_contrast_timeout_ids[i] > 0) {
                g_source_remove(_pending_contrast_timeout_ids[i]);
            }
        }
        free(_pending_contrast_timeout_ids);
        _pending_contrast_timeout_ids = NULL;
    }

    if (_pending_contrast_values != NULL) {
        free(_pending_contrast_values);
        _pending_contrast_values = NULL;
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

void set_contrast_row_visibility(gboolean visible) {
    if (_display_sections == NULL) return;
    for (guint i = 0; i < _display_sections_count; i++) {
        if (_display_sections[i]->contrast_row != NULL) {
            gtk_widget_set_visible(_display_sections[i]->contrast_row, visible);
        }
        if (_display_sections[i]->brightness_icon != NULL) {
            gtk_widget_set_visible(_display_sections[i]->brightness_icon, visible);
        }
    }
}

void _on_setup_permissions_banner_button_clicked(GtkWidget *widget, gpointer data) {
    (void)data;
    show_flatpak_setup_dialog(GTK_WINDOW(gtk_widget_get_root(widget)));
}

static GtkWidget* _make_secondary_icon(const char *icon_name) {
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
    gtk_widget_set_opacity(icon, 0.55);
    gtk_widget_set_margin_start(icon, MARGIN_UNIT);
    gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
    return icon;
}

GtkWidget* get_show_displays_screen() {
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

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
	_pending_contrast_timeout_ids = calloc(_display_sections_count, sizeof(guint));
	_pending_contrast_values = calloc(_display_sections_count, sizeof(gdouble));

	if (_pending_brightness_timeout_ids == NULL || _pending_brightness_values == NULL ||
	    _pending_contrast_timeout_ids == NULL || _pending_contrast_values == NULL) {
		fprintf(stderr, "Failed to allocate debounce tracking arrays\n");
	}

	gboolean contrast_hidden = get_is_contrast_hidden();

	for (guint index = 0; index < displays_count(); index++) {
		display_section *display_section_instance = malloc(sizeof(display_section));
		display_section_instance->display_index = index;
		display_section_instance->scale = get_display_brightness_scale(get_display_brightness_percentage(index), 100.0);
		gtk_widget_set_margin_bottom(display_section_instance->scale, 0);
		g_signal_connect(display_section_instance->scale, "value-changed", G_CALLBACK(_update_display_brightness), GUINT_TO_POINTER(index));

		display_section_instance->separator_left_column = get_separator();
		display_section_instance->separator_right_column = get_separator();
		display_section_instance->icon = get_display_icon();

		gboolean has_contrast = get_display_has_contrast(index);
		if (has_contrast) {
			display_section_instance->contrast_scale = get_display_brightness_scale(get_display_contrast_percentage(index), 100.0);
			gtk_widget_set_margin_bottom(display_section_instance->contrast_scale, 0);
			g_signal_connect(display_section_instance->contrast_scale, "value-changed", G_CALLBACK(_update_display_contrast), GUINT_TO_POINTER(index));
		} else {
			display_section_instance->contrast_scale = NULL;
		}

		sections[index] = display_section_instance;

		// Build the content VBox: label + brightness row + optional contrast row
		GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

		display_section_instance->label = get_display_label(get_display_name(index));
		gtk_box_append(GTK_BOX(content_box), display_section_instance->label);

		// Brightness row: small icon left of scale (icon only shown when contrast also visible)
		GtkWidget *brightness_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

		if (has_contrast) {
			GtkWidget *brightness_icon = _make_secondary_icon("display-brightness-symbolic");
			display_section_instance->brightness_icon = brightness_icon;
			gtk_widget_set_visible(brightness_icon, !contrast_hidden);
			gtk_box_append(GTK_BOX(brightness_row), brightness_icon);
		} else {
			display_section_instance->brightness_icon = NULL;
		}

		gtk_box_append(GTK_BOX(brightness_row), display_section_instance->scale);

		if (has_contrast) {
			gtk_widget_set_margin_bottom(brightness_row, MARGIN_UNIT / 2);
			GtkWidget *contrast_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
			GtkWidget *contrast_icon = _make_secondary_icon("weather-clear-night-symbolic");
			gtk_box_append(GTK_BOX(contrast_row), contrast_icon);
			gtk_box_append(GTK_BOX(contrast_row), display_section_instance->contrast_scale);
			gtk_widget_set_margin_bottom(contrast_row, MARGIN_UNIT);
			display_section_instance->contrast_row = contrast_row;
			gtk_widget_set_visible(contrast_row, !contrast_hidden);
			gtk_box_append(GTK_BOX(content_box), brightness_row);
			gtk_box_append(GTK_BOX(content_box), contrast_row);
		} else {
			gtk_widget_set_margin_bottom(brightness_row, MARGIN_UNIT);
			display_section_instance->contrast_row = NULL;
			gtk_box_append(GTK_BOX(content_box), brightness_row);
		}

		// Attach icon and content_box to grid
		if (sibling == NULL) {
			gtk_grid_attach(GTK_GRID(grid), display_section_instance->icon, 0, 0, 1, 1);
			gtk_grid_attach(GTK_GRID(grid), content_box, 1, 0, 1, 1);
		} else {
			gtk_grid_attach_next_to(GTK_GRID(grid), display_section_instance->icon, sibling->separator_left_column, GTK_POS_BOTTOM, 1, 1);
			gtk_grid_attach_next_to(GTK_GRID(grid), content_box, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);
		}

		gtk_grid_attach_next_to(GTK_GRID(grid), display_section_instance->separator_left_column, display_section_instance->icon, GTK_POS_BOTTOM, 1, 1);
		gtk_grid_attach_next_to(GTK_GRID(grid), display_section_instance->separator_right_column, content_box, GTK_POS_BOTTOM, 1, 1);
		sibling = display_section_instance;
	}

	link_brightness_checkbox = get_link_brightness_checkbox(get_is_brightness_linked());
	gtk_grid_attach_next_to(GTK_GRID(grid), link_brightness_checkbox, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);
	g_signal_connect(link_brightness_checkbox, "toggled", G_CALLBACK(_link_brightness), NULL);

	return main_box;
}
