#ifndef DEFAULTTHEME_H
#define DEFAULTTHEME_H

#include "new.inc"
#include "plugintclient.h"
#include "preferencesthread.inc"
#include "statusbar.inc"
#include "theme.h"
#include "timebar.inc"

class SUV : public Theme
{
public:
	SUV();
	~SUV();

	void initialize();
	void draw_mwindow_bg(MWindowGUI *gui);

	void draw_rwindow_bg(RecordGUI *gui);
	void draw_rmonitor_bg(RecordMonitorGUI *gui);
	void draw_cwindow_bg(CWindowGUI *gui);
	void draw_vwindow_bg(VWindowGUI *gui);
	void draw_preferences_bg(PreferencesWindow *gui);

	void get_mwindow_sizes(MWindowGUI *gui, int w, int h);
	void get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls);
	void get_vwindow_sizes(VWindowGUI *gui);
	void get_preferences_sizes();
	void get_recordgui_sizes(RecordGUI *gui, int w, int h);
	void get_rmonitor_sizes(int do_audio, 
		int do_video,
		int do_channel,
		int do_interlace,
		int do_avc,
		int audio_channels);

	void get_new_sizes(NewWindow *gui);
	void draw_new_bg(NewWindow *gui);
	void draw_setformat_bg(SetFormatWindow *gui);
	void get_plugindialog_sizes();

private:
	void build_icons();
	void build_bg_data();
	void build_patches();
	void build_overlays();





// Record windows
	VFrame *rgui_batch;
	VFrame *rgui_controls;
	VFrame *rgui_list;
	VFrame *rmonitor_panel;
	VFrame *rmonitor_meters;
};



class SUVMain : public PluginTClient
{
public:
	SUVMain(PluginServer *server);
	~SUVMain();
	
	char* plugin_title();
	Theme* new_theme();
	
	SUV *theme;
};


#endif
