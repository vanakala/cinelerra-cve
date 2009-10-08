
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

#include "automation.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "filexml.h"
#include "localsession.h"


static char *xml_autogrouptypes_titlesmax[] = 
{
	"AUTOGROUPTYPE_AUDIO_FADE_MAX",
	"AUTOGROUPTYPE_VIDEO_FADE_MAX",
	"AUTOGROUPTYPE_ZOOM_MAX",
	"AUTOGROUPTYPE_X_MAX",
	"AUTOGROUPTYPE_Y_MAX",
	"AUTOGROUPTYPE_INT255_MAX"
};

static char *xml_autogrouptypes_titlesmin[] = 
{
	"AUTOGROUPTYPE_AUDIO_FADE_MIN",
	"AUTOGROUPTYPE_VIDEO_FADE_MIN",
	"AUTOGROUPTYPE_ZOOM_MIN",
	"AUTOGROUPTYPE_X_MIN",
	"AUTOGROUPTYPE_Y_MIN",
	"AUTOGROUPTYPE_INT255_MIN"
};


LocalSession::LocalSession(EDL *edl)
{
	this->edl = edl;

	selectionstart = selectionend = 0;
	in_point = out_point = -1;
	strcpy(folder, CLIP_FOLDER);
	sprintf(clip_title, "Program");
	strcpy(clip_notes, "Hello world");
	clipboard_length = 0;
	preview_start = preview_end = 0;
	loop_playback = 0;
	loop_start = 0;
	loop_end = 0;
	zoom_sample = 16384;
	zoom_y = 0;
	zoom_track = 0;
	view_start = 0;
	track_start = 0;

	automation_mins[AUTOGROUPTYPE_AUDIO_FADE] = -80;
	automation_maxs[AUTOGROUPTYPE_AUDIO_FADE] = 6;

	automation_mins[AUTOGROUPTYPE_VIDEO_FADE] = 0;
	automation_maxs[AUTOGROUPTYPE_VIDEO_FADE] = 100;

	automation_mins[AUTOGROUPTYPE_ZOOM] = 0.001;
	automation_maxs[AUTOGROUPTYPE_ZOOM] = 4;

	automation_mins[AUTOGROUPTYPE_X] = -100;
	automation_maxs[AUTOGROUPTYPE_X] = 100;

	automation_mins[AUTOGROUPTYPE_Y] = -100;
	automation_maxs[AUTOGROUPTYPE_Y] = 100;

	automation_mins[AUTOGROUPTYPE_INT255] = 0;
	automation_maxs[AUTOGROUPTYPE_INT255] = 255;

	zoombar_showautotype = AUTOGROUPTYPE_AUDIO_FADE;
	red = green = blue = 0;
}

LocalSession::~LocalSession()
{
}

void LocalSession::copy_from(LocalSession *that)
{
	strcpy(clip_title, that->clip_title);
	strcpy(clip_notes, that->clip_notes);
	strcpy(folder, that->folder);
	in_point = that->in_point;
	loop_playback = that->loop_playback;
	loop_start = that->loop_start;
	loop_end = that->loop_end;
	out_point = that->out_point;
	selectionend = that->selectionend;
	selectionstart = that->selectionstart;
	track_start = that->track_start;
	view_start = that->view_start;
	zoom_sample = that->zoom_sample;
	zoom_y = that->zoom_y;
	zoom_track = that->zoom_track;
	preview_start = that->preview_start;
	preview_end = that->preview_end;
	red = that->red;
	green = that->green;
	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		automation_mins[i] = that->automation_mins[i];
		automation_maxs[i] = that->automation_maxs[i];
	}
	blue = that->blue;
}

