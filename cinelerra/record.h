#ifndef RECORD_H
#define RECORD_H

#include "assets.inc"
#include "audiodevice.inc"
#include "batch.inc"
#include "guicast.h"
#include "bitspopup.h"
#include "browsebutton.h"
#include "channel.inc"
#include "defaults.inc"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "filethread.inc"
#include "formatpopup.h"
#include "formattools.inc"
#include "loadmode.inc"
#include "mwindow.inc"
#include "maxchannels.h"
#include "mutex.h"
#include "playbackengine.inc"
#include "record.inc"
#include "recordengine.inc"
#include "recordgui.inc"
#include "recordmonitor.inc"
#include "recordthread.inc"
#include "recordwindow.inc"
#include "videodevice.inc"

class Record;

class RecordMenuItem : public BC_MenuItem
{
public:
	RecordMenuItem(MWindow *mwindow);
	~RecordMenuItem();

	int handle_event();

	Record *thread;
	MWindow *mwindow;
// State of recording dialog
	int current_state;
};

class Record : public Thread
{
public:
	Record(MWindow *mwindow, RecordMenuItem *menu_item);
	~Record();

	void run();
	int load_defaults();
	int save_defaults();
	Batch* new_batch();
	void delete_batch();
	void activate_batch(int number, int stop_operation);
	void change_editing_batch(int number);
	void close_output_file();
	void delete_batch(int number);
	void swap_batches(int number1, int number2);
	void get_audio_write_length(int64_t &buffer_size, int64_t &fragment_size);
	int open_input_devices(int duplex, int context);
	int close_input_devices();
	void start_file_threads();
	int start_recording(int duplex, 
		int context /* = CONTEXT_INTERACTIVE */);
	int record_frame();
	int start_monitor();
	int pause_monitor();
	int resume_monitor();
// no_monitor causes the monitor not to be resumed after the operation
	int stop_operation(int resume_monitor);
	int stop_duplex();
	void stop_file_threads();
	int stop_monitor();
	int get_in_length();
	int set_video_picture();
// Set screencapture translation
	void set_translation(int x, int y);
// Set the channel in the current batch
	int set_channel(int channel);
	void set_channel(Channel *channel);
	void toggle_label();
// Set values in batch structures
	void configure_batches();
	void source_to_text(char *string, Batch *batch);
// Create first file in batch
	int open_output_file();
// Delete the output file for overwrite if it exists.
	int delete_output_file();
// Create next file in batch
	int init_next_file();
	void rewind_file();
// Get the channeldb used by the device
	ArrayList<Channel*>* current_channeldb();
	ArrayList<char*>* get_video_inputs();


// Copied to each batch for the files
	Asset *default_asset;
	ArrayList<Batch*> batches;
// How to combine the batches with the project
	int load_mode;
	int do_audio;
	int do_video;
	int monitor_audio;
	int monitor_video;
	int video_window_open;
// Prompt before cancel
	int prompt_cancel;
// Compression is fixed by the driver
	int fixed_compression;
// Current batch being recorded to
	int current_batch;
// Current batch being edited.  Don't want to interrupt recording to edit a different batch.
	int editing_batch;
// Get next batch using activation or -1
	int get_next_batch();
// Information about the current batch.
	Batch* get_current_batch();
// Information about the batch being edited
	Batch* get_editing_batch();
	char* current_mode();
	char* current_source();
	int get_current_channel();
	int get_editing_channel();
	void get_current_time(double &seconds, int &day);
	Channel* get_current_channel_struct();
	char* current_news();
	double* current_start();
	double* current_duration();
	Asset* current_asset();
	int* current_offset_type();
// Total number of samples since record sequence started
	int64_t sync_position();
// Current position for GUI relative to batch
	double current_display_position();
	double current_display_length();
	int64_t current_audio_position();
	int64_t current_duration_samples();
	int64_t current_video_position();
	int64_t current_duration_frames();
// Number of frames between the current file and the start of the batch
	int64_t batch_video_offset();
// Rewind the current file in the current batch
	void start_over();

	LoadMode *loadmode;
	MWindow *mwindow;
	RecordGUI *record_gui;
	RecordMonitor *record_monitor;
	RecordThread *monitor_engine;
	RecordThread *record_engine;
	AudioDevice *adevice;
	VideoDevice *vdevice;
// File handle of last asset.in current batch
	File *file;

	PlaybackEngine *duplex_engine;
	RecordWindow *record_window;
// Table for LML conversion
//	unsigned char _601_to_rgb_table[256];
// For video synchronization when no audio thread
	Timer monitor_timer;
// State of the capturing operation
	int capture_state;
// Translation of screencapture input
	int video_x;
	int video_y;
	float video_zoom;
	int reverse_interlace;     // Reverse the interlace in the video window display only
// Color model for uncompressed device interface
	int color_model;
// Picture quality
	int video_brightness;
	int video_hue;
	int video_color;
	int video_contrast;
	int video_whiteness;
// Fill dropped frames with duplicates
	int fill_frames;
// Parameters for video monitor
	EDL *edl;


















// Number of channeldb entry
	int current_channel;

	FileXML *script;
	RecordMenuItem *menu_item;

	int set_script(FileXML *script);
	int run_script(Asset *asset, int &do_audio, int &do_video);
	int save_engine_defaults();

	float get_min_db();
	int get_samplerate();
	float get_framerate();
	int get_everyframe();
	int get_time_format();
	int get_realtime();
	float get_frame_rate();
	char* get_in_path();
	char* get_video_inpath();
	int get_video_driver();
	int get_vu_format();
	int get_rec_mode();
	int set_rec_mode(int value);
	int set_loop_duration(int64_t value);
	int use_floatingpoint();

	int get_out_length();   // Length to write during playback
	int64_t get_out_buffersize();  // Same as get_out_length
	int get_software_positioning();
	int64_t get_in_buffersize();      // Length to write to disk at a time
	int get_video_buffersize();    // Number of frames to write to disk at a time
	int get_video_capturesize();    // Number of frames to read from device at a time
	int get_meter_over_hold(int divisions);
	int get_meter_peak_hold(int divisions);
	int get_meter_speed();

	int64_t get_playback_buffer();
	int enable_duplex();

	int realtime;
	int to_tracks;
	int64_t loop_duration;
	int64_t startsource_sample;           // start in source file of this recording
	int64_t endsource_sample;
	int64_t startsource_frame;
	int64_t endsource_frame;
	int64_t dc_offset[MAXCHANNELS];
	int append_to_file;
	int record_mode;
	int frame_w;
	int frame_h;
	int video_window_w;       // Width of record video window
	int cpus;
};


#endif
