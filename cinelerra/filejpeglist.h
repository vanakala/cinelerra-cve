#ifndef FILEJPEGLIST_H
#define FILEJPEGLIST_H

#include "file.inc"
#include "filebase.h"
#include "filelist.h"
#include "jpegwrapper.h"
#include "vframe.inc"

class FileJPEGList : public FileList
{
public:
	FileJPEGList(Asset *asset, File *file);
	~FileJPEGList();


	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);

	int get_best_colormodel(int driver, int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data);

	char* list_title();
	char* extension();

// Direct copy routines
	int can_copy_from(Asset *asset);
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
