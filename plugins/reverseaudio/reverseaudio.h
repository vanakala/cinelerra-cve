#ifndef REVERSEAUDIO_H
#define REVERSEAUDIO_H

#include "mainprogress.inc"
#include "pluginaclient.h"
#include "vframe.inc"

class ReverseAudio : public PluginAClient
{
public:
	ReverseAudio(PluginServer *server);
	~ReverseAudio();

// required for  non realtime/multichannel plugins

	char* plugin_title();
	int is_realtime();
	VFrame* new_picon();
	int start_loop();
	int process_loop(double *buffer, int64_t &write_length);
	int stop_loop();

	Defaults *defaults;
	MainProgressBar *progress;

	int64_t current_position;
	int64_t total_written;
};


#endif
