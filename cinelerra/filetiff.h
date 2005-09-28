#ifndef FILETIFF_H
#define FILETIFF_H

#include <stdlib.h>

#include "file.inc"
#include "filelist.h"
#include "mutex.inc"
#include <tiffio.h>
#include "vframe.inc"

// This header file is representative of any single frame file format.

class FileTIFF : public FileList
{
public:
	FileTIFF(Asset *asset, File *file);
	~FileTIFF();

	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	static int check_sig(Asset *asset);
	static char* compression_to_str(int value);
	static char* cmodel_to_str(int value);
	int can_copy_from(Edit *edit, int64_t position);
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset, int driver);
	int read_frame_header(char *path);
	int read_frame(VFrame *output, VFrame *input);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

	enum
	{
		NONE,
// values stored in Asset::tiff_cmodel
// Data types
		RGB_888,
		RGB_161616,
		RGBA_8888,
		RGBA_16161616,
		RGB_FLOAT,
		RGBA_FLOAT,
		GREYSCALE,
// values stored in Asset::tiff_compression
// Compression types
		LZW,
		PACK_BITS,
		DEFLATE,
		JPEG
	};

	Mutex *unit_lock;
};


// For each write frame call, since multiple write_frames are running concurrently.
class FileTIFFUnit : public FrameWriterUnit
{
public:
	FileTIFFUnit(FileTIFF *file, FrameWriter *writer);
	~FileTIFFUnit();

	int offset;
	VFrame *data;
	VFrame *temp;
	FileTIFF *file;
};




class TIFFConfigVideo : public  BC_Window
{
public:
	TIFFConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~TIFFConfigVideo();


	int create_objects();
	int close_event();
	static char* alpha_to_codec(int use_alpha);
	static int codec_to_alpha(char *codec);
	BC_WindowBase *parent_window;
	Asset *asset;
};


class TIFFColorspace : public BC_PopupMenu
{
public:
	TIFFColorspace(TIFFConfigVideo *gui, int x, int y, int w);
	void create_objects();
	int handle_event();
	TIFFConfigVideo *gui;
};

class TIFFColorspaceItem : public BC_MenuItem
{
public:
	TIFFColorspaceItem(TIFFConfigVideo *gui, int value);
	int handle_event();
	TIFFConfigVideo *gui;
	int value;
};




class TIFFCompression : public BC_PopupMenu
{
public:
	TIFFCompression(TIFFConfigVideo *gui, int x, int y, int w);
	void create_objects();
	int handle_event();
	TIFFConfigVideo *gui;
};

class TIFFCompressionItem : public BC_MenuItem
{
public:
	TIFFCompressionItem(TIFFConfigVideo *gui, int value);
	int handle_event();
	TIFFConfigVideo *gui;
	int value;
};





#endif
