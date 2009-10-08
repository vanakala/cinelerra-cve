
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

#include "asset.inc"
#include "condition.inc"
#include "edit.inc"
#include "filebase.inc"
#include "file.inc"
#include "filethread.inc"
#include "filexml.inc"
#include "formatwindow.inc"
#include "formattools.h"
#include "framecache.inc"
#include "guicast.h"
#include "mutex.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "resample.inc"
#include "vframe.inc"
#include "packagingengine.h"

// ======================================= include file types here



// generic file opened by user
class File
{
public:
	File();
	~File();

// Get attributes for various file formats.
// The dither parameter is carried over from recording, where dither is done at the device.
	int get_options(FormatTools *format, 
		int audio_options,
		int video_options);

	int raise_window();
// Close parameter window
	void close_window();

// ===================================== start here
	int set_processors(int cpus);   // Set the number of cpus for certain codecs.
// Set the number of bytes to preload during reads for Quicktime.
	int set_preload(int64_t size);
// Set the subtitle for libmpeg3.  -1 disables subtitles.
	void set_subtitle(int value);
// Set whether to interpolate raw images
	void set_interpolate_raw(int value);
// Set whether to white balance raw images.  Always 0 if no interpolation.
	void set_white_balance_raw(int value);
// When loading, the asset is deleted and a copy created in the EDL.
	void set_asset(Asset *asset);

// Enable or disable frame caching.  Must be tied to file to know when 
// to delete the file object.  Otherwise we'd delete just the cached frames
// while the list of open files grew.
	void set_cache_frames(int value);
// Delete oldest frame from cache.  Return 0 if successful.  Return 1 if 
// nothing to delete.
	int purge_cache();

// Format may be preset if the asset format is not 0.
	int open_file(Preferences *preferences, 
		Asset *asset, 
		int rd, 
		int wr,
		int64_t base_samplerate,
		float base_framerate);

// Get index from the file if one exists.  Returns 0 on success.
	int get_index(char *index_path);

// start a thread for writing to avoid blocking during record
	int start_audio_thread(int64_t buffer_size, int ring_buffers);
	int stop_audio_thread();
// The ring buffer must either be 1 or 2.
// The buffer_size for video needs to be > 1 on SMP systems to utilize 
// multiple processors.
// For audio it's the number of samples per buffer.
// compressed - if 1 write_compressed_frame is called
//              if 0 write_frames is called
	int start_video_thread(int64_t buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed);
	int stop_video_thread();

	int start_video_decode_thread();

// Return the thread.
// Used by functions that read only.
	FileThread* get_video_thread();

// write any headers and close file
// ignore_thread is used by SigHandler to break out of the threads.
	int close_file(int ignore_thread = 0);

// get length of file normalized to base samplerate
	int64_t get_audio_length(int64_t base_samplerate = -1);
	int64_t get_video_length(float base_framerate = -1);

// get current position
	int64_t get_audio_position(int64_t base_samplerate = -1);
	int64_t get_video_position(float base_framerate = -1);
	


// write samples for the current channel
// written to disk and file pointer updated after last channel is written
// return 1 if failed
// subsequent writes must be <= than first write's size because of buffers
	int write_samples(double **buffer, int64_t len);

// Only called by filethread to write an array of an array of channels of frames.
	int write_frames(VFrame ***frames, int len);



// For writing buffers in a background thread use these functions to get the buffer.
// Get a pointer to a buffer to write to.
	double** get_audio_buffer();
	VFrame*** get_video_buffer();

// Used by ResourcePixmap to directly access the cache.
	FrameCache* get_frame_cache();

// Schedule a buffer for writing on the thread.
// thread calls write_samples
	int write_audio_buffer(int64_t len);
	int write_video_buffer(int64_t len);




// set channel for buffer accesses
	int set_channel(int channel);
// set position in samples
	int set_audio_position(int64_t position, float base_samplerate);

// Read samples for one channel into a shared memory segment.
// The offset is the offset in floats from the beginning of the buffer and the len
// is the length in floats from the offset.
// advances file pointer
// return 1 if failed
	int read_samples(double *buffer, int64_t len, int64_t base_samplerate, float *buffer_float = 0);


// set layer for video read
// is_thread is used by FileThread::run to prevent recursive lockup.
	int set_layer(int layer, int is_thread = 0);
// set position in frames
// is_thread is used by FileThread::run to prevent recursive lockup.
	int set_video_position(int64_t position, float base_framerate = -1, int is_thread = 0);

// Read frame of video into the argument
// is_thread is used by FileThread::run to prevent recursive lockup.
	int read_frame(VFrame *frame, int is_thread = 0);


// The following involve no extra copies.
// Direct copy routines for direct copy playback
	int can_copy_from(Edit *edit, int64_t position, int output_w, int output_h); // This file can copy frames directly from the asset
	int get_render_strategy(ArrayList<int>* render_strategies);
	int64_t compressed_frame_size();
	int read_compressed_frame(VFrame *buffer);
	int write_compressed_frame(VFrame *buffer);

// These are separated into two routines so a file doesn't have to be
// allocated.
// Get best colormodel to translate for hardware accelerated playback.
// Called by VRender.
	int get_best_colormodel(int driver);
// Get best colormodel for hardware accelerated recording.
// Called by VideoDevice.
	static int get_best_colormodel(Asset *asset, int driver);
// Get nearest colormodel that can be decoded without a temporary frame.
// Used by read_frame.
	int colormodel_supported(int colormodel);

// Used by CICache to calculate the total size of the cache.
// Based on temporary frames and a call to the file subclass.
// The return value is limited 1MB each in case of audio file.
// The minimum setting for cache_size should be bigger than 1MB.
	int64_t get_memory_usage();

	static int supports_video(ArrayList<PluginServer*> *plugindb, char *format);   // returns 1 if the format supports video or audio
	static int supports_audio(ArrayList<PluginServer*> *plugindb, char *format);
// Get the extension for the filename
	static char* get_tag(int format);
	static int supports_video(int format);   // returns 1 if the format supports video or audio
	static int supports_audio(int format);
	static int strtoformat(char *format);
	static char* formattostr(int format);
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

// Resampling engine
	Resample *resample;
	Resample_float *resample_float;

// Lock writes while recording video and audio.
// A binary lock won't do.  We need a FIFO lock.
	Condition *write_lock;
	int cpus;
	int64_t playback_preload;
	int playback_subtitle;
	int interpolate_raw;
	int white_balance_raw;

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
	Preferences *preferences;

	static PackagingEngine *new_packaging_engine(Asset *asset);

private:
	void reset_parameters();

	int getting_options;
	BC_WindowBase *format_window;
	Mutex *format_completion;
	FrameCache *frame_cache;
// Copy read frames to the cache
	int use_cache;
};

#endif
