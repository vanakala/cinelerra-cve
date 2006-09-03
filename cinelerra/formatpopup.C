#include "bcsignals.h"
#include "file.inc"
#include "formatpopup.h"
#include "language.h"
#include "pluginserver.h"



FormatPopup::FormatPopup(ArrayList<PluginServer*> *plugindb, 
	int x, 
	int y,
	int use_brender)
 : BC_ListBox(x, 
 	y, 
	200, 
	200,
	LISTBOX_TEXT,
	0,
	0,
	0,
	1,
	0,
	1)
{
	this->plugindb = plugindb;
	this->use_brender = use_brender;
	set_tooltip(_("Change file format"));
}

int FormatPopup::create_objects()
{
	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(AC3_NAME)));
		format_items.append(new BC_ListBoxItem(_(AIFF_NAME)));
		format_items.append(new BC_ListBoxItem(_(AU_NAME)));
#ifdef USE_AVIFILE
		format_items.append(new BC_ListBoxItem(_(AVI_AVIFILE_NAME)));
#endif
		format_items.append(new BC_ListBoxItem(_(JPEG_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(JPEG_LIST_NAME)));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(AVI_NAME)));
		format_items.append(new BC_ListBoxItem(_(EXR_NAME)));
		format_items.append(new BC_ListBoxItem(_(EXR_LIST_NAME)));
		format_items.append(new BC_ListBoxItem(_(YUV_NAME)));
		format_items.append(new BC_ListBoxItem(_(WAV_NAME)));
		format_items.append(new BC_ListBoxItem(_(MOV_NAME)));
		format_items.append(new BC_ListBoxItem(_(RAWDV_NAME)));
		format_items.append(new BC_ListBoxItem(_(AMPEG_NAME)));
		format_items.append(new BC_ListBoxItem(_(VMPEG_NAME)));
		format_items.append(new BC_ListBoxItem(_(VORBIS_NAME)));
		format_items.append(new BC_ListBoxItem(_(OGG_NAME)));
		format_items.append(new BC_ListBoxItem(_(PCM_NAME)));
		format_items.append(new BC_ListBoxItem(_(PNG_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(PNG_LIST_NAME)));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(TGA_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(TGA_LIST_NAME)));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(TIFF_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(TIFF_LIST_NAME)));
	update(&format_items,
		0,
		0,
		1);
	return 0;
}

FormatPopup::~FormatPopup()
{
	for(int i = 0; i < format_items.total; i++) delete format_items.values[i];
}

int FormatPopup::handle_event()
{
}
