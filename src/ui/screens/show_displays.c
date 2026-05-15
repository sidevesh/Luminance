#include <gtk/gtk.h>
#include <stdlib.h>
#include "../../states/displays.c"
#include "../../states/brightness_range.c"
#include "../../states/display_names.c"
#include "../components/flatpak_setup_dialog.c"
#include "../components/range_label.c"
#include "../components/editable_label.c"

#ifndef BRIGHTNESS_DEBOUNCE_DELAY_MS
#define BRIGHTNESS_DEBOUNCE_DELAY_MS 300
#endif

typedef struct display_section {
  GtkWidget *icon;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *scale_row;  // hbox containing min_label + scale + max_label
  GtkWidget *min_label;
  GtkWidget *max_label;
  GtkWidget *separator_left_column;
  GtkWidget *separator_right_column;
  guint display_index;
  gdouble min_percentage;
  gdouble max_percentage;
  char *display_name;
} display_section;

display_section **_display_sections;
guint _display_sections_count = 0;
gboolean _show_brightness_range = FALSE;
static GtkWidget *_sync_exact_checkbox = NULL;
static GtkWidget *_sync_proportional_checkbox = NULL;

static guint *_pending_brightness_timeout_ids = NULL;
static gdouble *_pending_brightness_values = NULL;

static void _rebuild_slider_for_display(guint index);
void _link_proportional_brightness(GtkCheckButton *checkbox);

// Compute proportional brightness for a target display given a source display's value
static gdouble _proportional_brightness(guint source_section, gdouble source_value, guint target_section) {
	gdouble src_min = _display_sections[source_section]->min_percentage;
	gdouble src_max = _display_sections[source_section]->max_percentage;
	gdouble tgt_min = _display_sections[target_section]->min_percentage;
	gdouble tgt_max = _display_sections[target_section]->max_percentage;

	gdouble src_range = src_max - src_min;
	if (src_range == 0) return tgt_min; // source is fixed, just use target min

	gdouble position = (source_value - src_min) / src_range; // 0.0 to 1.0
	return tgt_min + position * (tgt_max - tgt_min);
}

// Returns true if any sync mode is active
static gboolean _is_any_sync_active() {
	return get_is_brightness_linked() || get_is_brightness_proportionally_linked();
}

