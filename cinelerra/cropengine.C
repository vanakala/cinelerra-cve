
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

#include "clip.h"
#include "cropauto.h"
#include "cropautos.h"
#include "cropengine.h"
#include "mainerror.h"
#include "vframe.h"

CropEngine::CropEngine()
{
}

VFrame *CropEngine::do_crop(Autos *autos, VFrame *frame, int before_plugins)
{
	int left, right, top, bottom;
	int w, h;
	CropAuto *cropauto;

	if(!autos->first)
		return frame;

	cropauto = ((CropAutos*)autos)->get_values(frame->get_pts(),
		&left, &right, &top, &bottom);


	if(left == 0 && top == 0 && right >= frame->get_w() && bottom >= frame->get_h())
		return frame;

	if(before_plugins)
	{
		if(!cropauto || !cropauto->apply_before_plugins)
			return frame;
	}
	else
	{
		if(cropauto && cropauto->apply_before_plugins)
			return frame;
	}

	w = frame->get_w();
	h = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
			if(i < top || i > bottom)
			{
				for(int j = 0; j < w; j++)
				{
					row[j * 4] = 0;
					row[j * 4 + 1] = 0;
					row[j * 4 + 2] = 0x8000;
					row[j * 4 + 3] = 0x8000;
				}
			}
			else
			{
				for(int j = 0; j < left; j++)
				{
					row[j * 4] = 0;
					row[j * 4 + 1] = 0;
					row[j * 4 + 2] = 0x8000;
					row[j * 4 + 3] = 0x8000;
				}
				for(int j = right; j < w; j++)
				{
					row[j * 4] = 0;
					row[j * 4 + 1] = 0;
					row[j * 4 + 2] = 0x8000;
					row[j * 4 + 3] = 0x8000;
				}
			}
		}
		break;
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
			if(i < top || i > bottom)
			{
				for(int j = 0; j < w; j++)
				{
					row[j * 4] = 0;
					row[j * 4 + 1] = 0;
					row[j * 4 + 2] = 0;
					row[j * 4 + 3] = 0;
				}
			}
			else
			{
				for(int j = 0; j < left; j++)
				{
					row[j * 4] = 0;
					row[j * 4 + 1] = 0;
					row[j * 4 + 2] = 0;
					row[j * 4 + 3] = 0;
				}
				for(int j = right; j < w; j++)
				{
					row[j * 4] = 0;
					row[j * 4 + 1] = 0;
					row[j * 4 + 2] = 0;
					row[j * 4 + 3] = 0;
				}
			}
		}
		break;
	default:
		break;
	}
	return frame;
}
