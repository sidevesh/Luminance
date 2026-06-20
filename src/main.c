#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gtk/gtk.h>
#include <adwaita.h>
#include "./dbus_service.c"
#include "./constants/main.c"
#include "./osd/main.c"
#include "./states/displays.c"
#include "./states/is_brightness_linked.c"
#include "./states/is_contrast_hidden.c"
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

#define OPT_GET_CONTRAST      256
#define OPT_SET_CONTRAST      257
#define OPT_INCREASE_CONTRAST 258
#define OPT_DECREASE_CONTRAST 259

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

static void on_app_startup(GApplication *app, gpointer user_data) {
  (void)user_data;
  // Set inactivity timeout to 10 seconds.
  // This ensures the service stays alive for a short while after D-Bus activation
  // to handle requests, then quits if no window starts or requests cease.
  g_application_set_inactivity_timeout(app, 10000);
}

static void activate_gtk_ui(GtkApplication *app) {
	initialize_application_window(GTK_APPLICATION(app));
	update_window_contents_in_ui();
  set_displays_update_callback(update_window_contents_in_ui);
  load_displays(update_window_contents_in_ui, update_window_contents_in_ui);
}

static void quit_application(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  (void)action;
  (void)parameter;
  AdwApplication *app = user_data;
  g_application_quit(G_APPLICATION(app));
}

int ensure_displays_are_present_in_cli() {
  if (displays_count() == 0) {
    fprintf(stderr, "No displays found.\n");
    free_displays();
    return 1;
  }
  return 0;
}

int list_displays_in_cli() {
  for (guint index = 0; index < displays_count(); index++) {
    gchar *label = get_display_name(index);
    gdouble brightness_percentage = get_display_brightness_percentage(index);
    if (get_display_has_contrast(index)) {
      gdouble contrast_percentage = get_display_contrast_percentage(index);
      printf("Display %d: Label: %s, Brightness: %.2f%%, Contrast: %.2f%%\n",
        index + 1, label, brightness_percentage, contrast_percentage);
    } else {
      printf("Display %d: Label: %s, Brightness: %.2f%%\n", index + 1, label, brightness_percentage);
    }
  }

	return 0;
}

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
      set_display_brightness_percentage(index, brightness_percentage, TRUE);
      return;
    }
  }

	fprintf(stderr, "Invalid display number: %d\n", display_index + 1);
}

