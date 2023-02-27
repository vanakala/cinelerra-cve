// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUTO_H
#define AUTO_H

#include "auto.inc"
#include "autos.inc"
#include "datatype.h"
#include "filexml.inc"
#include "linklist.h"


class Auto : public ListItem<Auto>
{
public:
	Auto();
	Auto(Autos *autos);

	virtual Auto& operator=(Auto &that);
	virtual int operator==(Auto &that) { return 0; };
	virtual void copy_from(Auto *that);
// create an interpolation using a1 and a2, (defaulting to previous and next)
//  if not possible, just fill from a1 (or from template if given)
	virtual void interpolate_from(Auto *a1, Auto *a2, ptstime new_postime, Auto *templ);
	virtual void save_xml(FileXML *file) {};
	virtual void copy(Auto *that, ptstime start, ptstime end) {};

	virtual void set_compat_value(double value) {};
	virtual void load(FileXML *file) {};

	virtual void dump(int indent = 0) {};

	Autos *autos;
	ptstime pos_time;
};

#endif
