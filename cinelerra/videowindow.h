#ifndef VIDEOWINDOW_H
#define VIDEOWINDOW_H


#include "defaults.inc"
#include "mwindow.inc"
#include "thread.h"
#include "vframe.inc"
#include "videowindowgui.inc"


class VideoWindow : public Thread
{
public:
	VideoWindow(MWindow *mwindow);
	~VideoWindow();
	
	int create_objects();
	int init_window();
	int load_defaults(Defaults *defaults);
	int update_defaults(Defaults *defaults);
	int get_aspect_ratio(float &aspect_w, float &aspect_h);
	int fix_size(int &w, int &h, int width_given, float aspect_ratio);
	int get_full_sizes(int &w, int &h);
	void run();

	int show_window();
	int hide_window();
	int resize_window();
	int original_size(); // Put the window at its original size
	int reset();
	int init_video();
	int stop_video();
	int update(BC_Bitmap *frame);
	int get_w();
	int get_h();
	int start_cropping();
	int stop_cropping();
	BC_Bitmap* get_bitmap();  // get a bitmap for playback

// allocated according to playback buffers
	float **peak_history;

	int video_visible;
	int video_cropping;    // Currently performing a cropping operation
//	float zoom_factor;
	int video_window_w;    // Horizontal size of the window independant of frame size
	VFrame **vbuffer;      // output frame buffer
	VideoWindowGUI *gui;
	MWindow *mwindow;
};





#endif
