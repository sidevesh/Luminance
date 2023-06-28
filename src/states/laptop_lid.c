#include <gtk/gtk.h>

#ifndef LAPTOP_LID_STATE
#define LAPTOP_LID_STATE

gboolean get_has_lid() {
  FILE* lid_state_file = fopen("/proc/acpi/button/lid/LID0/state", "r");
  if (lid_state_file == NULL) {
    return FALSE;
  }
  fclose(lid_state_file);
  return TRUE;
}

gboolean is_lid_open() {
  FILE* lid_state_file = fopen("/proc/acpi/button/lid/LID0/state", "r");
  if (lid_state_file == NULL) {
    return FALSE;
  }

  char state[10];
  if (fscanf(lid_state_file, "state: %9s", state) != 1) {
    fclose(lid_state_file);
    return FALSE;
  }

  fclose(lid_state_file);

  if (strcmp(state, "open") == 0) {
    return TRUE;
  }

  return FALSE;
}

#endif
