// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	void transform_point(double x, double y, double *newx, double *newy);
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
	double transform_cubic(double dx,
		double jm1,
		double j,
		double jp1,
		double jp2);

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
		VFrame *input, VFrame *temp, int mode,
		double x1, double y1,
		double x2, double y2,
		double x3, double y3,
		double x4, double y4,
		int forward);
// Do rotation with the affine/perspective transform.
// This removes some of the extremely faint artifacts in the trig rotation.
	void rotate(VFrame *output, VFrame *input, double angle);
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
	double x1, y1, x2, y2, x3, y3, x4, y4;
	int x, y, w, h;
	int pivot_x, pivot_y;
	int user_pivot;
	int user_viewport;
	int forward;
	int use_opengl;
};

#endif
