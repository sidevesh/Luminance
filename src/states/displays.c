#include "../constants/main.c"

#ifndef BRIGHTNESS_CODE
#include "../../ddcbc-api/ddcbc-api.c"
#endif

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
GlobalDisplayIndex display_indexes[MAX_DISPLAYS];
gchar* _internal_backlight_display_directories[MAX_INTERNAL_BACKLIGHT_DISPLAYS];
guint _internal_backlight_display_directories_count = 0;

gboolean _is_displays_loading = TRUE;
ddcbc_display_list *_display_list;
ddcbc_display_list _display_list_instance;
int _last_displays_load_time = 0;
void (*_on_refresh_completed_callback)() = NULL;

void _initialize_displays(gboolean first_time_loading) {
  time_t start_loading_displays_time;
  time(&start_loading_displays_time);

  if (!first_time_loading) {
    ddcbc_display_list_free(_display_list);
  }

  _display_list_instance = ddcbc_display_list_init(FALSE);
  _display_list = &_display_list_instance;

  // Populate internal backlight display directories
  guint internal_backlight_count = 0;
  DIR* dir = opendir("/sys/class/backlight");
  if (dir != NULL) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && internal_backlight_count < MAX_INTERNAL_BACKLIGHT_DISPLAYS) {
			if (
				entry->d_type == DT_DIR &&
				strcmp(entry->d_name, ".") != 0 &&
				strcmp(entry->d_name, "..") != 0
			) {
        _internal_backlight_display_directories[internal_backlight_count] = strdup(entry->d_name);
        internal_backlight_count++;
      }
    }
		_internal_backlight_display_directories_count = internal_backlight_count;
    closedir(dir);
  }

  time_t end_loading_displays_time;
  time(&end_loading_displays_time);
  _last_displays_load_time = difftime(end_loading_displays_time, start_loading_displays_time);
}

void _on_refresh_displays_completed_callback_wrapper() {
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
      g_timeout_add_once((MINIMUM_REFRESH_TIME_IN_SECONDS - _last_displays_load_time) * 1000, (GSourceOnceFunc)_on_refresh_displays_completed_callback_wrapper, NULL);
    } else {
      _on_refresh_displays_completed_callback_wrapper();
    }
  } else {
    _is_displays_loading = FALSE;
  }
}

ddcbc_display* _get_ddcbc_display(guint index) {
  GlobalDisplayIndex display_index = display_indexes[index];
  return ddcbc_display_list_get(_display_list, display_index.index);
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

int displays_count() {
  return _display_list->ct + _internal_backlight_display_directories_count;
}

int last_displays_load_time() {
  return _last_displays_load_time;
}

char* get_display_name(guint index) {
  GlobalDisplayIndex display_index = display_indexes[index];
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    return "Built-in display %d", display_index.index + 1;
  } else {
    ddcbc_display* display = _get_ddcbc_display(index);
    return display->info.model_name;
  }
}

int get_display_brightness(guint index) {
  GlobalDisplayIndex display_index = display_indexes[index];
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar brightness_file_path[256];
    snprintf(brightness_file_path, sizeof(brightness_file_path), "/sys/class/backlight/%s/brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(brightness_file_path, "r");
    if (file != NULL) {
      guint brightness;
      fscanf(file, "%u", &brightness);
      fclose(file);
      return brightness;
    }
  } else {
    ddcbc_display* display = _get_ddcbc_display(index);
    return display->last_val;
  }
}

int get_display_max_brightness(guint index) {
  GlobalDisplayIndex display_index = display_indexes[index];
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

void set_display_brightness(guint index, guint16 brightness) {
  GlobalDisplayIndex display_index = display_indexes[index];
  if (display_index.type == DISPLAY_TYPE_INTERNAL_BACKLIGHT) {
    gchar brightness_file_path[256];
    snprintf(brightness_file_path, sizeof(brightness_file_path), "/sys/class/backlight/%s/brightness", _internal_backlight_display_directories[display_index.index]);

    FILE* file = fopen(brightness_file_path, "w");
    if (file != NULL) {
      fprintf(file, "%u", brightness);
      fclose(file);
    } else {
      fprintf(stderr, "Error opening brightness file for writing: %s\n", brightness_file_path);
    }
  } else {
    ddcbc_display* display = _get_ddcbc_display(index);
    DDCBC_Status rc = ddcbc_display_set_brightness(display, brightness);

    if (rc == 1) {
      fprintf(stderr, "Partial success in setting the brightness of display no %d to %u. Code: %d\n",
        display->info.dispno, brightness, rc);
    } else if (rc != 0) {
      fprintf(stderr, "An error occurred when setting the brightness of display no %d to %u. Code: %d\n",
        display->info.dispno, brightness, rc);
    }
  }
}

#endif
