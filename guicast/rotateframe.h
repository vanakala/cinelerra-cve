// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
		double angle);
	void rotate_rightangle(VFrame *input,
		VFrame *output, 
		int angle);
	void rotate_obliqueangle(VFrame *input,
		VFrame *output, 
		double angle);
	void get_rightdimensions(VFrame *frame,
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
// Interpolation uses input coordinates for each output coordinate.
	SourceCoord *float_matrix, **float_rows;
	VFrame *output, *input;
// Compare new angle with old angle
	double last_angle;
	double angle;
};


class RotateEngine : public Thread
{
public:
	RotateEngine(RotateFrame *plugin, int row1, int row2);
	~RotateEngine();

	void generate_matrix();
	void perform_rotation(VFrame *input, VFrame *output);
	void wait_completion();
	void create_matrix();
	void coords_to_pixel(SourceCoord &float_pixel, float &input_y, float &input_x);
	void perform_rotation();
	void run();

	int row1, row2;
	int do_matrix, do_rotation;
	int done;
	RotateFrame *plugin;
	Condition *input_lock;
	Condition *output_lock;
	VFrame *output;
	VFrame *input;
};

#endif
