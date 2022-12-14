// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "auto.h"
#include "autos.h"
#include "bcsignals.h"

Auto::Auto()
 : ListItem<Auto>()
{
	this->edl = 0;
	this->autos = 0;
	pos_time = 0;
}

Auto::Auto(EDL *edl, Autos *autos)
 : ListItem<Auto>()
{
	this->edl = edl;
	this->autos = autos;
	pos_time = 0;
}

Auto& Auto::operator=(Auto& that)
{
	copy_from(&that);
	return *this;
}

void Auto::copy_from(Auto *that)
{
	this->pos_time = that->pos_time;
}

void Auto::interpolate_from(Auto *a1, Auto *a2, ptstime new_position, Auto *templ)
{
	if(!templ)
		templ = a1;
	if(!templ)
		templ = previous;
	if(!templ && autos && autos->first)
		templ = autos->first;
	if(templ)
		copy_from(templ);
	pos_time = new_position;
}
