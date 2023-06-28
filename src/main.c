#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gtk/gtk.h>

#include "./constants/main.c"
#include "./osd/main.c"
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

gboolean is_cli_mode = FALSE;

void update_window_contents_in_ui() {
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
	update_window_contents_in_ui();
  load_displays(update_window_contents_in_ui, update_window_contents_in_ui);
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
  for (guint index = 0; index < displays_count(); index++) {
    gchar *label = get_display_name(index);
    gdouble brightness_percentage = get_display_brightness_percentage(index);
    printf("Display %d: Label: %s, Brightness: %.2f%%\n", index + 1, label, brightness_percentage);
  }

	return 0;
}

// Function to get the brightness percentage of a specified display
int get_display_brightness_in_cli(guint display_number) {
	for (guint index = 0; index < displays_count(); index++) {
		if ((index + 1) == display_number) {
			gdouble brightness_percentage = get_display_brightness_percentage(index);
			printf("%.2f%%\n", brightness_percentage);
			return 0;
		}
	}

	fprintf(stderr, "Invalid display number: %d\n", display_number);
	return 1;
}

void set_brightness_percentage_in_cli(guint display_index, double brightness_percentage) {
  for (guint index = 0; index < displays_count(); index++) {
    if (index == display_index) {
      set_display_brightness_percentage(index, brightness_percentage);
      return;
    }
  }

	fprintf(stderr, "Invalid display number: %d\n", display_index + 1);
}

// Function to set the brightness of a specified display
int set_display_brightness_if_needed_in_cli(guint display_number, guint brightness_percentage, gchar option, gchar show_osd) {
  load_displays(NULL, NULL);
  ensure_displays_are_present_in_cli();

	gdouble linked_all_displays_brightness_percentage = -1;
	gdouble non_linked_all_displays_brightness_percentages_average = -1;

	for (guint index = 0; index < displays_count(); index++) {
		if (display_number == 0 || (index + 1) == display_number) {
			gdouble current_brightness_percentage = get_display_brightness_percentage(index);
			gdouble final_brightness_percentage = brightness_percentage;
			if (get_is_brightness_linked() && display_number == 0 && linked_all_displays_brightness_percentage != -1) {
				final_brightness_percentage = linked_all_displays_brightness_percentage;
			} else {
				if (option == 'i') {
					final_brightness_percentage = current_brightness_percentage + brightness_percentage;
				}
				if (option == 'd') {
					final_brightness_percentage = current_brightness_percentage - brightness_percentage;
				}
				if (final_brightness_percentage > 100) {
					final_brightness_percentage = 100;
				} else if (final_brightness_percentage < 0) {
					final_brightness_percentage = 0;
				}
				if (get_is_brightness_linked() && display_number == 0) {
					if (linked_all_displays_brightness_percentage == -1) {
						linked_all_displays_brightness_percentage = final_brightness_percentage;
					}
				} else {
					if (index == 0) {
						non_linked_all_displays_brightness_percentages_average = final_brightness_percentage;
					} else {
						non_linked_all_displays_brightness_percentages_average = ((non_linked_all_displays_brightness_percentages_average * index) + final_brightness_percentage) / (index + 1);
					}
				}
			}
			if (final_brightness_percentage != current_brightness_percentage) {
				set_brightness_percentage_in_cli(index, final_brightness_percentage);
			}
			if (display_number > 0) {
				free_displays();
				if (show_osd != 0) {
					set_osd_brightness_percentage_to_show(non_linked_all_displays_brightness_percentages_average);
				}
				return 0;
			}
		}
	}

	if (display_number > 0) {
		fprintf(stderr, "Invalid display number: %d\n", display_number);
		free_displays();
		return 1;
	}

	if (show_osd != 0) {
		if (get_is_brightness_linked() && display_number == 0) {
			set_osd_brightness_percentage_to_show(linked_all_displays_brightness_percentage);
		} else {
			set_osd_brightness_percentage_to_show(non_linked_all_displays_brightness_percentages_average);
		}
	}
	free_displays();
	return 0;
}

