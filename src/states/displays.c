#include "../constants/main.c"

#ifndef BRIGHTNESS_CODE
#include "../../ddc/ddc.c"
#endif

#include "./laptop_lid.c"
#include "./should_hide_internal_if_lid_closed.c"

#ifndef DISPLAYS_STATE
#define DISPLAYS_STATE

#ifndef DT_DIR
#define DT_DIR 4
#endif

// Define constants for display types
#define DISPLAY_TYPE_DDC 0
#define DISPLAY_TYPE_INTERNAL_BACKLIGHT 1

#define MAX_DISPLAYS 20
#define MAX_INTERNAL_BACKLIGHT_DISPLAYS 10

// Array structure for display indexes
typedef struct {
    guint type;
    guint index;
} GlobalDisplayIndex;

// Declare the display indexes array
GlobalDisplayIndex _display_indexes[MAX_DISPLAYS];
guint _display_indexes_count = 0;
gchar* _internal_backlight_display_directories[MAX_INTERNAL_BACKLIGHT_DISPLAYS];
guint _internal_backlight_display_directories_count = 0;

gboolean _is_displays_loading = TRUE;
ddc_display_list *_display_list;
ddc_display_list _display_list_instance;
int _last_displays_load_time = 0;
void (*_on_refresh_completed_callback)() = NULL;

guint _on_refresh_displays_completed_callback_timeout_id = 0;

void _initialize_displays(void) {
  time_t start_loading_displays_time;
  time(&start_loading_displays_time);

  guint total_displays_count = 0;
  guint internal_backlight_count = 0;

  if (get_has_lid() == FALSE || get_should_hide_internal_if_lid_closed() == FALSE || is_lid_open() == TRUE) {
    DIR* dir = opendir("/sys/class/backlight");
    if (dir != NULL) {
      struct dirent* entry;
      while ((entry = readdir(dir)) != NULL && internal_backlight_count < MAX_INTERNAL_BACKLIGHT_DISPLAYS) {
        if (
          g_strcmp0(entry->d_name, ".") != 0 &&
          g_strcmp0(entry->d_name, "..") != 0
        ) {
          _internal_backlight_display_directories[internal_backlight_count] = g_strdup(entry->d_name);
          // insert the internal backlight display into the _display_indexes array
          _display_indexes[internal_backlight_count].type = DISPLAY_TYPE_INTERNAL_BACKLIGHT;
          _display_indexes[internal_backlight_count].index = internal_backlight_count;
          internal_backlight_count++;
          total_displays_count++;
        }
      }
      _internal_backlight_display_directories_count = internal_backlight_count;
      closedir(dir);
    }
  }

  // Always close previously opened displays before enumerating again: ddcutil
  // refuses to open a display this process still holds open ("already locked
  // by current thread"), which would silently drop it from the new list.
  if (_display_list != NULL) {
    ddc_display_list_free(_display_list);
    _display_list = NULL;
  }

  _display_list_instance = ddc_display_list_init(FALSE);
  _display_list = &_display_list_instance;

	// insert all DDC displays into the _display_indexes array
	guint ddc_display_count = 0;
  if (_display_list != NULL && _display_list->list != NULL) {
    for (guint index = 0; index < _display_list->ct; index++) {
      _display_indexes[internal_backlight_count + ddc_display_count].type = DISPLAY_TYPE_DDC;
      _display_indexes[internal_backlight_count + ddc_display_count].index = index;
      ddc_display_count++;
      total_displays_count++;
    }
  }

  _display_indexes_count = total_displays_count;

  time_t end_loading_displays_time;
  time(&end_loading_displays_time);
  _last_displays_load_time = difftime(end_loading_displays_time, start_loading_displays_time);
}

void _on_refresh_displays_completed_callback_wrapper();

gboolean _on_refresh_displays_completed_callback_wrapper_for_timeout(gpointer data) {
  (void)data;
  _on_refresh_displays_completed_callback_wrapper();
  return G_SOURCE_REMOVE;
}

void _on_refresh_displays_completed_callback_wrapper() {
  if (_on_refresh_displays_completed_callback_timeout_id != 0) {
    g_source_remove(_on_refresh_displays_completed_callback_timeout_id);
    _on_refresh_displays_completed_callback_timeout_id = 0;
  }
  _is_displays_loading = FALSE;
  if (_on_refresh_completed_callback != NULL) {
    void (*callback)() = _on_refresh_completed_callback;
    _on_refresh_completed_callback = NULL;
    callback();
  }
}

