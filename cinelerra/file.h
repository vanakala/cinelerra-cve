#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

#include "assets.inc"
#include "edit.inc"
#include "file.inc"
#include "filebase.inc"
#include "filethread.inc"
#include "filexml.inc"
#include "formatwindow.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginserver.inc"
#include "resample.inc"
#include "sema.h"
#include "vframe.inc"


// ======================================= include file types here

// generic file opened by user
class File
{
public:
	File();
	~File();

// Get attributes for various file formats.
// The dither parameter is carried over from recording, where dither is done at the device.
	int get_options(BC_WindowBase *parent_window, 
		ArrayList<PluginServer*> *plugindb, 
		Asset *asset,
		int audio_options,
		int video_options,
		int lock_compressor);
	int raise_window();

// ===================================== start here
	int set_processors(int cpus);   // Set the number of cpus for certain codecs.
	int set_preload(int64_t size);     // Set the number of bytes to preload during reads.
// When loading, the asset is deleted and a copy created in the EDL.
	void set_asset(Asset *asset);

// Format may be preset if the asset format is not 0.
	int open_file(ArrayList<PluginServer*> *plugindb, 
		Asset *asset, 
		int rd, 
		int wr,
		int64_t base_samplerate,
		float base_framerate);

// start a thread for writing to avoid blocking during record
	int start_audio_thread(int64_t buffer_size, int ring_buffers);
	int stop_audio_thread();
	int start_video_thread(int64_t buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed);
	int stop_video_thread();
	int lock_read();
	int unlock_read();

// write any headers and close file
// ignore_thread is used by SigHandler to break out of the threads.
	int close_file(int ignore_thread = 0);

// set channel for buffer accesses
	int set_channel(int channel);

// set layer for video read
	int set_layer(int layer);

// get length of file normalized to base samplerate
	int64_t get_audio_length(int64_t base_samplerate = -1);
	int64_t get_video_length(float base_framerate = -1);

// get current position
	int64_t get_audio_position(int64_t base_samplerate = -1);
	int64_t get_video_position(float base_framerate = -1);
	

// set position in samples
	int set_audio_position(int64_t position, float base_samplerate);
// set position in frames
	int set_video_position(int64_t position, float base_framerate);

// write samples for the current channel
// written to disk and file pointer updated after last channel is written
// return 1 if failed
// subsequent writes must be <= than first write's size because of buffers
	int write_samples(double **buffer, int64_t len);

// Only called by filethread
	int write_frames(VFrame ***frames, int len);

// For writing buffers in a background thread use these functions to get the buffer.
// Get a pointer to a buffer to write to.
	double** get_audio_buffer();
	VFrame*** get_video_buffer();

// Schedule a buffer for writing on the thread.
// thread calls write_samples
	int write_audio_buffer(int64_t len);
	int write_video_buffer(int64_t len);

// Read samples for one channel into a shared memory segment.
// The offset is the offset in floats from the beginning of the buffer and the len
// is the length in floats from the offset.
// advances file pointer
// return 1 if failed
	int read_samples(double *buffer, int64_t len, int64_t base_samplerate);

// Return a pointer to the frame in a video file for drawing or 0.
// The following routine copies a frame once to a temporary buffer and either 
// returns a pointer to the temporary buffer or copies the temporary buffer again.
	VFrame* read_frame(int color_model);

	int read_frame(VFrame *frame);

// The following involve no extra copies.
// Direct copy routines for direct copy playback
	int can_copy_from(Edit *edit, int64_t position, int output_w, int output_h); // This file can copy frames directly from the asset
	int get_render_strategy(ArrayList<int>* render_strategies);
	int64_t compressed_frame_size();
	int read_compressed_frame(VFrame *buffer);
	int write_compressed_frame(VFrame *buffer);

// These are separated into two routines so a file doesn't have to be
// allocated.
// Get best colormodel to translate for hardware acceleration
	int get_best_colormodel(int driver);
	static int get_best_colormodel(Asset *asset, int driver);
// Get nearest colormodel in format after the file is opened and the 
// direction determined to know whether to use a temp.
	int colormodel_supported(int colormodel);



	static int supports_video(ArrayList<PluginServer*> *plugindb, char *format);   // returns 1 if the format supports video or audio
	static int supports_audio(ArrayList<PluginServer*> *plugindb, char *format);
	static int supports_video(int format);   // returns 1 if the format supports video or audio
	static int supports_audio(int format);
	static int strtoformat(ArrayList<PluginServer*> *plugindb, char *format);
	static char* formattostr(ArrayList<PluginServer*> *plugindb, int format);
	static int strtobits(char *bits);
	static char* bitstostr(int bits);
	static int str_to_byteorder(char *string);
	static char* byteorder_to_str(int byte_order);
	int bytes_per_sample(int bits); // Convert the bit descriptor into a byte count.

	Asset *asset;    // Copy of asset since File outlives EDL
	FileBase *file; // virtual class for file type
// Threads for writing data in the background.
	FileThread *audio_thread, *video_thread; 
// Temporary storage for color conversions
	VFrame *temp_frame;
// Frame to return pointer to
	VFrame *return_frame;
// Resampling engine
	Resample *resample;

// Lock writes while recording video and audio.
// A binary lock won't do.  We need a FIFO lock.
	Sema write_lock;
	int cpus;
	int64_t playback_preload;

// Position information is migrated here to allow samplerate conversion.
// Current position in file's samplerate.
// Can't normalize to base samplerate because this would 
// require fractional positioning to know if the file's position changed.
	int64_t current_sample;
	int64_t current_frame;
	int current_channel;
	int current_layer;

// Position information normalized
	int64_t normalized_sample;
	int64_t normalized_sample_rate;

private:
	void reset_parameters();

	int getting_options;
	BC_WindowBase *format_window;
	Mutex format_completion;
};

#endif
