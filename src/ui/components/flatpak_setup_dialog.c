#ifndef UI_COMPONENTS_FLATPAK_SETUP_DIALOG_C
#define UI_COMPONENTS_FLATPAK_SETUP_DIALOG_C

#include <gtk/gtk.h>
#include <adwaita.h>
#include "../../constants/main.c"

void show_flatpak_setup_dialog(GtkWindow *parent_window) {
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
    if (parent_window) {
        gtk_window_set_transient_for(GTK_WINDOW(dialog), parent_window);
    }
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

#endif
