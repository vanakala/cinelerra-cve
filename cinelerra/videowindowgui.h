#ifndef VIDEOWINDOWGUI_H
#define VIDEOWINDOWGUI_H

#include "guicast.h"
#include "videowindow.inc"

class VideoWindowGUI : public BC_Window
{
public:
	VideoWindowGUI(VideoWindow *thread, int w, int h);
	~VideoWindowGUI();

	int create_objects();
	int resize_event(int w, int h);
	int close_event();
	int keypress_event();
	int update_title();
	int start_cropping();
	int stop_cropping();

	int x1, y1, x2, y2, center_x, center_y;
	int x_offset, y_offset;
	VideoWindow *thread;
	VideoWindowCanvas *canvas;
};

class VideoWindowCanvas : public BC_SubWindow
{
public:
	VideoWindowCanvas(VideoWindowGUI *gui, int w, int h);
	~VideoWindowCanvas();

	int button_press();
	int button_release();
	int cursor_motion();
	int draw_crop_box();

	int button_down;
	VideoWindowGUI *gui;
	int corner_selected;
};


#endif
