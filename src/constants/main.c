#include "config.h"

#ifndef LOCK_FILE_PATH
#define LOCK_FILE_PATH "/tmp/" APPLICATION_ID ".lock"
#endif

#ifndef MINIMUM_REFRESH_TIME_IN_SECONDS
#define MINIMUM_REFRESH_TIME_IN_SECONDS 2
#endif

#ifndef IS_BRIGHTNESS_LINKED_GSETTINGS_KEY
#define IS_BRIGHTNESS_LINKED_GSETTINGS_KEY "is-brightness-linked"
#endif

#ifndef SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_GSETTINGS_KEY
#define SHOULD_HIDE_INTERNAL_IF_LID_CLOSED_GSETTINGS_KEY "should-hide-internal-if-lid-closed"
#endif
