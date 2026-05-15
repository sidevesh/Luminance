#include <gtk/gtk.h>
#include "../constants/main.c"

#ifndef RANGE_LABEL_COMPONENT
#define RANGE_LABEL_COMPONENT

typedef struct {
  GtkWidget *box;       // Contains either label or entry
  GtkWidget *label;     // The visible label (e.g., "60%")
  GtkWidget *entry;     // Hidden until clicked
  gdouble current_value;
  gdouble min_allowed;
  gdouble max_allowed;
  void (*on_value_changed)(gdouble new_value, gpointer user_data);
  gpointer user_data;
} RangeLabel;

static void _range_label_show_label(RangeLabel *rl) {
  gtk_widget_set_visible(rl->entry, FALSE);
  gtk_widget_set_visible(rl->label, TRUE);
}

static void _range_label_show_entry(RangeLabel *rl) {
  gchar text[16];
  snprintf(text, sizeof(text), "%d", (int)rl->current_value);
  gtk_editable_set_text(GTK_EDITABLE(rl->entry), text);
  gtk_widget_set_visible(rl->label, FALSE);
  gtk_widget_set_visible(rl->entry, TRUE);
  gtk_widget_grab_focus(rl->entry);
}

static void _range_label_commit(RangeLabel *rl) {
  const gchar *text = gtk_editable_get_text(GTK_EDITABLE(rl->entry));
  gint new_value = atoi(text);

  // Validate
  if (new_value < (gint)rl->min_allowed || new_value > (gint)rl->max_allowed) {
    // Revert
    _range_label_show_label(rl);
    return;
  }

  gdouble new_pct = (gdouble)new_value;
  if (new_pct == rl->current_value) {
    _range_label_show_label(rl);
    return;
  }

  rl->current_value = new_pct;
  gchar label_text[16];
  snprintf(label_text, sizeof(label_text), "%d%%", (int)rl->current_value);
  gtk_label_set_text(GTK_LABEL(rl->label), label_text);

  _range_label_show_label(rl);

  if (rl->on_value_changed != NULL) {
    rl->on_value_changed(new_pct, rl->user_data);
  }
}

static void _on_range_label_entry_activate(GtkEntry *entry, gpointer user_data) {
  (void)entry;
  RangeLabel *rl = (RangeLabel *)user_data;
  _range_label_commit(rl);
}

static gboolean _on_range_label_entry_focus_out(GtkEventControllerFocus *controller, gpointer user_data) {
  (void)controller;
  RangeLabel *rl = (RangeLabel *)user_data;
  _range_label_commit(rl);
  return FALSE;
}

static gboolean _on_range_label_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
  (void)controller;
  (void)keycode;
  (void)state;
  RangeLabel *rl = (RangeLabel *)user_data;
  if (keyval == GDK_KEY_Escape) {
    _range_label_show_label(rl);
    return TRUE;
  }
  return FALSE;
}

static void _on_range_label_clicked(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
  RangeLabel *rl = (RangeLabel *)user_data;
  _range_label_show_entry(rl);
}

GtkWidget* get_range_label(gdouble value, gdouble min_allowed, gdouble max_allowed, void (*on_value_changed)(gdouble, gpointer), gpointer user_data) {
  RangeLabel *rl = malloc(sizeof(RangeLabel));
  rl->current_value = value;
  rl->min_allowed = min_allowed;
  rl->max_allowed = max_allowed;
  rl->on_value_changed = on_value_changed;
  rl->user_data = user_data;

  rl->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_valign(rl->box, GTK_ALIGN_CENTER);

  gchar label_text[16];
  snprintf(label_text, sizeof(label_text), "%d%%", (int)value);
  rl->label = gtk_label_new(label_text);
  gtk_widget_add_css_class(rl->label, "dim-label");
  gtk_widget_set_visible(rl->label, TRUE);
  gtk_label_set_width_chars(GTK_LABEL(rl->label), 4);
  gtk_label_set_max_width_chars(GTK_LABEL(rl->label), 4);

  rl->entry = gtk_entry_new();
  gtk_editable_set_width_chars(GTK_EDITABLE(rl->entry), 4);
  gtk_entry_set_max_length(GTK_ENTRY(rl->entry), 3);
  gtk_widget_set_visible(rl->entry, FALSE);

  // Style the entry to be minimal
  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_string(css,
    ".dim-label { font-size: 0.8em; opacity: 0.6; padding: 2px 4px; }"
    ".dim-label:hover { opacity: 1.0; text-decoration: underline; }");
  gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_box_append(GTK_BOX(rl->box), rl->label);
  gtk_box_append(GTK_BOX(rl->box), rl->entry);

  // Store pointer so update_range_label can find it
  g_object_set_data(G_OBJECT(rl->box), "range-label", rl);

  // Click on label to edit
  GtkGesture *click_gesture = gtk_gesture_click_new();
  g_signal_connect(click_gesture, "pressed", G_CALLBACK(_on_range_label_clicked), rl);
  gtk_widget_add_controller(rl->label, GTK_EVENT_CONTROLLER(click_gesture));

  // Entry signals
  g_signal_connect(rl->entry, "activate", G_CALLBACK(_on_range_label_entry_activate), rl);

  // Focus-out on entry
  GtkEventController *focus_controller = gtk_event_controller_focus_new();
  g_signal_connect(focus_controller, "leave", G_CALLBACK(_on_range_label_entry_focus_out), rl);
  gtk_widget_add_controller(rl->entry, focus_controller);

  // Escape key on entry
  GtkEventController *key_controller = gtk_event_controller_key_new();
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(_on_range_label_key_pressed), rl);
  gtk_widget_add_controller(rl->entry, key_controller);

  return rl->box;
}

// Update the allowed range and current value (e.g., when min/max changes)
void update_range_label(GtkWidget *range_label_box, gdouble new_value, gdouble new_min_allowed, gdouble new_max_allowed) {
  // Find the RangeLabel by walking the widget hierarchy — we store it as qdata
  RangeLabel *rl = (RangeLabel *)g_object_get_data(G_OBJECT(range_label_box), "range-label");
  if (rl == NULL) return;

  rl->min_allowed = new_min_allowed;
  rl->max_allowed = new_max_allowed;

  // Clamp current value to new range
  if (rl->current_value != new_value) {
    rl->current_value = new_value;
    gchar label_text[16];
    snprintf(label_text, sizeof(label_text), "%d%%", (int)rl->current_value);
    gtk_label_set_text(GTK_LABEL(rl->label), label_text);
  }
}

#endif
