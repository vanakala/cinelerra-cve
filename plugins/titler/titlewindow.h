#ifndef TITLEWINDOW_H
#define TITLEWINDOW_H

#include "guicast.h"

class TitleThread;
class TitleWindow;
class TitleInterlace;

#include "../colors/colorpicker.h"
#include "filexml.h"
#include "mutex.h"
#include "title.h"

class TitleThread : public Thread
{
public:
	TitleThread(TitleMain *client);
	~TitleThread();

	void run();

// prevent loading data until the GUI is started
 	Mutex gui_started, completion;
	TitleMain *client;
	TitleWindow *window;
};



class TitleFontTumble;
class TitleItalic;
class TitleBold;
class TitleSize;
class TitleColorButton;
class TitleDropShadow;
class TitleMotion;
class TitleLoop;
class TitleFade;
class TitleFont;
class TitleText;
class TitleX;
class TitleY;
class TitleLeft;
class TitleCenter;
class TitleRight;
class TitleTop;
class TitleMid;
class TitleBottom;
class TitleColorThread;
class TitleSpeed;
class TitleTimecode;

class TitleWindow : public BC_Window
{
public:
	TitleWindow(TitleMain *client, int x, int y);
	~TitleWindow();
	
	int create_objects();
	int close_event();
	int resize_event(int w, int h);
	void update_color();
	void update_justification();
	void update();
	void previous_font();
	void next_font();

	TitleMain *client;

	BC_Title *font_title;
	TitleFont *font;
	TitleFontTumble *font_tumbler;
	BC_Title *x_title;
	TitleX *title_x;
	BC_Title *y_title;
	TitleY *title_y;
	BC_Title *dropshadow_title;
	TitleDropShadow *dropshadow;
	BC_Title *style_title;
	TitleItalic *italic;
	TitleBold *bold;
	BC_Title *size_title;
	TitleSize *size;
	TitleColorButton *color_button;
	TitleColorThread *color_thread;
	BC_Title *motion_title;
	TitleMotion *motion;
	TitleLoop *loop;
	BC_Title *fadein_title;
	TitleFade *fade_in;
	BC_Title *fadeout_title;
	TitleFade *fade_out;
	BC_Title *text_title;
	TitleText *text;
	BC_Title *justify_title;
	TitleLeft *left;
	TitleCenter *center;
	TitleRight *right;
	TitleTop *top;
	TitleMid *mid;
	TitleBottom *bottom;
	BC_Title *speed_title;
	TitleSpeed *speed;
	TitleTimecode *timecode;

// Color preview
	int color_x, color_y;
	ArrayList<BC_ListBoxItem*> sizes;
	ArrayList<BC_ListBoxItem*> paths;
	ArrayList<BC_ListBoxItem*> fonts;
};


class TitleFontTumble : public BC_Tumbler
{
public:
	TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y);
	
	int handle_up_event();
	int handle_down_event();
	
	TitleMain *client;
	TitleWindow *window;
};

class TitleItalic : public BC_CheckBox
{
public:
	TitleItalic(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBold : public BC_CheckBox
{
public:
	TitleBold(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleSize : public BC_PopupTextBox
{
public:
	TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text);
	~TitleSize();
	int handle_event();
	void update(int size);
	TitleMain *client;
	TitleWindow *window;
};
class TitleColorButton : public BC_GenericButton
{
public:
	TitleColorButton(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleMotion : public BC_PopupTextBox
{
public:
	TitleMotion(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleLoop : public BC_CheckBox
{
public:
	TitleLoop(TitleMain *client, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleTimecode : public BC_CheckBox
{
public:
	TitleTimecode(TitleMain *client, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleFade : public BC_TextBox
{
public:
	TitleFade(TitleMain *client, TitleWindow *window, double *value, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
	double *value;
};
class TitleFont : public BC_PopupTextBox
{
public:
	TitleFont(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleText : public BC_ScrollTextBox
{
public:
	TitleText(TitleMain *client, 
		TitleWindow *window, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleX : public BC_TumbleTextBox
{
public:
	TitleX(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleY : public BC_TumbleTextBox
{
public:
	TitleY(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleDropShadow : public BC_TumbleTextBox
{
public:
	TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleSpeed : public BC_TumbleTextBox
{
public:
	TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
};

class TitleLeft : public BC_Radial
{
public:
	TitleLeft(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleCenter : public BC_Radial
{
public:
	TitleCenter(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleRight : public BC_Radial
{
public:
	TitleRight(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleTop : public BC_Radial
{
public:
	TitleTop(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleMid : public BC_Radial
{
public:
	TitleMid(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBottom : public BC_Radial
{
public:
	TitleBottom(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleColorThread : public ColorThread
{
public:
	TitleColorThread(TitleMain *client, TitleWindow *window);
	int handle_event(int output);
	TitleMain *client;
	TitleWindow *window;
};

#endif
