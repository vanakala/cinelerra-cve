#ifndef RESOURCEPIXMAP_H
#define RESOURCEPIXMAP_H

#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"

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
		int force,
		int indexes_only);
	void draw_audio_resource(Edit *edit, int x, int w);
	void draw_video_resource(Edit *edit, 
		int64_t edit_x, 
		int64_t edit_w, 
		int64_t pixmap_x,
		int64_t pixmap_w,
		int refresh_x, 
		int refresh_w);
	void draw_audio_source(Edit *edit, int x, int w);
	void draw_title(Edit *edit, int64_t edit_x, int64_t edit_w, int64_t pixmap_x, int64_t pixmap_w);
	void reset();

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
};

#endif
