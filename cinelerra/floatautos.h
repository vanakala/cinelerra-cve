#ifndef FLOATAUTOS_H
#define FLOATAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "floatauto.inc"

class FloatAutos : public Autos
{
public:
	FloatAutos(EDL *edl,
				Track *track, 
				int color, 
				float min, 
				float max, 
				float default_,
				int virtual_h = AUTOS_VIRTUAL_HEIGHT,
				int use_floats = 0);
	~FloatAutos();

	int get_track_pixels(int zoom_track, int pixel, int &center_pixel, float &yscale);
	int draw_joining_line(BC_SubWindow *canvas, int vertical, int center_pixel, int x1, int y1, int x2, int y2);
	float fix_value(float value);
	int get_testy(float slope, int cursor_x, int ax, int ay);
	int automation_is_constant(long start, 
		long length, 
		int direction,
		double &constant);
	double get_automation_constant(long start, long end);
// Get value at a specific point
	float get_value(long position, 
		int direction,
		FloatAuto* &previous,
		FloatAuto* &next);
	void get_fade_automation(double &slope,
		double &intercept,
		long input_position,
		long &slope_len,
		int direction);
	float value_to_percentage(float value);

	int dump();
	Auto* add_auto(long position, float value);
	Auto* append_auto();
	Auto* new_auto();
	int use_floats;
};


#endif
