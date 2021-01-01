// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef OVERLAYFRAME_H
#define OVERLAYFRAME_H

#include "loadbalance.h"
#include "overlayframe.inc"
#include "vframe.inc"

#define DIRECT_COPY 0
#define BILINEAR 1
#define BICUBIC 2
#define LANCZOS 3

struct transfers
{
	const char *text;
	int value;
};

class OverlayKernel
{
public:
	OverlayKernel(int interpolation_type);
	~OverlayKernel();

	double *lookup;
	double width;
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
	double alpha;
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
	double in_x1;
	double in_x2;
	double in_y1;
	double in_y2;
	double out_x1;
	double out_x2;
	int out_x1i;
	int out_x2i;
	double out_y1;
	double out_y2;
	double alpha;
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

	double in1;
	double in2;
	double out1;
	double out2;

	double alpha;
	int mode;

	int *lookup_sx0;
	int *lookup_sx1;
	int *lookup_sk;
	double *lookup_wacc;
	int kd;
};

class OverlayFrame
{
public:
	OverlayFrame(int cpus = 1);
	~OverlayFrame();

	int overlay(VFrame *output,
		VFrame *input,
		double in_x1,
		double in_y1,
		double in_x2,
		double in_y2,
		double out_x1,
		double out_y1,
		double out_x2,
		double out_y2,
		double alpha,
		int mode,
		int interpolation_type);

	static const char *transfer_name(int transfer);
	static int transfer_mode(const char *text);

	DirectEngine *direct_engine;
	NNEngine *nn_engine;
	SampleEngine *sample_engine;

	VFrame *temp_frame;
	int cpus;
	OverlayKernel *kernel[4];
private:
	static struct transfers transfer_names[];
};

#endif
