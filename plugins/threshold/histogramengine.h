// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HISTOGRAMENGINE_H
#define HISTOGRAMENGINE_H

#include "histogramengine.inc"
#include "loadbalance.h"
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
	int *accum;
	int done;
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
	int *accum;
};


#endif
