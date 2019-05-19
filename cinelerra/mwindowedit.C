
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

#include "asset.h"
#include "assetlist.h"
#include "awindowgui.h"
#include "awindow.h"
#include "bcclipboard.h"
#include "bcpan.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "cache.h"
#include "clip.h"
#include "clipedit.h"
#include "cliplist.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "bchash.h"
#include "edl.h"
#include "edit.h"
#include "edlsession.h"
#include "filexml.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keyframes.h"
#include "language.h"
#include "labels.h"
#include "levelwindow.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maskautos.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "panauto.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "plugin.h"
#include "samplescroll.h"
#include "selection.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracks.h"
#include "units.h"
#include "undostackitem.h"
#include "vplayback.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zoombar.h"
#include "automation.h"
#include "maskautos.h"
#include <string.h>


void MWindow::add_audio_track_entry(int above, Track *dst)
{
	add_audio_track(above, dst);
	save_backup();
	undo->update_undo(_("add track"), LOAD_ALL);

	restart_brender();
	gui->get_scrollbars();
	gui->canvas->draw();
	gui->patchbay->update();
	gui->cursor->draw(1);
	gui->canvas->flash();
	gui->canvas->activate();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::add_video_track_entry(Track *dst)
{
	add_video_track(1, dst);
	undo->update_undo(_("add track"), LOAD_ALL);

	restart_brender();
	gui->get_scrollbars();
	gui->canvas->draw();
	gui->patchbay->update();
	gui->cursor->draw(1);
	gui->canvas->flash();
	gui->canvas->activate();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
	save_backup();
}

void MWindow::add_audio_track(int above, Track *dst)
{
	master_edl->tracks->add_audio_track(above, dst);
	master_edl->tracks->update_y_pixels(theme);
	save_backup();
}

void MWindow::add_video_track(int above, Track *dst)
{
	master_edl->tracks->add_video_track(above, dst);
	master_edl->tracks->update_y_pixels(theme);
	save_backup();
}

void MWindow::asset_to_size()
{
	if(mainsession->drag_assets->total &&
		mainsession->drag_assets->values[0]->video_data)
	{
		int w, h;

// Get w and h
		w = mainsession->drag_assets->values[0]->width;
		h = mainsession->drag_assets->values[0]->height;

		edlsession->output_w = w;
		edlsession->output_h = h;

		if(((edlsession->output_w % 4) ||
			(edlsession->output_h % 4)) &&
			edlsession->playback_config->vconfig->driver == PLAYBACK_X11_GL)
		{
			errormsg(_("This project's dimensions are not multiples of 4 so\n"
				"it can't be rendered by OpenGL."));
		}

		edlsession->sample_aspect_ratio =
			mainsession->drag_assets->values[0]->sample_aspect_ratio;
		AspectRatioSelection::limits(&edlsession->sample_aspect_ratio);
		save_backup();

		undo->update_undo(_("asset to size"), LOAD_ALL);
		restart_brender();
		sync_parameters(CHANGE_ALL);
	}
}

void MWindow::asset_to_rate()
{
	if(mainsession->drag_assets->total &&
		mainsession->drag_assets->values[0]->video_data)
	{
		if(EQUIV(edlsession->frame_rate, mainsession->drag_assets->values[0]->frame_rate))
			return;

		edlsession->frame_rate = mainsession->drag_assets->values[0]->frame_rate;

		save_backup();

		undo->update_undo(_("asset to rate"), LOAD_ALL);
		restart_brender();
		gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
		sync_parameters(CHANGE_ALL);
	}
}

void MWindow::clear_entry()
{
	clear(1);

	master_edl->optimize();
	save_backup();
	undo->update_undo(_("clear"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::clear(int clear_handle)
{
	cwindow->stop_playback();
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();
	if(clear_handle || !EQUIV(start, end))
	{
		master_edl->clear(start,
			end, 
			edlsession->edit_actions());
	}
}

void MWindow::straighten_automation()
{
	master_edl->tracks->straighten_automation(
		master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	save_backup();
	undo->update_undo(_("straighten curves"), LOAD_AUTOMATION); 

	restart_brender();
	update_plugin_guis();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(WUPD_POSITION);
}

void MWindow::clear_automation()
{
	master_edl->tracks->clear_automation(
		master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	save_backup();
	undo->update_undo(_("clear keyframes"), LOAD_AUTOMATION); 

	restart_brender();
	update_plugin_guis();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(WUPD_POSITION);
}

void MWindow::clear_labels()
{
	clear_labels(master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	undo->update_undo(_("clear labels"), LOAD_TIMEBAR);

	gui->timebar->update();
	cwindow->update(WUPD_TIMEBAR);
	save_backup();
}

void MWindow::clear_labels(ptstime start, ptstime end)
{
	master_edl->labels->clear(start, end, 0);
}

void MWindow::concatenate_tracks()
{
	if(cwindow->stop_playback())
		return;
	master_edl->tracks->concatenate_tracks(
		edlsession->plugins_follow_edits ? EDIT_PLUGINS : 0);
	save_backup();
	undo->update_undo(_("concatenate tracks"), LOAD_EDITS);

	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::copy()
{
	copy(master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
}

void MWindow::copy(ptstime start, ptstime end)
{
	if(PTSEQU(start, end))
		return;

	FileXML file;
	EDL edl(0);

	edl.copy(master_edl, start, end);
	edl.save_xml(&file, "", 0, 0);

	gui->get_clipboard()->to_clipboard(file.string, strlen(file.string), SECONDARY_SELECTION);
	save_backup();
}

void MWindow::copy_automation()
{
	FileXML file;
	Tracks tracks(0);

	tracks.copy(master_edl->tracks, master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	tracks.automation_xml(&file);
	gui->get_clipboard()->to_clipboard(file.string, 
		strlen(file.string), 
		SECONDARY_SELECTION);
}

// Uses cropping coordinates in edl session to crop and translate video.
// We modify the projector since camera automation depends on the track size.
void MWindow::crop_video()
{
// Clamp EDL crop region
	if(edlsession->crop_x1 > edlsession->crop_x2)
	{
		edlsession->crop_x1 ^= edlsession->crop_x2;
		edlsession->crop_x2 ^= edlsession->crop_x1;
		edlsession->crop_x1 ^= edlsession->crop_x2;
	}
	if(edlsession->crop_y1 > edlsession->crop_y2)
	{
		edlsession->crop_y1 ^= edlsession->crop_y2;
		edlsession->crop_y2 ^= edlsession->crop_y1;
		edlsession->crop_y1 ^= edlsession->crop_y2;
	}

	float old_projector_x = (float)edlsession->output_w / 2;
	float old_projector_y = (float)edlsession->output_h / 2;
	float new_projector_x = (float)(edlsession->crop_x1 +
		edlsession->crop_x2) / 2;
	float new_projector_y = (float)(edlsession->crop_y1 +
		edlsession->crop_y2) / 2;
	float projector_offset_x = -(new_projector_x - old_projector_x);
	float projector_offset_y = -(new_projector_y - old_projector_y);

	master_edl->tracks->translate_projector(projector_offset_x, projector_offset_y);

	edlsession->output_w = edlsession->crop_x2 - edlsession->crop_x1;
	edlsession->output_h = edlsession->crop_y2 - edlsession->crop_y1;
	edlsession->crop_x1 = 0;
	edlsession->crop_y1 = 0;
	edlsession->crop_x2 = edlsession->output_w;
	edlsession->crop_y2 = edlsession->output_h;

	undo->update_undo(_("crop"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_ALL);
	save_backup();
}

void MWindow::cut()
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();

	copy(start, end);
	master_edl->clear(start,
		end,
		edlsession->edit_actions());

	master_edl->optimize();
	save_backup();
	undo->update_undo(_("cut"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::cut_automation()
{
	copy_automation();

	master_edl->tracks->clear_automation(master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	save_backup();
	undo->update_undo(_("cut keyframes"), LOAD_AUTOMATION); 

	restart_brender();
	update_plugin_guis();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(WUPD_POSITION);
}

void MWindow::delete_inpoint()
{
	master_edl->local_session->unset_inpoint();
	save_backup();
}

void MWindow::delete_outpoint()
{
	master_edl->local_session->unset_outpoint();
	save_backup();
}

void MWindow::delete_track()
{
	if(cwindow->stop_playback())
		return;
	if(master_edl->tracks->last)
		delete_track(master_edl->tracks->last);
}

void MWindow::delete_tracks()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->delete_tracks();
	undo->update_undo(_("delete tracks"), LOAD_ALL);
	save_backup();

	restart_brender();
	update_plugin_states();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::delete_track(Track *track)
{
	master_edl->tracks->delete_track(track);
	undo->update_undo(_("delete track"), LOAD_ALL);

	restart_brender();
	update_plugin_states();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
	save_backup();
}

void MWindow::detach_transition(Plugin *transition)
{
	hide_plugin(transition, 1);
	int is_video = (transition->track->data_type == TRACK_VIDEO);
	transition->track->detach_transition(transition);
	save_backup();
	undo->update_undo(_("detach transition"), LOAD_ALL);

	if(is_video) restart_brender();
	gui->update(WUPD_CANVINCR);
	sync_parameters(CHANGE_EDL);
}


// Insert data from clipboard
void MWindow::insert(ptstime position,
	FileXML *file,
	int actions,
	EDL *parent_edl)
{
// For clipboard pasting make the new edl use a separate session 
// from the master EDL.  Then it can be resampled to the master rates.
// For splice, overwrite, and dragging need same session to get the assets.
	EDL edl(0);
	uint32_t load_flags = LOAD_ALL;

	if(parent_edl) load_flags &= ~LOAD_SESSION;
	if(!edlsession->labels_follow_edits) load_flags &= ~LOAD_TIMEBAR;

	edl.load_xml(file, load_flags, 0);

	paste_edl(&edl,
		LOADMODE_PASTE, 
		0, 
		position,
		actions,
		0); // overwrite
}

void MWindow::insert(EDL *edl, ptstime position, int actions)
{
	paste_edl(edl,
		LOADMODE_PASTE,
		0,
		position,
		actions,
		0); // overwrite
}

void MWindow::insert_effects_canvas(ptstime start,
	ptstime length)
{
	Track *dest_track = mainsession->track_highlighted;
	if(!dest_track) return;

	for(int i = 0; i < mainsession->drag_pluginservers->total; i++)
	{
		PluginServer *plugin = mainsession->drag_pluginservers->values[i];

		insert_effect(plugin->title,
			dest_track,
			start,
			length,
			PLUGIN_STANDALONE);
	}

	save_backup();
	undo->update_undo(_("insert effect"), LOAD_EDITS | LOAD_PATCHES);
	restart_brender();
	sync_parameters(CHANGE_EDL);
// GUI updated in TrackCanvas, after current_operations are reset
}

void MWindow::insert_effects_cwindow(Track *dest_track)
{
	if(!dest_track) return;

	ptstime start = 0;
	ptstime length = dest_track->get_length();

	if(master_edl->local_session->get_selectionend() >
		master_edl->local_session->get_selectionstart())
	{
		start = master_edl->local_session->get_selectionstart();
		length = master_edl->local_session->get_selectionend() -
			master_edl->local_session->get_selectionstart();
	}

	for(int i = 0; i < mainsession->drag_pluginservers->total; i++)
	{
		PluginServer *plugin = mainsession->drag_pluginservers->values[i];

		insert_effect(plugin->title,
			dest_track,
			start,
			length,
			PLUGIN_STANDALONE);
	}

	save_backup();
	undo->update_undo(_("insert effect"), LOAD_EDITS | LOAD_PATCHES);
	restart_brender();
	sync_parameters(CHANGE_EDL);
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
}

void MWindow::insert_effect(const char *title, 
	Track *track,
	ptstime start,
	ptstime length,
	int plugin_type,
	Plugin *shared_plugin,
	Track *shared_track)
{
	PluginServer *server = 0;
	Plugin *new_plugin;
	int result;

	if(plugin_type == PLUGIN_STANDALONE)
		server = scan_plugindb(title, track->data_type);

	if(EQUIV(length, 0))
	{
		if(master_edl->local_session->get_selectionend() >
			master_edl->local_session->get_selectionstart())
		{
			start = master_edl->local_session->get_selectionstart();
			length = master_edl->local_session->get_selectionend() - start;
		}
		else
		{
			start = 0;
			length = track->get_length();
		}
	}
	start = master_edl->align_to_frame(start, 1);
	length = master_edl->align_to_frame(length, 1);

// Insert plugin object
	new_plugin = track->insert_effect(title,
		start, length,
		plugin_type,
		shared_plugin, shared_track);

	if(server && !(result = server->open_plugin(1, preferences, 0, 0)))
	{
		server->save_data((KeyFrame*)new_plugin->keyframes->first);
		server->close_plugin();
	}
}

void MWindow::modify_edithandles(void)
{
	master_edl->modify_edithandles(mainsession->drag_start,
		mainsession->drag_position,
		mainsession->drag_handle,
		edlsession->edit_handle_mode[mainsession->drag_button],
		edlsession->edit_actions());

	finish_modify_handles();
}

void MWindow::modify_pluginhandles()
{
	master_edl->modify_pluginhandles(mainsession->drag_start,
		mainsession->drag_position,
		mainsession->drag_handle,
		edlsession->edit_handle_mode[mainsession->drag_button],
		edlsession->edit_actions());

	finish_modify_handles();
}

// Common to edithandles and plugin handles
void MWindow::finish_modify_handles()
{
	int edit_mode = edlsession->edit_handle_mode[mainsession->drag_button];

	if(edit_mode != MOVE_NO_EDITS)
		master_edl->local_session->set_selection(mainsession->drag_position);
	else
		master_edl->local_session->set_selection(mainsession->drag_start);

	if(master_edl->local_session->get_selectionstart(1) < 0)
		master_edl->local_session->set_selection(0);

	save_backup();
	undo->update_undo(_("drag handle"), LOAD_EDITS | LOAD_TIMEBAR);
	restart_brender();
	sync_parameters(CHANGE_EDL);
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
}

void MWindow::match_output_size(Track *track)
{
	track->track_w = edlsession->output_w;
	track->track_h = edlsession->output_h;
	save_backup();
	undo->update_undo(_("match output size"), LOAD_ALL);

	restart_brender();
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_edits(ArrayList<Edit*> *edits, 
		Track *track,
		ptstime position,
		int behaviour)
{
	EDL edl(0);
	ptstime start, end;
	ArrayList<Track*> tracks;

	if(!edits->total)
		return;

	start = edits->values[0]->get_pts();
	end = edits->values[0]->end_pts();

	for(int i = 0; i < edits->total; i++)
		tracks.append(edits->values[i]->track);
	edl.copy(master_edl, start, end, &tracks);
	paste_edl(&edl, LOADMODE_PASTE, track, position,
		edlsession->edit_actions(), behaviour);
	master_edl->clear(start, end, edlsession->edit_actions());
	save_backup();
	undo->update_undo(_("move edit"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);

	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR);
}

void MWindow::move_effect(Plugin *plugin,
	Track *dest_track,
	ptstime dest_position)
{

	master_edl->tracks->move_effect(plugin,
		dest_track, 
		dest_position);

	save_backup();
	undo->update_undo(_("move effect"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);

	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR);
}

void MWindow::move_plugin_up(Plugin *plugin)
{
	plugin->track->move_plugin_up(plugin);

	save_backup();
	undo->update_undo(_("move effect up"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_plugin_down(Plugin *plugin)
{
	plugin->track->move_plugin_down(plugin);

	save_backup();
	undo->update_undo(_("move effect down"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_track_down(Track *track)
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_track_down(track);
	save_backup();
	undo->update_undo(_("move track down"), LOAD_ALL);

	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_down()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_tracks_down();
	save_backup();
	undo->update_undo(_("move tracks down"), LOAD_ALL);

	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_track_up(Track *track)
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_track_up(track);
	save_backup();
	undo->update_undo(_("move track up"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_up()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_tracks_up();
	save_backup();
	undo->update_undo(_("move tracks up"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
}

void MWindow::mute_selection()
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();

	if(!PTSEQU(start,end))
	{
		master_edl->clear(start,
			end, 
			edlsession->plugins_follow_edits ? EDIT_PLUGINS : 0);
		master_edl->local_session->set_selectionend(end);
		master_edl->local_session->set_selectionstart(start);
		master_edl->paste_silence(start, end, edlsession->edit_actions() & EDIT_EDITS);
		save_backup();
		undo->update_undo(_("mute"), LOAD_EDITS);

		restart_brender();
		update_plugin_guis();
		gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
		cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
	}
}

void MWindow::overwrite(EDL *source)
{
	ptstime src_start = source->local_session->get_selectionstart();
	ptstime overwrite_len = source->local_session->get_selectionend() - src_start;
	ptstime dst_start = master_edl->local_session->get_selectionstart();
	ptstime dst_len = master_edl->local_session->get_selectionend() - dst_start;
	EDL edl(0);

	if (!EQUIV(dst_len, 0) && (dst_len < overwrite_len))
	{
// in/out points or selection present and shorter than overwrite range
// shorten the copy range
		overwrite_len = dst_len;
	}

// HACK around paste_edl get_start/endselection on its own
// so we need to clear only when not using both io points
// FIXME: need to write simple overwrite_edl to be used for overwrite function
	if (master_edl->local_session->get_inpoint() < 0 ||
			master_edl->local_session->get_outpoint() < 0)
		master_edl->clear(dst_start,
			dst_start + overwrite_len, 
			0);
	edl.copy(source, src_start, src_start + overwrite_len);
	insert(&edl, dst_start, EDIT_NONE);
	master_edl->local_session->set_selection(dst_start + overwrite_len);

	save_backup();
	undo->update_undo(_("overwrite"), LOAD_EDITS);

	restart_brender();
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);
	sync_parameters(CHANGE_EDL);
}

// For splice and overwrite
void MWindow::paste(ptstime start,
	ptstime end,
	FileXML *file,
	int actions)
{
	clear(0);

// Want to insert with assets shared with the master EDL.
	insert(start, 
			file,
			actions,
			master_edl);
}

// For editing use insertion point position
void MWindow::paste()
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();
	int len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

	if(len)
	{
		char *string = new char[len + 1];

		gui->get_clipboard()->from_clipboard(string, 
			len, 
			SECONDARY_SELECTION);
		FileXML file;
		file.read_from_string(string);

		clear(0);

		insert(start, 
			&file, 
			edlsession->edit_actions());
		master_edl->optimize();
		delete [] string;

		save_backup();
		undo->update_undo(_("paste"), LOAD_EDITS | LOAD_TIMEBAR);

		restart_brender();
		update_plugin_guis();
		gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_CLOCK);
		awindow->gui->async_update_assets();
		sync_parameters(CHANGE_EDL);
	}
}

int MWindow::paste_assets(ptstime position, Track *dest_track, int overwrite)
{
	int result = 0;

	if(mainsession->drag_assets->total)
	{
		load_assets(mainsession->drag_assets,
			position, 
			dest_track, 
			edlsession->edit_actions(),
			overwrite);
		result = 1;
	}

	if(mainsession->drag_clips->total)
	{
		paste_edls(mainsession->drag_clips,
			LOADMODE_PASTE, 
			dest_track,
			position, 
			edlsession->edit_actions(),
			overwrite); // o
		result = 1;
	}

	save_backup();

	undo->update_undo(_("paste assets"), LOAD_EDITS);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR | WUPD_CLOCK);
	sync_parameters(CHANGE_EDL);
	return result;
}

void MWindow::load_assets(ArrayList<Asset*> *new_assets, 
	ptstime position,
	Track *first_track,
	int actions,
	int overwrite)
{
	ptstime duration;

	if(position < 0)
		position = master_edl->local_session->get_selectionstart();

	for(int i = 0; i < new_assets->total; i++)
	{
		master_edl->update_assets(new_assets->values[i]);
		duration = master_edl->tracks->append_asset(new_assets->values[i],
			position, first_track, overwrite);
	}
	save_backup();
}

void MWindow::paste_automation()
{
	int len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

	if(len)
	{
		char *string = new char[len + 1];
		gui->get_clipboard()->from_clipboard(string, 
			len, 
			SECONDARY_SELECTION);
		FileXML file;
		file.read_from_string(string);

		master_edl->tracks->clear_automation(
			master_edl->local_session->get_selectionstart(),
			master_edl->local_session->get_selectionend());
		master_edl->tracks->paste_automation(
			master_edl->local_session->get_selectionstart(),
			&file,
			0); 
		save_backup();
		undo->update_undo(_("paste keyframes"), LOAD_AUTOMATION);
		delete [] string;

		restart_brender();
		update_plugin_guis();
		gui->canvas->draw_overlays();
		gui->canvas->flash();
		sync_parameters(CHANGE_PARAMS);
		gui->patchbay->update();
		cwindow->update(WUPD_POSITION);
	}
}

ptstime MWindow::paste_edl(EDL *new_edl,
	int load_mode,
	Track *first_track,
	ptstime current_position,
	int actions,
	int overwrite)
{
	ptstime original_length;
	ptstime original_preview_end;
	ptstime pasted_length = 0;

	if(new_edl == 0)
		return 0;

	original_length = master_edl->total_length();
	original_preview_end = master_edl->local_session->preview_end;

	switch(load_mode)
	{
	case LOADMODE_RESOURCESONLY:
	case LOADMODE_REPLACE:
	case LOADMODE_REPLACE_CONCATENATE:
		errorbox("paste_edl: Unsupported '%s'", LoadMode::name(load_mode));
		return 0;
	case LOADMODE_NEW_TRACKS:
		master_edl->tracks->create_new_tracks(new_edl->tracks);
		break;
	case LOADMODE_CONCATENATE:
		master_edl->tracks->append_tracks(new_edl->tracks);
		break;
	case LOADMODE_PASTE:
		if(current_position < 0)
			current_position = master_edl->local_session->get_selectionstart();
		pasted_length = master_edl->tracks->append_tracks(new_edl->tracks,
			current_position, first_track, overwrite);
		break;
	}
// Fix preview range
	if(!EQUIV(original_length, original_preview_end))
		master_edl->local_session->preview_end = master_edl->total_length();

// Start examining next batch of index files
	mainindexes->start_build();
	return pasted_length;
}

void MWindow::paste_edls(ArrayList<EDL*> *new_edls,
	int load_mode,
	Track *first_track,
	ptstime current_position,
	int actions,
	int overwrite)
{
	ArrayList<Track*> destination_tracks;
	int need_new_tracks = 0;

	for(int i = 0; i < new_edls->total; i++)
	{
		paste_edl(new_edls->values[i], load_mode,
			first_track, current_position,
			actions, overwrite);

		if(load_mode == LOADMODE_REPLACE ||
				load_mode == LOADMODE_REPLACE_CONCATENATE)
			load_mode = LOADMODE_NEW_TRACKS;
	}
}

void MWindow::paste_silence()
{
	cwindow->stop_playback();
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();
	master_edl->paste_silence(start,
		end, 
		edlsession->edit_actions());
	master_edl->optimize();
	save_backup();
	undo->update_undo(_("silence"), LOAD_EDITS | LOAD_TIMEBAR);

	update_plugin_guis();
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::paste_transition()
{
// Only the first transition gets dropped.
	PluginServer *server = mainsession->drag_pluginservers->values[0];
	if(server->audio)
		strcpy(edlsession->default_atransition, server->title);
	else
		strcpy(edlsession->default_vtransition, server->title);

	master_edl->tracks->paste_transition(server, mainsession->edit_highlighted);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	if(server->video) restart_brender();
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_transition_cwindow(Track *dest_track)
{
	PluginServer *server = mainsession->drag_pluginservers->values[0];
	master_edl->tracks->paste_video_transition(server, 1);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);
	restart_brender();
	gui->update(WUPD_CANVINCR);
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_audio_transition()
{
	PluginServer *server = scan_plugindb(edlsession->default_atransition,
		TRACK_AUDIO);
	if(!server)
	{
		gui->show_message(_("No default transition '%s' found."),
			edlsession->default_atransition);
		return;
	}

	master_edl->tracks->paste_audio_transition(server);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_ALL);
	gui->update(WUPD_CANVINCR);
}

void MWindow::paste_video_transition()
{
	PluginServer *server = scan_plugindb(edlsession->default_vtransition,
		TRACK_VIDEO);
	if(!server)
	{
		gui->show_message(_("No default transition '%s' found."),
			edlsession->default_vtransition);
		return;
	}

	master_edl->tracks->paste_video_transition(server);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_ALL);
	restart_brender();
	gui->update(WUPD_CANVINCR);
}

void MWindow::redo_entry()
{
	cwindow->playback_engine->send_command(STOP);
	vwindow->playback_engine->send_command(STOP);

	undo->redo();

	save_backup();
	update_plugin_states();
	update_plugin_guis();
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);
	cwindow->update(WUPD_POSITION | WUPD_OVERLAYS | WUPD_TOOLWIN |
		WUPD_OPERATION | WUPD_TIMEBAR);

	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_ALL);
}

void MWindow::resize_track(Track *track, int w, int h)
{
// We have to move all maskpoints so they do not move in relation to image areas
	((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->translate_masks(
		(w - track->track_w) / 2, 
		(h - track->track_h) / 2);
	track->track_w = w;
	track->track_h = h;
	undo->update_undo(_("resize track"), LOAD_ALL);
	save_backup();

	restart_brender();
	sync_parameters(CHANGE_EDL);
}


class InPointUndoItem : public UndoStackItem
{
public:
	InPointUndoItem(ptstime old_position, ptstime new_position, EDL *edl);
	void undo();
	int get_size();
private:
	ptstime old_position;
	ptstime new_position;
	EDL *edl;
};

InPointUndoItem::InPointUndoItem(ptstime old_position, ptstime new_position, EDL *edl)
{
	set_description(_("in point"));
	this->old_position = old_position;
	this->new_position = new_position;
	this->edl = edl;
}

void InPointUndoItem::undo()
{
	edl->set_inpoint(old_position);
// prepare to undo the undo
	ptstime tmp = new_position;
	new_position = old_position;
	old_position = tmp;
}

int InPointUndoItem::get_size()
{
	return 20;
}

void MWindow::set_inpoint()
{
	InPointUndoItem *undo_item;

	undo_item = new InPointUndoItem(master_edl->local_session->get_inpoint(),
	master_edl->local_session->get_selectionstart(1), master_edl);
	undo->push_undo_item(undo_item);

	master_edl->set_inpoint(master_edl->local_session->get_selectionstart(1));
	save_backup();

	gui->timebar->update();
	gui->flush();

	cwindow->gui->timebar->update();
	cwindow->gui->flush();
}

class OutPointUndoItem : public UndoStackItem
{
public:
	OutPointUndoItem(ptstime old_position, ptstime new_position, EDL *edl);
	void undo();
	int get_size();
private:
	ptstime old_position;
	ptstime new_position;
	EDL *edl;
};

OutPointUndoItem::OutPointUndoItem(ptstime old_position, 
	ptstime new_position, EDL *edl)
{
	set_description(_("out point"));
	this->old_position = old_position;
	this->new_position = new_position;
	this->edl = edl;
}

void OutPointUndoItem::undo()
{
	edl->set_outpoint(old_position);
// prepare to undo the undo
	ptstime tmp = new_position;
	new_position = old_position;
	old_position = tmp;
}

int OutPointUndoItem::get_size()
{
	return 20;
}

void MWindow::set_outpoint()
{
	OutPointUndoItem *undo_item;

	undo_item = new OutPointUndoItem(master_edl->local_session->get_outpoint(),
		master_edl->local_session->get_selectionend(1), master_edl);
	undo->push_undo_item(undo_item);

	master_edl->set_outpoint(master_edl->local_session->get_selectionend(1));
	save_backup();

	gui->timebar->update();
	gui->flush();

	cwindow->gui->timebar->update();
	cwindow->gui->flush();
}

void MWindow::splice(EDL *source)
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime source_start = source->local_session->get_selectionstart();
	ptstime source_end = source->local_session->get_selectionend();
	EDL edl(0);

	edl.copy(source, source_start, source_end);
	insert(&edl, start, edlsession->edit_actions());

// Position at end of clip
	master_edl->local_session->set_selection(start + source_end - source_start);

	save_backup();
	undo->update_undo(_("splice"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);
	sync_parameters(CHANGE_EDL);
}

void MWindow::to_clip()
{
	EDL *new_edl = new EDL(0);
	ptstime start, end;
	char string[BCTEXTLEN];

	start = master_edl->local_session->get_selectionstart();
	end = master_edl->local_session->get_selectionend();

	if(EQUIV(end, start)) 
	{
		start = 0;
		end = master_edl->total_length();
	}

	new_edl->copy(master_edl, start, end);
	sprintf(new_edl->local_session->clip_title, _("Clip %d"), mainsession->clip_number++);
	edlsession->ptstotext(string, end - start);

	sprintf(new_edl->local_session->clip_notes, _("%s\nCreated from main window"), string);

	new_edl->local_session->set_selection(0);
	awindow->clip_edit->create_clip(new_edl);
	save_backup();
}


class LabelUndoItem : public UndoStackItem
{
public:
	LabelUndoItem(ptstime position1, ptstime position2, EDL *edl);
	void undo();
	int get_size();
private:
	ptstime position1;
	ptstime position2;
	EDL *edl;
};

LabelUndoItem::LabelUndoItem(ptstime position1,
	ptstime position2, EDL *edl)
{
	set_description(_("label"));
	this->position1 = position1;
	this->position2 = position2;
	this->edl = edl;
}

void LabelUndoItem::undo()
{
	edl->labels->toggle_label(position1, position2);
}

int LabelUndoItem::get_size()
{
	return 20;
}


void MWindow::toggle_label()
{
	LabelUndoItem *undo_item;
	ptstime position1, position2;

	if(cwindow->playback_engine->is_playing_back)
	{
		position1 = position2 = 
			cwindow->playback_engine->get_tracking_position();
	}
	else
	{
		position1 = master_edl->local_session->get_selectionstart(1);
		position2 = master_edl->local_session->get_selectionend(1);
	}

	position1 = master_edl->align_to_frame(position1);
	position2 = master_edl->align_to_frame(position2);

	undo_item = new LabelUndoItem(position1, position2, master_edl);
	undo->push_undo_item(undo_item);

	master_edl->labels->toggle_label(position1, position2);
	save_backup();

	gui->timebar->update();
	gui->canvas->activate();
	gui->flush();

	cwindow->gui->timebar->update();
	cwindow->gui->flush();
	awindow->gui->async_update_assets();
}

void MWindow::trim_selection()
{
	master_edl->trim_selection(master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend(),
		edlsession->edit_actions());

	save_backup();
	undo->update_undo(_("trim selection"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_EDL);
}

void MWindow::undo_entry()
{
	cwindow->playback_engine->send_command(STOP);
	vwindow->playback_engine->send_command(STOP);

	undo->undo(); 

	save_backup();
	restart_brender();
	update_plugin_states();
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);
	cwindow->update(WUPD_POSITION | WUPD_OVERLAYS | WUPD_TOOLWIN |
		WUPD_OPERATION | WUPD_TIMEBAR);

	awindow->gui->async_update_assets();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_ALL);
}

void MWindow::select_point(ptstime position)
{
	master_edl->local_session->set_selection(position);

// Que the CWindow
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->hide(0);
	gui->cursor->draw(1);
	gui->mainclock->update(master_edl->local_session->get_selectionstart(1));
	gui->zoombar->update();
	gui->canvas->flash();
	gui->flush();
}

void MWindow::map_audio(int pattern)
{
	int current_channel = 0;
	int current_track = 0;
	for(Track *current = master_edl->first_track(); current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO && 
			current->record)
		{
			Autos *pan_autos = current->automation->autos[AUTOMATION_PAN];
			PanAuto *pan_auto = (PanAuto*)pan_autos->get_auto_for_editing(-1);

			for(int i = 0; i < MAXCHANNELS; i++)
			{
				pan_auto->values[i] = 0.0;
			}

			if(pattern == MWindow::AUDIO_1_TO_1)
			{
				pan_auto->values[current_channel] = 1.0;
			}
			else
			if(pattern == MWindow::AUDIO_5_1_TO_2)
			{
				switch(current_track)
				{
				case 0:
					pan_auto->values[0] = 0.5;
					pan_auto->values[1] = 0.5;
					break;
				case 1:
					pan_auto->values[0] = 1;
					break;
				case 2:
					pan_auto->values[1] = 1;
					break;
				case 3:
					pan_auto->values[0] = 1;
					break;
				case 4:
					pan_auto->values[1] = 1;
					break;
				case 5:
					pan_auto->values[0] = 0.5;
					pan_auto->values[1] = 0.5;
					break;
				}
			}

			BC_Pan::calculate_stick_position(
				edlsession->audio_channels,
				edlsession->achannel_positions,
				pan_auto->values, 
				MAX_PAN, 
				PAN_RADIUS,
				pan_auto->handle_x,
				pan_auto->handle_y);

			current_channel++;
			current_track++;
			if(current_channel >= edlsession->audio_channels)
				current_channel = 0;
		}
	}
	undo->update_undo(pattern == MWindow::AUDIO_1_TO_1 ? _("map 1:1") : _("map 5.1:2"),
		LOAD_AUTOMATION, 0);
	sync_parameters(CHANGE_PARAMS);
	gui->update(WUPD_CANVINCR | WUPD_PATCHBAY);
}
