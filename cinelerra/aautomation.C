
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

#include "aautomation.h"
#include "atrack.inc"
#include "colors.h"
#include "edl.inc"
#include "floatauto.h"
#include "floatautos.h"
#include "panautos.h"

AAutomation::AAutomation(EDL *edl, Track *track)
 : Automation(edl, track)
{
	autos[AUTOMATION_FADE] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_FADE]->autoidx = AUTOMATION_FADE;
	autos[AUTOMATION_FADE]->autogrouptype = AUTOGROUPTYPE_AUDIO_FADE;
	autos[AUTOMATION_PAN] = new PanAutos(edl, track);
	autos[AUTOMATION_PAN]->autoidx = AUTOMATION_PAN;
	autos[AUTOMATION_PAN]->autogrouptype = -1;
}

size_t AAutomation::get_size()
{
	size_t size = sizeof(*this);

	if(autos[AUTOMATION_FADE])
		size += ((FloatAutos*)autos[AUTOMATION_FADE])->get_size();
	if(autos[AUTOMATION_PAN])
		size += ((PanAutos*)autos[AUTOMATION_PAN])->get_size();
	size += Automation::get_size();
	return size;
}
