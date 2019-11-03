
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

#include "bcsignals.h"
#include "clip.h"
#include "colors.h"
#include "cropautos.h"
#include "edl.inc"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "maskautos.h"
#include "overlayframe.inc"
#include "track.h"
#include "vautomation.h"


VAutomation::VAutomation(EDL *edl, Track *track)
 : Automation(edl, track)
{
	autos[AUTOMATION_FADE] = new FloatAutos(edl, track, 100);
	autos[AUTOMATION_MODE] = new IntAutos(edl, track, TRANSFER_NORMAL);
	autos[AUTOMATION_MASK] = new MaskAutos(edl, track);
	autos[AUTOMATION_CAMERA_X] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_CAMERA_Y] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_PROJECTOR_X] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_PROJECTOR_Y] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_CAMERA_Z] = new FloatAutos(edl, track, 1.0);
	autos[AUTOMATION_PROJECTOR_Z] = new FloatAutos(edl, track, 1.0);
	autos[AUTOMATION_CROP] = new CropAutos(edl, track);

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		if (autos[i]) 
		{
			autos[i]->autoidx = i;
			autos[i]->autogrouptype = autogrouptype(i, autos[i]->track);
		}
}

void VAutomation::get_projector(double *x,
	double *y,
	double *z,
	ptstime position)
{
	*x = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_X])->get_value(position);
	*y = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Y])->get_value(position);
	*z = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Z])->get_value(position);
}

void VAutomation::get_camera(double *x,
	double *y,
	double *z,
	ptstime position)
{
	*x = ((FloatAutos*)autos[AUTOMATION_CAMERA_X])->get_value(position);
	*y = ((FloatAutos*)autos[AUTOMATION_CAMERA_Y])->get_value(position);
	*z = ((FloatAutos*)autos[AUTOMATION_CAMERA_Z])->get_value(position);
}

size_t VAutomation::get_size()
{
	size_t size = sizeof(*this);

	if(autos[AUTOMATION_FADE])
		size += ((FloatAutos*)autos[AUTOMATION_FADE])->get_size();
	if(autos[AUTOMATION_MODE])
		size += ((IntAutos*)autos[AUTOMATION_MODE])->get_size();
	if(autos[AUTOMATION_MASK])
		size += ((MaskAutos*)autos[AUTOMATION_MASK])->get_size();
	if(autos[AUTOMATION_CAMERA_X])
		size += ((FloatAutos*)autos[AUTOMATION_CAMERA_X])->get_size();
	if(autos[AUTOMATION_CAMERA_Y])
		size += ((FloatAutos*)autos[AUTOMATION_CAMERA_Y])->get_size();
	if(autos[AUTOMATION_PROJECTOR_X])
		size += ((FloatAutos*)autos[AUTOMATION_PROJECTOR_X])->get_size();
	if(autos[AUTOMATION_PROJECTOR_Y])
		size += ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Y])->get_size();
	if(autos[AUTOMATION_CAMERA_Z])
		size += ((FloatAutos*)autos[AUTOMATION_CAMERA_Z])->get_size();
	if(autos[AUTOMATION_PROJECTOR_Z])
		size += ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Z])->get_size();
	if(autos[AUTOMATION_CROP])
		size += ((CropAutos*)autos[AUTOMATION_CROP])->get_size();
	size += Automation::get_size();
	return size;
}
