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

	int64_t get_bufsize();
	void create_buffers();
	void create_modules();
//	void load_module(int module, int64_t input_position, int64_t len);
	void process_realtime(int module, int64_t input_position, int64_t len);
	int process_loop(int module, int64_t &write_length);
	int write_buffers(int64_t len);
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
