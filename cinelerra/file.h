// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

#include "aframe.inc"
#include "asset.inc"
#include "condition.inc"
#include "datatype.h"
#include "edit.inc"
#include "filebase.inc"
#include "file.inc"
#include "filexml.inc"
#include "formattools.h"
#include "framecache.inc"
#include "mutex.inc"
#include "pluginserver.inc"
#include "vframe.inc"

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

// Detect image list
	int is_imagelist(int format);

// Probe the file type
	int probe_file(Asset *asset);

// Format may be preset if the asset format is not 0.
	int open_file(Asset *asset, int open_method, int stream,
		const char *filepath = 0);

// write any headers and close file
// ignore_thread is used by SigHandler to break out of the threads.
	void close_file();

// get length of file normalized to base samplerate
	samplenum get_audio_length();

// write audio frames
// written to disk and file pointer updated after
// return 1 if failed
	int write_aframes(AFrame **buffer);

// Only called by filethread to write an array of an array of channels of frames.
	int write_frames(VFrame **frames, int len);

// Read audio into aframe
// aframe->source_duration secs starting from aframe->source_pts
	int get_samples(AFrame *aframe);

// pts API - frame must have source_pts, and layer set
	int get_frame(VFrame *frame);

// The following involve no extra copies.
// Direct copy routines for direct copy playback
	int can_copy_from(Edit *edit, int output_w, int output_h) { return 0; };

// Used by CICache to calculate the total size of the cache.
// Based on temporary frames and a call to the file subclass.
// The return value is limited 1MB each in case of audio file.
// The minimum setting for cache_size should be bigger than 1MB.
	size_t get_memory_usage();

// Returns SUPPORTS_* bits for format
	static int supports(int format);

	Asset *asset;    // Copy of asset since File outlives EDL
	FileBase *file; // virtual class for file type

// Lock writes while recording video and audio.
// A binary lock won't do.  We need a FIFO lock.
	Condition *write_lock;
	int cpus;

private:
	int asset_stream;
	void reset_parameters();
	int get_this_frame(framenum pos, VFrame *frame, int is_thread = 0);

	int getting_options;
	BC_WindowBase *format_window;
	Mutex *format_completion;
	int writing;
	VFrame *last_frame;
};

#endif
