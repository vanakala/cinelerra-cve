#ifndef HISTOGRAMENGINE_H
#define HISTOGRAMENGINE_H

#include "histogramengine.inc"
#include "loadbalance.h"
#include "../colors/plugincolors.inc"
#include "vframe.inc"

#include <stdint.h>

class HistogramPackage : public LoadPackage
{
public:
	HistogramPackage();
	int start, end;
};

class HistogramUnit : public LoadClient
{
public:
	HistogramUnit(HistogramEngine *server);
	~HistogramUnit();
	void process_package(LoadPackage *package);
	HistogramEngine *server;
	int64_t *accum[5];
};

class HistogramEngine : public LoadServer
{
public:
	HistogramEngine(int total_clients, int total_packages);
	~HistogramEngine();
	void process_packages(VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VFrame *data;
	YUV *yuv;
	int64_t *accum[5];
};


#endif