int set_display_brightness_if_needed_in_cli(guint display_number, guint brightness_percentage, gchar option, gchar show_osd) {
	gdouble linked_all_displays_brightness_percentage = -1;
	gdouble non_linked_all_displays_brightness_percentages_average = -1;

  load_displays(NULL, NULL);
  ensure_displays_are_present_in_cli();

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

int get_display_contrast_in_cli(guint display_number) {
	for (guint index = 0; index < displays_count(); index++) {
		if ((index + 1) == display_number) {
			if (!get_display_has_contrast(index)) {
				fprintf(stderr, "Display %d does not support contrast control via DDC/CI.\n", display_number);
				return 1;
			}
			gdouble contrast_percentage = get_display_contrast_percentage(index);
			printf("%.2f%%\n", contrast_percentage);
			return 0;
		}
	}

	fprintf(stderr, "Invalid display number: %d\n", display_number);
	return 1;
}

void set_contrast_percentage_in_cli(guint display_index, double contrast_percentage) {
  for (guint index = 0; index < displays_count(); index++) {
    if (index == display_index) {
      if (!get_display_has_contrast(index)) {
        fprintf(stderr, "Display %d does not support contrast control via DDC/CI.\n", display_index + 1);
        return;
      }
      set_display_contrast_percentage(index, contrast_percentage, FALSE);
      return;
    }
  }

	fprintf(stderr, "Invalid display number: %d\n", display_index + 1);
}

int set_display_contrast_if_needed_in_cli(guint display_number, guint contrast_percentage, int option) {
	gboolean any_ddc_display_processed = FALSE;

  load_displays(NULL, NULL);
  ensure_displays_are_present_in_cli();

	for (guint index = 0; index < displays_count(); index++) {
		if (display_number == 0 || (index + 1) == display_number) {
			if (!get_display_has_contrast(index)) {
				if (display_number > 0) {
					fprintf(stderr, "Display %d does not support contrast control via DDC/CI.\n", display_number);
					free_displays();
					return 1;
				}
				continue;
			}

			gdouble current_contrast_percentage = get_display_contrast_percentage(index);
			gdouble final_contrast_percentage = contrast_percentage;
			if (option == OPT_INCREASE_CONTRAST) {
				final_contrast_percentage = current_contrast_percentage + contrast_percentage;
			}
			if (option == OPT_DECREASE_CONTRAST) {
				final_contrast_percentage = current_contrast_percentage - contrast_percentage;
			}
			if (final_contrast_percentage > 100) {
				final_contrast_percentage = 100;
			} else if (final_contrast_percentage < 0) {
				final_contrast_percentage = 0;
			}

			if (final_contrast_percentage != current_contrast_percentage) {
				set_contrast_percentage_in_cli(index, final_contrast_percentage);
			}
			any_ddc_display_processed = TRUE;

			if (display_number > 0) {
				free_displays();
				return 0;
			}
		}
	}

	if (display_number > 0) {
		fprintf(stderr, "Invalid display number: %d\n", display_number);
		free_displays();
		return 1;
	}

	if (!any_ddc_display_processed) {
		fprintf(stderr, "No DDC/CI displays with contrast control found.\n");
		free_displays();
		return 1;
	}

	free_displays();
	return 0;
}

int display_help_in_cli() {
  printf("Usage: %s [OPTIONS]\n", APP_INFO_PACKAGE_NAME);
  printf("An application to control brightness of displays including external displays supporting DDC/CI\n");
  printf("\n");
  printf("Options:\n");
  printf("  -l, --list-displays                         List displays and their brightness\n");
  printf("  -g, --get-percentage [DISPLAY NUMBER]       Get the brightness percentage of a display\n");
  printf("  -s, --set-brightness [DISPLAY NUMBER]       Set the brightness of a display to a percentage value\n");
	printf("  -i, --increase-brightness [DISPLAY NUMBER]  Increase the brightness of a display by a percentage value\n");
	printf("  -d, --decrease-brightness [DISPLAY NUMBER]  Decrease the brightness of a display by a percentage value\n");
	printf("                                                - If DISPLAY NUMBER is not provided, the brightness of all displays will be changed\n");
	printf("      --get-contrast DISPLAY NUMBER            Get the contrast percentage of a DDC/CI display\n");
	printf("      --set-contrast [DISPLAY NUMBER]          Set the contrast of a DDC/CI display to a percentage value\n");
	printf("      --increase-contrast [DISPLAY NUMBER]     Increase the contrast of a DDC/CI display by a percentage value\n");
	printf("      --decrease-contrast [DISPLAY NUMBER]     Decrease the contrast of a DDC/CI display by a percentage value\n");
	printf("                                                - If DISPLAY NUMBER is not provided, the contrast of all DDC/CI displays will be changed\n");
  printf("  -p  --percentage [PERCENTAGE]               Percentage value for brightness or contrast operations\n");
	printf("  -h, --help                                  Show help information\n");
  printf("\n");
  printf("When no arguments are provided, the application starts in GUI mode.\n");

	return 0;
}

int parse_cli_arguments(int argc, char **argv) {
  struct option long_options[] = {
    {"list-displays", no_argument, NULL, 'l'},
    {"get-percentage", required_argument, NULL, 'g'},
    {"set-brightness", optional_argument, NULL, 's'},
		{"increase-brightness", optional_argument, NULL, 'i'},
		{"decrease-brightness", optional_argument, NULL, 'd'},
		{"get-contrast", required_argument, NULL, OPT_GET_CONTRAST},
		{"set-contrast", optional_argument, NULL, OPT_SET_CONTRAST},
		{"increase-contrast", optional_argument, NULL, OPT_INCREASE_CONTRAST},
		{"decrease-contrast", optional_argument, NULL, OPT_DECREASE_CONTRAST},
    {"percentage", required_argument, NULL, 'p'},
		{"show-osd", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

	int option;
	int option_index;

	gchar set_brightness_option = -1;
	guint set_brightness_display_number = -1;
	int set_contrast_option = -1;
	guint set_contrast_display_number = -1;
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
	  	{
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
	  	}
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
      case OPT_GET_CONTRAST: // --get-contrast option
			{
        guint get_contrast_display_number = atoi(optarg);
				if (get_contrast_display_number == 0) {
					fprintf(stderr, "Invalid display number: %d\n", get_contrast_display_number);
					status = 1;
					return status;
				}
				load_displays(NULL, NULL);
				status = ensure_displays_are_present_in_cli();
				status = get_display_contrast_in_cli(get_contrast_display_number);
				free_displays();
				return status;
			}
			case OPT_SET_CONTRAST: // --set-contrast option
			case OPT_INCREASE_CONTRAST: // --increase-contrast option
			case OPT_DECREASE_CONTRAST: // --decrease-contrast option
				set_contrast_option = option;
				if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
					optarg = argv[optind++];
				}
				if (optarg != NULL) {
					set_contrast_display_number = atoi(optarg);
					if (set_contrast_display_number == 0) {
						set_contrast_display_number = 99;
					}
				} else {
					set_contrast_display_number = 0;
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

	gboolean has_brightness_op = (set_brightness_option != (gchar)-1);
	gboolean has_contrast_op = (set_contrast_option != (gchar)-1);

	if (!has_brightness_op && !has_contrast_op) {
		display_help_in_cli();
		status = 1;
		return status;
	}

	// Validate display number 0 (invalid) for whichever ops are set
	if (has_brightness_op && set_brightness_display_number == 99) {
		fprintf(stderr, "Invalid display number: 0\n");
		status = 1;
		return status;
	}

	if (has_contrast_op && set_contrast_display_number == 99) {
		fprintf(stderr, "Invalid display number: 0\n");
		status = 1;
		return status;
	}

	if (set_brightness_percentage_value == (guint) -1) {
		fprintf(stderr, "Percentage value is required. Use -p or --percentage to specify the percentage.\n");
		status = 1;
		return status;
	}

	if (has_brightness_op) {
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
		if (status != 0) {
			return status;
		}
	}

	if (has_contrast_op) {
		status = set_display_contrast_if_needed_in_cli(
			set_contrast_display_number,
			set_brightness_percentage_value,
			set_contrast_option
		);
	}

	return status;
}

gboolean already_running(void) {
  intmax_t pid;
  FILE *lock_file = fopen(LOCK_FILE_PATH, "r");
  if (lock_file == NULL) {
    return false;
  }

  if (
    fscanf(lock_file, "%jd\n", &pid) == 1
    // kill(pid, 0) does not send any signal but still validates the PID.
    && kill((pid_t)pid, 0) == 0
  ) {
    fclose(lock_file);
		return true;
  }

  fclose(lock_file);
  return false;
}

int main(int argc, char **argv) {
  if (already_running()) {
    fprintf(stderr, "Another instance of the application is already running.\n");
    return 1;
  }

  // Create the lock file
  FILE *lock_file = fopen(LOCK_FILE_PATH, "w");
  if (lock_file == NULL) {
    fprintf(stderr, "Failed to create lock file.\n");
    return 1;
  }
  fprintf(lock_file, "%jd\n", (intmax_t)getpid());
  fclose(lock_file);

  // Initialize DBus service early to ensure object is exported before
  // GApplication acquires the bus name. This prevents "Object does not exist"
  // errors during D-Bus activation.
  GError *error = NULL;
  GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (conn) {
    setup_dbus_service(conn);
    g_object_unref(conn);
  } else {
    g_warning("Failed to connect to session bus: %s", error ? error->message : "Unknown error");
    if (error) g_error_free(error);
  }

  int status = 0;

  gboolean service_mode = FALSE;
  if (argc > 1 && strcmp(argv[1], "--gapplication-service") == 0) {
    service_mode = TRUE;
  }

  if (argc > 1 && !service_mode) {
    is_cli_mode = TRUE;
    status = parse_cli_arguments(argc, argv);
  } else {
		GApplicationFlags flags;

		#if GLIB_CHECK_VERSION(2, 74, 0)
		flags = G_APPLICATION_DEFAULT_FLAGS;
		#else
		flags = G_APPLICATION_FLAGS_NONE;
		#endif

    g_set_application_name(APP_INFO_DISPLAY_NAME);

    AdwApplication *app;
    app = adw_application_new(APP_INFO_PACKAGE_NAME, flags);

    static const GActionEntry app_actions[] = {
        { "quit", quit_application, NULL, NULL, NULL, {0, 0, 0} },
    };

    g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions, G_N_ELEMENTS(app_actions), app);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", (const char *[]) { "<Ctrl>q", NULL });

    g_signal_connect(app, "startup", G_CALLBACK(on_app_startup), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(activate_gtk_ui), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    free_displays();
  }

  // Remove the lock file when the application exits
  remove(LOCK_FILE_PATH);

	return status;
}
