// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "colorspaces.h"
#include "histogramengine.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>

HistogramPackage::HistogramPackage()
 : LoadPackage()
{
	start = end = 0;
}


HistogramUnit::HistogramUnit(HistogramEngine *server)
 : LoadClient(server)
{
	accum = new int[HISTOGRAM_RANGE];
}

HistogramUnit::~HistogramUnit()
{
	delete [] accum;
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;
	HistogramEngine *server = (HistogramEngine*)get_server();
	VFrame *data = server->data;

	int w = data->get_w();
	int h = data->get_h();
	int *accum_v = accum;
	int r, g, b, a, y, u, v;

	memset(accum, 0, sizeof(int) * HISTOGRAM_RANGE);

	switch(data->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->start; i < pkg->end; i++)
		{
			uint16_t *row = (uint16_t*)data->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				r = row[0];
				g = row[1];
				b = row[2];
				v = ColorSpaces::rgb_to_y_16(r, g, b);
				v += -(int)(HISTOGRAM_MIN * 0xffff);
				CLAMP(v, 0, HISTOGRAM_RANGE);
				accum_v[v]++;
				row += 4;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = pkg->start; i < pkg->end; i++)
		{
			uint16_t *row = (uint16_t*)data->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				v = row[1];
				v += -(int)(HISTOGRAM_MIN * 0xffff);
				CLAMP(v, 0, HISTOGRAM_RANGE);
				accum_v[v]++;
				row += 4;
			}
		}
		break;
	}
	done = 1;
}


HistogramEngine::HistogramEngine(int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	data = 0;
	accum = new int[HISTOGRAM_RANGE];
}

HistogramEngine::~HistogramEngine()
{
	delete [] accum;
}

void HistogramEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();

	memset(accum, 0, sizeof(int) * HISTOGRAM_RANGE);

	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)get_client(i);

		if(!unit->done)
			continue;
		for(int j = 0; j < HISTOGRAM_RANGE; j++)
			accum[j] += unit->accum[j];
	}
}

void HistogramEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}

	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		unit->done = 0;
	}
}

LoadClient* HistogramEngine::new_client()
{
	return (LoadClient*)new HistogramUnit(this);
}

LoadPackage* HistogramEngine::new_package()
{
	return (LoadPackage*)new HistogramPackage;
}