void _refresh_displays_with_callbacks(gboolean first_time_loading, void (*on_refresh_started_callback)(), void (*on_refresh_completed_callback)()) {
  if (!first_time_loading) {
    _is_displays_loading = TRUE;
    if (on_refresh_started_callback != NULL) {
      on_refresh_started_callback();
    }
  }

  _initialize_displays();

  if (on_refresh_completed_callback != NULL) {
    _on_refresh_completed_callback = on_refresh_completed_callback;
    // if the time it took to load the display list is less than a minimum amount of time, wait for the remaining time
    if (_last_displays_load_time < MINIMUM_REFRESH_TIME_IN_SECONDS) {
      _on_refresh_displays_completed_callback_timeout_id = g_timeout_add((MINIMUM_REFRESH_TIME_IN_SECONDS - _last_displays_load_time) * 1000, _on_refresh_displays_completed_callback_wrapper_for_timeout, NULL);
    } else {
      _on_refresh_displays_completed_callback_wrapper();
    }
  } else {
    _is_displays_loading = FALSE;
  }
}

ddc_display* _get_ddc_display(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
  return ddc_display_list_get(_display_list, display_index.index);
}

guint16 _get_display_max_brightness_value(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar max_brightness_file_path[256];
    snprintf(max_brightness_file_path, sizeof(max_brightness_file_path), "/sys/class/backlight/%s/max_brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(max_brightness_file_path, "r");
    if (file != NULL) {
        guint max_brightness = 0;
        if (fscanf(file, "%u", &max_brightness) != 1) {
            max_brightness = 0;
        }
        fclose(file);
        return max_brightness;
    }
	return 0;
  } else {
    ddc_display* display = _get_ddc_display(index);
    return display->max_val;
  }
}

void reload_displays(void (*)(), void (*)());

static void (*_global_ui_callback)() = NULL;

void set_displays_update_callback(void (*callback)()) {
    _global_ui_callback = callback;
}

// Called when brightness/contrast was changed from outside the window (D-Bus:
// GNOME extension, CLI). Must only update values in place: rebuilding the
// window would destroy a slider the user may be dragging.
static void (*_values_changed_callback)() = NULL;

void set_display_values_changed_callback(void (*callback)()) {
    _values_changed_callback = callback;
}

void notify_display_values_changed(void) {
    if (_values_changed_callback != NULL) {
        _values_changed_callback();
    }
}

// Re-enumerate displays (e.g. after a monitor was plugged in) and rebuild the
// window, if any, once done.
void rescan_displays(void) {
    reload_displays(_global_ui_callback, _global_ui_callback);
}

static void on_lid_state_changed() {
    gboolean should_hide = get_should_hide_internal_if_lid_closed();
    if (should_hide) {
        g_print("Lid state changed and 'hide internal' is enabled. Reloading displays...\n");
        reload_displays(_global_ui_callback, _global_ui_callback);
    } else {
        g_print("Lid state changed but 'hide internal' is disabled. Ignoring.\n");
    }
}

void load_displays(void (*on_load_started_callback)(), void (*on_load_completed_callback)()) {
  static gboolean is_observer_registered = FALSE;
  if (!is_observer_registered) {
       register_lid_state_observer(on_lid_state_changed);
       is_observer_registered = TRUE;
  }
  _refresh_displays_with_callbacks(TRUE, on_load_started_callback, on_load_completed_callback);
}

void reload_displays(void (*on_reload_started_callback)(), void (*on_reload_completed_callback)()) {
  _refresh_displays_with_callbacks(FALSE, on_reload_started_callback, on_reload_completed_callback);
}

void free_displays() {
  if (_display_list != NULL) {
    ddc_display_list_free(_display_list);
    _display_list = NULL;
  }
}

gboolean is_displays_loading() {
  return _is_displays_loading;
}

guint displays_count() {
  return _display_indexes_count;
}

int last_displays_load_time() {
  return _last_displays_load_time;
}

char* get_display_name(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
		if (_internal_backlight_display_directories_count == 1) {
			return "Built-in Display";
		}
		gchar *display_name = malloc(256);
		snprintf(display_name, 256, "Built-in Display %d", display_index.index + 1);
		return display_name;
  } else {
    ddc_display* display = _get_ddc_display(index);
    return display->info.model_name;
  }
}

// Re-read brightness/contrast of already opened DDC displays without
// re-enumerating them. Internal backlights are read from sysfs on demand.
void refresh_display_values(void) {
  for (guint i = 0; i < _display_indexes_count; i++) {
    if (_display_indexes[i].type != DISPLAY_TYPE_DDC) continue;
    ddc_display* display = _get_ddc_display(i);
    ddc_display_get_brightness(display);
    if (display->has_contrast) ddc_display_get_contrast(display);
  }
}

gboolean get_display_is_internal(guint index) {
  return _display_indexes[index].type == DISPLAY_TYPE_INTERNAL_BACKLIGHT;
}

