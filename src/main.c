#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gtk/gtk.h>

#ifndef BRIGHTNESS_CODE
#include "../ddcbc-api/ddcbc-api.c"
#endif

#include "./constants/main.c"
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

gboolean is_cli_mode = FALSE;

gdouble brightness_percentage_to_show_after_change = -1;
void show_gnome_osd_popup_after_change() {
	if (brightness_percentage_to_show_after_change == -1) {
		return;
	}

	char command[300];
	sprintf(
		command,
		"gdbus call --session --dest org.gnome.Shell --object-path /dev/ramottamado/EvalGjs --method dev.ramottamado.EvalGjs.Eval \"Main.osdWindowManager.show(-1, Gio.Icon.new_for_string('display-brightness-symbolic'), null, %f, 1);\" > /dev/null",
		brightness_percentage_to_show_after_change / 100.0
	);
	system(command);

	brightness_percentage_to_show_after_change = -1;
}

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
	for (guint index = 0; index < displays_count(); index++) {
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

void set_brightness_percentage_in_cli(guint display_number, double brightness_percentage) {
  for (guint index = 0; index < displays_count(); index++) {
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
int set_display_brightness_if_needed_in_cli(guint display_number, guint brightness_percentage, char option, gboolean show_gnome_osd) {
  load_displays(NULL, NULL);
  ensure_displays_are_present_in_cli();

	gdouble linked_all_displays_brightness_percentage = -1;
	gdouble non_linked_all_displays_brightness_percentages_average = -1;

	for (guint index = 0; index < displays_count(); index++) {
		ddcbc_display *display = get_display(index);
		if (display_number == 0 || display->info.dispno == display_number) {
			gdouble current_brightness_percentage = (display->last_val * 100.0) / display->max_val;
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
				set_brightness_percentage_in_cli(display->info.dispno, final_brightness_percentage);
			}
			if (display_number > 0) {
				free_displays();
				if (show_gnome_osd) {
					brightness_percentage_to_show_after_change = non_linked_all_displays_brightness_percentages_average;
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

	if (show_gnome_osd) {
		if (get_is_brightness_linked() && display_number == 0) {
			brightness_percentage_to_show_after_change = linked_all_displays_brightness_percentage;
		} else {
			brightness_percentage_to_show_after_change = non_linked_all_displays_brightness_percentages_average;
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
  printf("  -o, --show-gnome-osd             Show GNOME OSD popup when brightness is changed, only works on gnome shell with https://extensions.gnome.org/extension/5952/eval-gjs/ extension installed\n");
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
		{"increase-brightness", optional_argument, NULL, 'i'},
		{"decrease-brightness", optional_argument, NULL, 'd'},
    {"percentage", required_argument, NULL, 'p'},
		{"show-gnome-osd", no_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

	char option;
	int option_index;

	char set_brightness_option = -1;
	guint set_brightness_display_number = -1;
	guint set_brightness_percentage_value = -1;
	gboolean show_gnome_osd = FALSE;

	int status = 0;

  while ((option = getopt_long(argc, argv, "lg:s::i::d::p:oh", long_options, &option_index)) != -1) {
		switch (option) {
      case 'l': // --list-displays option
        load_displays(NULL, NULL);
				status = ensure_displays_are_present_in_cli();
        status = list_displays_in_cli();
        free_displays();
        return status;
      case 'g': // --get-percentage option
        guint get_percentage_display_number = atoi(optarg);
				// Display number returned from ddcbc is 1-based, so 0 is invalid
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
			case 'o': // --show-gnome-osd option
			  show_gnome_osd = TRUE;
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

  status = set_display_brightness_if_needed_in_cli(
		set_brightness_display_number,
		set_brightness_percentage_value,
		set_brightness_option,
		show_gnome_osd
	);
	if (status == 0 && show_gnome_osd) {
		show_gnome_osd_popup_after_change();
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
    is_cli_mode = true;
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
