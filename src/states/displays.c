#include "states/displays.h"

#include "constants.h"

#include "ddcbc-api.h"

#include "states/lid.h"

// Array structure for display indexes
typedef struct {
    guint type;
    guint index;
} GlobalDisplayIndex;

// Declare the display indexes array
static GlobalDisplayIndex _display_indexes[MAX_DISPLAYS];
static guint _display_indexes_count = 0;
static gchar* _internal_backlight_display_directories[MAX_INTERNAL_BACKLIGHT_DISPLAYS];
static guint _internal_backlight_display_directories_count = 0;

static gboolean _is_displays_loading = TRUE;
static ddcbc_display_list *_display_list;
static ddcbc_display_list _display_list_instance;

static int _last_displays_load_time = 0;
static void (*_on_refresh_completed_callback)() = NULL;

static guint _on_refresh_displays_completed_callback_timeout_id = 0;

void _initialize_displays(gboolean first_time_loading) {
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
          strcmp(entry->d_name, ".") != 0 &&
          strcmp(entry->d_name, "..") != 0
        ) {
          _internal_backlight_display_directories[internal_backlight_count] = strdup(entry->d_name);
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

  if (!first_time_loading) {
    ddcbc_display_list_free(_display_list);
  }

  _display_list_instance = ddcbc_display_list_init(FALSE);
  _display_list = &_display_list_instance;

	// insert all DDC displays into the _display_indexes array
	guint ddc_display_count = 0;
	for (guint index = 0; index < _display_list->ct; index++) {
		_display_indexes[internal_backlight_count + ddc_display_count].type = DISPLAY_TYPE_DDC;
		_display_indexes[internal_backlight_count + ddc_display_count].index = index;
		ddc_display_count++;
    total_displays_count++;
	}

  _display_indexes_count = total_displays_count;

  time_t end_loading_displays_time;
  time(&end_loading_displays_time);
  _last_displays_load_time = difftime(end_loading_displays_time, start_loading_displays_time);
}

void _on_refresh_displays_completed_callback_wrapper() {
  if (_on_refresh_displays_completed_callback_timeout_id != 0) {
    g_source_remove(_on_refresh_displays_completed_callback_timeout_id);
    _on_refresh_displays_completed_callback_timeout_id = 0;
  }
  _is_displays_loading = FALSE;
  if (_on_refresh_completed_callback != NULL) {
    _on_refresh_completed_callback();
    _on_refresh_completed_callback = NULL;
  }
}

void _refresh_displays_with_callbacks(gboolean first_time_loading, void (*on_refresh_started_callback)(), void (*on_refresh_completed_callback)()) {
  if (!first_time_loading) {
    _is_displays_loading = TRUE;
    if (on_refresh_started_callback != NULL) {
      on_refresh_started_callback();
    }
  }

  _initialize_displays(first_time_loading);

  if (on_refresh_completed_callback != NULL) {
    _on_refresh_completed_callback = on_refresh_completed_callback;
    // if the time it took to load the display list is less than a minimum amount of time, wait for the remaining time
    if (_last_displays_load_time < MINIMUM_REFRESH_TIME_IN_SECONDS) {
      _on_refresh_displays_completed_callback_timeout_id = g_timeout_add((MINIMUM_REFRESH_TIME_IN_SECONDS - _last_displays_load_time) * 1000, (GSourceFunc)_on_refresh_displays_completed_callback_wrapper, NULL);
    } else {
      _on_refresh_displays_completed_callback_wrapper();
    }
  } else {
    _is_displays_loading = FALSE;
  }
}

ddcbc_display* _get_ddcbc_display(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
  return ddcbc_display_list_get(_display_list, display_index.index);
}

guint16 _get_display_max_brightness_value(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar max_brightness_file_path[256];
    snprintf(max_brightness_file_path, sizeof(max_brightness_file_path), "/sys/class/backlight/%s/max_brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(max_brightness_file_path, "r");
    if (file != NULL) {
        guint max_brightness;
        fscanf(file, "%u", &max_brightness);
        fclose(file);
        return max_brightness;
    }
  } else {
    ddcbc_display* display = _get_ddcbc_display(index);
    return display->max_val;
  }
}

void load_displays(void (*on_load_started_callback)(), void (*on_load_completed_callback)()) {
  _refresh_displays_with_callbacks(TRUE, on_load_started_callback, on_load_completed_callback);
}

void reload_displays(void (*on_reload_started_callback)(), void (*on_reload_completed_callback)()) {
  _refresh_displays_with_callbacks(FALSE, on_reload_started_callback, on_reload_completed_callback);
}

void free_displays() {
  ddcbc_display_list_free(_display_list);
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
			return "Built-in display";
		}
		gchar *display_name = malloc(256);
		snprintf(display_name, 256, "Built-in display %d", display_index.index + 1);
		return display_name;
  } else {
    ddcbc_display* display = _get_ddcbc_display(index);
    return display->info.model_name;
  }
}

gdouble get_display_brightness_percentage(guint index) {
  GlobalDisplayIndex display_index = _display_indexes[index];
	guint16 max_brightness_value = _get_display_max_brightness_value(index);
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar brightness_file_path[256];
    snprintf(brightness_file_path, sizeof(brightness_file_path), "/sys/class/backlight/%s/brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(brightness_file_path, "r");
    if (file != NULL) {
      guint brightness;
			fscanf(file, "%u", &brightness);
      fclose(file);
      return brightness * 100.0 / max_brightness_value;
    }
  }
  ddcbc_display* display = _get_ddcbc_display(index);
  return display->last_val * 100.0 / max_brightness_value;
}

void set_display_brightness_percentage(guint index, gdouble brightness_percentage) {
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
    ddcbc_display* display = _get_ddcbc_display(index);
    DDCBC_Status rc = ddcbc_display_set_brightness(display, brightness_value);

    if (rc == 1) {
      fprintf(stderr, "Partial success in setting the brightness of display no %d to %u. Code: %d\n",
        display->info.dispno, brightness_value, rc);
    } else if (rc != 0) {
      fprintf(stderr, "An error occurred when setting the brightness of display no %d to %u. Code: %d\n",
        display->info.dispno, brightness_value, rc);
    }
  }
}

