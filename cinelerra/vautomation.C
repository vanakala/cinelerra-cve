
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
#include "edl.h"
#include "edlsession.h"
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
}



VAutomation::~VAutomation()
{
}


int VAutomation::create_objects()
{
	Automation::create_objects();

	autos[AUTOMATION_FADE] = new FloatAutos(edl, track, 100);
	autos[AUTOMATION_FADE]->create_objects();

	autos[AUTOMATION_MODE] = new IntAutos(edl, track, TRANSFER_NORMAL);
	autos[AUTOMATION_MODE]->create_objects();

	autos[AUTOMATION_MASK] = new MaskAutos(edl, track);
	autos[AUTOMATION_MASK]->create_objects();

	autos[AUTOMATION_CAMERA_X] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_CAMERA_X]->create_objects();

	autos[AUTOMATION_CAMERA_Y] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_CAMERA_Y]->create_objects();

	autos[AUTOMATION_PROJECTOR_X] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_PROJECTOR_X]->create_objects();

	autos[AUTOMATION_PROJECTOR_Y] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_PROJECTOR_Y]->create_objects();

	autos[AUTOMATION_CAMERA_Z] = new FloatAutos(edl, track, 1.0);
	autos[AUTOMATION_CAMERA_Z]->create_objects();

	autos[AUTOMATION_PROJECTOR_Z] = new FloatAutos(edl, track, 1.0);
	autos[AUTOMATION_PROJECTOR_Z]->create_objects();

// 	autos[AUTOMATION_NUDGE] = new FloatAutos(edl, track, 0.0);
// 	autos[AUTOMATION_NUDGE]->create_objects();

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		if (autos[i]) 
		{
			autos[i]->autoidx = i;
			autos[i]->autogrouptype = autogrouptype(i, autos[i]->track);
		}

	return 0;
}

void VAutomation::get_projector(float *x, 
	float *y, 
	float *z, 
	ptstime position)
{
	FloatAuto *before, *after;
	before = 0;
	after = 0;
	*x = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_X])->get_value(position,
		before,
		after);
	before = 0;
	after = 0;
	*y = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Y])->get_value(position,
		before,
		after);
	before = 0;
	after = 0;
	*z = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Z])->get_value(position,
		before,
		after);
}


void VAutomation::get_camera(float *x, 
	float *y, 
	float *z, 
	ptstime position)
{
	FloatAuto *before, *after;
	before = 0;
	after = 0;
	*x = ((FloatAutos*)autos[AUTOMATION_CAMERA_X])->get_value(position,
		before,
		after);
	before = 0;
	after = 0;
	*y = ((FloatAutos*)autos[AUTOMATION_CAMERA_Y])->get_value(position,
		before,
		after);
	before = 0;
	after = 0;
	*z = ((FloatAutos*)autos[AUTOMATION_CAMERA_Z])->get_value(position,
		before,
		after);
}

