#include <gtk/gtk.h>

#ifndef BRIGHTNESS_CODE
#include "../../ddcbc-api/ddcbc-api.c"
#endif

#ifndef DISPLAY_SECTION
#define DISPLAY_SECTION
typedef struct display_section {
  GtkWidget *icon;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *separator_left_column;
  GtkWidget *separator_right_column;
  ddcbc_display *display;
} display_section;
#endif
