#ifndef FLASH_H
#define FLASH_H

class FlashMain;

#include "pluginvclient.h"
#include "vframe.inc"

class FlashMain : public PluginVClient
{
public:
	FlashMain(PluginServer *server);
	~FlashMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int uses_gui();
	int is_transition();
	int is_video();
	char* plugin_title();
	VFrame* new_picon();
};

#endif
