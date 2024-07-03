#include "osd/gnome.h"

#include <gtk/gtk.h>

void show_gnome_osd(gdouble brightness_percentage) {
	gchar command[300];
	sprintf(
		command,
		"gdbus call --session --dest org.gnome.Shell --object-path /dev/ramottamado/EvalGjs --method dev.ramottamado.EvalGjs.Eval \"Main.osdWindowManager.show(-1, Gio.Icon.new_for_string('display-brightness-symbolic'), null, %f, 1);\" > /dev/null",
		brightness_percentage / 100.0
	);
	system(command);
}
