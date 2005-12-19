#ifndef PIPE_H
#define PIPE_H

#include <fcntl.h>
#include "guicast.h"
#include "asset.h"

extern "C" {
	extern int sigpipe_received;
}

class Pipe {
 public:
	Pipe(char *command, char *sub_str = 0, char sub_char = '%');
 	~Pipe() ;		
	int open_read() ;
	int open_write() ;
	void close() ;

	int fd;
 private:
	int substitute() ;
 	int open(char *mode) ;
	
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
	BC_WindowBase *window;
	Defaults *defaults;
	Asset *asset;
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
