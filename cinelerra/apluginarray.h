#ifndef APLUGINARRAY_H
#define APLUGINARRAY_H

#include "amodule.inc"
#include "edl.inc"
#include "pluginarray.h"
#include "pluginserver.inc"
#include "recordableatracks.inc"
#include "track.inc"

class APluginArray : public PluginArray
{
public:
	APluginArray();
	~APluginArray();

	long get_bufsize();
	void create_buffers();
	void create_modules();
	void load_module(int module, long input_position, long len);
	void process_realtime(int module, long input_position, long len);
	int process_loop(int module, long &write_length);
	int write_buffers(long len);
	int total_tracks();
	void get_recordable_tracks();
	Track* track_number(int number);

	RecordableATracks *tracks;
// Pointers to plugin buffers for plugin output
	double **buffer;         // Buffer for processing
// Pointer to file output
	double **output_buffer;
	double **realtime_buffers;
};

#endif
