#ifndef UI_WINDOW_H
#define UI_WINDOW_H

void _on_should_hide_internal_if_lid_closed_checkbox_toggled (GtkCheckButton * widget,gpointer data);
void _on_window_refresh_button_clicked (GtkWidget * widget,gpointer data);
void _open_about_dialog_and_close_popover ();
void initialize_application_window (GtkApplication * app);
void update_window_content_screen (GtkWidget * new_window_content_screen);

#endif // UI_WINDOW_H
