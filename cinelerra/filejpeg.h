#ifndef FILEJPEG_H
#define FILEJPEG_H

#include "file.inc"
#include "filelist.h"
#include "vframe.inc"

class FileJPEG : public FileList
{
public:
	FileJPEG(Asset *asset, File *file);
	~FileJPEG();

// basic commands for every file interpreter
	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);

	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	int can_copy_from(Edit *edit, long position);
	int read_frame_header(char *path);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

	void *decompressor;
};


class JPEGUnit : public FrameWriterUnit
{
public:
	JPEGUnit(FileJPEG *file, FrameWriter *writer);
	~JPEGUnit();

	FileJPEG *file;
	void *compressor;
};

class JPEGConfigVideo : public BC_Window
{
public:
	JPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~JPEGConfigVideo();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
};

#endif
