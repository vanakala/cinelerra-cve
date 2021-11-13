// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILEBASE_H
#define FILEBASE_H

#include "aframe.inc"
#include "asset.inc"
#include "bcwindow.h"
#include "colormodels.h"
#include "datatype.h"
#include "edit.inc"
#include "filetoc.inc"
#include "file.inc"
#include "filelist.inc"
#include "overlayframe.inc"
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

	virtual int open_file(int open_method, int streamix,
		const char *filepath) { return 1; };
	virtual void close_file() {};

	void set_dither();

// Subclass should call this to add the base class allocation.
// Only used in read mode.
	virtual size_t get_memory_usage() { return 0; };

	virtual int write_aframes(AFrame **buffer) { return 1; };
	virtual int write_frames(VFrame ***frames, int len) { return 0; };
	virtual int read_aframe(AFrame *aframe)  { return 1; };
// Can convert samplerate and channels of audio
	virtual int converts_samples() { return 0; };

// Can convert colormodel and size
	virtual int converts_frame() { return 0; };
	virtual int read_frame(VFrame *frame) { return 1; };

// Callbacks for FileTOC
	// returns number of streams and array of stream descriptions
	virtual int get_streamcount() { return 0; };

	// returns data for the track
	virtual stream_params *get_track_data(int track) { return 0; };

	// tocfile is generated - filebase can do cleanup
	virtual void toc_is_made(int canceled) {};

// Return either the argument or another colormodel which read_frame should use.
	virtual int colormodel_supported(int colormodel) { return colormodel; };

protected:
	static int match4(const char *in, const char *out);   // match 4 bytes for a quicktime type
	int dither;
	Asset *asset;
	File *file;
};

class FBConfig: public BC_Window
{
public:
	FBConfig(BC_WindowBase *parent_window, int type, int absx, int absy);
};

#endif
