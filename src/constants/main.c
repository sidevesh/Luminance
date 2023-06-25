#ifndef APP_INFO_VERSION_NUMBER
#include "../../info.c"
#endif

#ifndef LOCK_FILE_PATH
#define LOCK_FILE_PATH "/tmp/" APP_INFO_PACKAGE_NAME ".lock"
#endif

#ifndef MINIMUM_REFRESH_TIME_IN_SECONDS
#define MINIMUM_REFRESH_TIME_IN_SECONDS 2
#endif

#ifndef IS_BRIGHTNESS_LINKED_GSETTINGS_KEY
#define IS_BRIGHTNESS_LINKED_GSETTINGS_KEY "is-brightness-linked"
#endif