void LocalSession::save_xml(FileXML *file, double start)
{
	file->tag.set_title("LOCALSESSION");

	file->tag.set_property("IN_POINT", in_point - start);
	file->tag.set_property("LOOP_PLAYBACK", loop_playback);
	file->tag.set_property("LOOP_START", loop_start - start);
	file->tag.set_property("LOOP_END", loop_end - start);
	file->tag.set_property("OUT_POINT", out_point - start);
	file->tag.set_property("SELECTION_START", selectionstart - start);
	file->tag.set_property("SELECTION_END", selectionend - start);
	file->tag.set_property("CLIP_TITLE", clip_title);
	file->tag.set_property("CLIP_NOTES", clip_notes);
	file->tag.set_property("FOLDER", folder);
	file->tag.set_property("TRACK_START", track_start);
	file->tag.set_property("VIEW_START", view_start);
	file->tag.set_property("ZOOM_SAMPLE", zoom_sample);
//printf("EDLSession::save_session 1\n");
	file->tag.set_property("ZOOMY", zoom_y);
//printf("EDLSession::save_session 1 %d\n", zoom_track);
	file->tag.set_property("ZOOM_TRACK", zoom_track);
	
	double preview_start = this->preview_start - start;
	if(preview_start < 0) preview_start = 0;
	double preview_end = this->preview_end - start;
	if(preview_end < 0) preview_end = 0;
	
	file->tag.set_property("PREVIEW_START", preview_start);
	file->tag.set_property("PREVIEW_END", preview_end);
	file->tag.set_property("RED", red);
	file->tag.set_property("GREEN", green);
	file->tag.set_property("BLUE", blue);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		if (!Automation::autogrouptypes_fixedrange[i]) {
			file->tag.set_property(xml_autogrouptypes_titlesmin[i],automation_mins[i]);
			file->tag.set_property(xml_autogrouptypes_titlesmax[i],automation_maxs[i]);
		}
	}
	file->append_tag();
	file->tag.set_title("/LOCALSESSION");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void LocalSession::synchronize_params(LocalSession *that)
{
	loop_playback = that->loop_playback;
	loop_start = that->loop_start;
	loop_end = that->loop_end;
	preview_start = that->preview_start;
	preview_end = that->preview_end;
	red = that->red;
	green = that->green;
	blue = that->blue;
}


void LocalSession::load_xml(FileXML *file, unsigned long load_flags)
{
	if(load_flags & LOAD_SESSION)
	{
		clipboard_length = 0;
// Overwritten by MWindow::load_filenames	
		file->tag.get_property("CLIP_TITLE", clip_title);
		file->tag.get_property("CLIP_NOTES", clip_notes);
		file->tag.get_property("FOLDER", folder);
		loop_playback = file->tag.get_property("LOOP_PLAYBACK", 0);
		loop_start = file->tag.get_property("LOOP_START", (double)0);
		loop_end = file->tag.get_property("LOOP_END", (double)0);
		selectionstart = file->tag.get_property("SELECTION_START", (double)0);
		selectionend = file->tag.get_property("SELECTION_END", (double)0);
		track_start = file->tag.get_property("TRACK_START", track_start);
		view_start = file->tag.get_property("VIEW_START", view_start);
		zoom_sample = file->tag.get_property("ZOOM_SAMPLE", zoom_sample);
		zoom_y = file->tag.get_property("ZOOMY", zoom_y);
		zoom_track = file->tag.get_property("ZOOM_TRACK", zoom_track);
		preview_start = file->tag.get_property("PREVIEW_START", preview_start);
		preview_end = file->tag.get_property("PREVIEW_END", preview_end);
		red = file->tag.get_property("RED", red);
		green = file->tag.get_property("GREEN", green);
		blue = file->tag.get_property("BLUE", blue);

		for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
			if (!Automation::autogrouptypes_fixedrange[i]) {
				automation_mins[i] = file->tag.get_property(xml_autogrouptypes_titlesmin[i],automation_mins[i]);
				automation_maxs[i] = file->tag.get_property(xml_autogrouptypes_titlesmax[i],automation_maxs[i]);
			}
		}
	}


