#ifndef FADEENGINE_H
#define FADEENGINE_H

#include "loadbalance.h"
#include "vframe.inc"


class FadeEngine;

class FadePackage : public LoadPackage
{
public:
	FadePackage();

	int out_row1, out_row2;
};



class FadeUnit : public LoadClient
{
public:
	FadeUnit(FadeEngine *engine);
	~FadeUnit();
	
	void process_package(LoadPackage *package);
	
	FadeEngine *engine;
};

class FadeEngine : public LoadServer
{
public:
	FadeEngine(int cpus);
	~FadeEngine();

// the input pointer is never different than the output pointer in any
// of the callers
	void do_fade(VFrame *output, VFrame *input, float alpha);
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	VFrame *output;
	VFrame *input;
	float alpha;
};



#endif
