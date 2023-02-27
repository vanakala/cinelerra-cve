// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PANAUTO_H
#define PANAUTO_H

#include "auto.h"
#include "cinelerra.h"
#include "filexml.inc"
#include "panautos.inc"

class PanAuto : public Auto
{
public:
	PanAuto(PanAutos *autos);

	int operator==(Auto &that);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	void copy_from(Auto *that);
	size_t get_size();
	void dump(int indent = 0);
	void rechannel();

	double values[MAXCHANNELS];
	int handle_x;
	int handle_y;
};

#endif
