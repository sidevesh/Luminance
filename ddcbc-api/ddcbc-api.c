// Simple API for brightness control with ddcutil

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>
#include <gmodule.h>

#include "ddcutil_c_api.h"
#include "ddcutil_status_codes.h"

#define BRIGHTNESS_CODE 0x10

// DDCBC_Status adds a couple of possible errors in the -4000 range.
typedef DDCA_Status DDCBC_Status;
#define DDCBC_BAD_VALUE -4000

// ddcbc_display encompasses info of a supported ddcbc display.
typedef struct 
ddcbc_display 
{
	guint16 max_val;
	guint16 last_val; // last recorded value set after get call.
	DDCA_Display_Info info;
	DDCA_Display_Handle dh;
} ddcbc_display;

// ddcbc_display_get_brightness get the brightness of `disp` and caches the 
// maximum possible brightness value. Any errors will be a DDC error.
DDCBC_Status 
ddcbc_display_get_brightness (ddcbc_display *disp) 
{

	DDCA_Non_Table_Vcp_Value valrec;
    DDCA_Status ddcrc = ddca_get_non_table_vcp_value (disp->dh,
        BRIGHTNESS_CODE, &valrec);

    if (ddcrc != 0)
        return ddcrc;
	
	disp->last_val = valrec.sh << 8 | valrec.sl;
	disp->max_val = valrec.mh << 8 | valrec.ml;

	return 0;
}

// ddcbc_bundle_set_brightness sets the brightness of a given monitor.
DDCBC_Status 
ddcbc_display_set_brightness (ddcbc_display *disp, guint16 new_val) 
{

	if (new_val > disp->max_val)
		return DDCBC_BAD_VALUE;

    guint8 new_val_high = new_val >> 8;  
    guint8 new_val_low = new_val & 0xff;

    DDCA_Status ddcrc = ddca_set_non_table_vcp_value (disp->dh, 
        BRIGHTNESS_CODE, new_val_high, new_val_low);

    if (ddcrc != 0)
        return ddcrc;

	disp->last_val = new_val;
    ddca_enable_verify(true);

	return 0;
}

// ddcbc_display_list is a GArray of ddcbc_displays
typedef struct 
{
	guint ct;
	GArray* list;
} ddcbc_display_list;

// ddcbc_display_list_init returns a list of supported ddcbc displays.
// If wait is true, the function will instruct ddca to wait if a display
// is locked by another process.
ddcbc_display_list 
ddcbc_display_list_init (gboolean wait) 
{
	DDCA_Display_Info_List *dinfos;
	ddca_get_display_info_list2(FALSE, &dinfos);

	ddcbc_display_list dlist;
	dlist.list = g_array_new(FALSE, FALSE, sizeof (ddcbc_display*));
	dlist.ct = 0;
	for (guint i = 0; i < dinfos->ct; i++) {
		ddcbc_display *disp = malloc (sizeof (ddcbc_display));
		disp->info = dinfos->info[i];

		DDCA_Status ddcrc = ddca_open_display2 (dinfos->info[i].dref, false,
			&(disp->dh));
		if (ddcrc != 0)
			continue; // don't add to list, since we can't open display.

		// If we can't get the brightness, don't add to the list.
		if (ddcbc_display_get_brightness (disp) != 0)
			continue;
		
		g_array_append_val(dlist.list, disp);
		dlist.ct++;
	}

	// Free display infos since they were copied onto the display struct.
	ddca_free_display_info_list (dinfos);

	return dlist;
}

// ddcbc_display_list_free frees the display list and closes all displays.
void 
ddcbc_display_list_free (ddcbc_display_list *dlist) 
{
	for (guint i = 0; i < dlist->ct; i++) {
		ddcbc_display *disp = g_array_index (dlist->list, ddcbc_display*, i);
		
		DDCA_Status ddcrc = ddca_close_display (disp->dh);
		if (ddcrc != 0)
			g_warning ("Error closing display %u", i);

		free (disp);
	}

	free (g_array_free (dlist->list, TRUE));
}

// ddcbc_display_list_get retrives the display at 'dispno'
ddcbc_display* 
ddcbc_display_list_get(ddcbc_display_list *dlist, guint index) 
{
	return g_array_index (dlist->list, ddcbc_display*, index);
}