// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MASKENGINE_H
#define MASKENGINE_H

#include "condition.inc"
#include "datatype.h"
#include "loadbalance.h"
#include "maskautos.inc"
#include "maskauto.inc"
#include "mutex.h"
#include "vframe.inc"

#define OVERSAMPLE 8
#define NUM_SPANS 4 /* starting number of spans to be allocated for */

class MaskEngine;

enum
{
	RECALCULATE_PART,
	APPLY_PART
};


class MaskPackage : public LoadPackage
{
public:
	MaskPackage();

	int row1, row2;
};

class MaskUnit : public LoadClient
{
public:
	MaskUnit(MaskEngine *engine);
	~MaskUnit();

	void process_package(LoadPackage *package);
private:
	void draw_line_clamped(int x1, int y1, int x2, int y2, int w, int h, int hoffset);
	void do_feather(VFrame *output,
		VFrame *input, 
		int feather,
		int start_out, 
		int end_out);
	int do_feather_2(VFrame *output,
		VFrame *input, 
		int feather,
		int start_out, 
		int end_out);
	void blur_strip(double *val_p, double *val_m,
		double *dst, double *src,
		int size, double max);

	double n_p[5], n_m[5];
	double d_p[5], d_m[5];
	double bd_p[5], bd_m[5];
	MaskEngine *engine;
	short **row_spans;
	int row_spans_h;
	int size_allocated;
	double *src;
	double *dst;
	double *val_p;
	double *val_m;
};


class MaskEngine : public LoadServer
{
public:
	MaskEngine(int cpus);
	~MaskEngine();

	void do_mask(VFrame *output, 
		MaskAutos *keyframe_set, 
		int before_plugins);

	int points_equivalent(ArrayList<MaskPoint*> *new_points, 
		ArrayList<MaskPoint*> *points);

	void delete_packages();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
// State of last mask
	VFrame *mask;
// Temporary for feathering
	VFrame *temp_mask;
	ArrayList<ArrayList<MaskPoint*>*> point_sets;
	int mode;
	int feather;              // gui feather
	int realfeather;          // real feather
	int recalculate;
	int value;
	pthread_mutex_t stage1_finished_mutex;
	pthread_cond_t stage1_finished_cond;
	int stage1_finished_count;
	Mutex protect_data;	// protects the following members
	int first_nonempty_rowspan;
	int last_nonempty_rowspan;
};

#endif
