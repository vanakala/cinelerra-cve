
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

#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "atrack.inc"
#include "bcsignals.h"
#include "colors.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "intautos.h"
#include "track.h"

int Automation::autogrouptypes_fixedrange[] =
{
	0,
	0,
	0,
	0,
	0,
	1
};


Automation::Automation(EDL *edl, Track *track)
{
	this->edl = edl;
	this->track = track;
	memset(autos, 0, sizeof(Autos*) * AUTOMATION_TOTAL);

	autos[AUTOMATION_MUTE] = new IntAutos(edl, track, 0);
	autos[AUTOMATION_MUTE]->autoidx = AUTOMATION_MUTE; 
	autos[AUTOMATION_MUTE]->autogrouptype = AUTOGROUPTYPE_INT255;
}

Automation::~Automation()
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		delete autos[i];
	}
}

int Automation::autogrouptype(int autoidx, Track *track)
{
	int autogrouptype = -1;
	switch (autoidx) 
	{
	case AUTOMATION_CAMERA_X:
	case AUTOMATION_PROJECTOR_X:
		autogrouptype = AUTOGROUPTYPE_X;
		break;
	case AUTOMATION_CAMERA_Y:
	case AUTOMATION_PROJECTOR_Y:
		autogrouptype = AUTOGROUPTYPE_Y;
		break;
	case AUTOMATION_CAMERA_Z:
	case AUTOMATION_PROJECTOR_Z:
		autogrouptype = AUTOGROUPTYPE_ZOOM;
		break;
	case AUTOMATION_FADE:
		if (track->data_type == TRACK_AUDIO)
			autogrouptype = AUTOGROUPTYPE_AUDIO_FADE;
		else
			autogrouptype = AUTOGROUPTYPE_VIDEO_FADE;
		break;
	case AUTOMATION_MUTE:
		autogrouptype = AUTOGROUPTYPE_INT255;
		break;
	}
	return (autogrouptype);
}

Automation& Automation::operator=(Automation& automation)
{
	copy_from(&automation);
	return *this;
}

void Automation::equivalent_output(Automation *automation, ptstime *result)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
			autos[i]->equivalent_output(automation->autos[i], 0, result);
	}
}

void Automation::copy_from(Automation *automation)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
			autos[i]->copy_from(automation->autos[i]);
	}
}

// These must match the enumerations
static const char *xml_titles[] = 
{
	"MUTEAUTOS",
	"CAMERA_X",
	"CAMERA_Y",
	"CAMERA_Z",
	"PROJECTOR_X",
	"PROJECTOR_Y",
	"PROJECTOR_Z",
	"FADEAUTOS",
	"PANAUTOS",
	"MODEAUTOS",
	"MASKAUTOS",
	"NUDGEAUTOS"
};

int Automation::load(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(file->tag.title_is(xml_titles[i]) && autos[i])
		{
			autos[i]->load(file);
			return 1;
		}
	}
	return 0;
}

int Automation::paste(ptstime start,
	ptstime length,
	double scale,
	FileXML *file, 
	AutoConf *autoconf)
{
	if(!autoconf) autoconf = edl->session->auto_conf;

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(file->tag.title_is(xml_titles[i]) && autos[i] && autoconf->autos[i])
		{
			autos[i]->paste(start, length, scale, file);
			return 1;
		}
	}
	return 0;
}

int Automation::copy(ptstime start,
	ptstime end,
	FileXML *file)
{
// Copy regardless of what's visible.
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
		{
			file->tag.set_title(xml_titles[i]);
			file->append_tag();
			file->append_newline();
			autos[i]->copy(start, 
					end,
					file);
			char string[BCTEXTLEN];
			sprintf(string, "/%s", xml_titles[i]);
			file->tag.set_title(string);
			file->append_tag();
			file->append_newline();
		}
	}

	return 0;
}


void Automation::clear(ptstime start,
	ptstime end,
	AutoConf *autoconf, 
	int shift_autos)
{
	AutoConf *temp_autoconf = 0;

	if(!autoconf)
	{
		temp_autoconf = new AutoConf;
		temp_autoconf->set_all(1);
		autoconf = temp_autoconf;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && autoconf->autos[i])
		{
			autos[i]->clear(start, end, shift_autos);
		}
	}

	if(temp_autoconf) delete temp_autoconf;
}

void Automation::straighten(ptstime start,
	ptstime end,
	AutoConf *autoconf)
{
	AutoConf *temp_autoconf = 0;

	if(!autoconf)
	{
		temp_autoconf = new AutoConf;
		temp_autoconf->set_all(1);
		autoconf = temp_autoconf;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && autoconf->autos[i])
		{
			autos[i]->straighten(start, end);
		}
	}

	if(temp_autoconf) delete temp_autoconf;
}


void Automation::paste_silence(ptstime start, ptstime end)
{
// Unit conversion done in calling routine
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
			autos[i]->paste_silence(start, end);
	}
}

// We don't replace it in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframe.
void Automation::insert_track(Automation *automation, 
	ptstime start,
	ptstime length)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
		{
			autos[i]->insert_track(automation->autos[i], 
				start,
				length);
		}
	}
}

void Automation::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	ptstime start,
	ptstime end,
	int autogrouptype)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && edl->session->auto_conf->autos[i])
		{
			if (autos[i]->autogrouptype == autogrouptype)
				autos[i]->get_extents(min, max, coords_undefined, start, end);
		}
	}
}

void Automation::dump(int indent)
{
	printf("%*sAutomation %p dump:\n", indent, "", this);

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
		{
			printf("%*s%s:\n", indent, "", xml_titles[i]);
			autos[i]->dump(indent + 2);
		}
	}

}

// Coversions between position and ptstime
ptstime Automation::pos2pts(posnum position)
{
	if(track)
		return track->from_units(position);
	return 0;
}

posnum Automation::pts2pos(ptstime position)
{
	if(track)
		return track->to_units(position, 0);
	return 0;
}
