
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

#ifndef MASKENGINE_H
#define MASKENGINE_H


#include "condition.inc"
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
	~MaskPackage();

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
		float feather, 
		int start_out, 
		int end_out);
	int do_feather_2(VFrame *output,
		VFrame *input, 
		float feather, 
		int start_out, 
		int end_out);
	template<class T>
	void blur_strip(float *val_p, 
		float *val_m, 
		float *dst, 
		float *src, 
		int size,
		T max);

    float n_p[5], n_m[5];
    float d_p[5], d_m[5];
    float bd_p[5], bd_m[5];
	MaskEngine *engine;
	short **row_spans;
	short row_spans_h;
};


class MaskEngine : public LoadServer
{
public:
	MaskEngine(int cpus);
	~MaskEngine();


	void do_mask(VFrame *output, 
		int64_t start_position,
		double frame_rate,
		double project_frame_rate,
		MaskAutos *keyframe_set, 
		int direction,
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
	float feather;              // gui feather
	float realfeather;          // real feather
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
