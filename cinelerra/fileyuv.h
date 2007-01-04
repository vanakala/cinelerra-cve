#ifndef FILEYUV_H
#define FILEYUV_H

#include "yuvstream.h"
#include "file.inc"
#include "filelist.h"
#include "vframe.inc"
#include "formattools.h"
#include "ffmpeg.h"

class PipeCheckBox;
class PipePreset;

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

	// below here are local routines not required by interface
	void ensure_temp(int width, int height);

 private:
	VFrame *temp;
	YUVStream *stream;
	Asset *incoming_asset;
	FFMPEG *ffmpeg;
	int pipe_latency;
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
	BC_Hash *defaults;
	BC_TextBox *path_textbox;
	BC_RecentList *path_recent;
	PipeCheckBox  *pipe_checkbox;
	BC_TextBox    *pipe_textbox;
	BC_RecentList *pipe_recent;
	PipePreset *mpeg2enc;
	PipePreset *ffmpeg;
};


class PipeCheckBox : public BC_CheckBox 
{
 public:
	PipeCheckBox(int x, int y, int value);
	int handle_event();
	BC_TextBox *textbox;
};



class PipePreset : public BC_PopupMenu
{
 public:
	PipePreset(int x, int y, char *title, BC_TextBox *textbox, BC_CheckBox *checkbox);
	int handle_event();
	
 private: 
	BC_TextBox *pipe_textbox;
	BC_CheckBox *pipe_checkbox;
	char *title;
};


#endif
