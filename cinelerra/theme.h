#ifndef THEME_H
#define THEME_H

#include "awindowgui.inc"
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



class Theme
{
public:
	Theme();
	virtual ~Theme();


	virtual void initialize();
	virtual void get_mwindow_sizes(MWindowGUI *gui, 
		int w, 
		int h);
	virtual void get_vwindow_sizes(VWindowGUI *gui);
	virtual void get_cwindow_sizes(CWindowGUI *gui);
	virtual void get_awindow_sizes(AWindowGUI *gui);
	virtual void get_recordmonitor_sizes(int do_audio, 
		int do_video);
	virtual void get_recordgui_sizes(RecordGUI *gui,
		int w,
		int h);
	virtual void get_plugindialog_sizes();
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
	unsigned char* get_image(char *title);
	void flush_images();

	ArrayList<BC_ListBoxItem*> aspect_ratios;
	ArrayList<BC_ListBoxItem*> frame_rates;
	ArrayList<BC_ListBoxItem*> frame_sizes;
	ArrayList<BC_ListBoxItem*> sample_rates;
	ArrayList<BC_ListBoxItem*> zoom_values;
	char *theme_title;

// Tools for building widgets
	void overlay(VFrame *dst, VFrame *src, int in_x1 = -1, int in_x2 = -1);
	void build_transport(VFrame** &data,
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
	int mcanvas_x, mcanvas_y, mcanvas_w, mcanvas_h;
	int mclock_x, mclock_y, mclock_w, mclock_h;
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
	int patchbay_x, patchbay_y, patchbay_w, patchbay_h;

	int plugindialog_new_x, plugindialog_new_y, plugindialog_new_w, plugindialog_new_h;
	int plugindialog_shared_x, plugindialog_shared_y, plugindialog_shared_w, plugindialog_shared_h;
	int plugindialog_module_x, plugindialog_module_y, plugindialog_module_w, plugindialog_module_h;
	int plugindialog_newattach_x, plugindialog_newattach_y;
	int plugindialog_sharedattach_x, plugindialog_sharedattach_y;
	int plugindialog_moduleattach_x, plugindialog_moduleattach_y;

	int recordgui_batches_w, recordgui_batches_h;
	int recordgui_batches_x, recordgui_batches_y;
	int recordgui_batch_x, recordgui_batch_y, recordgui_batchcaption_x;
	int recordgui_buttons_x, recordgui_buttons_y;
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
	int setformat_x1, setformat_x2, setformat_x3, setformat_x4, setformat_y1, setformat_w, setformat_h, setformat_margin;
	int setformat_channels_x, setformat_channels_y, setformat_channels_w, setformat_channels_h;
	int title_h;
	int title_font, title_color;
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
	VFrame *about_microsoft;
	VFrame **appendasset_data;
	VFrame **append_data;
	VFrame **arrow_data;
	VFrame **asset_append_data;
	VFrame **asset_disk_data;
	VFrame **asset_index_data;
	VFrame **asset_info_data;
	VFrame **asset_project_data;
	VFrame **autokeyframe_data;
	VFrame *awindow_icon;
	VFrame **bottom_justify;
	VFrame **browse_data;
	VFrame **calibrate_data;
	VFrame **camera_data;
	VFrame *camerakeyframe_data;
	VFrame **cancel_data;
	VFrame **center_justify;
	VFrame **chain_data;
	VFrame *channel_bg_data;
	VFrame **channel_data;
	VFrame *channel_position_data;
	VFrame *clip_icon;
	VFrame **copy_data;
	VFrame **crop_data;
	VFrame **cut_data;
	VFrame *cwindow_icon;
	VFrame **delete_all_indexes_data;
	VFrame **deletebin_data;
	VFrame **delete_data;
	VFrame **deletedisk_data;
	VFrame **deleteproject_data;
	VFrame **detach_data;
	VFrame **dntriangle_data;
	VFrame **drawpatch_data;
	VFrame **duplex_data;
	VFrame **edit_data;
	VFrame **edithandlein_data;
	VFrame **edithandleout_data;
	VFrame **end_data;
	VFrame **expandpatch_data;
	VFrame **extract_data;
	VFrame **fastfwd_data;
	VFrame **fastrev_data;
	VFrame **fit_data;
	VFrame **forward_data;
	VFrame **framefwd_data;
	VFrame **framerev_data;
	VFrame **gangpatch_data;
	VFrame **ibeam_data;
	VFrame **in_data;
	VFrame **indelete_data;
	VFrame **infoasset_data;
	VFrame **in_point;
	VFrame **insert_data;
	VFrame *keyframe_data;
	VFrame **labelbutton_data;
	VFrame **label_toggle;
	VFrame **left_justify;
	VFrame **lift_data;
	VFrame **magnify_button_data;
	VFrame **magnify_data;
	VFrame **mask_data;
	VFrame *maskkeyframe_data;
	VFrame **middle_justify;
	VFrame *modekeyframe_data;
	VFrame **movedn_data;
	VFrame **moveup_data;
	VFrame **mutepatch_data;
	VFrame *mwindow_icon;
	VFrame **newbin_data;
	VFrame **nextlabel_data;
	VFrame **no_data;
	VFrame **options_data;
	VFrame **out_data;
	VFrame **outdelete_data;
	VFrame **out_point;
	VFrame **over_button;
	VFrame **overwrite_data;
	VFrame *pankeyframe_data;
	VFrame **pasteasset_data;
	VFrame **paste_data;
	VFrame *patchbay_bg;
	VFrame **pause_data;
	VFrame **paused_data;
	VFrame **picture_data;
	VFrame **playpatch_data;
	VFrame *plugin_bg_data;
	VFrame **presentation_data;
	VFrame **presentation_loop;
	VFrame **presentation_stop;
	VFrame **prevlabel_data;
	VFrame **proj_data;
	VFrame *projectorkeyframe_data;
	VFrame **protect_data;
	VFrame **rec_data;
	VFrame **recframe_data;
	VFrame *record_icon;
	VFrame **recordpatch_data;
	VFrame **redo_data;
	VFrame **redrawindex_data;
	VFrame **renamebin_data;
	VFrame **reset_data;
	VFrame *resource1024_bg_data;
	VFrame *resource128_bg_data;
	VFrame *resource256_bg_data;
	VFrame *resource32_bg_data;
	VFrame *resource512_bg_data;
	VFrame *resource64_bg_data;
	VFrame **reverse_data;
	VFrame **rewind_data;
	VFrame **right_justify;
	VFrame **select_data;
	VFrame **show_meters;
	VFrame **splice_data;
	VFrame **start_over_data;
	VFrame **statusbar_cancel_data;
	VFrame **stop_data;
	VFrame **stoprec_data;
	VFrame *timebar_bg_data;
	VFrame *timebar_brender_data;
	VFrame *timebar_view_data;
	VFrame *title_bg_data;
	VFrame **titlesafe_data;
	VFrame **toclip_data;
	VFrame **tool_data;
	VFrame **top_justify;
	VFrame **transition_data;
	VFrame **undo_data;
	VFrame **uptriangle_data;
	VFrame **viewasset_data;
	VFrame *vtimebar_bg_data;
	VFrame *vwindow_icon;
	VFrame **wrench_data;
	VFrame **yes_data;


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
