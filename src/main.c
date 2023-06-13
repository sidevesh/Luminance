#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gtk/gtk.h>

#ifndef BRIGHTNESS_CODE
#include "../ddcbc-api/ddcbc-api.c"
#endif

#include "./types/display_section.c"
#include "./states/displays.c"
#include "./states/is_brightness_linked.c"
#include "./ui/constants/main.c"
#include "./ui/components/display_brightness_scale.c"
#include "./ui/components/display_icon.c"
#include "./ui/components/display_label.c"
#include "./ui/components/link_brightness_check_button.c"
#include "./ui/components/separator.c"
#include "./ui/screens/show_displays.c"
#include "./ui/screens/no_displays.c"
#include "./ui/screens/refreshing_displays.c"
#include "./ui/window.c"

#define LOCK_FILE_PATH "/tmp/com.sidevesh.Luminance.lock"

gboolean is_cli_mode = FALSE;

display_section **display_sections;
guint display_sections_count = 0;

gboolean set_brightness(GtkWidget *widget, GdkEvent *event, guint data) {
	guint index_of_display_section = GPOINTER_TO_UINT(data);

	display_section *display_section = display_sections[index_of_display_section];
	ddcbc_display *display = display_section->display;

	guint16 new_val = gtk_range_get_value(GTK_RANGE(widget));
	DDCBC_Status rc = ddcbc_display_set_brightness(display, new_val);

	// If the link checkbox is checked, set all other displays to the same value:
	// TODO: It might be the case that the new value will exceed the max of other
	// displays.
	if (get_is_brightness_linked()) {
		for (guint index = 0; index < display_sections_count; index++) {
			GtkWidget *scale = display_sections[index]->scale;
			gtk_range_set_value(GTK_RANGE(scale), new_val);

			ddcbc_display *display = display_sections[index]->display;
			ddcbc_display_set_brightness(display, new_val);
		}
	}

	if (rc == 1) {
		g_printerr(
			"Partial sucess in setting the brightness of display no"
			" %d to %u. Code: %d\n",
			display->info.dispno, new_val, rc
		);
	} else if (rc != 0) {
		g_printerr(
			"An error occured when setting the brightness of display no"
			" %d to %u. Code: %d\n",
			display->info.dispno, new_val, rc
		);
	}

	return FALSE;
}

void update_display_brightness_scales(GtkRange *range, guint data) {
	guint index_of_display_section = GPOINTER_TO_UINT(data);

	guint new_value = gtk_range_get_value(range);
	for (guint index = 0; index < display_sections_count; index++) {
		if (get_is_brightness_linked() || index == index_of_display_section) {
			gtk_range_set_value(GTK_RANGE(display_sections[index]->scale), new_value);
		}
	}
}

void link_brightness(GtkToggleButton *link_brightness_check_button) {
	gboolean is_brightness_linked = gtk_toggle_button_get_active(link_brightness_check_button);
	set_is_brightness_linked(is_brightness_linked);

	if (!is_brightness_linked) {
		return;
	}

	guint max_scale = 0;
	for (guint index = 0; index < display_sections_count; index++) {
		guint val = gtk_range_get_value(GTK_RANGE(display_sections[index]->scale));
		max_scale = val > max_scale ? val : max_scale;
	}

	for (guint index = 0; index < display_sections_count; index++) {
		gtk_range_set_value(GTK_RANGE(display_sections[index]->scale), max_scale);
		ddcbc_display *display = display_sections[index]->display;
		DDCBC_Status rc = ddcbc_display_set_brightness(display, max_scale);
		
		if (rc == 1) {
			g_printerr(
				"Partial success in setting the brightness of display no"
				" %d to %u while linking displays. Code: %d\n",
				display->info.dispno, max_scale, rc
			);	
		} else if (rc != 0) {
			g_printerr(
				"An error occured when setting the brightness of display no"
				" %d to %u while linking displays. Code: %d\n",
				display->info.dispno, max_scale, rc
			);	
		}
	}
}

void update_window_contents() {
    if (is_cli_mode) {
        return;
    }

	GtkWidget *current_screen;

	if (is_displays_loading() == TRUE) {
		current_screen = get_refreshing_displays_screen();
	} else if (displays_count() == 0) {
		current_screen = get_no_displays_screen();
	} else {
		current_screen = get_show_displays_screen();
	}

	update_window_content_screen(current_screen);
}

static void activate_gtk_ui(GtkApplication *app) {
	initialize_application_window(app);
	update_window_contents();
    load_displays_with_ui_updates();
}

// Function to list displays and their brightness
void list_displays_in_cli() {
    guint count = displays_count();

    for (guint index = 0; index < count; index++) {
        ddcbc_display *display = get_display(index);

        guint dispno = display->info.dispno;
        const char *label = display->info.model_name;
        guint16 brightness = display->last_val;
        guint16 max_brightness = display->max_val;
        gdouble brightness_percentage = (brightness * 100.0) / max_brightness;

        printf("Display %d: Label: %s, Brightness: %.2f%%\n", dispno, label, brightness_percentage);
    }
}


// Function to display command-line arguments and help information
void display_help_in_cli() {
    printf("Usage: com.sidevesh.Luminance [OPTIONS]\n");
    printf("A graphical application to control display brightness.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -l, --list-displays  List displays and their brightness\n");
    printf("  -h, --help           Show help information\n");
    printf("\n");
    printf("When no arguments are provided, the application starts in GUI mode.\n");
}

// CLI argument parsing
int parse_cli_arguments(int argc, char **argv) {
    struct option long_options[] = {
        {"list-displays", no_argument, NULL, 'l'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int option;
    int option_index;

    while ((option = getopt_long(argc, argv, "lh", long_options, &option_index)) != -1) {
        switch (option) {
            case 'l': // --list-displays option
                load_displays();
                list_displays_in_cli();
                free_displays();
                return 0;
            case 'h': // --help option
                display_help_in_cli();
                return 0;
            default:
                fprintf(stderr, "Unknown option: %s\n", argv[optind - 1]);
                return 1;
        }
    }
}

// Entry point of the program
int main(int argc, char **argv) {
    // Check if the lock file exists
    if (g_file_test(LOCK_FILE_PATH, G_FILE_TEST_EXISTS)) {
        fprintf(stderr, "Another instance of the application is already running.\n");
        return 1;
    }

    // Create the lock file
    FILE *lock_file = fopen(LOCK_FILE_PATH, "w");
    if (lock_file == NULL) {
        fprintf(stderr, "Failed to create lock file.\n");
        return 1;
    }
    fclose(lock_file);

    int status = 0;

    if (argc > 1) {
        is_cli_mode = true;
        status = parse_cli_arguments(argc, argv);

    } else {
        GtkApplication *app;
        app = gtk_application_new("com.sidevesh.Luminance", G_APPLICATION_DEFAULT_FLAGS);
        g_signal_connect(app, "activate", G_CALLBACK(activate_gtk_ui), NULL);
        status = g_application_run(G_APPLICATION(app), argc, argv);
        g_object_unref(app);
        free_displays();
    }

    // Remove the lock file when the application exits
    remove(LOCK_FILE_PATH);
}
