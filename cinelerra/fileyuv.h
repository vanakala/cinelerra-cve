#ifndef FILEYUV_H
#define FILEYUV_H

#include "yuvstream.h"
#include "file.inc"
#include "filelist.h"
#include "vframe.inc"
#include "recentlist.h"
#include "formattools.h"


class FileYUV : public FileBase
{
 public:
	FileYUV(Asset *asset, File *file);
	~FileYUV();

	static void get_parameters(BC_WindowBase *parent_window, 
				   Asset *asset, 
				   BC_WindowBase* &format_window,
				   int video_options,
				   FormatTools *format);

        int open_file(int rd, int wr);
	static int check_sig(Asset *asset);
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int read_frame(VFrame *frame);
	int write_frames(VFrame ***frame, int len);
	int can_copy_from(Edit *edit, int64_t position);
	int close_file();
	int set_video_position(int64_t x);

	

 private:
	VFrame *temp;
	YUVStream stream;
};


class YUVConfigVideo;

class YUVPathText : public BC_TextBox
{
 public:
        YUVPathText(int x, int y, YUVConfigVideo *config);

 private:
	YUVConfigVideo *config;
};

class YUVMpeg2Enc : public BC_PopupMenu
{
 public:
	YUVMpeg2Enc(int x, int y, YUVConfigVideo *config);
	int handle_event();
	int create_objects();
	
 private: 
	YUVConfigVideo *config;
};
	


class YUVConfigVideo : public BC_Window
{
 public:
	YUVConfigVideo(BC_WindowBase *parent_window, Asset *asset,
		       FormatTools *format);
	~YUVConfigVideo();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
	FormatTools *format;
	YUVPathText *textbox;
	YUVMpeg2Enc *mpeg2enc;
	RecentList *recent;
};

#endif
