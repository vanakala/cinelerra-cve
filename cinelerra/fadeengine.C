// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "clip.h"
#include "fadeengine.h"
#include "vframe.h"

#include <stdint.h>

FadeUnit::FadeUnit(FadeEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

void FadeUnit::process_package(LoadPackage *package)
{
	FadePackage *pkg = (FadePackage*)package;
	VFrame *frame = engine->frame;
	int row1 = pkg->out_row1;
	int row2 = pkg->out_row2;
	int width = frame->get_w();
	unsigned int opacity = engine->alpha * 0xffff;
	unsigned int product;

	switch(frame->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = row1; i < row2; i++)
		{
			uint16_t *row = (uint16_t*)frame->get_row_ptr(i);

			for(int j = 0; j < width; j++, row += 4)
			{
				if(row[3] == 0xffff)
					row[3] = opacity;
				else
					row[3] = row[3] * opacity / 0xffff;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = row1; i < row2; i++)
		{
			uint16_t *row = (uint16_t*)frame->get_row_ptr(i);

			for(int j = 0; j < width; j++, row += 4)
			{
				if(row[0] == 0xffff)
					row[0] = opacity;
				else
					row[0] = row[0] * opacity / 0xffff;
			}
		}
		break;
	}
}


FadeEngine::FadeEngine(int cpus)
 : LoadServer(cpus, cpus)
{
}

void FadeEngine::do_fade(VFrame *frame, double alpha)
{
	this->frame = frame;
	this->alpha = alpha;

// Sanity
	if(!EQUIV(alpha, 1.0))
		process_packages();
}

void FadeEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		FadePackage *package = (FadePackage*)get_package(i);
		package->out_row1 = frame->get_h() * i / get_total_packages();
		package->out_row2 = frame->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* FadeEngine::new_client()
{
	return new FadeUnit(this);
}

LoadPackage* FadeEngine::new_package()
{
	return new FadePackage;
}
