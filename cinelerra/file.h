
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

#include "aframe.inc"
#include "asset.inc"
#include "condition.inc"
#include "edit.inc"
#include "filebase.inc"
#include "file.inc"
#include "filethread.inc"
#include "filexml.inc"
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
	void get_options(FormatTools *format, int options);

	void raise_window();
// Close parameter window
	void close_window();

// ===================================== start here
	void set_processors(int cpus);   // Set the number of cpus for certain codecs.
// Set the number of bytes to preload during reads for Quicktime.
	void set_preload(int64_t size);
// Set the subtitle for libmpeg3.  -1 disables subtitles.
	void set_subtitle(int value);

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
		int base_samplerate,
		float base_framerate);

// Get index from the file if one exists.  Returns 0 on success.
	int get_index(const char *index_path);

// start a thread for writing to avoid blocking during record
	void start_audio_thread(int buffer_size, int ring_buffers);
	void stop_audio_thread();
// The ring buffer must either be 1 or 2.
// The buffer_size for video needs to be > 1 on SMP systems to utilize 
// multiple processors.
// For audio it's the number of samples per buffer.
// compressed - if 1 write_compressed_frame is called
//              if 0 write_frames is called
	void start_video_thread(int buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed);
	void stop_video_thread();

	void start_video_decode_thread();

// Return the thread.
// Used by functions that read only.
	FileThread* get_video_thread();

// write any headers and close file
// ignore_thread is used by SigHandler to break out of the threads.
	void close_file(int ignore_thread = 0);

// get length of file normalized to base samplerate
	samplenum get_audio_length(int base_samplerate = -1);
	framenum get_video_length(float base_framerate = -1);
// get length of file in seconds
	ptstime get_video_ptslen(void);

// write audio frames
// written to disk and file pointer updated after
// return 1 if failed
	int write_aframes(AFrame **buffer);

// Only called by filethread to write an array of an array of channels of frames.
	int write_frames(VFrame ***frames, int len);



// For writing buffers in a background thread use these functions to get the buffer.
// Get a pointer to a buffer to write to.
	AFrame** get_audio_buffer();
	VFrame*** get_video_buffer();

// Used by ResourcePixmap to directly access the cache.
	FrameCache* get_frame_cache();

// Schedule a buffer for writing on the thread.
// thread calls write_samples
	int write_audio_buffer(int len);
	int write_video_buffer(int len);

// Read audio into aframe
// aframe->source_duration secs starting from aframe->source_pts
	int get_samples(AFrame *aframe);


// pts API - frame must have source_pts, and layer set
	int get_frame(VFrame *frame, int is_thread = 0);

// The following involve no extra copies.
// Direct copy routines for direct copy playback
	int can_copy_from(Edit *edit, int output_w, int output_h) { return 0; };
	int get_render_strategy(ArrayList<int>* render_strategies);

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
	size_t get_memory_usage();

	static int supports_video(int format);   // returns 1 if the format supports video or audio
	static int supports_audio(int format);
	static int supports(int format);

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

	Preferences *preferences;

	static PackagingEngine *new_packaging_engine(Asset *asset);

private:
	void reset_parameters();
	int get_this_frame(framenum pos, VFrame *frame, int is_thread = 0);
	int getting_options;
	BC_WindowBase *format_window;
	Mutex *format_completion;
	FrameCache *frame_cache;
	int writing;
// Copy read frames to the cache
	int use_cache;
};

#endif
