#ifndef DISSOLVE_H
#define DISSOLVE_H

class DissolveMain;

#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class DissolveMain : public PluginVClient
{
public:
	DissolveMain(PluginServer *server);
	~DissolveMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int uses_gui();
	int is_transition();
	int is_video();
	char* plugin_title();
	VFrame* new_picon();
	OverlayFrame *overlayer;
};

#endif
