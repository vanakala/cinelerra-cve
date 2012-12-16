
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

#ifndef OVERLAYFRAME_H
#define OVERLAYFRAME_H

#include "loadbalance.h"
#include "overlayframe.inc"
#include "vframe.inc"

#define DIRECT_COPY 0
#define BILINEAR 1
#define BICUBIC 2
#define LANCZOS 3

class OverlayKernel
{
public:
	OverlayKernel(int interpolation_type);
	~OverlayKernel();

	float *lookup;
	float width;
	int n;
	int type;
};

class DirectEngine;

class DirectPackage : public LoadPackage
{
public:
	DirectPackage();
	int out_row1, out_row2;
};

class NNEngine;

class NNPackage : public LoadPackage
{
public:
	NNPackage();
	int out_row1, out_row2;
};

class SampleEngine;


class SamplePackage : public LoadPackage
{
public:
	SamplePackage();

	int out_col1;
	int out_col2;
};

class DirectUnit : public LoadClient
{
public:
	DirectUnit(DirectEngine *server);
	~DirectUnit();
	void process_package(LoadPackage *package);
	DirectEngine *engine;
};

class NNUnit : public LoadClient
{
public:
	NNUnit(NNEngine *server);
	~NNUnit();
	void process_package(LoadPackage *package);
	NNEngine *engine;
};

class SampleUnit : public LoadClient
{
public:
	SampleUnit(SampleEngine *server);
	~SampleUnit();

	void process_package(LoadPackage *package);
	SampleEngine *engine;
};

class DirectEngine : public LoadServer
{
public:
	DirectEngine(int cpus);
	~DirectEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	int in_x1;
	int in_y1;
	int out_x1;
	int out_x2;
	int out_y1;
	int out_y2;
	int alpha;
	int mode;
};

class NNEngine : public LoadServer
{
public:
	NNEngine(int cpus);
	~NNEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	float in_x1;
	float in_x2;
	float in_y1;
	float in_y2;
	float out_x1;
	float out_x2;
	int out_x1i;
	int out_x2i;
	float out_y1;
	float out_y2;
	int alpha;
	int mode;

	int *in_lookup_x;
	int *in_lookup_y;
};

class SampleEngine : public LoadServer
{
public:
	SampleEngine(int cpus);
	~SampleEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	OverlayKernel *kernel;

	int col_out1;
	int col_out2;
	int row_in;

	float in1;
	float in2;
	float out1;
	float out2;

	int alpha;
	int mode;

	int *lookup_sx0;
	int *lookup_sx1;
	int *lookup_sk;
	float *lookup_wacc;
	int kd;
};

class OverlayFrame
{
public:
	OverlayFrame(int cpus = 1);
	virtual ~OverlayFrame();

	int overlay(VFrame *output,
		VFrame *input,
		float in_x1,
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2,
		float alpha,
		int mode,
		int interpolation_type);

	DirectEngine *direct_engine;
	NNEngine *nn_engine;
	SampleEngine *sample_engine;

	VFrame *temp_frame;
	int cpus;
	OverlayKernel *kernel[4];
};

#endif
