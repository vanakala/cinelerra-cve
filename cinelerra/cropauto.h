
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef CROPAUTO_H
#define CROPAUTO_H

#include "auto.h"
#include "edl.inc"
#include "filexml.inc"
#include "cropauto.inc"
#include "cropautos.inc"

class CropAuto : public Auto
{
public:
	CropAuto(EDL *edl, CropAutos *autos);

	void copy_from(Auto *that);
	void copy_from(CropAuto *that);

	int identical(CropAuto *that);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	size_t get_size();
	void dump(int indent = 0);

	int left;
	int right;
	int top;
	int bottom;
	int apply_before_plugins;
};

#endif
