
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

#ifndef AFFINE_H
#define AFFINE_H


#include "affine.inc"
#include "loadbalance.h"
#include "vframe.inc"

// Affine transform library

#define AFFINE_OVERSAMPLE 2

class AffineMatrix
{
public:
	AffineMatrix();
	void identity();
	void translate(double x, double y);
	void scale(double x, double y);
// Result is put in dst
	void multiply(AffineMatrix *dst);
	void copy_from(AffineMatrix *src);
	void invert(AffineMatrix *dst);
	void transform_point(float x, float y, float *newx, float *newy);
	double determinant();
	void dump();
	double values[3][3];
};

class AffinePackage : public LoadPackage
{
public:
	AffinePackage();
	int y1, y2;
};

class AffineUnit : public LoadClient
{
public:
	AffineUnit(AffineEngine *server);
	void process_package(LoadPackage *package);
	void calculate_matrix(
		double in_x1,
		double in_y1,
		double in_x2,
		double in_y2,
		double out_x1,
		double out_y1,
		double out_x2,
		double out_y2,
		double out_x3,
		double out_y3,
		double out_x4,
		double out_y4,
		AffineMatrix *result);
	float transform_cubic(float dx,
    	float jm1,
    	float j,
    	float jp1,
    	float jp2);
	AffineEngine *server;
};

class AffineEngine : public LoadServer
{
public:
	AffineEngine(int total_clients, 
		int total_packages);

// Temp is only used for STRETCH oversampling
// Range of coords is 0 to 100 for coordinates in the image.
// The coordinate locations are clockwise around the image.
	void process(VFrame *output,
		VFrame *input, 
		VFrame *temp,
		int mode,
		float x1, 
		float y1, 
		float x2, 
		float y2, 
		float x3, 
		float y3, 
		float x4, 
		float y4,
		int forward);
// Do rotation with the affine/perspective transform.
// This removes some of the extremely faint artifacts in the trig rotation.
	void rotate(VFrame *output,
		VFrame *input, 
		float angle);
// Set the viewport to transform.  All operations are clamped to this area
// instead of the frame dimensions.  The rotation operation centers on this
// area.
	void set_viewport(int x, int y, int w, int h);
// For rotation, set the pivot point.  The default is in the middle of the viewport.
	void set_pivot(int x, int y);
	void unset_pivot();
	void unset_viewport();
// To use OpenGL for the processing, set to 1
	void set_opengl(int value);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VFrame *input, *output, *temp;
	int mode;
	enum
	{
		PERSPECTIVE,
		SHEER,
		STRETCH,
		ROTATE
	};
	float x1, y1, x2, y2, x3, y3, x4, y4;
	int x, y, w, h;
	int pivot_x, pivot_y;
	int user_pivot;
	int user_viewport;
	int forward;
	int use_opengl;
};



#endif
