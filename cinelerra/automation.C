// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "atrack.inc"
#include "bcsignals.h"
#include "colors.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "language.h"
#include "mainsession.inc"
#include "intautos.h"
#include "track.h"

#include <string.h>

struct autogrouptype_def Automation::autogrouptypes[] =
{
	{ "AUTOGROUPTYPE_AUDIO_FADE_MAX", "AUTOGROUPTYPE_AUDIO_FADE_MIN", 0,},
	{ "AUTOGROUPTYPE_VIDEO_FADE_MAX", "AUTOGROUPTYPE_VIDEO_FADE_MIN", 0 },
	{ "AUTOGROUPTYPE_ZOOM_MAX", "AUTOGROUPTYPE_ZOOM_MIN", 0 },
	{ "AUTOGROUPTYPE_X_MAX", "AUTOGROUPTYPE_X_MIN", 0 },
	{ "AUTOGROUPTYPE_Y_MAX", "AUTOGROUPTYPE_Y_MIN", 0 },
	{ "AUTOGROUPTYPE_INT255_MAX", "AUTOGROUPTYPE_INT255_MIN", 1 }
};

struct automation_def Automation::automation_tbl[] =
{
	{ N_("Mute"), "MUTEAUTOS", "SHOW_MUTE",
		0, AUTOGROUPTYPE_INT255, AUTOMATION_TYPE_INT,
		BLUE, DRAG_MUTE, DRAG_MUTE },
	{ N_("Camera X"), "CAMERA_X", "SHOW_CAMERA_X",
		0, AUTOGROUPTYPE_X, AUTOMATION_TYPE_FLOAT,
		RED, DRAG_CAMERA_X, DRAG_CAMERA_X },
	{ N_("Camera Y"), "CAMERA_Y", "SHOW_CAMERA_Y",
		0, AUTOGROUPTYPE_Y, AUTOMATION_TYPE_FLOAT,
		GREEN, DRAG_CAMERA_Y, DRAG_CAMERA_Y },
	{ N_("Camera Z"), "CAMERA_Z", "SHOW_CAMERA_Z",
		0, AUTOGROUPTYPE_ZOOM, AUTOMATION_TYPE_FLOAT,
		BLUE, DRAG_CAMERA_Z, DRAG_CAMERA_Z },
	{ N_("Projector X"), "PROJECTOR_X", "SHOW_PROJECTOR_X",
		0, AUTOGROUPTYPE_X, AUTOMATION_TYPE_FLOAT,
		RED, DRAG_PROJECTOR_X, DRAG_PROJECTOR_X },
	{ N_("Projector Y"), "PROJECTOR_Y", "SHOW_PROJECTOR_Y",
		0, AUTOGROUPTYPE_Y, AUTOMATION_TYPE_FLOAT,
		GREEN, DRAG_PROJECTOR_Y, DRAG_PROJECTOR_Y },
	{ N_("Projector Z"), "PROJECTOR_Z", "SHOW_PROJECTOR_Z",
		0, AUTOGROUPTYPE_ZOOM, AUTOMATION_TYPE_FLOAT,
		BLUE, DRAG_PROJECTOR_Z, DRAG_PROJECTOR_Z },
	{ N_("Fade"), "FADEAUTOS", "SHOW_FADE",
		1, AUTOGROUPTYPE_AUDIO_FADE, AUTOMATION_TYPE_FLOAT,
		WHITE, DRAG_FADE, DRAG_FADE },
	{ N_("Pan"), "PANAUTOS", "SHOW_PAN",
		0, -1, AUTOMATION_PAN,
		0, DRAG_PAN_PRE, DRAG_PAN },
	{ N_("Mode"), "MODEAUTOS", "SHOW_MODE",
		0, -1, AUTOMATION_TYPE_INT,
		0, DRAG_MODE, DRAG_MODE_PRE },
	{ N_("Mask"), "MASKAUTOS", "SHOW_MASK",
		0, -1, AUTOMATION_TYPE_MASK,
		0, DRAG_MASK_PRE, DRAG_MASK },
	{ N_("Crop"), "CROPAUTOS", "SHOW_CROP",
		0, -1, AUTOMATION_TYPE_CROP,
		0, DRAG_CROP, DRAG_CROP }
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
	int autogrouptype;

