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
	int can_copy_from(Edit *edit, long position);
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset, int driver);
	int read_frame_header(char *path);
	int read_frame(VFrame *output, VFrame *input);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

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
	static void fix_codec(char *codec);
	BC_WindowBase *parent_window;
	Asset *asset;
};


class TIFFConfigAlpha : public BC_CheckBox
{
public:
	TIFFConfigAlpha(TIFFConfigVideo *gui, int x, int y);
	
	int handle_event();
	
	TIFFConfigVideo *gui;
};






#endif
