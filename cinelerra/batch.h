#ifndef RECORDBATCH_H
#define RECORDBATCH_H

#include "asset.inc"
#include "channel.inc"
#include "edl.inc"
#include "file.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "record.inc"
#include "recordlabel.inc"

class Batch
{
public:
	Batch(MWindow *mwindow, Record *record);
	~Batch();

	int create_objects();
	static char* mode_to_text(int record_mode);
	static int text_to_mode(char *text);
	Asset* get_current_asset();
	char* get_source_text();
	Channel* get_current_channel_struct();
	void calculate_news();
	void create_default_path();
	void copy_from(Batch *batch);
	void toggle_label(double position);
	void start_over();

	MWindow *mwindow;
	Record *record;
// List of assets for every file created.
// First asset is the default asset.
	ArrayList<Asset*> assets;
// Created during recording if scene to scene.
// Created after recording if NULL
	EDL *edl;

	int current_asset;
	int recorded;

// Add new sources for every input device
// Entry in channel table to record from
	int channel;

// Time offset of start of current file from batch start
	double file_offset;
// Type of start time to begin at
	int start_type;
// Batch is enabled
	int enabled;
// Start time of batch
	double start_time;
	int start_day;
// Number of seconds for all recording except infinite
	double duration;
// Flash the color of the display while waiting for start.of batch
	int waiting;
	RecordLabels *labels;
// Totals for current batch
	long total_samples, total_frames;
// Current positions
	long current_sample, current_frame;
// Totals for synchronization
	long session_samples, session_frames;

// Info for warning field
	int file_exists;
// Record mode #define
	int record_mode;
	char news[BCTEXTLEN];
};

#endif
