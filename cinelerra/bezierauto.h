#ifndef BEZIERAUTO_H
#define BEZIERAUTO_H

// Automation point that takes floating point values

#include "auto.h"
#include "bezierautos.inc"
#include "filexml.inc"

class BezierAuto : public Auto
{
public:
	BezierAuto() {};
	BezierAuto(EDL *edl, BezierAutos *autos);
	~BezierAuto();

	int operator==(Auto &that);
	int operator==(BezierAuto &that);
	void copy_from(Auto *that);
	void copy_from(BezierAuto *that);
	void load(FileXML *file);
	void copy(long start, long end, FileXML *file, int default_only);
	int draw(BC_SubWindow *canvas, 
					int x, 
					int center_pixel, 
					float scale,
					int vertical,
					int show_value);

// return a selection type if selected
// 0 - none
// 1 - frame number
// 2 - xy
// 3 - zoom
// 4 - control_in_xy
// 5 - control_out_xy
// 6 - control_in_zoom
// 7 - control_out_zoom
	int select(BC_SubWindow *canvas, 
				int x, 
				int center_pixel, 
				float scale,
				int cursor_x, 
				int cursor_y, 
				int shift_down,
				int ctrl_down,
				int mouse_button,
				int vertical);

// Z is no longer used
	float center_x, center_y, center_z;
// ***control points are relative to center points***
	float control_in_x, control_in_y;
	float control_out_x, control_out_y;
	float control_in_z, control_out_z;

private:
	int identical(BezierAuto *src);
	int WIDTH, HEIGHT;
	int value_to_str(char *string, float value) {};
	int value_to_str(char *string); 
	int get_control_points(int x, int center_pixel, float scale, int *control_x, int *control_y, int vertical);
	int test_control_point(int center_x, int center_y, int x, int y, int cursor_x, int cursor_y);
	float get_distance(int x1, int y1, int x2, int y2);
};



#endif
