#ifndef BRIGHTNESS_CODE
#include "../../ddcbc-api/ddcbc-api.c"
#endif

#ifndef DISPLAY_LIST_STATE
#define DISPLAY_LIST_STATE

#define MINIMUM_REFRESH_TIME_IN_SECONDS 2

gboolean _is_display_list_loading = FALSE;
ddcbc_display_list *_display_list;
ddcbc_display_list _display_list_instance;

extern void update_window_contents();

void initialize_display_list() {
  _display_list_instance = ddcbc_display_list_init(FALSE);
  _display_list = &_display_list_instance;
}

void free_display_list() {
  ddcbc_display_list_free(_display_list);
}

void _start_refreshing_displays() {
	_is_display_list_loading = TRUE;
	update_window_contents();
}

void _finish_refreshing_displays() {
	_is_display_list_loading = FALSE;
	update_window_contents();
}

void refresh_displays() {
	_start_refreshing_displays();

	// save current epoch time before loading display list
	time_t start_loading_display_list_time;
	time(&start_loading_display_list_time);
	
	free_display_list();
	_display_list_instance = ddcbc_display_list_init(FALSE);
	_display_list = &_display_list_instance;

	// if the time it took to load the display list is less than a minimum amount of time, wait for the remaining time
	time_t end_loading_display_list_time;
	time(&end_loading_display_list_time);
	int time_elapsed = difftime(end_loading_display_list_time, start_loading_display_list_time);

	if (time_elapsed < MINIMUM_REFRESH_TIME_IN_SECONDS) {
		g_timeout_add_seconds(MINIMUM_REFRESH_TIME_IN_SECONDS - time_elapsed, (GSourceFunc) _finish_refreshing_displays, NULL);
	} else {
		_finish_refreshing_displays();
	}
}

gboolean is_display_list_loading() {
  return _is_display_list_loading;
}

int display_list_count() {
  return _display_list->ct;
}

ddcbc_display* get_display(int index) {
  return ddcbc_display_list_get(_display_list, index);
}

#endif