// EDID identity of a DDC display (vendor PNP id, model name, serial string),
// used by the GNOME extension to match DDC displays to Mutter monitors.
// Returns FALSE for internal backlight displays. Strings are owned by the
// display list and stay valid until the next display reload.
gboolean get_display_edid_identity(guint index, const char **vendor, const char **model, const char **serial) {
  if (get_display_is_internal(index)) return FALSE;
  ddc_display* display = _get_ddc_display(index);
  *vendor = display->info.mfg_id;
  *model = display->info.model_name;
  *serial = display->info.sn;
  return TRUE;
}

gdouble get_display_brightness_percentage(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
	guint16 max_brightness_value = _get_display_max_brightness_value(index);
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar brightness_file_path[256];
    snprintf(brightness_file_path, sizeof(brightness_file_path), "/sys/class/backlight/%s/brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(brightness_file_path, "r");
    if (file != NULL) {
      guint brightness = 0;
			if (fscanf(file, "%u", &brightness) != 1) {
        brightness = 0;
      }
      fclose(file);
      return brightness * 100.0 / max_brightness_value;
    }

	  return 0.0;
  } else {
    ddc_display* display = _get_ddc_display(index);
    return display->last_val * 100.0 / max_brightness_value;
  }
}

void set_display_brightness_percentage(guint index, gdouble brightness_percentage, gboolean emit_osd_signal) {
  GlobalDisplayIndex display_index = _display_indexes[index];
	guint16 max_brightness_value = _get_display_max_brightness_value(index);
	guint16 brightness_value = (guint16)(brightness_percentage * max_brightness_value / 100);
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar brightness_file_path[256];
    snprintf(brightness_file_path, sizeof(brightness_file_path), "/sys/class/backlight/%s/brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(brightness_file_path, "w");
    if (file != NULL) {
      fprintf(file, "%u", brightness_value);
      fclose(file);
    } else {
      fprintf(stderr, "Error opening brightness file for writing: %s\n", brightness_file_path);
    }
  } else {
    ddc_display* display = _get_ddc_display(index);
    DDC_Status rc = ddc_display_set_brightness(display, brightness_value);

    if (rc == 1) {
      fprintf(stderr, "Partial success in setting the brightness of display no %d to %u. Code: %d\n",
        display->info.dispno, brightness_value, rc);
    } else if (rc != 0) {
      fprintf(stderr, "An error occurred when setting the brightness of display no %d to %u. Code: %d\n",
        display->info.dispno, brightness_value, rc);
    }
  }

  // Emit OSD signal
  if (emit_osd_signal) {
    gchar model_name[256];
    if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
      snprintf(model_name, sizeof(model_name), "Built-in Display");
    } else {
      ddc_display* display = _get_ddc_display(index);
      if (display)
          snprintf(model_name, sizeof(model_name), "%s", display->info.model_name);
      else
          snprintf(model_name, sizeof(model_name), "Unknown Display"); 
    }
    emit_osd_signal_dbus(brightness_percentage, model_name);
  }
}

gboolean get_display_has_contrast(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
  if (display_index.type != DISPLAY_TYPE_DDC) {
    return FALSE;
  }
  ddc_display* display = _get_ddc_display(index);
  return display->has_contrast;
}

gdouble get_display_contrast_percentage(guint index) {
  ddc_display* display = _get_ddc_display(index);
  if (display->contrast_max_val == 0) {
    return 0.0;
  }
  return display->contrast_last_val * 100.0 / display->contrast_max_val;
}

void set_display_contrast_percentage(guint index, gdouble contrast_percentage, gboolean emit_osd_signal) {
  ddc_display* display = _get_ddc_display(index);
  guint16 contrast_value = (guint16)(contrast_percentage * display->contrast_max_val / 100);
  DDC_Status rc = ddc_display_set_contrast(display, contrast_value);

  if (rc == 1) {
    fprintf(stderr, "Partial success in setting the contrast of display no %d to %u. Code: %d\n",
      display->info.dispno, contrast_value, rc);
  } else if (rc != 0) {
    fprintf(stderr, "An error occurred when setting the contrast of display no %d to %u. Code: %d\n",
      display->info.dispno, contrast_value, rc);
  }

  if (emit_osd_signal) {
    emit_contrast_osd_signal_dbus(contrast_percentage, display->info.model_name);
  }
}

gboolean have_internal_displays_without_permission_in_flatpak() {
  if (is_running_in_flatpak() == FALSE) {
    return FALSE;
  }
  
  if (_internal_backlight_display_directories_count == 0) {
    return FALSE;
  }

  for (guint i = 0; i < _internal_backlight_display_directories_count; i++) {
    gchar brightness_file_path[256];
    snprintf(brightness_file_path, sizeof(brightness_file_path), "/sys/class/backlight/%s/brightness", _internal_backlight_display_directories[i]);
    if (access(brightness_file_path, W_OK) != 0) {
      return TRUE;
    }
  }

  return FALSE;
}

#endif
