#ifndef NORMALIZE_H
#define NORMALIZE_H

#include "bchash.inc"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "vframe.inc"


class NormalizeMain : public PluginAClient
{
public:
	NormalizeMain(PluginServer *server);
	~NormalizeMain();

// normalizing engine

// parameters needed

	float db_over;
	int separate_tracks;

// required for all non realtime/multichannel plugins

	VFrame* new_picon();
	char* plugin_title();
	int is_realtime();
	int is_multichannel();
	int get_parameters();
	int start_loop();
	int process_loop(double **buffer, int64_t &write_length);
	int stop_loop();

	int load_defaults();  
	int save_defaults();  

	BC_Hash *defaults;
	MainProgressBar *progress;

// Current state of process_loop
	int writing;
	int64_t current_position;
	double *peak, *scale;
};


#endif
