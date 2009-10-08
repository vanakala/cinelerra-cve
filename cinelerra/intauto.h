
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

#ifndef INTAUTO_H
#define INTAUTO_H

#include "auto.h"
#include "edl.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "intautos.inc"

class IntAuto : public Auto
{
public:
	IntAuto(EDL *edl, IntAutos *autos);
	~IntAuto();

	void copy_from(Auto *that);
	void copy_from(IntAuto *that);
	int operator==(Auto &that);
	int operator==(IntAuto &that);

	int identical(IntAuto *that);
	void load(FileXML *file);
	void copy(int64_t start, int64_t end, FileXML *file, int default_only);
	float value_to_percentage();
	int percentage_to_value(float percentage);

	int value;
};

#endif
