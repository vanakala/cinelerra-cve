// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef THEME_H
#define THEME_H

#include "awindowgui.inc"
#include "batchrender.inc"
#include "bctheme.h"
#include "cwindowgui.inc"
#include "levelwindowgui.inc"
#include "mbuttons.inc"
#include "mwindowgui.inc"
#include "new.inc"
#include "overlayframe.inc"
#include "patchbay.inc"
#include "preferencesthread.inc"
#include "resourcepixmap.inc"
#include "theme.inc"
#include "timebar.inc"
#include "trackcanvas.inc"
#include "setformat.inc"
#include "statusbar.inc"
#include "vframe.inc"
#include "vwindowgui.inc"
#include "zoombar.inc"


// Theme uses static png resources at startup.
// The reason is GUI elements must be constructed at startup from multiple
// pngs.

class Theme : public BC_Theme
{
public:
	Theme();
	virtual ~Theme();

	virtual void initialize();
	virtual void get_mwindow_sizes(MWindowGUI *gui, int w, int h) {};
	virtual void get_vwindow_sizes(VWindowGUI *gui) {};
	virtual void get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls) {};
	virtual void get_awindow_sizes(AWindowGUI *gui);
	virtual void get_batchrender_sizes(BatchRenderGUI *gui, int w, int h);
	virtual void get_plugindialog_sizes();
	virtual void get_menueffect_sizes(int use_list);
	virtual void draw_awindow_bg(AWindowGUI *gui);
	virtual void draw_cwindow_bg(CWindowGUI *gui);
	virtual void draw_lwindow_bg(LevelWindowGUI *gui);
	virtual void draw_mwindow_bg(MWindowGUI *gui) {};
	virtual void draw_vwindow_bg(VWindowGUI *gui) {};
	virtual void draw_resource_bg(TrackCanvas *canvas, ResourcePixmap *pixmap,
		int edit_x, int edit_w, int pixmap_x,
		int x1, int y1, int x2, int y2);

	virtual void get_preferences_sizes() {};
	virtual void draw_preferences_bg(PreferencesWindow *gui) {};
	virtual void get_new_sizes(NewWindow *gui) {};
	virtual void draw_new_bg(NewWindow *gui) {};
	virtual void draw_setformat_bg(SetFormatWindow *window) {};
	void flush_images();
	const char *theme_title;

// colors for the main message text
	int message_normal, message_error;

// Locations
	int abinbuttons_x, abinbuttons_y;
	int abuttons_x, abuttons_y;
	int adivider_x, adivider_y, adivider_w, adivider_h;
	int afolders_x, afolders_y, afolders_w, afolders_h;
	int alist_x, alist_y, alist_w, alist_h;
	int audio_color;
	int browse_pad;
	int cauto_x, cauto_y, cauto_w, cauto_h;
	int ccanvas_x, ccanvas_y, ccanvas_w, ccanvas_h;
	int ccomposite_x, ccomposite_y, ccomposite_w, ccomposite_h;
	int cstatus_x, cstatus_y;
	int cdest_x, cdest_y;
	int cedit_x, cedit_y;
	int channel_position_color;
	int cmeter_x, cmeter_y, cmeter_h;
	int cslider_x, cslider_y, cslider_w;
	int ctime_x, ctime_y;
	int ctimebar_x, ctimebar_y, ctimebar_w, ctimebar_h;
	int ctransport_x, ctransport_y;
	int czoom_x, czoom_y;
	int fade_h;
	int loadfile_pad;
	int loadmode_w;
	int mbuttons_x, mbuttons_y, mbuttons_w, mbuttons_h;
// pixels between end transport button and arrow button
	int mtransport_margin;
	int mcanvas_x, mcanvas_y, mcanvas_w, mcanvas_h;
	int mclock_x, mclock_y, mclock_w, mclock_h;
	int mhscroll_x, mhscroll_y, mhscroll_w;
	int mvscroll_x, mvscroll_y, mvscroll_h;
	int meter_h;
	int mode_h;
	int mstatus_x, mstatus_y, mstatus_w, mstatus_h;
	int mstatus_message_x, mstatus_message_y;
	int mstatus_progress_x, mstatus_progress_y, mstatus_progress_w;
	int mstatus_cancel_x, mstatus_cancel_y;
	int mtimebar_x, mtimebar_y, mtimebar_w, mtimebar_h;
	int mzoom_x, mzoom_y, mzoom_w, mzoom_h;
	int new_audio_x, new_audio_y;
	int new_ok_x, new_ok_y;
	int new_video_x, new_video_y;
	int pan_h;
	int pan_x;
	int play_h;
	int preferencescategory_x, preferencescategory_y;
