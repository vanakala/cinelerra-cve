#ifndef SVGWIN_H
#define SVGWIN_H

#include "guicast.h"
#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "svg.h"
#include "thread.h"

class SvgThread;
class SvgWin;

PLUGIN_THREAD_HEADER(SvgMain, SvgThread, SvgWin)

class SvgCoord;
class NewSvgButton;
class NewSvgWindow;
class EditSvgButton;

class SvgWin : public BC_Window
{
public:
	SvgWin(SvgMain *client, int x, int y);
	~SvgWin();

	int create_objects();
	int close_event();

	SvgCoord *in_x, *in_y, *in_w, *in_h, *out_x, *out_y, *out_w, *out_h;
	SvgMain *client;
	BC_Title *svg_file_title;
	NewSvgButton *new_svg_button;
	NewSvgWindow *new_svg_thread;
	EditSvgButton *edit_svg_button;
};

class SvgCoord : public BC_TumbleTextBox
{
public:
	SvgCoord(SvgWin *win, 
		SvgMain *client, 
		int x, 
		int y, 
		float *value);
	~SvgCoord();
	int handle_event();

	SvgMain *client;
	SvgWin *win;
	float *value;

};

class NewSvgButton : public BC_GenericButton, public Thread
{
public:
	NewSvgButton(SvgMain *client, SvgWin *window, int x, int y);
	int handle_event();
	void run();
	
	int quit_now;
	SvgMain *client;
	SvgWin *window;
};

class EditSvgButton : public BC_GenericButton, public Thread
{
public:
	EditSvgButton(SvgMain *client, SvgWin *window, int x, int y);
	int handle_event();
	void run();
	
	int quit_now;
	SvgMain *client;
	SvgWin *window;
	Mutex editing_lock;
	int editing;
};

class NewSvgWindow : public BC_FileBox
{
public:
	NewSvgWindow(SvgMain *client, SvgWin *window, char *init_directory);
	~NewSvgWindow();
	SvgMain *client;
	SvgWin *window;
};

class SvgSodipodiThread : public Thread
{
public:
	SvgSodipodiThread(SvgMain *client, SvgWin *window);
	~SvgSodipodiThread();
	void run();
	SvgMain *client;
	SvgWin *window;
};


#endif