	if(autoidx < AUTOMATION_TOTAL)
	{
		autogrouptype = automation_tbl[autoidx].autogrouptype;
		if(autogrouptype == AUTOGROUPTYPE_AUDIO_FADE)
		{
			switch(track->data_type)
			{
			case TRACK_AUDIO:
				break;
			case TRACK_VIDEO:
				autogrouptype = AUTOGROUPTYPE_VIDEO_FADE;
				break;
			}
		}
		return autogrouptype;
	}
	return -1;
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

int Automation::load(FileXML *file, int operation)
{
	if(operation != PASTE_ALL && operation != PASTE_AUTOS)
		return 0;

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(file->tag.title_is(automation_tbl[i].xml_title) && autos[i])
		{
			if(operation == PASTE_AUTOS && !edlsession->auto_conf->auto_visible[i])
				return 0;

			autos[i]->load(file);
			return 1;
		}
	}
	return 0;
}

void Automation::save_xml(FileXML *file)
{
// Copy regardless of what's visible.
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && autos[i]->total())
		{
			file->tag.set_title(automation_tbl[i].xml_title);
			file->append_tag();
			file->append_newline();
			autos[i]->save_xml(file);
			char string[BCTEXTLEN];
			sprintf(string, "/%s", automation_tbl[i].xml_title);
			file->tag.set_title(string);
			file->append_tag();
			file->append_newline();
		}
	}
}

void Automation::copy(Automation *automation, ptstime start, ptstime end)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(automation->autos[i] && automation->autos[i]->total())
			autos[i]->copy(automation->autos[i], start, end);
	}
}

void Automation::clear(ptstime start,
	ptstime end,
	AutoConf *autoconf, 
	int shift_autos)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && (!autoconf || autoconf->auto_visible[i]))
			autos[i]->clear(start, end, shift_autos);
	}
}

void Automation::clear_after(ptstime pts)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
			autos[i]->clear_after(pts);
	}
}

void Automation::straighten(ptstime start,
	ptstime end)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && edlsession->auto_conf->auto_visible[i])
			autos[i]->straighten(start, end);
	}
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

void Automation::insert_track(Automation *automation, 
	ptstime start,
	ptstime length,
	int overwrite)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
		{
			autos[i]->insert_track(automation->autos[i], 
				start, length, overwrite);
		}
	}
}

void Automation::get_extents(double *min,
	double *max,
	int *coords_undefined,
	ptstime start,
	ptstime end,
	int autogrouptype)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && edlsession->auto_conf->auto_visible[i])
		{
			if (autos[i]->autogrouptype == autogrouptype)
				autos[i]->get_extents(min, max, coords_undefined, start, end);
		}
	}
}

ptstime Automation::get_length()
{
	ptstime last_pts = 0;
	ptstime cur_pts;

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && autos[i]->last)
		{
			cur_pts = autos[i]->last->pos_time;
			if(cur_pts > last_pts)
				last_pts = cur_pts;
		}
	}
	return last_pts;
}

size_t Automation::get_size()
{
	size_t size = 0;

	if(autos[AUTOMATION_MUTE])
		size += ((IntAutos*)autos[AUTOMATION_MUTE])->get_size();
	return size;
}

const char *Automation::name(int index)
{
	if(index >= 0 && index < AUTOMATION_TOTAL)
		return automation_tbl[index].name;
	return 0;
}

int Automation::index(const char *name)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		if(!strcmp(automation_tbl[i].name, name))
			return i;
	return -1;
}

void Automation::dump(int indent)
{
	printf("%*sAutomation %p dump:\n", indent, "", this);

	indent += 2;
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
		{
			printf("%*s%s:\n", indent, "", automation_tbl[i].name);
			autos[i]->dump(indent + 2);
		}
	}
}
