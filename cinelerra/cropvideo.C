#include "cropvideo.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mainsession.h"
#include "tracks.h"
#include "videowindow.h"
#include "videowindowgui.h"

CropVideo::CropVideo(MWindow *mwindow)
 : BC_MenuItem("Crop Video..."), Thread()
{
	this->mwindow = mwindow;
}

CropVideo::~CropVideo()
{
}

int CropVideo::handle_event()
{
	start();
}

void CropVideo::run()
{
	float aspect_w, aspect_h;
	int result = 0;
	{
		mwindow->video_window->start_cropping();
		CropVideoWindow window(mwindow, this);
		window.create_objects();
		result = window.run_window();
		mwindow->video_window->get_aspect_ratio(aspect_w, aspect_h);
		mwindow->video_window->stop_cropping();
	}

	if(!result)
	{
		int offsets[4], dummy_dimension[4];
		dummy_dimension[0] = dummy_dimension[1] = dummy_dimension[2] = dummy_dimension[3] = 0;
		offsets[0] = -(mwindow->video_window->gui->x1 + mwindow->video_window->gui->x2 - mwindow->session->output_w) / 2;
		offsets[1] = -(mwindow->video_window->gui->y1 + mwindow->video_window->gui->y2 - mwindow->session->output_h) / 2;
		offsets[2] = offsets[3] = 0;
//		mwindow->undo->update_undo_edits("Crop", 0);
		
		mwindow->tracks->scale_video(dummy_dimension, offsets, 0);
		mwindow->session->track_w = mwindow->video_window->gui->x2 - mwindow->video_window->gui->x1;
		mwindow->session->track_h = mwindow->video_window->gui->y2 - mwindow->video_window->gui->y1;
		mwindow->session->output_w = mwindow->video_window->gui->x2 - mwindow->video_window->gui->x1;
		mwindow->session->output_h = mwindow->video_window->gui->y2 - mwindow->video_window->gui->y1;
		mwindow->session->aspect_w = aspect_w;
		mwindow->session->aspect_h = aspect_h;
		mwindow->video_window->resize_window();
		mwindow->draw();
//		mwindow->undo->update_undo_edits();
		mwindow->session->changes_made = 1;
	}
	else
	{
	}
}

int CropVideo::load_defaults()
{
}

int CropVideo::save_defaults()
{
}

CropVideoWindow::CropVideoWindow(MWindow *mwindow, CropVideo *thread)
 : BC_Window(PROGRAM_NAME ": Crop", 380, 75, 0, 0)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

CropVideoWindow::~CropVideoWindow()
{
}

int CropVideoWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, "Select a region to crop in the video output window"));
	y += 30;
	add_subwindow(new BC_OKButton(x, y));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(x, y));
	return 0;
}



