#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

GtkWidget *get_display_brightness_scale(gdouble last_value, gdouble max_value);
GtkWidget *get_display_icon();
GtkWidget *get_display_label(gchar *text);
GtkWidget *get_link_brightness_checkbox(gboolean initial_value);
GtkWidget *get_separator();

#endif // UI_COMPONENTS_H
