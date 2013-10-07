
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
#include "bcsignals.h"
#include "filexml.h"
#include "track.h"

Auto::Auto()
 : ListItem<Auto>()
{
	this->edl = 0;
	this->autos = 0;
	pos_time = 0;
	skip = 0;
	is_default = 0;
}

Auto::Auto(EDL *edl, Autos *autos)
 : ListItem<Auto>()
{
	this->edl = edl;
	this->autos = autos;
	pos_time = 0;
	skip = 0;
	is_default = 0;
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
