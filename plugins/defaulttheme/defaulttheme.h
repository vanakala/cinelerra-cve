#ifndef DEFAULTTHEME_H
#define DEFAULTTHEME_H

#include "new.inc"
#include "plugintclient.h"
#include "preferencesthread.inc"
#include "statusbar.inc"
#include "theme.h"
#include "timebar.inc"

class DefaultTheme : public Theme
{
public:
	DefaultTheme();
	~DefaultTheme();

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

	void get_new_sizes(NewWindow *gui);
	void draw_new_bg(NewWindow *gui);
	void draw_setformat_bg(SetFormatWindow *gui);

private:
	void build_icons();
	void build_bg_data();
	void build_patches();
	void build_overlays();


// MWindow
	VFrame *mbutton_left;
	VFrame *mbutton_right;
	VFrame *clock_bg;
	VFrame *patchbay_bg;
	VFrame *tracks_bg;
	VFrame *zoombar_left;
	VFrame *zoombar_right;
	VFrame *statusbar_left;
	VFrame *statusbar_right;

// CWindow
	VFrame *cpanel_bg;
	VFrame *cbuttons_left;
	VFrame *cbuttons_right;
	VFrame *cmeter_bg;

// VWindow
	VFrame *vbuttons_left;
	VFrame *vbuttons_right;
	VFrame *vmeter_bg;

	VFrame *preferences_bg;
	VFrame *new_bg;
	VFrame *setformat_bg;

// Record windows
	VFrame *rgui_batch;
	VFrame *rgui_controls;
	VFrame *rgui_list;
	VFrame *rmonitor_panel;
	VFrame *rmonitor_meters;
};



class DefaultThemeMain : public PluginTClient
{
public:
	DefaultThemeMain(PluginServer *server);
	~DefaultThemeMain();
	
	char* plugin_title();
	Theme* new_theme();
	
	DefaultTheme *theme;
};


#endif
