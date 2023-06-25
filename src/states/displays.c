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
void (*_on_refresh_completed_callback)() = NULL;

void _initialize_displays(gboolean first_time_loading) {
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

void _on_refresh_displays_completed_callback_wrapper() {
	_is_displays_loading = FALSE;
	_on_refresh_completed_callback();
	_on_refresh_completed_callback = NULL;
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
			g_timeout_add_once((MINIMUM_REFRESH_TIME_IN_SECONDS - _last_displays_load_time) * 1000, (GSourceOnceFunc) _on_refresh_displays_completed_callback_wrapper, NULL);
		} else {
			_on_refresh_displays_completed_callback_wrapper();
		}
	} else {
		_is_displays_loading = FALSE;
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

int displays_count() {
  return _display_list->ct;
}

ddcbc_display* get_display(guint index) {
  return ddcbc_display_list_get(_display_list, index);
}

int last_displays_load_time() {
	return _last_displays_load_time;
}

#endif
