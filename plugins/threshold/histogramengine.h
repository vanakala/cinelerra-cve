
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
