#ifndef BCBAR_H
#define BCBAR_H


#include "bcsubwindow.h"
#include "vframe.inc"

class BC_Bar : public BC_SubWindow
{
public:
	BC_Bar(int x, int y, int w, VFrame *data = 0);
	virtual ~BC_Bar();
	
	int initialize();
	void set_image(VFrame *data);
	void draw();
	int reposition_window(int x, int y, int w);
	int resize_event(int w, int h);

	BC_Pixmap *image;
	VFrame *data;
};


#endif
