
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

#ifndef ROTATEFRAME_H
#define ROTATEFRAME_H

#include "condition.inc"
#include "thread.h"
#include "vframe.inc"

class RotateEngine;

typedef struct
{
	float x, y;
} SourceCoord;

class RotateFrame
{
public:
	RotateFrame(int cpus, int width, int height);
	~RotateFrame();

// angle is 0 - 360
	void rotate(VFrame *output, 
		VFrame *input, 
		double angle, 
		int interpolate);
	int rotate_rightangle(VFrame *input, 
		VFrame *output, 
		int angle);
	int rotate_obliqueangle(VFrame *input, 
		VFrame *output, 
		double angle,
		int interpolate);
	int get_rightdimensions(VFrame *frame, 
		int &diameter, 
		int &in_x1, 
		int &in_y1, 
		int &in_x2, 
		int &in_y2, 
		int &out_x1, 
		int &out_y1, 
		int &out_x2, 
		int &out_y2);

	int cpus;
	RotateEngine **engine;
// Matrix of source pixel offsets
	int *int_matrix, **int_rows;
// Interpolation uses input coordinates for each output coordinate.
	SourceCoord *float_matrix, **float_rows;
	VFrame *output, *input;
	int interpolate;
	int last_interpolate;
// Compare new angle with old angle
	double last_angle;
	double angle;
};


class RotateEngine : public Thread
{
public:
	RotateEngine(RotateFrame *plugin, int row1, int row2);
	~RotateEngine();
	
	int generate_matrix(int interpolate);
	int perform_rotation(VFrame *input, 
		VFrame *output, 
		int interpolate);
	int wait_completion();
	int create_matrix();
	int coords_to_pixel(int &input_y, int &input_x);
	int coords_to_pixel(SourceCoord &float_pixel, float &input_y, float &input_x);
	int perform_rotation();
	void run();
	
	int row1, row2;
	int interpolate;
	int do_matrix, do_rotation;
	int done;
	RotateFrame *plugin;
	Condition *input_lock;
	Condition *output_lock;
	VFrame *output;
	VFrame *input;
};

#endif
