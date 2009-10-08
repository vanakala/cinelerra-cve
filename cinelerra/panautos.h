
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

#ifndef PANAUTOS_H
#define PANAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "panauto.inc"
#include "track.inc"

class PanAutos : public Autos
{
public:
	PanAutos(EDL *edl, Track *track);
	~PanAutos();
	
	Auto* new_auto();
	void get_handle(int &handle_x,
		int &handle_y,
		int64_t position, 
		int direction,
		PanAuto* &previous,
		PanAuto* &next);
	void dump();
};

#endif
