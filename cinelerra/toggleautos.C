#include "filexml.h"
#include "intauto.h"
#include "intautos.h"

#define MINSTACKHEIGHT 16

IntAutos::IntAutos(Track *track, 
			int color, 
			int default_,
			int stack_number, 
			int stack_total)
 : Autos(track, color, default_, stack_number, stack_total)
{
// 1 is on            -1 is off
	this->max = 1; this->min = -1;
	this->virtual_h = 100;
}

IntAutos::~IntAutos()
{
}

int IntAutos::slope_adjustment(int64_t ax, float slope)
{
	return 0;
}

int IntAutos::get_track_pixels(int zoom_track, int pixel, int &center_pixel, float &yscale)
{
	if(zoom_track < MINSTACKHEIGHT)
	{
		center_pixel = pixel + zoom_track / 2;
		yscale = -(float)zoom_track / (max - min) * .75;
	}
	else
	if(zoom_track / stack_total < MINSTACKHEIGHT)
	{
		center_pixel = pixel + MINSTACKHEIGHT / 2 + (stack_number * MINSTACKHEIGHT % zoom_track) * zoom_track;
		yscale = -(float)MINSTACKHEIGHT / (max - min) * .75;
	}
	else
	{
		center_pixel = pixel + (zoom_track / stack_total) / 2 + (zoom_track / stack_total) * stack_number;
		yscale = -(float)(zoom_track / stack_total) / (max - min) * .75;
	}
}

int IntAutos::draw_joining_line(BC_SubWindow *canvas, int vertical, int center_pixel, int x1, int y1, int x2, int y2)
{
	if(vertical)
	canvas->draw_line(center_pixel - y1, x1, center_pixel - y1, x2);
	else
	canvas->draw_line(x1, center_pixel + y1, x2, center_pixel + y1);
	
	if(y1 != y2)
	{
		if(vertical)
		canvas->draw_line(center_pixel - y1, x2, center_pixel - y2, x2);
		else
		canvas->draw_line(x2, center_pixel + y1, x2, center_pixel + y2);
	}
}


Auto* IntAutos::add_auto(int64_t position, float value)
{
	IntAuto* current = (IntAuto*)autoof(position);
	IntAuto* new_auto;
	
	insert_before(current, new_auto = new IntAuto(this));

	new_auto->position = position;
	new_auto->value = value;
	
	return new_auto;
}


Auto* IntAutos::append_auto()
{
	return append(new IntAuto(this));
}


float IntAutos::fix_value(float value)
{
	if(value >= 0) value = 1;
	else
	if(value < 0) value = -1;
	return value;	
}

int IntAutos::get_testy(float slope, int cursor_x, int ax, int ay)
{
	return ay;
}

int IntAutos::dump()
{
}
