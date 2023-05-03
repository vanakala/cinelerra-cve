// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "atrack.inc"
#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "bcsignals.h"
#include "colors.h"
#include "cropautos.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatautos.h"
#include "intautos.h"
#include "language.h"
#include "mainsession.inc"
#include "maskautos.h"
#include "panautos.h"
#include "overlayframe.inc"
#include "track.h"

#include <string.h>

struct autogrouptype_def Automation::autogrouptypes[] =
{
	{ "AUTOGROUPTYPE_AUDIO_FADE_MAX", "AUTOGROUPTYPE_AUDIO_FADE_MIN", 0,},
	{ "AUTOGROUPTYPE_VIDEO_FADE_MAX", "AUTOGROUPTYPE_VIDEO_FADE_MIN", 0 },
	{ "AUTOGROUPTYPE_ZOOM_MAX", "AUTOGROUPTYPE_ZOOM_MIN", 0 },
	{ "AUTOGROUPTYPE_X_MAX", "AUTOGROUPTYPE_X_MIN", 1 },
	{ "AUTOGROUPTYPE_Y_MAX", "AUTOGROUPTYPE_Y_MIN", 1 },
	{ "AUTOGROUPTYPE_INT255_MAX", "AUTOGROUPTYPE_INT255_MIN", 1 }
};

struct automation_def Automation::automation_tbl[] =
{
	{ N_("Mute"), "MUTEAUTOS", "SHOW_MUTE",
		0, AUTOGROUPTYPE_INT255, AUTOMATION_TYPE_INT,
		BLUE, DRAG_MUTE, DRAG_MUTE, SUPPORTS_AUDIO | SUPPORTS_VIDEO, 0 },
	{ N_("Camera X"), "CAMERA_X", "SHOW_CAMERA_X",
		0, AUTOGROUPTYPE_X, AUTOMATION_TYPE_FLOAT,
		RED, DRAG_CAMERA_X, DRAG_CAMERA_X, SUPPORTS_VIDEO, 0 },
	{ N_("Camera Y"), "CAMERA_Y", "SHOW_CAMERA_Y",
		0, AUTOGROUPTYPE_Y, AUTOMATION_TYPE_FLOAT,
		GREEN, DRAG_CAMERA_Y, DRAG_CAMERA_Y, SUPPORTS_VIDEO, 0 },
	{ N_("Camera Z"), "CAMERA_Z", "SHOW_CAMERA_Z",
		0, AUTOGROUPTYPE_ZOOM, AUTOMATION_TYPE_FLOAT,
		BLUE, DRAG_CAMERA_Z, DRAG_CAMERA_Z, SUPPORTS_VIDEO, 1 },
	{ N_("Projector X"), "PROJECTOR_X", "SHOW_PROJECTOR_X",
		0, AUTOGROUPTYPE_X, AUTOMATION_TYPE_FLOAT,
		RED, DRAG_PROJECTOR_X, DRAG_PROJECTOR_X , SUPPORTS_VIDEO, 0 },
	{ N_("Projector Y"), "PROJECTOR_Y", "SHOW_PROJECTOR_Y",
		0, AUTOGROUPTYPE_Y, AUTOMATION_TYPE_FLOAT,
		GREEN, DRAG_PROJECTOR_Y, DRAG_PROJECTOR_Y, SUPPORTS_VIDEO, 0 },
	{ N_("Projector Z"), "PROJECTOR_Z", "SHOW_PROJECTOR_Z",
		0, AUTOGROUPTYPE_ZOOM, AUTOMATION_TYPE_FLOAT,
		BLUE, DRAG_PROJECTOR_Z, DRAG_PROJECTOR_Z, SUPPORTS_VIDEO, 1 },
	{ N_("Fade"), "FADEAUTOS", "SHOW_FADE",
		1, AUTOGROUPTYPE_AUDIO_FADE, AUTOMATION_TYPE_FLOAT,
		WHITE, DRAG_FADE, DRAG_FADE, SUPPORTS_AUDIO, 0 },
	{ N_("Fade"), "FADEAUTOS", "SHOW_FADE",
		1, AUTOGROUPTYPE_VIDEO_FADE, AUTOMATION_TYPE_FLOAT,
		WHITE, DRAG_FADE, DRAG_FADE, SUPPORTS_VIDEO, 100 },
	{ N_("Pan"), "PANAUTOS", "SHOW_PAN",
		0, -1, AUTOMATION_TYPE_PAN,
		0, DRAG_PAN_PRE, DRAG_PAN, SUPPORTS_AUDIO },
	{ N_("Mode"), "MODEAUTOS", "SHOW_MODE",
		0, -1, AUTOMATION_TYPE_INT,
		0, DRAG_MODE, DRAG_MODE_PRE, SUPPORTS_VIDEO, TRANSFER_NORMAL },
	{ N_("Mask"), "MASKAUTOS", "SHOW_MASK",
		0, -1, AUTOMATION_TYPE_MASK,
		0, DRAG_MASK_PRE, DRAG_MASK, SUPPORTS_VIDEO, 0 },
	{ N_("Crop"), "CROPAUTOS", "SHOW_CROP",
		0, -1, AUTOMATION_TYPE_CROP,
		0, DRAG_CROP_PRE, DRAG_CROP, SUPPORTS_VIDEO, 0 }
};

Automation::Automation(EDL *edl, Track *track)
{
	this->edl = edl;
	this->track = track;
	memset(autos, 0, sizeof(Autos*) * AUTOMATION_TOTAL);
}

Automation::~Automation()
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		delete autos[i];
	}
}

Autos *Automation::have_autos(int autoidx)
{
	return autos[autoidx];
}

int Automation::total_autos(int autoidx)
{
	if(autos[autoidx])
		return autos[autoidx]->total();
	return 0;
}

