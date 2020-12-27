// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FADEENGINE_H
#define FADEENGINE_H

#include "loadbalance.h"
#include "vframe.inc"


class FadeEngine;

class FadePackage : public LoadPackage
{
public:
	FadePackage() {};

	int out_row1, out_row2;
};


class FadeUnit : public LoadClient
{
public:
	FadeUnit(FadeEngine *engine);

	void process_package(LoadPackage *package);

	FadeEngine *engine;
};

class FadeEngine : public LoadServer
{
public:
	FadeEngine(int cpus);

	void do_fade(VFrame *frame, double alpha);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *frame;
	double alpha;
};

#endif
