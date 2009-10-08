
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
