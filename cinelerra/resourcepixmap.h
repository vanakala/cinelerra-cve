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
		long edit_x,
		long edit_w, 
		long pixmap_x, 
		long pixmap_w,
		long pixmap_h,
		int force,
		int indexes_only);
	void draw_audio_resource(Edit *edit, int x, int w);
	void draw_video_resource(Edit *edit, 
		long edit_x, 
		long edit_w, 
		long pixmap_x,
		long pixmap_w,
		int refresh_x, 
		int refresh_w);
	void draw_audio_source(Edit *edit, int x, int w);
	void draw_title(Edit *edit, long edit_x, long edit_w, long pixmap_x, long pixmap_w);
	void reset();

	void dump();

	MWindow *mwindow;
	TrackCanvas *canvas;
// Visible in entire track canvas
	int visible;
// Section drawn
	long edit_id;
	long edit_x, pixmap_x, pixmap_w, pixmap_h;
	long zoom_sample, zoom_track, zoom_y;
	long startsource;
	double source_framerate, project_framerate;
	long source_samplerate, project_samplerate;
	int data_type;
};

#endif
