#ifndef CROSSFADE_H
#define CROSSFADE_H

class CrossfadeMain;

#include "overlayframe.inc"
#include "pluginaclient.h"
#include "vframe.inc"

class CrossfadeMain : public PluginAClient
{
public:
	CrossfadeMain(PluginServer *server);
	~CrossfadeMain();

// required for all realtime plugins
	int process_realtime(long size, double *input_ptr, double *output_ptr);
	int uses_gui();
	int is_transition();
	char* plugin_title();
	VFrame* new_picon();
};

#endif
