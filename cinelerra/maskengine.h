#ifndef MASKENGINE_H
#define MASKENGINE_H


#include "loadbalance.h"
#include "maskautos.inc"
#include "maskauto.inc"
#include "mutex.inc"
#include "vframe.inc"


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
	int part;
	Mutex *apply_mutex;
};

class MaskUnit : public LoadClient
{
public:
	MaskUnit(MaskEngine *engine);
	~MaskUnit();

	void process_package(LoadPackage *package);
	void draw_line_clamped(VFrame *frame, int &x1, int &y1, int x2, int y2, unsigned char value);
	void do_feather(VFrame *output,
		VFrame *input, 
		float feather, 
		int start_out, 
		int end_out);
	void blur_strip(float *val_p, 
		float *val_m, 
		float *dst, 
		float *src, 
		int size,
		int max);

    float n_p[5], n_m[5];
    float d_p[5], d_m[5];
    float bd_p[5], bd_m[5];
	MaskEngine *engine;
	VFrame *temp;
};


class MaskEngine : public LoadServer
{
public:
	MaskEngine(int cpus);
	~MaskEngine();

	void do_mask(VFrame *output, 
		MaskAutos *keyframe_set, 
		long position, 
		int direction);
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
	float feather;
	int recalculate;
	int value;
};



#endif
