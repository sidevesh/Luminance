#ifndef UTILS_FLATPAK_C
#define UTILS_FLATPAK_C

#include <unistd.h>
#include <gtk/gtk.h>

gboolean is_running_in_flatpak() {
  return access("/.flatpak-info", F_OK) == 0;
}

#endif