// Function to display command-line arguments and help information
int display_help_in_cli() {
  printf("Usage: %s [OPTIONS]\n", APP_INFO_PACKAGE_NAME);
  printf("A graphical application to control display brightness.\n");
  printf("\n");
  printf("Options:\n");
  printf("  -l, --list-displays              List displays and their brightness\n");
  printf("  -g, --get-percentage NUM         Get the brightness percentage of a display\n");
  printf("  -s, --set-brightness [NUM]       Set the brightness of a display to a percentage value\n");
	printf("  -i, --increase-brightness [NUM]  Increase the brightness of a display by a percentage value\n");
	printf("  -d, --decrease-brightness [NUM]  Decrease the brightness of a display by a percentage value\n");
  printf("  -p  --percentage [PERCENT]       Percentage value to set the brightness to in case of --set-brightness option or to increase or decrease the brightness by in case of --increase-brightness or --decrease-brightness option\n");
  printf("  -o, --show-osd                   Show OSD popup when brightness is changed for specified environment:\n");
	printf("                                   g: GNOME, experimental, only works with https://extensions.gnome.org/extension/5952/eval-gjs/ extension installed\n");
	printf("  -h, --help                       Show help information\n");
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
		{"increase-brightness", optional_argument, NULL, 'i'},
		{"decrease-brightness", optional_argument, NULL, 'd'},
    {"percentage", required_argument, NULL, 'p'},
		{"show-osd", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

	gchar option;
	int option_index;

	gchar set_brightness_option = -1;
	guint set_brightness_display_number = -1;
	guint set_brightness_percentage_value = -1;
	gchar show_osd = 0;

	int status = 0;

  while ((option = getopt_long(argc, argv, "lg:s::i::d::p:o:h", long_options, &option_index)) != -1) {
		switch (option) {
      case 'l': // --list-displays option
        load_displays(NULL, NULL);
				status = ensure_displays_are_present_in_cli();
        status = list_displays_in_cli();
        free_displays();
        return status;
      case 'g': // --get-percentage option
        guint get_percentage_display_number = atoi(optarg);
				// Display numbers are 1-based, so 0 is invalid
				// and we use 0 to indicate that the brightness should be set for all displays,
				// so the user cannot input 0 as the display number
				if (get_percentage_display_number == 0) {
					fprintf(stderr, "Invalid display number: %d\n", get_percentage_display_number);
					status = 1;
					return status;
				}
				load_displays(NULL, NULL);
				status = ensure_displays_are_present_in_cli();
				status = get_display_brightness_in_cli(get_percentage_display_number);
				free_displays();
				return status;
			case 's': // --set-brightness option
			case 'i': // --increase-brightness option
			case 'd': // --decrease-brightness option
				set_brightness_option = option;
				// the below code is to handle the optional argument being provided with a space between the option and the argument value,
				// while getopt expexts there not to be a space between the option and the argument value
				// https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
				if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
					optarg = argv[optind++];
				}
				if (optarg != NULL) {
        	set_brightness_display_number = atoi(optarg);
					if (set_brightness_display_number == 0) {
						set_brightness_display_number = 99;
					}
				} else {
					set_brightness_display_number = 0;
				}
				break;
      case 'p': // --percentage option
				set_brightness_percentage_value = atoi(optarg);
				break;
			case 'o': // --show-osd option
				if (optarg != NULL) {
					show_osd = optarg[0];
				} else {
					show_osd = 0;
				}
			  break;
      default: // --help option or shown when arguments are invalid
				if (option != 'h') {
					display_help_in_cli();
					status = 1;
				} else {
					status = display_help_in_cli();
				}
				return status;
    }
  }

	// Same reason as get_percentage_display_number's check for zero above,
	// except we set the set_brightness_display_number to 99 to indicate the case of user submitting 0 as value
	if (set_brightness_display_number == 99) {
		fprintf(stderr, "Invalid display number: 0\n");
		status = 1;
		return status;
	}

	if (set_brightness_display_number == -1) {
		display_help_in_cli();
		status = 1;
		return status;
	}
	if (set_brightness_percentage_value == -1) {
		display_help_in_cli();
		status = 1;
		return status;
	}

	if (show_osd != 0) {
		if (is_osd_provider_supported(show_osd) == FALSE) {
			fprintf(stderr, "Invalid OSD provider: %c\n", show_osd);
			status = 1;
			return status;
		}
	}

  status = set_display_brightness_if_needed_in_cli(
		set_brightness_display_number,
		set_brightness_percentage_value,
		set_brightness_option,
		show_osd
	);
	if (status == 0 && show_osd != 0) {
		show_osd_after_brightness_change(show_osd);
	}

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
    is_cli_mode = TRUE;
    status = parse_cli_arguments(argc, argv);
  } else {
    GtkApplication *app;
    app = gtk_application_new(APP_INFO_PACKAGE_NAME, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate_gtk_ui), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    free_displays();
  }

  // Remove the lock file when the application exits
  remove(LOCK_FILE_PATH);

	return status;
}
