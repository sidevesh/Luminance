// DDC/CI API for brightness and contrast control using ddcutil

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>
#include <gmodule.h>

#include "ddcutil_c_api.h"
#include "ddcutil_status_codes.h"

#define BRIGHTNESS_CODE 0x10
#define CONTRAST_CODE 0x12

// DDC_Status adds a couple of possible errors in the -4000 range.
typedef DDCA_Status DDC_Status;
#define DDC_BAD_VALUE -4000

// ddc_display encompasses info of a supported DDC display.
typedef struct
ddc_display
{
	guint16 max_val;
	guint16 last_val;         // last recorded brightness value
	guint16 contrast_max_val;
	guint16 contrast_last_val; // last recorded contrast value
	gboolean has_contrast;
	DDCA_Display_Info info;
	DDCA_Display_Handle dh;
} ddc_display;

// ddc_display_get_brightness gets the brightness of `disp` and caches the
// maximum possible brightness value. Any errors will be a DDC error.
DDC_Status
ddc_display_get_brightness (ddc_display *disp)
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

// ddc_display_set_brightness sets the brightness of a given monitor.
DDC_Status
ddc_display_set_brightness (ddc_display *disp, guint16 new_val)
{
	if (new_val > disp->max_val)
		return DDC_BAD_VALUE;

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

// ddc_display_get_contrast gets the contrast of `disp` and caches the
// maximum possible contrast value. Returns non-zero if contrast is not
// supported by the display.
DDC_Status
ddc_display_get_contrast (ddc_display *disp)
{
	DDCA_Non_Table_Vcp_Value valrec;
	DDCA_Status ddcrc = ddca_get_non_table_vcp_value (disp->dh,
		CONTRAST_CODE, &valrec);

	if (ddcrc != 0)
		return ddcrc;

	disp->contrast_last_val = valrec.sh << 8 | valrec.sl;
	disp->contrast_max_val = valrec.mh << 8 | valrec.ml;

	return 0;
}

// ddc_display_set_contrast sets the contrast of a given monitor.
DDC_Status
ddc_display_set_contrast (ddc_display *disp, guint16 new_val)
{
	if (new_val > disp->contrast_max_val)
		return DDC_BAD_VALUE;

	guint8 new_val_high = new_val >> 8;
	guint8 new_val_low = new_val & 0xff;

	DDCA_Status ddcrc = ddca_set_non_table_vcp_value (disp->dh,
		CONTRAST_CODE, new_val_high, new_val_low);

	if (ddcrc != 0)
		return ddcrc;

	disp->contrast_last_val = new_val;

	return 0;
}

// ddc_display_list is a GArray of ddc_displays
typedef struct
{
	guint ct;
	GArray* list;
} ddc_display_list;

// ddc_display_list_init returns a list of supported DDC displays.
// If wait is true, the function will instruct ddca to wait if a display
// is locked by another process.
ddc_display_list
ddc_display_list_init (gboolean wait)
{
	DDCA_Display_Info_List *dinfos;
	ddca_get_display_info_list2(FALSE, &dinfos);

	ddc_display_list dlist;
	dlist.list = g_array_new(FALSE, FALSE, sizeof (ddc_display*));
	dlist.ct = 0;
	for (guint i = 0; i < dinfos->ct; i++) {
		ddc_display *disp = malloc (sizeof (ddc_display));
		disp->info = dinfos->info[i];
		disp->has_contrast = FALSE;
		disp->contrast_max_val = 0;
		disp->contrast_last_val = 0;

		DDCA_Status ddcrc = ddca_open_display2 (dinfos->info[i].dref, false,
			&(disp->dh));
		if (ddcrc != 0)
			continue;

		// If we can't get the brightness, don't add to the list.
		if (ddc_display_get_brightness (disp) != 0)
			continue;

		// Try to read contrast; not all monitors support it.
		if (ddc_display_get_contrast (disp) == 0)
			disp->has_contrast = TRUE;

		g_array_append_val(dlist.list, disp);
		dlist.ct++;
	}

	// Free display infos since they were copied onto the display struct.
	ddca_free_display_info_list (dinfos);

	return dlist;
}

// ddc_display_list_free frees the display list and closes all displays.
void
ddc_display_list_free (ddc_display_list *dlist)
{
	for (guint i = 0; i < dlist->ct; i++) {
		ddc_display *disp = g_array_index (dlist->list, ddc_display*, i);

		DDCA_Status ddcrc = ddca_close_display (disp->dh);
		if (ddcrc != 0)
			g_warning ("Error closing display %u", i);

		free (disp);
	}

	free (g_array_free (dlist->list, TRUE));
}

// ddc_display_list_get retrieves the display at 'index'
ddc_display*
ddc_display_list_get(ddc_display_list *dlist, guint index)
{
	return g_array_index (dlist->list, ddc_display*, index);
}
