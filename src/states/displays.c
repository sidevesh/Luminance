#ifndef BRIGHTNESS_CODE
#include "../../ddcbc-api/ddcbc-api.c"
#endif

#ifndef DISPLAYS_STATE
#define DISPLAYS_STATE

#define MINIMUM_REFRESH_TIME_IN_SECONDS 2

gboolean _is_displays_loading = TRUE;
ddcbc_display_list *_display_list;
ddcbc_display_list _display_list_instance;
int _last_displays_load_time = 0;

extern void update_window_contents();

void _initialize_displays(gboolean first_time_loading) {
	// save current epoch time before loading display list
	time_t start_loading_displays_time;
	time(&start_loading_displays_time);

	if (!first_time_loading) {
		ddcbc_display_list_free(_display_list);
	}

  _display_list_instance = ddcbc_display_list_init(FALSE);
  _display_list = &_display_list_instance;

	time_t end_loading_displays_time;
	time(&end_loading_displays_time);
	_last_displays_load_time = difftime(end_loading_displays_time, start_loading_displays_time);
}

void _start_refreshing_displays_with_ui_updates() {
	_is_displays_loading = TRUE;
	update_window_contents();
}

void _finish_refreshing_displays_with_ui_updates() {
	_is_displays_loading = FALSE;
	update_window_contents();
}

// TODO: Make this not depend on the UI and rather only handle the display list
void _refresh_displays_with_ui_updates(gboolean first_time_loading) {
	if (!first_time_loading) {
		_start_refreshing_displays_with_ui_updates();
	}

	_initialize_displays(first_time_loading);

	// if the time it took to load the display list is less than a minimum amount of time, wait for the remaining time
	if (_last_displays_load_time < MINIMUM_REFRESH_TIME_IN_SECONDS) {
		g_timeout_add_once((MINIMUM_REFRESH_TIME_IN_SECONDS - _last_displays_load_time) * 1000, (GSourceOnceFunc) _finish_refreshing_displays_with_ui_updates, NULL);
	} else {
		_finish_refreshing_displays_with_ui_updates();
	}
}

void load_displays() {
	_initialize_displays(TRUE);
}

void reload_displays() {
	_initialize_displays(FALSE);
}

void free_displays() {
  ddcbc_display_list_free(_display_list);
}

gboolean is_displays_loading() {
  return _is_displays_loading;
}

int displays_count() {
  return _display_list->ct;
}

ddcbc_display* get_display(int index) {
  return ddcbc_display_list_get(_display_list, index);
}

int last_displays_load_time() {
	return _last_displays_load_time;
}

void load_displays_with_ui_updates() {
  _refresh_displays_with_ui_updates(TRUE);
}

void reload_displays_with_ui_updates() {
  _refresh_displays_with_ui_updates(FALSE);
}

#endif
