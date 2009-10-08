
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

#ifndef CAMERAAUTO_H
#define CAMERAAUTO_H

#include "auto.h"


class CameraAuto : public Auto
{
public:
	CameraAuto();
	~CameraAuto();

	int camera_x, camera_y;		// center of view in source frame
								// 0 to source width/height
	float zoom;            		// amount to expand source
	int radius_in, radius_out;            		// radius of turn in or turn out, 
								// depending on which neighbor
								// isn't in a line with this point
	int acceleration_in, acceleration_out;			// acceleration into and out of the point
};





#endif