static gboolean _apply_debounced_brightness(gpointer user_data) {
    guint index = GPOINTER_TO_UINT(user_data);
    if (index >= _display_sections_count || _pending_brightness_values == NULL) {
        return G_SOURCE_REMOVE;
    }
    gdouble brightness = _pending_brightness_values[index];

    set_display_brightness_percentage(index, brightness, FALSE);

    gboolean exact_sync = get_is_brightness_linked();
    gboolean prop_sync = get_is_brightness_proportionally_linked();

    if (exact_sync || prop_sync) {
        for (guint i = 0; i < _display_sections_count; i++) {
            if (_display_sections[i]->display_index == index) continue;
            gdouble linked_brightness;
            if (prop_sync) {
                linked_brightness = _proportional_brightness(index, brightness, i);
            } else {
                linked_brightness = CLAMP(brightness, _display_sections[i]->min_percentage, _display_sections[i]->max_percentage);
            }
            set_display_brightness_percentage(_display_sections[i]->display_index, linked_brightness, FALSE);
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

	gdouble min_pct = _display_sections[index_of_display_section]->min_percentage;
	gdouble max_pct = _display_sections[index_of_display_section]->max_percentage;
	new_value = CLAMP(new_value, min_pct, max_pct);

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

	if (_is_any_sync_active()) {
		gboolean prop_sync = get_is_brightness_proportionally_linked();
		for (guint index = 0; index < _display_sections_count; index++) {
			if (_display_sections[index]->display_index == index_of_display_section) {
				continue;
			}
			GtkRange *linked_range = GTK_RANGE(_display_sections[index]->scale);
			g_signal_handlers_block_by_func(linked_range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
			gdouble linked_value;
			if (prop_sync) {
				linked_value = _proportional_brightness(index_of_display_section, new_value, index);
			} else {
				linked_value = CLAMP(new_value, _display_sections[index]->min_percentage, _display_sections[index]->max_percentage);
			}
			gtk_range_set_value(linked_range, linked_value);
			g_signal_handlers_unblock_by_func(linked_range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		}
	}
}

void _link_brightness(GtkCheckButton *link_brightness_checkbox) {
	gboolean is_brightness_linked = gtk_check_button_get_active(link_brightness_checkbox);

	set_is_brightness_linked(is_brightness_linked);

	// Mutual exclusion: uncheck proportional if exact is checked
	if (is_brightness_linked) {
		set_is_brightness_proportionally_linked(FALSE);
		if (_sync_proportional_checkbox != NULL) {
			g_signal_handlers_block_by_func(_sync_proportional_checkbox, (gpointer)_link_proportional_brightness, NULL);
			gtk_check_button_set_active(GTK_CHECK_BUTTON(_sync_proportional_checkbox), FALSE);
			g_signal_handlers_unblock_by_func(_sync_proportional_checkbox, (gpointer)_link_proportional_brightness, NULL);
		}
	}

	if (!is_brightness_linked) {
		return;
	}

	// Sync all displays to the highest value
	gdouble max_scale_percentage = 0;
	for (guint index = 0; index < _display_sections_count; index++) {
		guint percentage_value = gtk_range_get_value(GTK_RANGE(_display_sections[index]->scale));
		max_scale_percentage = percentage_value > max_scale_percentage ? percentage_value : max_scale_percentage;
	}

	for (guint index = 0; index < _display_sections_count; index++) {
		GtkRange *range = GTK_RANGE(_display_sections[index]->scale);
		g_signal_handlers_block_by_func(range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		gdouble capped = CLAMP(max_scale_percentage, _display_sections[index]->min_percentage, _display_sections[index]->max_percentage);
		gtk_range_set_value(range, capped);
		g_signal_handlers_unblock_by_func(range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		set_display_brightness_percentage(_display_sections[index]->display_index, capped, FALSE);
	}
}

void _link_proportional_brightness(GtkCheckButton *checkbox) {
	gboolean is_prop_linked = gtk_check_button_get_active(checkbox);

	set_is_brightness_proportionally_linked(is_prop_linked);

	// Mutual exclusion: uncheck exact if proportional is checked
	if (is_prop_linked) {
		set_is_brightness_linked(FALSE);
		if (_sync_exact_checkbox != NULL) {
			g_signal_handlers_block_by_func(_sync_exact_checkbox, (gpointer)_link_brightness, NULL);
			gtk_check_button_set_active(GTK_CHECK_BUTTON(_sync_exact_checkbox), FALSE);
			g_signal_handlers_unblock_by_func(_sync_exact_checkbox, (gpointer)_link_brightness, NULL);
		}
	}

	if (!is_prop_linked) {
		return;
	}

	// Find the display with the highest relative position in its range, then sync proportionally
	// Use the first display as reference — sync all to its relative position
	gdouble ref_value = gtk_range_get_value(GTK_RANGE(_display_sections[0]->scale));
	guint ref_index = 0;

	for (guint index = 0; index < _display_sections_count; index++) {
		GtkRange *range = GTK_RANGE(_display_sections[index]->scale);
		g_signal_handlers_block_by_func(range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		gdouble prop_value = _proportional_brightness(ref_index, ref_value, index);
		gtk_range_set_value(range, prop_value);
		g_signal_handlers_unblock_by_func(range, (gpointer)_update_display_brightness, GUINT_TO_POINTER(_display_sections[index]->display_index));
		set_display_brightness_percentage(_display_sections[index]->display_index, prop_value, FALSE);
	}
}

// --- Show/hide brightness range ---

// --- Display rename callback ---

static void _on_display_renamed(const gchar *new_name, gpointer user_data) {
	guint index = GPOINTER_TO_UINT(user_data);
	if (index >= _display_sections_count) return;
	const gchar *default_name = _display_sections[index]->display_name;
	// If new_name matches default, save empty (clears custom name)
	if (g_strcmp0(new_name, default_name) == 0) {
		set_custom_display_name(default_name, "");
	} else {
		set_custom_display_name(default_name, new_name);
	}
}

static void _toggle_brightness_range(GtkCheckButton *checkbox) {
	_show_brightness_range = gtk_check_button_get_active(checkbox);
	for (guint i = 0; i < _display_sections_count; i++) {
		gtk_widget_set_visible(_display_sections[i]->min_label, _show_brightness_range);
		gtk_widget_set_visible(_display_sections[i]->max_label, _show_brightness_range);
	}
}

// --- Min/Max label callbacks ---

static void _on_min_value_changed(gdouble new_min, gpointer user_data) {
	guint index = GPOINTER_TO_UINT(user_data);
	if (index >= _display_sections_count) return;

	// Enforce min <= max
	gdouble current_max = _display_sections[index]->max_percentage;
	if (new_min > current_max) new_min = current_max;
	if (new_min < 0) new_min = 0;

	_display_sections[index]->min_percentage = new_min;
	set_min_brightness_percentage(_display_sections[index]->display_name, (gint)new_min);

	// Rebuild slider — also recreates labels with updated ranges
	_rebuild_slider_for_display(index);
}

static void _on_max_value_changed(gdouble new_max, gpointer user_data) {
	guint index = GPOINTER_TO_UINT(user_data);
	if (index >= _display_sections_count) return;

	// Enforce max >= min
	gdouble current_min = _display_sections[index]->min_percentage;
	if (new_max < current_min) new_max = current_min;
	if (new_max > 100) new_max = 100;

	_display_sections[index]->max_percentage = new_max;
	set_max_brightness_percentage(_display_sections[index]->display_name, (gint)new_max);

	// Rebuild slider — also recreates labels with updated ranges
	_rebuild_slider_for_display(index);
}

static void _rebuild_slider_for_display(guint index) {
	if (index >= _display_sections_count || _display_sections[index]->scale == NULL) return;
	GtkRange *range = GTK_RANGE(_display_sections[index]->scale);
	gdouble min_pct = _display_sections[index]->min_percentage;
	gdouble max_pct = _display_sections[index]->max_percentage;

	if (min_pct == max_pct) {
		// Fixed brightness: use 0-100 range, set value, disable
		gtk_range_set_range(range, 0, 100);
		gtk_range_set_value(range, min_pct);
		gtk_widget_set_sensitive(_display_sections[index]->scale, FALSE);
		set_display_brightness_percentage(index, min_pct, FALSE);
	} else {
		gtk_widget_set_sensitive(_display_sections[index]->scale, TRUE);
		gtk_range_set_range(range, min_pct, max_pct);

		gdouble current = gtk_range_get_value(range);
		gdouble clamped = CLAMP(current, min_pct, max_pct);
		gtk_range_set_value(range, clamped);

		if (current != clamped) {
			set_display_brightness_percentage(index, clamped, FALSE);
		}
	}

	// Update min/max labels with new allowed ranges
	if (_display_sections[index]->min_label != NULL)
		update_range_label(_display_sections[index]->min_label, min_pct, 0, max_pct);
	if (_display_sections[index]->max_label != NULL)
		update_range_label(_display_sections[index]->max_label, max_pct, min_pct, 100);
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
                if (_display_sections[i]->display_name != NULL) {
                    free(_display_sections[i]->display_name);
                }
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
        
        GtkWidget *banner_label = gtk_label_new("Missing permissions for built-in displays.");
        gtk_label_set_wrap(GTK_LABEL(banner_label), TRUE);
        gtk_widget_set_hexpand(banner_label, TRUE);
        gtk_widget_set_halign(banner_label, GTK_ALIGN_START);
        
        GtkWidget *button = gtk_button_new_with_label("Setup");
        g_signal_connect(button, "clicked", G_CALLBACK(_on_setup_permissions_banner_button_clicked), NULL);
        
        gtk_box_append(GTK_BOX(banner), banner_label);
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

	if (_pending_brightness_timeout_ids == NULL || _pending_brightness_values == NULL) {
		fprintf(stderr, "Failed to allocate debounce tracking arrays\n");
	}

	for (guint index = 0; index < displays_count(); index++) {
		display_section *display_section_instance = malloc(sizeof(display_section));
		display_section_instance->display_index = index;
		char *display_name = get_display_name(index);
		display_section_instance->display_name = strdup(display_name);
		gdouble min_pct = get_min_brightness_percentage(display_name);
		gdouble max_pct = get_max_brightness_percentage(display_name);
		display_section_instance->min_percentage = min_pct;
		display_section_instance->max_percentage = max_pct;
		// Look up custom name, fall back to default
		gchar *custom_name = get_custom_display_name(display_name);
		const gchar *shown_name = custom_name != NULL ? custom_name : display_name;
		display_section_instance->label = get_editable_label(
			shown_name, display_name,
			_on_display_renamed, GUINT_TO_POINTER(index));
		if (custom_name != NULL) g_free(custom_name);
		gdouble current_pct = get_display_brightness_percentage(index);
		gdouble clamped_current = CLAMP(current_pct, min_pct, max_pct);
		// If current brightness is outside the range, force it into range
		if (current_pct < min_pct || current_pct > max_pct) {
			set_display_brightness_percentage(index, clamped_current, FALSE);
		}
		// Create slider — for min==max (fixed), use 0-100 range but disable
		gboolean is_fixed = (min_pct == max_pct);
		GtkWidget *scale_widget;
		if (is_fixed) {
			scale_widget = get_display_brightness_scale(min_pct, 0, 100);
			gtk_widget_set_sensitive(scale_widget, FALSE);
		} else {
			scale_widget = get_display_brightness_scale(clamped_current, min_pct, max_pct);
		}
		display_section_instance->scale = scale_widget;
		g_signal_connect(display_section_instance->scale, "value-changed", G_CALLBACK(_update_display_brightness), GUINT_TO_POINTER(index));

		// Create clickable min/max labels
		// Min label: allowed range 0 to max
		display_section_instance->min_label = get_range_label(
			min_pct, 0, max_pct,
			_on_min_value_changed, GUINT_TO_POINTER(index));
		gtk_widget_set_visible(display_section_instance->min_label, FALSE);
		// Max label: allowed range min to 100
		display_section_instance->max_label = get_range_label(
			max_pct, min_pct, 100,
			_on_max_value_changed, GUINT_TO_POINTER(index));
		gtk_widget_set_visible(display_section_instance->max_label, FALSE);

		display_section_instance->separator_left_column = get_separator();
		display_section_instance->separator_right_column = get_separator();
		display_section_instance->icon = get_display_icon();
		sections[index] = display_section_instance;

		// Create a row: min_label + slider + max_label
		GtkWidget *scale_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
		gtk_widget_set_hexpand(scale_row, TRUE);
		gtk_widget_set_valign(scale_row, GTK_ALIGN_CENTER);
		gtk_box_append(GTK_BOX(scale_row), display_section_instance->min_label);
		gtk_widget_set_valign(display_section_instance->min_label, GTK_ALIGN_CENTER);
		gtk_box_append(GTK_BOX(scale_row), display_section_instance->scale);
		gtk_widget_set_valign(display_section_instance->scale, GTK_ALIGN_CENTER);
		gtk_box_append(GTK_BOX(scale_row), display_section_instance->max_label);
		gtk_widget_set_valign(display_section_instance->max_label, GTK_ALIGN_CENTER);
		display_section_instance->scale_row = scale_row;

		if (sibling == NULL) {
			gtk_grid_attach(GTK_GRID(grid), sections[index]->label, 1, 0, 1, 1);
		} else {
			gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->label, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);
		}
		gtk_grid_attach_next_to(GTK_GRID(grid), scale_row, sections[index]->label, GTK_POS_BOTTOM, 1, 1);
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->icon, sections[index]->label, GTK_POS_LEFT, 1, 2);
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->separator_left_column, sections[index]->icon, GTK_POS_BOTTOM, 1, 1);
		gtk_grid_attach_next_to(GTK_GRID(grid), sections[index]->separator_right_column, scale_row, GTK_POS_BOTTOM, 1, 1);
		sibling = sections[index];
	}

	link_brightness_checkbox = get_link_brightness_checkbox(get_is_brightness_linked());
	gtk_widget_set_hexpand(link_brightness_checkbox, TRUE);
	_sync_exact_checkbox = link_brightness_checkbox;
	gtk_grid_attach_next_to(GTK_GRID(grid), link_brightness_checkbox, sibling->separator_right_column, GTK_POS_BOTTOM, 1, 1);
	g_signal_connect(link_brightness_checkbox, "toggled", G_CALLBACK(_link_brightness), NULL);

	// Proportional sync checkbox
	GtkWidget *prop_sync_checkbox = gtk_check_button_new_with_label("Sync brightness: proportional");
	gtk_widget_set_margin_start(prop_sync_checkbox, MARGIN_UNIT);
	gtk_widget_set_margin_end(prop_sync_checkbox, MARGIN_UNIT);
	gtk_widget_set_margin_top(prop_sync_checkbox, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(prop_sync_checkbox, MARGIN_UNIT);
	gtk_widget_set_halign(prop_sync_checkbox, GTK_ALIGN_END);
	gtk_widget_set_hexpand(prop_sync_checkbox, TRUE);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(prop_sync_checkbox), get_is_brightness_proportionally_linked());
	_sync_proportional_checkbox = prop_sync_checkbox;
	gtk_grid_attach_next_to(GTK_GRID(grid), prop_sync_checkbox, link_brightness_checkbox, GTK_POS_BOTTOM, 1, 1);
	g_signal_connect(prop_sync_checkbox, "toggled", G_CALLBACK(_link_proportional_brightness), NULL);

	// "Set brightness range" toggle — defaults to off, shows min/max labels when on
	GtkWidget *range_toggle = gtk_check_button_new_with_label("Set brightness range");
	gtk_widget_add_css_class(range_toggle, "flat");
	_show_brightness_range = FALSE;
	gtk_check_button_set_active(GTK_CHECK_BUTTON(range_toggle), FALSE);
	gtk_widget_set_margin_start(range_toggle, MARGIN_UNIT);
	gtk_widget_set_margin_end(range_toggle, MARGIN_UNIT);
	gtk_widget_set_margin_top(range_toggle, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(range_toggle, MARGIN_UNIT);
	gtk_widget_set_halign(range_toggle, GTK_ALIGN_END);
	gtk_widget_set_hexpand(range_toggle, TRUE);
	gtk_grid_attach_next_to(GTK_GRID(grid), range_toggle, prop_sync_checkbox, GTK_POS_BOTTOM, 1, 1);
	g_signal_connect(range_toggle, "toggled", G_CALLBACK(_toggle_brightness_range), NULL);

	return main_box;
}
