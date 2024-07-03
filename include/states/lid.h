#ifndef LAPTOP_LID_H
#define LAPTOP_LID_H

#include <gtk/gtk.h>

gboolean get_has_lid();
gboolean is_lid_open();

gboolean get_should_hide_internal_if_lid_closed();
void set_should_hide_internal_if_lid_closed (gboolean value);

#endif // LAPTOP_LID_H
