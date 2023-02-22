// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "automation.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "bchash.h"
#include "clip.h"
#include "colorspaces.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "localsession.h"
#include "zoompanel.h"

#include <inttypes.h>

LocalSession::LocalSession(EDL *edl)
{
	this->edl = edl;

	selectionstart = selectionend = 0;
	in_point = out_point = -1;
	awindow_folder = AW_CLIP_FOLDER;
	clip_title[0] = 0;
	clip_notes[0] = 0;
	preview_start = preview_end = 0;
	loop_playback = 0;
	loop_start = 0;
	loop_end = 0;
	zoom_time = 0;
	zoom_y = 0;
	zoom_track = 0;
	view_start_pts = 0;
	track_start = 0;

	automation_mins[AUTOGROUPTYPE_AUDIO_FADE] = -40;
	automation_maxs[AUTOGROUPTYPE_AUDIO_FADE] = 6;

	automation_mins[AUTOGROUPTYPE_VIDEO_FADE] = 0;
	automation_maxs[AUTOGROUPTYPE_VIDEO_FADE] = 100;

	automation_mins[AUTOGROUPTYPE_ZOOM] = 0.001;
	automation_maxs[AUTOGROUPTYPE_ZOOM] = 4;

	automation_mins[AUTOGROUPTYPE_X] = -1;
	automation_maxs[AUTOGROUPTYPE_X] = 1;

	automation_mins[AUTOGROUPTYPE_Y] = -1;
	automation_maxs[AUTOGROUPTYPE_Y] = 1;

	automation_mins[AUTOGROUPTYPE_INT255] = 0;
	automation_maxs[AUTOGROUPTYPE_INT255] = 255;

	zoombar_showautotype = AUTOGROUPTYPE_AUDIO_FADE;
	picker_red = picker_green = picker_blue = 0;
	picker_y = picker_u = picker_v = 0;
}

void LocalSession::reset_instance()
{
	clip_title[0] = 0;
	clip_notes[0] = 0;
	selectionstart = selectionend = 0;
	preview_start = preview_end = 0;
	in_point = out_point = -1;
	view_start_pts = 0;
	track_start = 0;
}

void LocalSession::copy_from(LocalSession *that)
{
	strcpy(clip_title, that->clip_title);
	strcpy(clip_notes, that->clip_notes);
	awindow_folder = that->awindow_folder;
	in_point = that->in_point;
	loop_playback = that->loop_playback;
	loop_start = that->loop_start;
	loop_end = that->loop_end;
	out_point = that->out_point;
	selectionend = that->selectionend;
	selectionstart = that->selectionstart;
	track_start = that->track_start;
	zoom_time = that->zoom_time;
	view_start_pts = that->view_start_pts;
	zoom_y = that->zoom_y;
	zoom_track = that->zoom_track;
	preview_start = that->preview_start;
	preview_end = that->preview_end;
	picker_red = that->picker_red;
	picker_green = that->picker_green;
	picker_blue = that->picker_blue;
	picker_y = that->picker_y;
	picker_u = that->picker_u;
	picker_v = that->picker_v;
	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		automation_mins[i] = that->automation_mins[i];
		automation_maxs[i] = that->automation_maxs[i];
	}
}

void LocalSession::copy(LocalSession *that, ptstime start, ptstime end)
{
	in_point = that->in_point - start;
	loop_playback = that->loop_playback;
	loop_start = that->loop_start - start;
	loop_end = that->loop_end - start;
	out_point = that->out_point - start;
	selectionstart = that->selectionstart - start;
	selectionend = that->selectionend - start;
	strcpy(clip_title, that->clip_title);
	strcpy(clip_notes, that->clip_notes);
	awindow_folder = that->awindow_folder;
	track_start = that->track_start;
	view_start_pts = that->view_start_pts;
	zoom_time = that->zoom_time;
	zoom_track = that->zoom_track;
	preview_start = that->preview_start - start;
	if(preview_start < 0)
		preview_start = 0;
	preview_end = this->preview_end - start;
	if(preview_end  < 0)
		preview_end = 0;
	picker_red = that->picker_red;
	picker_green = that->picker_green;
	picker_blue = that->picker_blue;
	picker_y = that->picker_y;
	picker_u = that->picker_u;
	picker_v = that->picker_v;
	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
	{
		automation_mins[i] = that->automation_mins[i];
		automation_maxs[i] = that->automation_maxs[i];
	}
}

