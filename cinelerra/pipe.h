#ifndef PIPE_H
#define PIPE_H

#include <fcntl.h>
#include "guicast.h"
#include "asset.h"

extern "C" {
	extern int sigpipe_received;
}

class Interlaceyuv4mpegmodeItem : public BC_ListBoxItem
{
public:
	Interlaceyuv4mpegmodeItem(char *text, int value);
	int value;
};


class Interlaceyuv4mpegmodePulldown : public BC_ListBox
{
public:
	Interlaceyuv4mpegmodePulldown(
				      BC_TextBox *output_text, 
				      int *output_value,
				      ArrayList<BC_ListBoxItem*> *data,
				      int x,
				      int y);
	int handle_event();
	char* interlacemode_to_text();
	BC_TextBox *output_text;
	int *output_value;
private:
  	char string[BCTEXTLEN];
};



class Pipe {
 public:
	Pipe::Pipe(char *command, char *sub_str = 0, char sub_char = '%');
 	Pipe::~Pipe() ;		
	int Pipe::open_read() ;
	int Pipe::open_write() ;
	void Pipe::close() ;

	int fd;
 private:
	int Pipe::substitute() ;
 	int Pipe::open(char *mode) ;
	
	char sub_char;
	char *sub_str;
	char *command;
	char complete[BCTEXTLEN];
	FILE *file;
};


class PipeCheckBox : public BC_CheckBox 
{
 public:
	PipeCheckBox(int x, int y, int value, BC_TextBox *textbox);
	int handle_event();

 private: 
	BC_TextBox *textbox;
};
	

class PipeConfig {
 public:
	PipeConfig(BC_WindowBase *window, Defaults *defaults, Asset *asset);
	// NOTE: Default destructor should destroy all subwindows

	int create_objects(int x, int y, int textbox_width, int format);

	// NOTE: these are public so caller can check final values
	PipeCheckBox *checkbox;
	BC_TextBox *textbox;
	BC_RecentList *recent;
	
 private:
	int init_menus();
	BC_WindowBase *window;
	Defaults *defaults;
	Asset *asset;
	ArrayList<Interlaceyuv4mpegmodeItem*>  interlace_asset_yuv4mpeg_modes;
};

class PipeStatus : public BC_Title
{
 public:
	PipeStatus(int x, int y, char *default_string);
	int set_status(Asset *asset);
 private:
	char *default_string;
	char status[BCTEXTLEN];
};


class PipePreset : public BC_PopupMenu
{
 public:
	PipePreset(int x, int y, char *title, PipeConfig *config);
	int handle_event();
	
 private: 
	PipeConfig *config;
	char *title;
};
	
	
#endif /* PIPE_H */
