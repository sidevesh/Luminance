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

	guint16 new_value = gtk_range_get_value(GTK_RANGE(widget));
	DDCBC_Status rc = ddcbc_display_set_brightness(display, new_value);

	// If the link checkbox is checked, set all other displays to the same value:
	// TODO: It might be the case that the new value will exceed the max of other
	// displays.
	if (get_is_brightness_linked()) {
		for (guint index = 0; index < display_sections_count; index++) {
			GtkWidget *scale = display_sections[index]->scale;
			gtk_range_set_value(GTK_RANGE(scale), new_value);

			ddcbc_display *display = display_sections[index]->display;
			ddcbc_display_set_brightness(display, new_value);
		}
	}

	if (rc == 1) {
		g_printerr(
			"Partial sucess in setting the brightness of display no"
			" %d to %u. Code: %d\n",
			display->info.dispno, new_value, rc
		);
	} else if (rc != 0) {
		g_printerr(
			"An error occured when setting the brightness of display no"
			" %d to %u. Code: %d\n",
			display->info.dispno, new_value, rc
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
		guint value = gtk_range_get_value(GTK_RANGE(display_sections[index]->scale));
		max_scale = value > max_scale ? value : max_scale;
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


int ensure_displays_are_present_in_cli() {
  if (displays_count() == 0) {
    fprintf(stderr, "No displays found.\n");
    free_displays();
    return 1;
  }
}

// Function to list all displays and their brightness
int list_displays_in_cli() {
  guint count = displays_count();
  for (guint index = 0; index < count; index++) {
    ddcbc_display *display = get_display(index);
    guint display_number = display->info.dispno;
    const char *label = display->info.model_name;
    guint16 brightness = display->last_val;
    guint16 max_brightness = display->max_val;
    gdouble brightness_percentage = (brightness * 100.0) / max_brightness;
    printf("Display %d: Label: %s, Brightness: %.2f%%\n", display_number, label, brightness_percentage);
  }

	return 0;
}

// Function to get the brightness percentage of a specified display
int get_display_brightness_in_cli(guint display_number) {
	guint count = displays_count();
	for (guint index = 0; index < count; index++) {
		ddcbc_display *display = get_display(index);
		if (display->info.dispno == display_number) {
			guint16 brightness = display->last_val;
			guint16 max_brightness = display->max_val;
			printf("%.2f%%\n", (brightness * 100.0) / max_brightness);
			return 0;
		}
	}

	fprintf(stderr, "Invalid display number: %d\n", display_number);
	return 1;
}

void set_brightness_percentage(guint display_number, double brightness_percentage) {
  guint count = displays_count();
  for (guint index = 0; index < count; index++) {
    ddcbc_display *display = get_display(index);
    if (display->info.dispno == display_number) {
      guint16 max_brightness = display->max_val;
      guint16 brightness = (guint16)(brightness_percentage * max_brightness / 100.0);
      DDCBC_Status rc = ddcbc_display_set_brightness(display, brightness);

			if (rc == 1) {
				fprintf(stderr,
					"Partial sucess in setting the brightness of display no"
					" %d to %u. Code: %d\n",
					display->info.dispno, brightness, rc
				);
			} else if (rc != 0) {
				fprintf(stderr,
					"An error occured when setting the brightness of display no"
					" %d to %u. Code: %d\n",
					display->info.dispno, brightness, rc
				);
			}

      return;
    }
  }

	fprintf(stderr, "Invalid display number: %d\n", display_number);
}

// Function to set the brightness of a specified display
int set_display_brightness_if_needed_in_cli(guint display_number, guint brightness_percentage) {
	if (display_number != -1) {
    if (brightness_percentage == -1) {
      fprintf(stderr, "Missing percentage value for --set-brightness option.\n");
			return 1;
    }

    load_displays();
    ensure_displays_are_present_in_cli();

    if (display_number > 0) {
			set_brightness_percentage(display_number, brightness_percentage);
    } else {
			guint count = displays_count();
      for (guint index = 0; index < count; index++) {
        ddcbc_display *display = get_display(index);
        set_brightness_percentage(display->info.dispno, brightness_percentage);
      }
    }
    free_displays();
		return 0;
  }
}

// Function to display command-line arguments and help information
int display_help_in_cli() {
  printf("Usage: com.sidevesh.Luminance [OPTIONS]\n");
  printf("A graphical application to control display brightness.\n");
  printf("\n");
  printf("Options:\n");
  printf("  -l, --list-displays         List displays and their brightness\n");
  printf("  -g, --get-percentage NUM    Get the brightness percentage of a display\n");
  printf("  -s, --set-brightness [NUM]  Set the brightness of a display to a percentage value\n");
  printf("      --percentage [PERCENT]  Set the brightness percentage for --set-brightness\n");
  printf("  -h, --help                  Show help information\n");
  printf("\n");
  printf("When no arguments are provided, the application starts in GUI mode.\n");

	return 0;
}

// CLI argument parsing
int parse_cli_arguments(int argc, char **argv) {
  struct option long_options[] = {
    {"list-displays", no_argument, NULL, 'l'},
    {"get-percentage", required_argument, NULL, 'g'},
    {"set-brightness", optional_argument, NULL, 's'},
    {"percentage", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

	int option;
	int option_index;

	guint set_brightness_display_number = -1;
	guint set_brightness_percentage_value = -1;

	int status = 0;

  while ((option = getopt_long(argc, argv, "lg:s::p:h", long_options, &option_index)) != -1) {
    switch (option) {
      case 'l': // --list-displays option
        load_displays();
				status = ensure_displays_are_present_in_cli();
        status = list_displays_in_cli();
        free_displays();
        return status;
      case 'g': // --get-percentage option
        guint get_percentage_display_number = atoi(optarg);
				load_displays();
				status = ensure_displays_are_present_in_cli();
				status = get_display_brightness_in_cli(get_percentage_display_number);
				free_displays();
				return status;
			case 's': // --set-brightness option
				// the below code is to handle the optional argument being provided with a space between the option and the argument value,
				// while getopt expexts there not to be a space between the option and the argument value
				// https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
				if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
					optarg = argv[optind++];
				}
				if (optarg != NULL) {
        	set_brightness_display_number = atoi(optarg);
				} else {
					set_brightness_display_number = 0;
				}
				break;
      case 'p': // --percentage option
				set_brightness_percentage_value = atoi(optarg);
				break;
      case 'h': // --help option
        status = display_help_in_cli();
        return status;
      default:
        fprintf(stderr, "Unknown option: %s\n", argv[optind - 1]);
				status = 1;
        return status;
    }
  }

  status = set_display_brightness_if_needed_in_cli(set_brightness_display_number, set_brightness_percentage_value);
	return status;
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
