
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef THEME_H
#define THEME_H

#include "awindowgui.inc"
#include "batchrender.inc"
#include "bctheme.h"
#include "cwindowgui.inc"
#include "guicast.h"
#include "levelwindowgui.inc"
#include "mbuttons.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "new.inc"
#include "overlayframe.inc"
#include "patchbay.inc"
#include "preferencesthread.inc"
#include "recordgui.inc"
#include "recordmonitor.inc"
#include "resourcepixmap.inc"
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
	virtual void get_mwindow_sizes(MWindowGUI *gui, 
		int w, 
		int h);
	virtual void get_vwindow_sizes(VWindowGUI *gui);
	virtual void get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls);
	virtual void get_awindow_sizes(AWindowGUI *gui);
	virtual void get_rmonitor_sizes(int do_audio, 
		int do_video,
		int do_channel,
		int do_interlace,
		int do_avc,
		int audio_channels);
	virtual void get_recordgui_sizes(RecordGUI *gui,
		int w,
		int h);
	virtual void get_batchrender_sizes(BatchRenderGUI *gui,
		int w, 
		int h);
	virtual void get_plugindialog_sizes();
	virtual void get_menueffect_sizes(int use_list);
	virtual void draw_rwindow_bg(RecordGUI *gui);
	virtual void draw_rmonitor_bg(RecordMonitorGUI *gui);
	virtual void draw_awindow_bg(AWindowGUI *gui);
	virtual void draw_cwindow_bg(CWindowGUI *gui);
	virtual void draw_lwindow_bg(LevelWindowGUI *gui);
	virtual void draw_mwindow_bg(MWindowGUI *gui);
	virtual void draw_vwindow_bg(VWindowGUI *gui);
	virtual void draw_resource_bg(TrackCanvas *canvas,
		ResourcePixmap *pixmap, 
		int edit_x,
		int edit_w,
		int pixmap_x,
		int x1, 
		int y1, 
		int x2,
		int y2);

	virtual void get_preferences_sizes();
	virtual void draw_preferences_bg(PreferencesWindow *gui);
	virtual void get_new_sizes(NewWindow *gui);
	virtual void draw_new_bg(NewWindow *gui);
	virtual void draw_setformat_bg(SetFormatWindow *window);

	virtual void build_menus();
//	unsigned char* get_image(char *title);
	void flush_images();

	ArrayList<BC_ListBoxItem*> aspect_ratios;
	ArrayList<BC_ListBoxItem*> frame_rates;
	ArrayList<BC_ListBoxItem*> frame_sizes;
	ArrayList<BC_ListBoxItem*> sample_rates;
	ArrayList<BC_ListBoxItem*> zoom_values;
	char *theme_title;

// Tools for building widgets
	void overlay(VFrame *dst, VFrame *src, int in_x1 = -1, int in_x2 = -1);
	void build_transport(char *title,
		unsigned char *png_overlay,
		VFrame **bg_data,
		int region);
	void build_patches(VFrame** &data,
		unsigned char *png_overlay,
		VFrame **bg_data,
		int region);
	void build_button(VFrame** &data,
		unsigned char *png_overlay,
		VFrame *up_vframe,
		VFrame *hi_vframe,
		VFrame *dn_vframe);
	void build_toggle(VFrame** &data,
		unsigned char *png_overlay,
		VFrame *up_vframe,
		VFrame *hi_vframe,
		VFrame *checked_vframe,
		VFrame *dn_vframe,
		VFrame *checkedhi_vframe);

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


	int recordgui_batches_w, recordgui_batches_h;
	int recordgui_batches_x, recordgui_batches_y;
	int recordgui_batch_x, recordgui_batch_y, recordgui_batchcaption_x;
	int recordgui_options_x, recordgui_options_y;
	int recordgui_controls_x, recordgui_controls_y;
	int recordgui_loadmode_x, recordgui_loadmode_y;
	int recordgui_status_x, recordgui_status_y, recordgui_status_x2;
	int recordgui_transport_x, recordgui_transport_y;
	int recordgui_fixed_color, recordgui_variable_color;
	int rmonitor_canvas_w, rmonitor_canvas_h;
	int rmonitor_canvas_x, rmonitor_canvas_y;
	int rmonitor_channel_x, rmonitor_channel_y;
	int rmonitor_interlace_x, rmonitor_interlace_y;
	int rmonitor_meter_h;
	int rmonitor_meter_x, rmonitor_meter_y;
	int rmonitor_source_x, rmonitor_source_y;
	int rmonitor_tx_x, rmonitor_tx_y;

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
	VFrame *camerakeyframe_data;
	VFrame **cancel_data;
	VFrame **chain_data;
	VFrame *channel_bg_data;
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
	VFrame *projectorkeyframe_data;
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



	MWindow *mwindow;
// Compressed images are loaded in here.
	char *data_buffer;
	char *contents_buffer;
	ArrayList<char*> contents;
	ArrayList<int> offsets;
	char path[BCTEXTLEN];
	char *last_image;
	int last_offset;
};

#endif