// Overlap between category buttons
	int preferences_category_overlap;
	int preferencestitle_x, preferencestitle_y;
	int preferencesoptions_x, preferencesoptions_y;
	int patchbay_x, patchbay_y, patchbay_w, patchbay_h;
// pixels between toggles and buttons in edit panel
	int toggle_margin;

	int plugindialog_new_x, plugindialog_new_y, plugindialog_new_w, plugindialog_new_h;
	int plugindialog_shared_x, plugindialog_shared_y, plugindialog_shared_w, plugindialog_shared_h;
	int plugindialog_module_x, plugindialog_module_y, plugindialog_module_w, plugindialog_module_h;
	int plugindialog_newattach_x, plugindialog_newattach_y;
	int plugindialog_sharedattach_x, plugindialog_sharedattach_y;
	int plugindialog_moduleattach_x, plugindialog_moduleattach_y;

	int menueffect_list_x, menueffect_list_y, menueffect_list_w, menueffect_list_h;
	int menueffect_file_x, menueffect_file_y;
	int menueffect_tools_x, menueffect_tools_y;

	int batchrender_x1, batchrender_x2, batchrender_x3;

	int setformat_x1, setformat_x2, setformat_x3, setformat_x4;
	int setformat_y1, setformat_y2, setformat_y3;
	int setformat_w, setformat_h, setformat_margin;
	int setformat_channels_x, setformat_channels_y, setformat_channels_w, setformat_channels_h;
	int title_h;
	int title_font, title_color;
	int edit_font_color;
	int vcanvas_x, vcanvas_y, vcanvas_w, vcanvas_h;
	int vedit_x, vedit_y;
	int vmeter_x, vmeter_y, vmeter_h;
	int vslider_x, vslider_y, vslider_w;
	int vsource_x, vsource_y;
	int vtimebar_x, vtimebar_y, vtimebar_w, vtimebar_h;
	int vtime_x, vtime_y, vtime_w;
	int vtransport_x, vtransport_y;
	int vzoom_x, vzoom_y;

// Bitmaps
	VFrame *about_bg;
	VFrame **appendasset_data;
	VFrame **append_data;
	VFrame **asset_append_data;
	VFrame **asset_disk_data;
	VFrame **asset_index_data;
	VFrame **asset_info_data;
	VFrame **asset_project_data;
	VFrame **browse_data;
	VFrame **calibrate_data;
	VFrame **cancel_data;
	VFrame **chain_data;
	VFrame *channel_position_data;
	VFrame **delete_all_indexes_data;
	VFrame **deletebin_data;
	VFrame **delete_data;
	VFrame **deletedisk_data;
	VFrame **deleteproject_data;
	VFrame **detach_data;
	VFrame **dntriangle_data;

	VFrame **edit_data;
	VFrame **edithandlein_data;
	VFrame **edithandleout_data;
	VFrame **extract_data;
	VFrame **infoasset_data;
	VFrame **in_point;
	VFrame **insert_data;
	VFrame *keyframe_data;
	VFrame **label_toggle;
	VFrame **lift_data;
	VFrame *maskkeyframe_data;
	VFrame *modekeyframe_data;
	VFrame *cropkeyframe_data;
	VFrame **movedn_data;
	VFrame **moveup_data;
	VFrame **newbin_data;
	VFrame **no_data;
	VFrame **options_data;
	VFrame **out_point;
	VFrame **over_button;
	VFrame **overwrite_data;
	VFrame *pankeyframe_data;
	VFrame **pasteasset_data;
	VFrame **paused_data;
	VFrame **picture_data;
	VFrame **presentation_data;
	VFrame **presentation_loop;
	VFrame **presentation_stop;
	VFrame **redrawindex_data;
	VFrame **renamebin_data;
	VFrame **reset_data;
	VFrame **reverse_data;
	VFrame **rewind_data;
	VFrame **select_data;
	VFrame **splice_data;
	VFrame **start_over_data;
	VFrame **statusbar_cancel_data;
	VFrame *timebar_view_data;
	VFrame **transition_data;
	VFrame **uptriangle_data;
	VFrame **viewasset_data;
	VFrame *vtimebar_bg_data;

// Compressed images are loaded in here.
	char *data_buffer;
	char *contents_buffer;
	ArrayList<char*> contents;
	ArrayList<int> offsets;
	char *last_image;
	int last_offset;
};

#endif
