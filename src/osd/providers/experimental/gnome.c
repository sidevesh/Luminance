#include <gtk/gtk.h>
#include "../../../utils/flatpak.c"

void show_gnome_osd(gdouble brightness_percentage) {
	gchar command[512];

	if (!is_running_in_flatpak()) {
		sprintf(
			command,
			"gdbus call --session --dest org.gnome.Shell --object-path /dev/ramottamado/EvalGjs --method dev.ramottamado.EvalGjs.Eval \"Main.osdWindowManager.showAll(Gio.Icon.new_for_string('display-brightness-symbolic'), null, %f, 1);\" > /dev/null",
			brightness_percentage / 100.0
		);
	} else {
		// sprintf(
		// 	command,
		// 	"flatpak-spawn --host gdbus call --session --dest org.gnome.Shell --object-path /dev/ramottamado/EvalGjs --method dev.ramottamado.EvalGjs.Eval \"Main.osdWindowManager.showAll(Gio.Icon.new_for_string('display-brightness-symbolic'), null, %f, 1);\" > /dev/null",
		// 	brightness_percentage / 100.0
		// );
	}

	system(command);
}
