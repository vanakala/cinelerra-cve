#ifndef REVERSEVIDEO_H
#define REVERSEVIDEO_H

#include "pluginvclient.h"

class ReverseVideo : public PluginVClient
{
public:
	ReverseVideo(PluginServer *server);
	~ReverseVideo();

	char* plugin_title();
	VFrame* new_picon();
	int process_loop(VFrame *buffer);

	int start_loop();
	int stop_loop();





	MainProgressBar *progress;
	long current_position;
};




#endif
