
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

#ifndef FILEBASE_H
#define FILEBASE_H

#include "aframe.inc"
#include "asset.inc"
#include "assets.inc"
#include "colormodels.h"
#include "datatype.h"
#include "edit.inc"
#include "filetoc.inc"
#include "guicast.h"
#include "file.inc"
#include "filelist.inc"
#include "overlayframe.inc"
#include "strategies.inc"
#include "vframe.inc"

#include <sys/types.h>

// inherited by every file interpreter
class FileBase
{
public:
	FileBase(Asset *asset, File *file);
	virtual ~FileBase();

	friend class File;
	friend class FileList;
	friend class FrameWriter;

	int reset_parameters();

	virtual void get_parameters(BC_WindowBase *parent_window, 
			Asset *asset, 
			BC_WindowBase **format_window,
			int audio_options,
			int video_options,
			int lock_compressor) {};

	virtual int get_index(const char *index_path) { return 1; };
	virtual void reset_parameters_derived() {};
	virtual int read_header() {};     // WAV files for getting header
	virtual int open_file(int rd, int wr) {};
	virtual void close_file();
	virtual void close_file_derived() {};
	void set_dither();
// Called only by recordengine
	virtual void seek_end() {};
	virtual void set_video_position(framenum x) {};
	virtual void set_audio_position(samplenum x) {};

// Subclass should call this to add the base class allocation.
// Only used in read mode.
	virtual int64_t get_memory_usage() { return 0; };

	virtual int write_aframes(AFrame **buffer);
	virtual int write_samples(double **buffer, int len) { return 0; };
	virtual int write_frames(VFrame ***frames, int len) { return 0; };
// Doubles are used to allow resampling
	virtual int read_samples(double *buffer, int len) { return 0; };

	virtual int prefer_samples_float() { return 0;};
	virtual int read_samples_float(float *buffer, int len) { return 0; };

	virtual int read_frame(VFrame *frame) { return 1; };

// Callbacks for FileTOC
	// returns number of streams and array of stream descriptions
	virtual int get_streamcount() { return 0; };

	// returns data for the track
	virtual stream_params *get_track_data(int track) { return 0; };

	// tocfile is generated - filebase can do cleanup
	virtual void toc_is_made(int canceled) {};

// Return either the argument or another colormodel which read_frame should
// use.
	virtual int colormodel_supported(int colormodel) { return BC_RGB888; };
// This file can copy compressed frames directly from the asset
	virtual int can_copy_from(Edit *edit) { return 0; };

protected:
	static int match4(const char *in, const char *out);   // match 4 bytes for a quicktime type

	char *audio_buffer_in, *audio_buffer_out;    // for raw audio reads and writes
	float *float_buffer;          // for floating point feathering
	unsigned char *video_buffer_in, *video_buffer_out;
	unsigned char **row_pointers_in, **row_pointers_out;

	int prev_bytes; // determines if new raw buffer is needed and used for getting memory usage
	int prev_len;
	int prev_track;
	int prev_layer;
	Asset *asset;
	int wr, rd;
	int dither;
	int internal_byte_order;
	File *file;
};
#endif
