#include "clip.h"
#include "defaults.h"
#include "edl.h"
#include "filexml.h"
#include "localsession.h"





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

int LocalSession::load_defaults(Defaults *defaults)
{
	loop_playback = defaults->get("LOOP_PLAYBACK", 0);
	loop_start = defaults->get("LOOP_START", (double)0);
	loop_end = defaults->get("LOOP_END", (double)0);
	selectionstart = defaults->get("SELECTIONSTART", selectionstart);
	selectionend = defaults->get("SELECTIONEND", selectionend);
	track_start = defaults->get("TRACK_START", 0);
	view_start = defaults->get("VIEW_START", 0);
	zoom_sample = defaults->get("ZOOM_SAMPLE", 1);
	zoom_y = defaults->get("ZOOMY", 64);
	zoom_track = defaults->get("ZOOM_TRACK", 64);
//	preview_start = defaults->get("PREVIEW_START", preview_start);
//	preview_end = defaults->get("PREVIEW_END", preview_end);
	return 0;
}

int LocalSession::save_defaults(Defaults *defaults)
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
//	defaults->update("PREVIEW_START", preview_start);
//	defaults->update("PREVIEW_END", preview_end);
	return 0;
}


double LocalSession::get_selectionstart()
{
	if(in_point >= 0)
		return in_point;
	else
	if(out_point >= 0)
		return out_point;
	else
		return selectionstart;
}

double LocalSession::get_selectionend()
{
	if(out_point >= 0)
		return out_point;
	else
	if(in_point >= 0)
		return in_point;
	else
		return selectionend;
}


