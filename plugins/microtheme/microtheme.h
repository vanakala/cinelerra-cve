#ifndef MICROTHEME_H
#define MICROTHEME_H

#include "plugintclient.h"
#include "statusbar.inc"
#include "theme.h"
#include "trackcanvas.inc"
#include "timebar.inc"

class MicroTheme : public Theme
{
public:
	MicroTheme();
	~MicroTheme();

	void draw_mwindow_bg(MWindowGUI *gui);
	void get_mwindow_sizes(MWindowGUI *gui, int w, int h);
	void get_cwindow_sizes(CWindowGUI *gui);
	void get_vwindow_sizes(VWindowGUI *gui);
	void get_recordgui_sizes(RecordGUI *gui, int w, int h);

	void initialize();
};

class MicroThemeMain : public PluginTClient
{
public:
	MicroThemeMain(PluginServer *server);
	~MicroThemeMain();

	char* plugin_title();
	Theme* new_theme();
	
	MicroTheme *theme;
};




#endif
