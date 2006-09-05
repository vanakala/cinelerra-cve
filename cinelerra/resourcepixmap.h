#ifndef RESOURCEPIXMAP_H
#define RESOURCEPIXMAP_H

#include "bctimer.inc"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"


// Can't use garbage collection for GUI elements because they need to
// lock the window for deletion.
class ResourcePixmap : public BC_Pixmap
{
public:
	ResourcePixmap(MWindow *mwindow, 
		TrackCanvas *canvas, 
		Edit *edit, 
		int w, 
		int h);
	~ResourcePixmap();

	void resize(int w, int h);
	void draw_data(Edit *edit,
		int64_t edit_x,
		int64_t edit_w, 
		int64_t pixmap_x, 
		int64_t pixmap_w,
		int64_t pixmap_h,
		int mode,
		int indexes_only);
	void draw_audio_resource(Edit *edit, 
		int x, 
		int w);
	void draw_video_resource(Edit *edit, 
		int64_t edit_x, 
		int64_t edit_w, 
		int64_t pixmap_x,
		int64_t pixmap_w,
		int refresh_x, 
		int refresh_w,
		int mode);
	void draw_audio_source(Edit *edit, int x, int w);
// Called by ResourceThread to update pixmap
	void draw_wave(int x, double high, double low);
	void draw_title(Edit *edit, int64_t edit_x, int64_t edit_w, int64_t pixmap_x, int64_t pixmap_w);
	void reset();
// Change to hourglass if timer expired
	void test_timer();

	void dump();

	MWindow *mwindow;
	TrackCanvas *canvas;
// Visible in entire track canvas
	int visible;
// Section drawn
	int64_t edit_id;
	int64_t edit_x, pixmap_x, pixmap_w, pixmap_h;
	int64_t zoom_sample, zoom_track, zoom_y;
	int64_t startsource;
	double source_framerate, project_framerate;
	int64_t source_samplerate, project_samplerate;
	int data_type;
// Timer to cause an hourglass to appear
	Timer *timer;
};

#endif
