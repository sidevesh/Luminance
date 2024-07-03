#ifndef STATES_DISPLAY_H
#define STATES_DISPLAY_H

#include "ddcbc-api.h"

// Define constants for display types
#define DISPLAY_TYPE_DDC 0
#define DISPLAY_TYPE_INTERNAL_BACKLIGHT 1

#define MAX_DISPLAYS 20
#define MAX_INTERNAL_BACKLIGHT_DISPLAYS 10

char * get_display_name (guint index);
ddcbc_display * _get_ddcbc_display (guint index);
gboolean is_displays_loading ();
gdouble get_display_brightness_percentage (guint index);
guint16 _get_display_max_brightness_value (guint index);
int displays_count ();
int last_displays_load_time ();
void _initialize_displays (gboolean first_time_loading);
void _on_refresh_displays_completed_callback_wrapper ();
void _refresh_displays_with_callbacks (gboolean first_time_loading,void (* on_refresh_started_callback)(),void (* on_refresh_completed_callback)());
void free_displays ();
void load_displays (void (* on_load_started_callback)(),void (* on_load_completed_callback)());
void reload_displays (void (* on_reload_started_callback)(),void (* on_reload_completed_callback)());
void set_display_brightness_percentage (guint index,gdouble brightness_percentage);

#endif // STATES_DISPLAY_H