void LocalSession::save_xml(FileXML *file)
{
	file->tag.set_title("LOCALSESSION");

	file->tag.set_property("IN_POINT", in_point);
	file->tag.set_property("LOOP_PLAYBACK", loop_playback);
	file->tag.set_property("LOOP_START", loop_start);
	file->tag.set_property("LOOP_END", loop_end);
	file->tag.set_property("OUT_POINT", out_point);
	file->tag.set_property("SELECTION_START", selectionstart);
	file->tag.set_property("SELECTION_END", selectionend);
	file->tag.set_property("CLIP_TITLE", clip_title);
	file->tag.set_property("CLIP_NOTES", clip_notes);
	file->tag.set_property("AWINDOW_FOLDER", awindow_folder);
	file->tag.set_property("TRACK_START", track_start);
	file->tag.set_property("VIEW_START_PTS", view_start_pts);
	file->tag.set_property("ZOOM_TIME", zoom_time);
	file->tag.set_property("ZOOMY", zoom_y);
	file->tag.set_property("ZOOM_TRACK", zoom_track);

	ptstime preview_start = this->preview_start;
	if(preview_start < 0) preview_start = 0;
	ptstime preview_end = this->preview_end;
	if(preview_end < 0) preview_end = 0;

	file->tag.set_property("PREVIEW_START", preview_start);
	file->tag.set_property("PREVIEW_END", preview_end);
	file->tag.set_property("RED", picker_red);
	file->tag.set_property("GREEN", picker_green);
	file->tag.set_property("BLUE", picker_blue);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
	{
		if(!Automation::autogrouptypes[i].fixedrange)
		{
			file->tag.set_property(Automation::autogrouptypes[i].titlemin,
				automation_mins[i]);
			file->tag.set_property(Automation::autogrouptypes[i].titlemax,
				automation_maxs[i]);
		}
	}
	file->append_tag();
	file->tag.set_title("/LOCALSESSION");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void LocalSession::load_xml(FileXML *file)
{
	double red, green, blue;

	file->tag.get_property("CLIP_TITLE", clip_title);
	file->tag.get_property("CLIP_NOTES", clip_notes);
	char *string = file->tag.get_property("FOLDER");
	if(string)
		awindow_folder = AWindowGUI::folder_number(string);
	awindow_folder = file->tag.get_property("AWINDOW_FOLDER", awindow_folder);
	loop_playback = file->tag.get_property("LOOP_PLAYBACK", 0);
	loop_start = file->tag.get_property("LOOP_START", (ptstime)0);
	loop_end = file->tag.get_property("LOOP_END", (ptstime)0);
	selectionstart = file->tag.get_property("SELECTION_START", (ptstime)0);
	selectionend = file->tag.get_property("SELECTION_END", (ptstime)0);
	track_start = file->tag.get_property("TRACK_START", track_start);
	int64_t zoom_sample = file->tag.get_property("ZOOM_SAMPLE", (int64_t)0);
	if(zoom_sample)
		zoom_time = (ptstime)zoom_sample / edlsession->sample_rate;
	zoom_time = file->tag.get_property("ZOOM_TIME", zoom_time);
	zoom_time = ZoomPanel::adjust_zoom(zoom_time, MIN_ZOOM_TIME, MAX_ZOOM_TIME);
	int64_t view_start = file->tag.get_property("VIEW_START", (int64_t)0);
	if(view_start)
		view_start_pts = (ptstime)view_start * zoom_time;
	view_start_pts = file->tag.get_property("VIEW_START_PTS", view_start_pts);
	zoom_y = file->tag.get_property("ZOOMY", zoom_y);
	zoom_track = file->tag.get_property("ZOOM_TRACK", zoom_track);
	preview_start = file->tag.get_property("PREVIEW_START", preview_start);
	preview_end = file->tag.get_property("PREVIEW_END", preview_end);

	// Compatibility: old values were 0 .. 1.0
	red = file->tag.get_property("RED", 0.0);
	green = file->tag.get_property("GREEN", 0.0);
	blue = file->tag.get_property("BLUE", 0.0);
	if(red > 0 && red < 1.0)
		picker_red = red * 0x10000;
	else
		picker_red = red;
	if(green > 0 && green < 1.0)
		picker_green = green * 0x10000;
	else
		picker_green = green;
	if(blue > 0 && blue < 1.0)
		picker_blue = blue * 0x10000;
	else
		picker_blue = blue;
	picker_to_yuv();

	for(int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
	{
		if(!Automation::autogrouptypes[i].fixedrange)
		{
			automation_mins[i] = file->tag.get_property(
				Automation::autogrouptypes[i].titlemin,
				automation_mins[i]);
			automation_maxs[i] = file->tag.get_property(
				Automation::autogrouptypes[i].titlemax,
				automation_maxs[i]);
		}
	}

// on operations like cut, paste, slice, clear... we should also undo the cursor position as users
// expect - this is additionally important in keyboard-only editing in viewer window
	selectionstart = file->tag.get_property("SELECTION_START", (ptstime)0);
	selectionend = file->tag.get_property("SELECTION_END", (ptstime)0);

	in_point = file->tag.get_property("IN_POINT", (ptstime)-1);
	out_point = file->tag.get_property("OUT_POINT", (ptstime)-1);
}

void LocalSession::set_clip_title(const char *path)
{
	char *name, *dot;
	char str[BCTEXTLEN];

	if(!path || path[0] == 0)
	{
		clip_title[0] = 0;
		return;
	}

	strncpy(str, path, BCTEXTLEN);
	str[BCTEXTLEN - 1] = 0;

	name = basename(str);
	if(!name)
		name = str;
	if(dot = strrchr(name, '.'))
		*dot = 0;
	strcpy(clip_title, name);
}

void LocalSession::boundaries()
{
	zoom_time = CLIP(zoom_time, MIN_ZOOM_TIME, MAX_ZOOM_TIME);
}

void LocalSession::load_defaults(BC_Hash *defaults)
{
	double red, green, blue;

	loop_playback = defaults->get("LOOP_PLAYBACK", 0);
	loop_start = defaults->get("LOOP_START", (ptstime)0);
	loop_end = defaults->get("LOOP_END", (ptstime)0);
// For backwards compatibility
	int64_t zoom_sample = defaults->get("ZOOM_SAMPLE", (int64_t)0);
	if(zoom_sample)
		zoom_time = (ptstime)zoom_sample / edlsession->sample_rate;
	else
		zoom_time = DEFAULT_ZOOM_TIME;
	zoom_time = defaults->get("ZOOM_TIME", zoom_time);
	zoom_y = defaults->get("ZOOMY", 64);
	zoom_track = defaults->get("ZOOM_TRACK", 64);
	red = defaults->get("RED", 0.0);
	green = defaults->get("GREEN", 0.0);
	blue = defaults->get("BLUE", 0.0);
	if(red > 0)
		picker_red = red * 0x10000;
	if(green > 0)
		picker_green = green * 0x10000;
	if(blue > 0)
		picker_blue = blue * 0x10000;
	picker_red = defaults->get("PICKER_RED", picker_red);
	picker_green = defaults->get("PICKER_GREEN", picker_green);
	picker_blue = defaults->get("PICKER_BLUE", picker_blue);
	picker_to_yuv();

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
	{
		if(!Automation::autogrouptypes[i].fixedrange)
		{
			automation_mins[i] = defaults->get(
				Automation::autogrouptypes[i].titlemin,
				automation_mins[i]);
			automation_maxs[i] = defaults->get(
				Automation::autogrouptypes[i].titlemax,
				automation_maxs[i]);
		}
	}
}

void LocalSession::save_defaults(BC_Hash *defaults)
{
	defaults->update("LOOP_PLAYBACK", loop_playback);
	defaults->update("LOOP_START", loop_start);
	defaults->update("LOOP_END", loop_end);
	defaults->delete_key("SELECTIONSTART");
	defaults->delete_key("SELECTIONEND");
	defaults->update("TRACK_START", track_start);
	defaults->delete_key("VIEW_START");
	defaults->update("VIEW_START_PTS", view_start_pts);
	defaults->update("ZOOM_TIME", zoom_time);
	defaults->delete_key("ZOOM_SAMPLE");
	defaults->update("ZOOMY", zoom_y);
	defaults->update("ZOOM_TRACK", zoom_track);
	defaults->delete_key("RED");
	defaults->delete_key("GREEN");
	defaults->delete_key("BLUE");
	defaults->update("PICKER_RED", picker_red);
	defaults->update("PICKER_GREEN", picker_green);
	defaults->update("PICKER_BLUE", picker_blue);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
	{
		if(!Automation::autogrouptypes[i].fixedrange)
		{
			defaults->update(Automation::autogrouptypes[i].titlemin,
				automation_mins[i]);
			defaults->update(Automation::autogrouptypes[i].titlemax,
				automation_maxs[i]);
		}
	}
}

void LocalSession::set_selectionstart(ptstime value)
{
	this->selectionstart = value;
}

void LocalSession::set_selectionend(ptstime value)
{
	this->selectionend = value;
}

void LocalSession::set_selection(ptstime start, ptstime end)
{
	selectionstart = start;
	selectionend = end;
}

void LocalSession::set_selection(ptstime pos)
{
	selectionstart = pos;
	selectionend = pos;
}

void LocalSession::set_inpoint(ptstime value)
{
	in_point = value;
}

void LocalSession::set_outpoint(ptstime value)
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

void LocalSession::get_selections(ptstime *values)
{
	values[0] = selectionstart;
	values[1] = selectionend;
	values[2] = in_point;
	values[3] = out_point;
}

ptstime LocalSession::get_selectionstart(int highlight_only)
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

ptstime LocalSession::get_selectionend(int highlight_only)
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

ptstime LocalSession::get_inpoint()
{
	return in_point;
}

ptstime LocalSession::get_outpoint()
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

void LocalSession::get_picker_rgb(int *r, int *g, int *b)
{
	if(r)
		*r = picker_red;
	if(g)
		*g = picker_green;
	if(b)
		*b = picker_blue;
}

void LocalSession::get_picker_yuv(int *y, int *u, int *v)
{
	if(y)
		*y = picker_y;
	if(u)
		*u = picker_u;
	if(v)
		*v = picker_v;
}

void LocalSession::get_picker_rgb(double *r, double *g, double *b)
{
	if(r)
		*r = (double)picker_red / 0x10000;
	if(g)
		*g = (double)picker_green / 0x10000;
	if(b)
		*b = (double)picker_blue / 0x10000;
}

void LocalSession::set_picker_yuv(int y, int u, int v)
{
	picker_y = y;
	picker_u = u;
	picker_v = v;
	picker_to_rgb();
}

void LocalSession::set_picker_rgb(int r, int g, int b)
{
	picker_red = r;
	picker_green = g;
	picker_blue = b;
	picker_to_yuv();
}

void LocalSession::set_picker_rgb(double r, double g, double b)
{
	double val;

	val = round(r * 0x10000);
	picker_red = CLIP(val, 0, 0xffff);
	val = round(g * 0x10000);
	picker_green =  CLIP(val, 0, 0xffff);
	val = round(b * 0x10000);
	picker_blue = CLIP(val, 0, 0xffff);
	picker_to_yuv();
}

void LocalSession::picker_to_yuv()
{
	ColorSpaces::rgb_to_yuv_16(picker_red, picker_green, picker_blue,
		picker_y, picker_u, picker_v);
}

void LocalSession::picker_to_rgb()
{
	ColorSpaces::yuv_to_rgb_16(picker_red, picker_green, picker_blue,
		picker_y, picker_u, picker_v);
}

size_t LocalSession::get_size()
{
	return sizeof(*this);
}

void LocalSession::dump(int indent)
{
	printf("%*sLocalsession %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*sselections %.3f..%.3f, points in %.3f out %.3f\n", indent, "",
		selectionstart, selectionend, in_point, out_point);
	printf("%*strack start %d view start %.3f preview %.3f..%.3f\n", indent, "",
		track_start, view_start_pts, preview_start, preview_end);
	printf("%*sloop_playback %d %.3f..%.3f\n", indent, "",
		loop_playback, loop_start, loop_end);
	printf("%*szoom_time %.3f zoom_y %d zoom_track %d showautotype %d\n",  indent, "",
		zoom_time, zoom_y, zoom_track, zoombar_showautotype);
	printf("%*spicker rgb:[%#04x %#04x %#04x] yuv:[%#04x %#04x %#04x]\n", indent, "",
		picker_red, picker_green, picker_blue, picker_y, picker_u, picker_v);
	printf("%*sfolder '%s(%d)'\n",  indent, "", AWindowGUI::folder_names[awindow_folder], awindow_folder);
	printf("%*sclip title '%s'\n",  indent, "", clip_title);
	printf("%*sclip notes '%s'\n",  indent, "", clip_notes);
	printf("%*sAutomation mins:",  indent, "");
	for(int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
		printf(" %.3f", automation_mins[i]);
	printf("\n%*sAutomation maxs:",  indent, "");
	for(int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
		printf(" %.3f", automation_maxs[i]);
	putchar('\n');
}