Auto *Automation::get_prev_auto(Auto *current, int autoidx)
{
	if(autos[autoidx])
		return autos[autoidx]->get_prev_auto(current);
	return 0;
}

Auto *Automation::get_prev_auto(ptstime pts, int autoidx)
{
	if(autos[autoidx])
		return autos[autoidx]->get_prev_auto(pts);
	return 0;
}

Autos *Automation::get_autos(int autoidx)
{
	if(autoidx < 0 || autoidx >= AUTOMATION_TOTAL)
		return 0;

	if(!autos[autoidx])
	{
		switch(automation_tbl[autoidx].type)
		{
		case AUTOMATION_TYPE_FLOAT:
			autos[autoidx] = new FloatAutos(edl, track,
				automation_tbl[autoidx].default_value);
			break;
		case AUTOMATION_TYPE_MASK:
			autos[autoidx] = new MaskAutos(edl, track);
			break;
		case AUTOMATION_TYPE_INT:
			autos[autoidx] = new IntAutos(edl, track,
				automation_tbl[autoidx].default_value);
			break;
		case AUTOMATION_TYPE_PAN:
			autos[autoidx] = new PanAutos(edl, track);
			break;
		case AUTOMATION_TYPE_CROP:
			autos[autoidx] = new CropAutos(edl, track);
			break;
		default:
			return 0;
		}
		autos[autoidx]->autoidx = autoidx;
		autos[autoidx]->autogrouptype = automation_tbl[autoidx].autogrouptype;
	}
	return autos[autoidx];
}

Auto *Automation::get_auto_for_editing(ptstime pos, int autoidx)
{
	Autos *autos = get_autos(autoidx);

	if(autos)
		return autos->get_auto_for_editing(pos);
	return 0;
}

int Automation::floatvalue_is_constant(ptstime start, ptstime length, int autoidx,
	double *constant)
{
	if(autos[autoidx])
		return ((FloatAutos*)autos[autoidx])->automation_is_constant(start, length, constant);

	*constant = (double)automation_tbl[autoidx].default_value;
	return 1;
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
		if(file->tag.title_is(automation_tbl[i].xml_title))
		{
			if(operation == PASTE_AUTOS && !edlsession->auto_conf->auto_visible[i])
				return 0;

			switch(i)
			{
			case AUTOMATION_CAMERA_X:
			case AUTOMATION_PROJECTOR_X:
				get_autos(i)->set_compat_value(track->track_w);
				break;
			case AUTOMATION_CAMERA_Y:
			case AUTOMATION_PROJECTOR_Y:
				get_autos(i)->set_compat_value(track->track_h);
				break;
			}

			get_autos(i)->load(file);
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

void Automation::get_projector(double *x, double *y, double *z, ptstime position)
{
	if(autos[AUTOMATION_PROJECTOR_X])
		*x = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_X])->get_value(position);
	else
		*x = (double)(automation_tbl[AUTOMATION_PROJECTOR_X].default_value * track->track_w);

	if(autos[AUTOMATION_PROJECTOR_Y])
		*y = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Y])->get_value(position);
	else
		*y = (double)(automation_tbl[AUTOMATION_PROJECTOR_Y].default_value * track->track_h);

	if(autos[AUTOMATION_PROJECTOR_Z])
		*z = ((FloatAutos*)autos[AUTOMATION_PROJECTOR_Z])->get_value(position);
	else
		*z = (double)automation_tbl[AUTOMATION_PROJECTOR_Z].default_value;
}

void Automation::get_camera(double *x, double *y, double *z, ptstime position)
{
	if(autos[AUTOMATION_CAMERA_X])
		*x = ((FloatAutos*)autos[AUTOMATION_CAMERA_X])->get_value(position);
	else
		*x = (double)(automation_tbl[AUTOMATION_CAMERA_X].default_value * track->track_w);

	if(autos[AUTOMATION_CAMERA_Y])
		*y = ((FloatAutos*)autos[AUTOMATION_CAMERA_Y])->get_value(position);
	else
		*y = (double)(automation_tbl[AUTOMATION_CAMERA_Y].default_value * track->track_h);

	if(autos[AUTOMATION_CAMERA_Z])
		*z = ((FloatAutos*)autos[AUTOMATION_CAMERA_Z])->get_value(position);
	else
		*z = (double)automation_tbl[AUTOMATION_CAMERA_Z].default_value;
}

int Automation::get_intvalue(ptstime pos, int autoidx)
{
	if(autos[autoidx])
		return ((IntAutos*)autos[autoidx])->get_value(pos);
	return automation_tbl[autoidx].default_value;
}

double Automation::get_floatvalue(ptstime pos, int autoidx, FloatAuto *prev, FloatAuto *next)
{
	if(autos[autoidx])
		return ((FloatAutos*)autos[autoidx])->get_raw_value(pos, prev, next);
	return (double)automation_tbl[autoidx].default_value;
}

int Automation::auto_exists_for_editing(ptstime pos, int autoidx)
{
	if(autos[autoidx])
		return autos[autoidx]->auto_exists_for_editing(pos);
	return 0;
}

size_t Automation::get_size()
{
	size_t size = sizeof(*this);

	if(autos[AUTOMATION_MUTE])
		size += ((IntAutos*)autos[AUTOMATION_MUTE])->get_size();
	if(autos[AUTOMATION_AFADE])
		size += ((FloatAutos*)autos[AUTOMATION_AFADE])->get_size();
	if(autos[AUTOMATION_VFADE])
		size += ((FloatAutos*)autos[AUTOMATION_VFADE])->get_size();
	if(autos[AUTOMATION_PAN])
		size += ((PanAutos*)autos[AUTOMATION_PAN])->get_size();
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
