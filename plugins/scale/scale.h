#ifndef SCALE_H
#define SCALE_H

// the simplest plugin possible

class ScaleMain;
class ScaleWidth;
class ScaleHeight;
class ScaleConstrain;
class ScaleThread;
class ScaleWin;

#include "defaults.h"
#include "guicast.h"
#include "mutex.h"
#include "scalewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class ScaleConfig
{
public:
	ScaleConfig();

	void copy_from(ScaleConfig &src);
	int equivalent(ScaleConfig &src);
	void interpolate(ScaleConfig &prev, 
		ScaleConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	float w, h;
	int constrain;
};


class ScaleWidth : public BC_TumbleTextBox
{
public:
	ScaleWidth(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleWidth();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
};

class ScaleHeight : public BC_TumbleTextBox
{
public:
	ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleHeight();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
};

class ScaleConstrain : public BC_CheckBox
{
public:
	ScaleConstrain(ScaleMain *client, int x, int y);
	~ScaleConstrain();
	int handle_event();

	ScaleMain *client;
};

class ScaleWin : public BC_Window
{
public:
	ScaleWin(ScaleMain *client, int x, int y);
	~ScaleWin();

	int create_objects();
	int close_event();

	ScaleMain *client;
	ScaleWidth *width;
	ScaleHeight *height;
	ScaleConstrain *constrain;
};

PLUGIN_THREAD_HEADER(ScaleMain, ScaleThread, ScaleWin)

class ScaleMain : public PluginVClient
{
public:
	ScaleMain(PluginServer *server);
	~ScaleMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	int load_configuration();

// a thread for the GUI
	ScaleThread *thread;

	OverlayFrame *overlayer;   // To scale images
	VFrame *temp_frame;        // Used if buffers are the same
	Defaults *defaults;
	ScaleConfig config;
};


#endif
