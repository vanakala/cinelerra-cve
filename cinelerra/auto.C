
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

#include "auto.h"
#include "autos.h"
#include "filexml.h"

Auto::Auto()
 : ListItem<Auto>()
{
	this->edl = 0;
	this->autos = 0;
	position = 0;
	skip = 0;
	WIDTH = 10;
	HEIGHT = 10;
	is_default = 0;
}

Auto::Auto(EDL *edl, Autos *autos)
 : ListItem<Auto>()
{
	this->edl = edl;
	this->autos = autos;
	position = 0;
	skip = 0;
	WIDTH = 10;
	HEIGHT = 10;
	is_default = 0;
}

Auto& Auto::operator=(Auto& that)
{
	copy_from(&that);
	return *this;
}

int Auto::operator==(Auto &that)
{
	printf("Auto::operator== called\n");
	return 0;
}

void Auto::copy(int64_t start, int64_t end, FileXML *file, int default_only)
{
	printf("Auto::copy called\n");
}

void Auto::copy_from(Auto *that)
{
	this->position = that->position;
}

int Auto::interpolate_from(Auto *a1, Auto *a2, int64_t position)
{
	copy_from(a1);
	return 0;
}

void Auto::load(FileXML *xml)
{
	printf("Auto::load\n");
}



float Auto::value_to_percentage()
{
	return 0;
}

float Auto::invalue_to_percentage()
{
	return 0;
}

float Auto::outvalue_to_percentage()
{
	return 0;
}

