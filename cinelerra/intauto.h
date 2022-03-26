// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INTAUTO_H
#define INTAUTO_H

#include "auto.h"
#include "edl.inc"
#include "filexml.inc"
#include "intautos.inc"

class IntAuto : public Auto
{
public:
	IntAuto(EDL *edl, IntAutos *autos);

	void copy_from(Auto *that);
	void copy_from(IntAuto *that);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	size_t get_size();
	void dump(int indent = 0);

	int value;
};

#endif
