
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
#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
#include "bcclipboard.h"
#include "bcpan.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "cache.h"
#include "clip.h"
#include "clipedit.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keyframe.h"
#include "language.h"
#include "labels.h"
#include "levelwindow.h"
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
#include "pluginset.h"
#include "samplescroll.h"
#include "selection.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracks.h"
#include "transition.h"
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
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
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
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
	save_backup();
}

void MWindow::add_audio_track(int above, Track *dst)
{
	edl->tracks->add_audio_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
}

void MWindow::add_video_track(int above, Track *dst)
{
	edl->tracks->add_video_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
}

void MWindow::asset_to_size()
{
	if(session->drag_assets->total &&
		session->drag_assets->values[0]->video_data)
	{
		int w, h;

// Get w and h
		w = session->drag_assets->values[0]->width;
		h = session->drag_assets->values[0]->height;

		edl->session->output_w = w;
		edl->session->output_h = h;

		if(((edl->session->output_w % 4) || 
			(edl->session->output_h % 4)) && 
			edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
		{
			errormsg(_("This project's dimensions are not multiples of 4 so\n"
				"it can't be rendered by OpenGL."));
		}

		edl->session->aspect_ratio =
			session->drag_assets->values[0]->aspect_ratio;
		AspectRatioSelection::limits(&edl->session->aspect_ratio, w, h);
		save_backup();

		undo->update_undo(_("asset to size"), LOAD_ALL);
		restart_brender();
		sync_parameters(CHANGE_ALL);
	}
}

void MWindow::clear_entry()
{
	clear(1);

	edl->optimize();
	save_backup();
	undo->update_undo(_("clear"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
}

void MWindow::clear(int clear_handle)
{
	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();
	if(clear_handle || !EQUIV(start, end))
	{
		edl->clear(start, 
			end, 
			edl->session->edit_actions());
	}
}

void MWindow::straighten_automation()
{
	edl->tracks->straighten_automation(
		edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
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
	edl->tracks->clear_automation(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
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
	clear_labels(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
	undo->update_undo(_("clear labels"), LOAD_TIMEBAR);

	gui->timebar->update();
	cwindow->update(WUPD_TIMEBAR);
	save_backup();
}

void MWindow::clear_labels(ptstime start, ptstime end)
{
	edl->labels->clear(start, end, 0);
}

void MWindow::concatenate_tracks()
{
	edl->tracks->concatenate_tracks(edl->session->plugins_follow_edits);
	save_backup();
	undo->update_undo(_("concatenate tracks"), LOAD_EDITS);

	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
}

void MWindow::copy()
{
	copy(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend());
}

void MWindow::copy(ptstime start, ptstime end)
{
	if(PTSEQU(start, end))
		return;

	FileXML file;
	edl->copy(start, 
		end, 
		0,
		0,
		0,
		&file, 
		plugindb,
		"",
		1);

// File is now terminated and rewound

	gui->get_clipboard()->to_clipboard(file.string, strlen(file.string), SECONDARY_SELECTION);
	save_backup();
}

void MWindow::copy_automation()
{
	FileXML file;
	edl->tracks->copy_automation(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend(),
		&file,
		0,
		0); 
	gui->get_clipboard()->to_clipboard(file.string, 
		strlen(file.string), 
		SECONDARY_SELECTION);
}

// Uses cropping coordinates in edl session to crop and translate video.
// We modify the projector since camera automation depends on the track size.
void MWindow::crop_video()
{
// Clamp EDL crop region
	if(edl->session->crop_x1 > edl->session->crop_x2)
	{
		edl->session->crop_x1 ^= edl->session->crop_x2;
		edl->session->crop_x2 ^= edl->session->crop_x1;
		edl->session->crop_x1 ^= edl->session->crop_x2;
	}
	if(edl->session->crop_y1 > edl->session->crop_y2)
	{
		edl->session->crop_y1 ^= edl->session->crop_y2;
		edl->session->crop_y2 ^= edl->session->crop_y1;
		edl->session->crop_y1 ^= edl->session->crop_y2;
	}

	float old_projector_x = (float)edl->session->output_w / 2;
	float old_projector_y = (float)edl->session->output_h / 2;
	float new_projector_x = (float)(edl->session->crop_x1 + edl->session->crop_x2) / 2;
	float new_projector_y = (float)(edl->session->crop_y1 + edl->session->crop_y2) / 2;
	float projector_offset_x = -(new_projector_x - old_projector_x);
	float projector_offset_y = -(new_projector_y - old_projector_y);

	edl->tracks->translate_projector(projector_offset_x, projector_offset_y);

	edl->session->output_w = edl->session->crop_x2 - edl->session->crop_x1;
	edl->session->output_h = edl->session->crop_y2 - edl->session->crop_y1;
	edl->session->crop_x1 = 0;
	edl->session->crop_y1 = 0;
	edl->session->crop_x2 = edl->session->output_w;
	edl->session->crop_y2 = edl->session->output_h;

	undo->update_undo(_("crop"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_ALL);
	save_backup();
}

void MWindow::cut()
{
	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();

	copy(start, end);
	edl->clear(start, 
		end,
		edl->session->edit_actions());

	edl->optimize();
	save_backup();
	undo->update_undo(_("cut"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
}

void MWindow::cut_automation()
{
	copy_automation();

	edl->tracks->clear_automation(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
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
	edl->local_session->unset_inpoint();
	save_backup();
}

void MWindow::delete_outpoint()
{
	edl->local_session->unset_outpoint();
	save_backup();
}

void MWindow::delete_track()
{
	if (edl->tracks->last)
		delete_track(edl->tracks->last);
}

void MWindow::delete_tracks()
{
	edl->tracks->delete_tracks();
	undo->update_undo(_("delete tracks"), LOAD_ALL);
	save_backup();

	restart_brender();
	update_plugin_states();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
}

void MWindow::delete_track(Track *track)
{
	edl->tracks->delete_track(track);
	undo->update_undo(_("delete track"), LOAD_ALL);

	restart_brender();
	update_plugin_states();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
	save_backup();
}

void MWindow::detach_transition(Transition *transition)
{
	hide_plugin(transition, 1);
	int is_video = (transition->edit->track->data_type == TRACK_VIDEO);
	transition->edit->detach_transition();
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
	EDL edl(parent_edl);
	ArrayList<EDL*> new_edls;
	uint32_t load_flags = LOAD_ALL;

	new_edls.append(&edl);

	if(parent_edl) load_flags &= ~LOAD_SESSION;
	if(!edl.session->autos_follow_edits) load_flags &= ~LOAD_AUTOMATION;
	if(!edl.session->labels_follow_edits) load_flags &= ~LOAD_TIMEBAR;

	edl.load_xml(plugindb, file, load_flags);

	paste_edls(&new_edls, 
		LOADMODE_PASTE, 
		0, 
		position,
		actions,
		0); // overwrite
	new_edls.remove_all();
}

void MWindow::insert_effects_canvas(ptstime start,
	ptstime length)
{
	Track *dest_track = session->track_highlighted;
	if(!dest_track) return;

	for(int i = 0; i < session->drag_pluginservers->total; i++)
	{
		PluginServer *plugin = session->drag_pluginservers->values[i];

		insert_effect(plugin->title,
			0,
			dest_track,
			i == 0 ? session->pluginset_highlighted : 0,
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

	if(edl->local_session->get_selectionend() > 
		edl->local_session->get_selectionstart())
	{
		start = edl->local_session->get_selectionstart();
		length = edl->local_session->get_selectionend() - 
			edl->local_session->get_selectionstart();
	}

	for(int i = 0; i < session->drag_pluginservers->total; i++)
	{
		PluginServer *plugin = session->drag_pluginservers->values[i];

		insert_effect(plugin->title,
			0,
			dest_track,
			0,
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
	SharedLocation *shared_location, 
	Track *track,
	PluginSet *plugin_set,
	ptstime start,
	ptstime length,
	int plugin_type)
{
	KeyFrame *default_keyframe = 0;

	if(plugin_type == PLUGIN_STANDALONE)
	{
		PluginServer *server = scan_plugindb(title, track->data_type);
		default_keyframe = &server->default_keyframe;
	}

// Insert plugin object
	track->insert_effect(title, 
		shared_location, 
		default_keyframe, 
		plugin_set,
		start,
		length,
		plugin_type);

	track->optimize();
}

void MWindow::modify_edithandles(void)
{
	edl->modify_edithandles(session->drag_start, 
		session->drag_position, 
		session->drag_handle, 
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->edit_actions());

	finish_modify_handles();
}

void MWindow::modify_pluginhandles()
{
	edl->modify_pluginhandles(session->drag_start, 
		session->drag_position, 
		session->drag_handle, 
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->labels_follow_edits);

	finish_modify_handles();
}

// Common to edithandles and plugin handles
void MWindow::finish_modify_handles()
{
	int edit_mode = edl->session->edit_handle_mode[session->drag_button];

	if(edit_mode != MOVE_NO_EDITS)
		edl->local_session->set_selection(session->drag_position);
	else
		edl->local_session->set_selection(session->drag_start);

	if(edl->local_session->get_selectionstart(1) < 0)
		edl->local_session->set_selection(0);

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
	track->track_w = edl->session->output_w;
	track->track_h = edl->session->output_h;
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
	edl->tracks->move_edits(edits, 
		track, 
		position,
		behaviour);

	save_backup();
	undo->update_undo(_("move edit"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);

	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR);
}

void MWindow::move_effect(Plugin *plugin,
	PluginSet *dest_plugin_set,
	Track *dest_track,
	ptstime dest_position)
{

	edl->tracks->move_effect(plugin, 
		dest_plugin_set, 
		dest_track, 
		dest_position);

	save_backup();
	undo->update_undo(_("move effect"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);

	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR);
}

void MWindow::move_plugins_up(PluginSet *plugin_set)
{
	plugin_set->track->move_plugins_up(plugin_set);

	save_backup();
	undo->update_undo(_("move effect up"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_plugins_down(PluginSet *plugin_set)
{
	plugin_set->track->move_plugins_down(plugin_set);

	save_backup();
	undo->update_undo(_("move effect down"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_track_down(Track *track)
{
	edl->tracks->move_track_down(track);
	save_backup();
	undo->update_undo(_("move track down"), LOAD_ALL);

	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_down()
{
	edl->tracks->move_tracks_down();
	save_backup();
	undo->update_undo(_("move tracks down"), LOAD_ALL);

	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_track_up(Track *track)
{
	edl->tracks->move_track_up(track);
	save_backup();
	undo->update_undo(_("move track up"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_up()
{
	edl->tracks->move_tracks_up();
	save_backup();
	undo->update_undo(_("move tracks up"), LOAD_ALL);
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters(CHANGE_EDL);
}

void MWindow::mute_selection()
{
	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();

	if(!PTSEQU(start,end))
	{
		edl->clear(start, 
			end, 
			edl->session->plugins_follow_edits ? EDIT_PLUGINS : 0);
		edl->local_session->set_selectionend(end);
		edl->local_session->set_selectionstart(start);
		edl->paste_silence(start, end, 0, edl->session->plugins_follow_edits);
		save_backup();
		undo->update_undo(_("mute"), LOAD_EDITS);

		restart_brender();
		update_plugin_guis();
		gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
		cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
	}
}

void MWindow::overwrite(EDL *source)
{
	FileXML file;

	ptstime src_start = source->local_session->get_selectionstart();
	ptstime overwrite_len = source->local_session->get_selectionend() - src_start;
	ptstime dst_start = edl->local_session->get_selectionstart();
	ptstime dst_len = edl->local_session->get_selectionend() - dst_start;

	if (!EQUIV(dst_len, 0) && (dst_len < overwrite_len))
	{
// in/out points or selection present and shorter than overwrite range
// shorten the copy range
		overwrite_len = dst_len;
	}

	source->copy(src_start, 
		src_start + overwrite_len, 
		1,
		0,
		0,
		&file,
		plugindb,
		"",
		1);

// HACK around paste_edl get_start/endselection on its own
// so we need to clear only when not using both io points
// FIXME: need to write simple overwrite_edl to be used for overwrite function
	if (edl->local_session->get_inpoint() < 0 || 
		edl->local_session->get_outpoint() < 0)
		edl->clear(dst_start, 
			dst_start + overwrite_len, 
			0);

	paste(dst_start, 
		dst_start + overwrite_len, 
		&file,
		EDIT_NONE);

	edl->local_session->set_selection(dst_start + overwrite_len);

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
			edl);
}

// For editing use insertion point position
void MWindow::paste()
{
	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();
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
			edl->session->edit_actions());
		edl->optimize();
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

	if(session->drag_assets->total)
	{
		load_assets(session->drag_assets, 
			position, 
			LOADMODE_PASTE,
			dest_track, 
			edl->session->edit_actions(),
			overwrite);
		result = 1;
	}

	if(session->drag_clips->total)
	{
		paste_edls(session->drag_clips, 
			LOADMODE_PASTE, 
			dest_track,
			position, 
			edl->session->edit_actions(),
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
	int load_mode,
	Track *first_track,
	int actions,
	int overwrite)
{
	if(position < 0) position = edl->local_session->get_selectionstart();

	ArrayList<EDL*> new_edls;
	for(int i = 0; i < new_assets->total; i++)
	{
		remove_asset_from_caches(new_assets->values[i]);
		EDL *new_edl = new EDL;
		new_edl->copy_session(edl);
		new_edls.append(new_edl);

		asset_to_edl(new_edl, new_assets->values[i]);
	}

	paste_edls(&new_edls, 
		load_mode, 
		first_track,
		position,
		actions,
		overwrite);

	save_backup();
	new_edls.remove_all_objects();
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

		edl->tracks->clear_automation(edl->local_session->get_selectionstart(), 
			edl->local_session->get_selectionend()); 
		edl->tracks->paste_automation(edl->local_session->get_selectionstart(), 
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

// Insert edls with project deletion and index file generation.
void MWindow::paste_edls(ArrayList<EDL*> *new_edls, 
	int load_mode, 
	Track *first_track,
	ptstime current_position,
	int actions,
	int overwrite)
{
	ArrayList<Track*> destination_tracks;
	int need_new_tracks = 0;

	if(!new_edls->total) return;

	ptstime original_length = edl->tracks->total_playable_length();
	ptstime original_preview_end = edl->local_session->preview_end;

// Delete current project
	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		reset_caches();
		edl->save_defaults(defaults);
		hide_plugins();

		delete edl;
		edl = new EDL;
		edl->copy_session(new_edls->values[0]);

		gui->mainmenu->update_toggles();
		gwindow->gui->update_toggles();

// Insert labels for certain modes constitutively
		actions = EDIT_LABELS|EDIT_PLUGINS;
// Force reset of preview
		original_length = 0;
		original_preview_end = -1;
	}

// Create new tracks in master EDL
	if(load_mode == LOADMODE_REPLACE || 
		load_mode == LOADMODE_REPLACE_CONCATENATE ||
		load_mode == LOADMODE_NEW_TRACKS)
	{

		need_new_tracks = 1;
		for(int i = 0; i < new_edls->total; i++)
		{
			EDL *new_edl = new_edls->values[i];
			for(Track *current = new_edl->tracks->first;
				current;
				current = NEXT)
			{
				if(current->data_type == TRACK_VIDEO)
				{
					edl->tracks->add_video_track(0, 0);
					if(current->draw) edl->tracks->last->draw = 1;
					destination_tracks.append(edl->tracks->last);
				}
				else
				if(current->data_type == TRACK_AUDIO)
				{
					edl->tracks->add_audio_track(0, 0);
					destination_tracks.append(edl->tracks->last);
				}
				edl->session->highlighted_track = edl->tracks->total() - 1;
			}

// Base track count on first EDL only for concatenation
			if(load_mode == LOADMODE_REPLACE_CONCATENATE) break;
		}
	}
	else
// Recycle existing tracks of master EDL
	if(load_mode == LOADMODE_CONCATENATE || load_mode == LOADMODE_PASTE)
	{

// The point of this is to shift forward labels after the selection so they can
// then be shifted back to their original locations without recursively
// shifting back every paste.
		if(load_mode == LOADMODE_PASTE && edl->session->labels_follow_edits)
			edl->labels->clear(edl->local_session->get_selectionstart(),
						edl->local_session->get_selectionend(),
						1);

		Track *current = first_track ? first_track : edl->tracks->first;
		for( ; current; current = NEXT)
		{
			if(current->record)
			{
				destination_tracks.append(current);
			}
		}
	}

	int destination_track = 0;
	ptstime *paste_position = new ptstime[destination_tracks.total];

// Iterate through the edls
	for(int i = 0; i < new_edls->total; i++)
	{
		EDL *new_edl = new_edls->values[i];

		ptstime edl_length = new_edl->tracks->total_length();

// Add assets and prepare index files
		for(Asset *new_asset = new_edl->assets->first;
			new_asset;
			new_asset = new_asset->next)
		{
			mainindexes->add_next_asset(0, new_asset);
		}

// Capture index file status from mainindex test
		edl->update_assets(new_edl);

// Get starting point of insertion.  Need this to paste labels.
		switch(load_mode)
		{
		case LOADMODE_REPLACE:
		case LOADMODE_NEW_TRACKS:
			current_position = 0;
			break;

		case LOADMODE_CONCATENATE:
		case LOADMODE_REPLACE_CONCATENATE:
			destination_track = 0;
			if(destination_tracks.total)
				current_position = destination_tracks.values[0]->get_length();
			else
				current_position = 0;
			break;

		case LOADMODE_PASTE:
			destination_track = 0;
			if(i == 0)
			{
				for(int j = 0; j < destination_tracks.total; j++)
				{
					paste_position[j] = (current_position >= 0) ? 
						current_position :
						edl->local_session->get_selectionstart();
				}
			}
			break;

		case LOADMODE_RESOURCESONLY:
			edl->add_clip(new_edl);
			break;
		}

// Insert edl
		if(load_mode != LOADMODE_RESOURCESONLY)
		{
// Insert labels
			if(load_mode == LOADMODE_PASTE)
				edl->labels->insert_labels(new_edl->labels, 
					destination_tracks.total ? paste_position[0] : 0.0,
					edl_length,
					actions & EDIT_LABELS);
			else
				edl->labels->insert_labels(new_edl->labels, 
					current_position,
					edl_length,
					actions & EDIT_LABELS);

			for(Track *new_track = new_edl->tracks->first; 
				new_track; 
				new_track = new_track->next)
			{
// Get destination track of same type as new_track
				for(int k = 0; 
					k < destination_tracks.total &&
					destination_tracks.values[destination_track]->data_type != new_track->data_type;
					k++, destination_track++)
				{
					if(destination_track >= destination_tracks.total - 1)
						destination_track = 0;
				}

// Insert data into destination track
				if(destination_track < destination_tracks.total &&
					destination_tracks.values[destination_track]->data_type == new_track->data_type)
				{
					Track *track = destination_tracks.values[destination_track];

// Replace default keyframes if first EDL and new tracks were created.
// This means data copied from one track and pasted to another won't retain
// the camera position unless it's a keyframe.  If it did, previous data in the
// track might get unknowingly corrupted.  Ideally we would detect when differing
// default keyframes existed and create discrete keyframes for both.
					int replace_default = (i == 0) && need_new_tracks;

// Insert new track at current position
					switch(load_mode)
					{
					case LOADMODE_REPLACE_CONCATENATE:
					case LOADMODE_CONCATENATE:
						current_position = track->get_length();
						break;

					case LOADMODE_PASTE:
						current_position = paste_position[destination_track];
						paste_position[destination_track] += new_track->get_length();
						break;
					}
					if (overwrite)
						track->clear(current_position, 
								current_position + new_track->get_length(), 
								actions | EDIT_EDITS);

					track->insert_track(new_track, 
						current_position, 
						replace_default,
						actions & EDIT_PLUGINS);
				}

// Get next destination track
				destination_track++;
				if(destination_track >= destination_tracks.total)
					destination_track = 0;
			}
		}

		if(load_mode == LOADMODE_PASTE)
			current_position += edl_length;
	}

// Move loading of clips and vwindow to the end - this fixes some
// strange issue, for index not being shown
// Assume any paste operation from the same EDL won't contain any clips.
// If it did it would duplicate every clip here.
	for(int i = 0; i < new_edls->total; i++)
	{
		EDL *new_edl = new_edls->values[i];

		for(int j = 0; j < new_edl->clips.total; j++)
		{
			edl->add_clip(new_edl->clips.values[j]);
		}

		if(new_edl->vwindow_edl)
		{
			if(edl->vwindow_edl) delete edl->vwindow_edl;
			edl->vwindow_edl = new EDL(edl);
			edl->vwindow_edl->copy_all(new_edl->vwindow_edl);
		}
	}
	delete [] paste_position;

// Fix preview range
	if(EQUIV(original_length, original_preview_end))
	{
		edl->local_session->preview_end = edl->tracks->total_playable_length();
	}

// Start examining next batch of index files
	mainindexes->start_build();

// Don't save a backup after loading since the loaded file is on disk already.
}

void MWindow::paste_silence()
{
	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();
	edl->paste_silence(start, 
		end, 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);
	edl->optimize();
	save_backup();
	undo->update_undo(_("silence"), LOAD_EDITS | LOAD_TIMEBAR);

	update_plugin_guis();
	restart_brender();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
}

void MWindow::paste_transition()
{
// Only the first transition gets dropped.
	PluginServer *server = session->drag_pluginservers->values[0];
	if(server->audio)
		strcpy(edl->session->default_atransition, server->title);
	else
		strcpy(edl->session->default_vtransition, server->title);

	edl->tracks->paste_transition(server, session->edit_highlighted);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	if(server->video) restart_brender();
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_transition_cwindow(Track *dest_track)
{
	PluginServer *server = session->drag_pluginservers->values[0];
	edl->tracks->paste_video_transition(server, 1);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);
	restart_brender();
	gui->update(WUPD_CANVINCR);
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_audio_transition()
{
	PluginServer *server = scan_plugindb(edl->session->default_atransition,
		TRACK_AUDIO);
	if(!server)
	{
		gui->show_message(_("No default transition '%s' found."), edl->session->default_atransition);
		return;
	}

	edl->tracks->paste_audio_transition(server);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_ALL);
	gui->update(WUPD_CANVINCR);
}

void MWindow::paste_video_transition()
{
	PluginServer *server = scan_plugindb(edl->session->default_vtransition,
		TRACK_VIDEO);
	if(!server)
	{
		gui->show_message(_("No default transition '%s' found."), edl->session->default_vtransition);
		return;
	}


	edl->tracks->paste_video_transition(server);
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

	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_ALL);
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

	undo_item = new InPointUndoItem(edl->local_session->get_inpoint(),
	edl->local_session->get_selectionstart(1), edl);
	undo->push_undo_item(undo_item);

	edl->set_inpoint(edl->local_session->get_selectionstart(1));
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

	undo_item = new OutPointUndoItem(edl->local_session->get_outpoint(),
				edl->local_session->get_selectionend(1), edl);
	undo->push_undo_item(undo_item);

	edl->set_outpoint(edl->local_session->get_selectionend(1));
	save_backup();

	gui->timebar->update();
	gui->flush();

	cwindow->gui->timebar->update();
	cwindow->gui->flush();
}

void MWindow::splice(EDL *source)
{
	FileXML file;

	source->copy(source->local_session->get_selectionstart(), 
		source->local_session->get_selectionend(), 
		1,
		0,
		0,
		&file,
		plugindb,
		"",
		1);

	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();
	ptstime source_start = source->local_session->get_selectionstart();
	ptstime source_end = source->local_session->get_selectionend();

	paste(start, 
		start, 
		&file,
		edl->session->edit_actions());

// Position at end of clip
	edl->local_session->set_selection(start + source_end - source_start);

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
	FileXML file;
	ptstime start, end;

	start = edl->local_session->get_selectionstart();
	end = edl->local_session->get_selectionend();

	if(EQUIV(end, start)) 
	{
		start = 0;
		end = edl->tracks->total_length();
	}

// Don't copy all since we don't want the clips twice.
	edl->copy(start, 
		end, 
		0,
		0,
		0,
		&file,
		plugindb,
		"",
		1);

	EDL *new_edl = new EDL(edl);

	new_edl->load_xml(plugindb, &file, LOAD_ALL);
	sprintf(new_edl->local_session->clip_title, _("Clip %d"), session->clip_number++);
	char string[BCTEXTLEN];
	Units::totext(string, 
			end - start, 
			edl->session->time_format, 
			edl->session->sample_rate, 
			edl->session->frame_rate,
			edl->session->frames_per_foot);

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
		position1 = edl->local_session->get_selectionstart(1);
		position2 = edl->local_session->get_selectionend(1);
	}

	position1 = edl->align_to_frame(position1);
	position2 = edl->align_to_frame(position2);

	undo_item = new LabelUndoItem(position1, position2, edl);
	undo->push_undo_item(undo_item);

	edl->labels->toggle_label(position1, position2);
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
	edl->trim_selection(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend(), 
		edl->session->edit_actions());

	save_backup();
	undo->update_undo(_("trim selection"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_EDL);
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
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_ALL);
}

void MWindow::select_point(ptstime position)
{
	edl->local_session->set_selection(position);

// Que the CWindow
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->hide(0);
	gui->cursor->draw(1);
	gui->mainclock->update(edl->local_session->get_selectionstart(1));
	gui->zoombar->update();
	gui->canvas->flash();
	gui->flush();
}

void MWindow::map_audio(int pattern)
{
	int current_channel = 0;
	int current_track = 0;
	for(Track *current = edl->tracks->first; current; current = NEXT)
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

			BC_Pan::calculate_stick_position(edl->session->audio_channels, 
				edl->session->achannel_positions, 
				pan_auto->values, 
				MAX_PAN, 
				PAN_RADIUS,
				pan_auto->handle_x,
				pan_auto->handle_y);

			current_channel++;
			current_track++;
			if(current_channel >= edl->session->audio_channels)
				current_channel = 0;
		}
	}
	undo->update_undo(pattern == MWindow::AUDIO_1_TO_1 ? _("map 1:1") : _("map 5.1:2"),
		LOAD_AUTOMATION, 0);
	sync_parameters(CHANGE_PARAMS);
	gui->update(WUPD_CANVINCR | WUPD_PATCHBAY);
}
