
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bchash.h"
#include "levelwindow.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "vframe.h"
#include "videowindow.h"
#include "videowindowgui.h"


VideoWindow::VideoWindow(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	vbuffer = 0;
	gui = 0;
	video_window_w = 320;
	video_visible = 0;
	video_cropping = 0;
}

VideoWindow::~VideoWindow()
{
	if(gui)
	{
		gui->set_done(0);
		Thread::join();
		delete gui;
	}
}

int VideoWindow::load_defaults(BC_Hash *defaults)
{
	video_visible = defaults->get("VIDEOVISIBLE", 1);
	video_window_w = defaults->get("PLAYVIDEOW", video_window_w);
}

int VideoWindow::update_defaults(BC_Hash *defaults)
{
	defaults->update("VIDEOVISIBLE", video_visible);
	defaults->update("PLAYVIDEOW", video_window_w);
}

int VideoWindow::create_objects()
{
	set_synchronous(1);
	if(mwindow->gui)
	{
		init_window();
	}
}

int VideoWindow::init_window()
{
// 	int w = mwindow->session->output_w;
// 	int h = mwindow->session->output_h;
// 
// 	fix_size(w, h, video_window_w, mwindow->get_aspect_ratio());
// 	gui = new VideoWindowGUI(this, w, h);
// 	gui->create_objects();
}

int VideoWindow::show_window()
{
	if(gui)
	{
		video_visible = 1;
		gui->show_window();
		mwindow->gui->mainmenu->set_show_video(1);
	}
}

int VideoWindow::hide_window()
{
	if(gui)
	{
		video_visible = 0;
		gui->hide_window();
		mwindow->gui->mainmenu->set_show_video(0);
	}
}

int VideoWindow::resize_window()
{
	int proper_w = mwindow->session->output_w;
	int proper_h = mwindow->session->output_h;

//	fix_size(proper_w, proper_h, video_window_w, mwindow->get_aspect_ratio());

	gui->update_title();
	if(gui && 
		(gui->get_w() != proper_w ||
			gui->get_h() != proper_h))
	{
		gui->resize_window(proper_w, proper_h);
		gui->canvas->reposition_window(0, 0, proper_w, proper_h);
		if(video_cropping) gui->canvas->draw_crop_box();
		gui->flash();
	}
}

int VideoWindow::fix_size(int &w, int &h, int width_given, float aspect_ratio)
{
	w = width_given;
	h = (int)((float)width_given / aspect_ratio);
}

int VideoWindow::original_size()
{
	int w, h;
	get_full_sizes(w, h);
	video_window_w = w;
	resize_window();
}

int VideoWindow::get_full_sizes(int &w, int &h)
{
// 	if(mwindow->get_aspect_ratio() > (float)mwindow->session->output_w / mwindow->session->output_h)
// 	{
// 		w = (int)((float)mwindow->session->output_h * mwindow->get_aspect_ratio() + 0.5);
// 		h = mwindow->session->output_h;
// 	}
// 	else
// 	{
// 		w = mwindow->session->output_w;
// 		h = (int)((float)mwindow->session->output_w / mwindow->get_aspect_ratio() + 0.5);
// 	}
}


void VideoWindow::run()
{
	if(gui) gui->run_window();
}

int VideoWindow::init_video()
{
	if(gui)
	{
		gui->canvas->start_video();
	}
}

int VideoWindow::stop_video()
{
	if(gui)
	{
		gui->canvas->stop_video();
	}
}

int VideoWindow::update(BC_Bitmap *frame)
{
	if(gui)
	{
		gui->lock_window();
//		gui->canvas->draw_bitmap(frame, 1);
		gui->unlock_window();
	}
}

int VideoWindow::get_w()
{
	if(gui) return gui->get_w();
	else return 0;
}

int VideoWindow::get_h()
{
	if(gui) return gui->get_h();
	else return 0;
}

BC_Bitmap* VideoWindow::get_bitmap()
{
	return gui->canvas->new_bitmap(gui->get_w(), gui->get_h());
}

int VideoWindow::reset()
{
}

int VideoWindow::start_cropping()
{
	video_cropping = 1;
	gui->x1 = 0;
	gui->y1 = 0;
	gui->x2 = gui->get_w();
	gui->y2 = gui->get_h();
	gui->canvas->draw_crop_box();
	gui->canvas->flash();
}

int VideoWindow::get_aspect_ratio(float &aspect_w, float &aspect_h)
{
	int new_w, new_h;
	float zoom_factor = (float)video_window_w / mwindow->session->output_w;

// For new aspect ratio
	new_w = (int)((float)(gui->x2 - gui->x1) / zoom_factor);
	new_h = (int)((float)(gui->y2 - gui->y1) / zoom_factor);

	mwindow->create_aspect_ratio(aspect_w, aspect_h, new_w, new_h);
}

int VideoWindow::stop_cropping()
{
// 	float x_zoom, y_zoom;
// 	int new_w, new_h;
// 	float zoom_factor;
// 
// 	if(mwindow->get_aspect_ratio() < (float)mwindow->session->output_w / mwindow->session->output_h)
// 	{
// 		x_zoom = 1;
// 		y_zoom = (float)mwindow->session->output_w / mwindow->get_aspect_ratio() / mwindow->session->output_h;
// 		zoom_factor = (float)video_window_w / mwindow->session->output_w;
// 	}
// 	else
// 	{
// 		x_zoom = (float)mwindow->session->output_h * mwindow->get_aspect_ratio() / mwindow->session->output_w;
// 		y_zoom = 1;
// 		zoom_factor = (float)video_window_w / (mwindow->session->output_h * mwindow->get_aspect_ratio());
// 	}
// 
// 
// 	gui->canvas->draw_crop_box();
// 	gui->canvas->flash();
// 	video_window_w = gui->x2 - gui->x1;
// 
// 	gui->x1 = (int)(gui->x1 / zoom_factor / x_zoom);
// 	gui->y1 = (int)(gui->y1 / zoom_factor / y_zoom);
// 	gui->x2 = (int)(gui->x2 / zoom_factor / x_zoom);
// 	gui->y2 = (int)(gui->y2 / zoom_factor / y_zoom);
// 
// 	video_cropping = 0;
}
