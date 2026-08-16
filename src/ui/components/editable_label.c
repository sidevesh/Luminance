#include <gtk/gtk.h>
#include "../constants/main.c"

#ifndef EDITABLE_LABEL_COMPONENT
#define EDITABLE_LABEL_COMPONENT

typedef struct {
  GtkWidget *stack;
  GtkWidget *label;
  GtkWidget *entry;
  gchar *default_text;
  void (*on_renamed)(const gchar *new_name, gpointer user_data);
  gpointer user_data;
} EditableLabel;

static void _editable_label_show_label(EditableLabel *el) {
  gtk_stack_set_visible_child(GTK_STACK(el->stack), el->label);
}

static void _editable_label_commit(EditableLabel *el) {
  const gchar *text = gtk_editable_get_text(GTK_EDITABLE(el->entry));
  const gchar *final_name;

  if (strlen(text) == 0) {
    // Empty = revert to default
    final_name = el->default_text;
  } else {
    final_name = text;
  }

  gtk_label_set_text(GTK_LABEL(el->label), final_name);
  _editable_label_show_label(el);

  if (el->on_renamed != NULL) {
    el->on_renamed(final_name, el->user_data);
  }
}

static void _on_entry_activate(GtkEntry *entry, gpointer user_data) {
  (void)entry;
  _editable_label_commit((EditableLabel *)user_data);
}

static gboolean _on_entry_focus_out(GtkEventControllerFocus *controller, gpointer user_data) {
  (void)controller;
  _editable_label_commit((EditableLabel *)user_data);
  return FALSE;
}

static gboolean _on_entry_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
  (void)controller;
  (void)keycode;
  (void)state;
  EditableLabel *el = (EditableLabel *)user_data;
  if (keyval == GDK_KEY_Escape) {
    _editable_label_show_label(el);
    return TRUE;
  }
  return FALSE;
}

static void _on_label_double_click(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
  (void)gesture;
  (void)x;
  (void)y;
  if (n_press != 2) return;

  EditableLabel *el = (EditableLabel *)user_data;
  gtk_editable_set_text(GTK_EDITABLE(el->entry), gtk_label_get_text(GTK_LABEL(el->label)));
  gtk_stack_set_visible_child(GTK_STACK(el->stack), el->entry);
  gtk_widget_grab_focus(el->entry);
}

GtkWidget* get_editable_label(const gchar *text, const gchar *default_text, void (*on_renamed)(const gchar *, gpointer), gpointer user_data) {
  EditableLabel *el = malloc(sizeof(EditableLabel));
  el->default_text = g_strdup(default_text);
  el->on_renamed = on_renamed;
  el->user_data = user_data;

  el->stack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(el->stack), GTK_STACK_TRANSITION_TYPE_NONE);

  el->label = gtk_label_new(text);
  gtk_label_set_xalign(GTK_LABEL(el->label), 0.0);
  gtk_widget_set_hexpand(el->label, FALSE);
  gtk_widget_set_margin_start(el->label, MARGIN_UNIT);
  gtk_widget_set_margin_end(el->label, MARGIN_UNIT);
  gtk_widget_set_margin_top(el->label, MARGIN_UNIT);

  el->entry = gtk_entry_new();
  gtk_editable_set_width_chars(GTK_EDITABLE(el->entry), 20);
  gtk_widget_set_margin_start(el->entry, MARGIN_UNIT);
  gtk_widget_set_margin_end(el->entry, MARGIN_UNIT);
  gtk_widget_set_margin_top(el->entry, MARGIN_UNIT);

  gtk_stack_add_named(GTK_STACK(el->stack), el->label, "label");
  gtk_stack_add_named(GTK_STACK(el->stack), el->entry, "entry");

  // Double-click on label to edit
  GtkGesture *click = gtk_gesture_click_new();
  g_signal_connect(click, "pressed", G_CALLBACK(_on_label_double_click), el);
  gtk_widget_add_controller(el->label, GTK_EVENT_CONTROLLER(click));

  // Entry signals
  g_signal_connect(el->entry, "activate", G_CALLBACK(_on_entry_activate), el);

  GtkEventController *focus = gtk_event_controller_focus_new();
  g_signal_connect(focus, "leave", G_CALLBACK(_on_entry_focus_out), el);
  gtk_widget_add_controller(el->entry, focus);

  GtkEventController *keys = gtk_event_controller_key_new();
  g_signal_connect(keys, "key-pressed", G_CALLBACK(_on_entry_key_pressed), el);
  gtk_widget_add_controller(el->entry, keys);

  g_object_set_data(G_OBJECT(el->stack), "editable-label", el);

  return el->stack;
}

#endif