// on operations like cut, paste, slice, clear... we should also undo the cursor position as users
// expect - this is additionally important in keyboard-only editing in viewer window
	if(load_flags & LOAD_SESSION || load_flags & LOAD_TIMEBAR)
	{
		selectionstart = file->tag.get_property("SELECTION_START", (double)0);
		selectionend = file->tag.get_property("SELECTION_END", (double)0);
	}



	if(load_flags & LOAD_TIMEBAR)
	{
		in_point = file->tag.get_property("IN_POINT", (double)-1);
		out_point = file->tag.get_property("OUT_POINT", (double)-1);
	}
}

void LocalSession::boundaries()
{
	zoom_sample = MAX(1, zoom_sample);
}

int LocalSession::load_defaults(BC_Hash *defaults)
{
	loop_playback = defaults->get("LOOP_PLAYBACK", 0);
	loop_start = defaults->get("LOOP_START", (double)0);
	loop_end = defaults->get("LOOP_END", (double)0);
	selectionstart = defaults->get("SELECTIONSTART", selectionstart);
	selectionend = defaults->get("SELECTIONEND", selectionend);
//	track_start = defaults->get("TRACK_START", 0);
//	view_start = defaults->get("VIEW_START", 0);
	zoom_sample = defaults->get("ZOOM_SAMPLE", 16384);
	zoom_y = defaults->get("ZOOMY", 64);
	zoom_track = defaults->get("ZOOM_TRACK", 64);
	red = defaults->get("RED", 0.0);
	green = defaults->get("GREEN", 0.0);
	blue = defaults->get("BLUE", 0.0);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		if (!Automation::autogrouptypes_fixedrange[i]) {
			automation_mins[i] = defaults->get(xml_autogrouptypes_titlesmin[i], automation_mins[i]);
			automation_maxs[i] = defaults->get(xml_autogrouptypes_titlesmax[i], automation_maxs[i]);
		}
	}

	return 0;
}

int LocalSession::save_defaults(BC_Hash *defaults)
{
	defaults->update("LOOP_PLAYBACK", loop_playback);
	defaults->update("LOOP_START", loop_start);
	defaults->update("LOOP_END", loop_end);
	defaults->update("SELECTIONSTART", selectionstart);
	defaults->update("SELECTIONEND", selectionend);
	defaults->update("TRACK_START", track_start);
	defaults->update("VIEW_START", view_start);
	defaults->update("ZOOM_SAMPLE", zoom_sample);
	defaults->update("ZOOMY", zoom_y);
	defaults->update("ZOOM_TRACK", zoom_track);
	defaults->update("RED", red);
	defaults->update("GREEN", green);
	defaults->update("BLUE", blue);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		if (!Automation::autogrouptypes_fixedrange[i]) {
			defaults->update(xml_autogrouptypes_titlesmin[i], automation_mins[i]);
			defaults->update(xml_autogrouptypes_titlesmax[i], automation_maxs[i]);
		}
	}

	return 0;
}

void LocalSession::set_selectionstart(double value)
{
	this->selectionstart = value;
}

void LocalSession::set_selectionend(double value)
{
	this->selectionend = value;
}

void LocalSession::set_inpoint(double value)
{
	in_point = value;
}

void LocalSession::set_outpoint(double value)
{
	out_point = value;
}

void LocalSession::unset_inpoint()
{
	in_point = -1;
}

void LocalSession::unset_outpoint()
{
	out_point = -1;
}



double LocalSession::get_selectionstart(int highlight_only)
{
	if(highlight_only || !EQUIV(selectionstart, selectionend))
		return selectionstart;

	if(in_point >= 0)
		return in_point;
	else
	if(out_point >= 0)
		return out_point;
	else
		return selectionstart;
}

double LocalSession::get_selectionend(int highlight_only)
{
	if(highlight_only || !EQUIV(selectionstart, selectionend))
		return selectionend;

	if(out_point >= 0)
		return out_point;
	else
	if(in_point >= 0)
		return in_point;
	else
		return selectionend;
}

double LocalSession::get_inpoint()
{
	return in_point;
}

double LocalSession::get_outpoint()
{
	return out_point;
}

int LocalSession::inpoint_valid()
{
	return in_point >= 0;
}

int LocalSession::outpoint_valid()
{
	return out_point >= 0;
}




