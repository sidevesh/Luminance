#include <gtk/gtk.h>
#include <adwaita.h>
#include "../constants/main.c"
#include "../../states/displays.c"
#include "../../utils/flatpak.c"

extern void update_window_contents_in_ui();

void _on_no_displays_screen_refresh_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
	reload_displays(update_window_contents_in_ui, update_window_contents_in_ui);
}

void _on_no_displays_screen_setup_button_clicked(GtkWidget *widget, gpointer data) {
    (void)data;
    GtkWidget *dialog;
    GtkWidget *main_box;
    GtkWidget *header;
    GtkWidget *content_box;
    GtkWidget *intro_label;
    GtkWidget *step1_label;
    GtkWidget *link_button;
    GtkWidget *step2_label;
    GtkWidget *step3_label;
    GtkWidget *command_label;
    GtkCssProvider *css_provider;
    
    dialog = gtk_window_new();
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(widget)));
    gtk_window_set_title(GTK_WINDOW(dialog), "Flatpak Setup");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, -1);

    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), main_box);

    header = adw_header_bar_new();
    gtk_widget_add_css_class(header, "flat");
    gtk_window_set_titlebar(GTK_WINDOW(dialog), header);

    // CSS for the command well
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider,
        ".command-well { background-color: alpha(currentColor, 0.08); border-radius: 6px; padding: 6px 12px; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(css_provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(content_box, 24);
    gtk_widget_set_margin_bottom(content_box, 24);
    gtk_widget_set_margin_start(content_box, 24);
    gtk_widget_set_margin_end(content_box, 24);
    gtk_box_append(GTK_BOX(main_box), content_box);
    
    // Intro
    intro_label = gtk_label_new("To control monitors from a Flatpak app, you need to grant permissions on your host system.");
    gtk_label_set_wrap(GTK_LABEL(intro_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(intro_label), 0);
    gtk_box_append(GTK_BOX(content_box), intro_label);

    // Step 1
    step1_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(step1_label), "<b>1.</b> Download the setup script below:");
    gtk_label_set_xalign(GTK_LABEL(step1_label), 0);
    gtk_box_append(GTK_BOX(content_box), step1_label);

    link_button = gtk_link_button_new_with_label(APP_INFO_FLATPAK_SETUP_SCRIPT_URL, "Download Setup Script");
    gtk_widget_set_halign(link_button, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(content_box), link_button);

    // Step 2
    step2_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(step2_label), "<b>2.</b> Allow the file to be executed (Properties > Permissions > Allow executing file as program).");
    gtk_label_set_wrap(GTK_LABEL(step2_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(step2_label), 0);
    gtk_box_append(GTK_BOX(content_box), step2_label);

    // Step 3
    step3_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(step3_label), "<b>3.</b> Run the script (double-click or run from terminal).");
    gtk_label_set_wrap(GTK_LABEL(step3_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(step3_label), 0);
    gtk_box_append(GTK_BOX(content_box), step3_label);

    command_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(command_label), "<b>4.</b> Restart this app for changes to take effect.");
    gtk_label_set_xalign(GTK_LABEL(command_label), 0);
    gtk_box_append(GTK_BOX(content_box), command_label);
    
    gtk_window_present(GTK_WINDOW(dialog));
}

GtkWidget* get_no_displays_screen() {
  GtkWidget *box, *image, *title, *subtitle, *button, *setup_button;
	GtkCssProvider *button_css_provider;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_top(box, MARGIN_UNIT * 2.4);
	gtk_widget_set_margin_start(box, MARGIN_UNIT);
	gtk_widget_set_margin_end(box, MARGIN_UNIT);
	gtk_widget_set_margin_bottom(box, MARGIN_UNIT);

	image = gtk_image_new_from_icon_name("video-display-symbolic");
  gtk_widget_add_css_class(image, "dim-label");
	gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
	gtk_widget_set_size_request(image, 128, 128);
	gtk_widget_set_margin_bottom(image, MARGIN_UNIT * 2);
	gtk_box_append(GTK_BOX(box), image);

	title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='heavy'>No compatible displays found</span>");
  gtk_box_append(GTK_BOX(box), title);

  button = gtk_button_new_with_label("Refresh");
  gtk_widget_add_css_class(button, "circular");
  button_css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(button_css_provider, "button {padding-left: 12px;padding-right: 12px;padding-top: 4px;padding-bottom: 4px;}");
  gtk_style_context_add_provider(gtk_widget_get_style_context(button), GTK_STYLE_PROVIDER(button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
  g_signal_connect(button, "clicked", G_CALLBACK(_on_no_displays_screen_refresh_button_clicked), NULL);

  if (is_running_in_flatpak()) {
    subtitle = gtk_label_new("If you don't see your displays, you may need to grant hardware permissions.");
    gtk_widget_set_margin_bottom(subtitle, MARGIN_UNIT);
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_widget_set_halign(subtitle, GTK_ALIGN_CENTER);
    gtk_label_set_justify(GTK_LABEL(subtitle), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(box), subtitle);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), button_box);

    setup_button = gtk_button_new_with_label("Setup Instructions");
    gtk_widget_add_css_class(setup_button, "circular");
    gtk_style_context_add_provider(gtk_widget_get_style_context(setup_button), GTK_STYLE_PROVIDER(button_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect(setup_button, "clicked", G_CALLBACK(_on_no_displays_screen_setup_button_clicked), NULL);
    gtk_box_append(GTK_BOX(button_box), setup_button);
    gtk_box_append(GTK_BOX(button_box), button);

  } else {
    subtitle = gtk_label_new("Try refreshing if you have just connected the displays");
	  gtk_widget_set_margin_bottom(subtitle, MARGIN_UNIT);
    gtk_box_append(GTK_BOX(box), subtitle);

    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), button);
  }

	return box;
}
