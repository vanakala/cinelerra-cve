// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetlist.h"
#include "autos.h"
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
#include "playbackconfig.h"
#include "plugin.h"
#include "plugindb.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "selection.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracks.h"
#include "units.h"
#include "undostackitem.h"
#include "videodevice.inc"
#include "vplayback.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zoombar.h"
#include "automation.h"
#include "maskautos.h"
#include <string.h>


void MWindow::add_track(int track_type, int above, Track *dst)
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->add_track(track_type, above, dst);
	master_edl->tracks->update_y_pixels(theme);
	undo->update_undo(_("add track"), LOAD_ALL);

	restart_brender();
	update_gui(WUPD_SCROLLBARS | WUPD_PATCHBAY | WUPD_CANVINCR);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
	save_backup();
}

void MWindow::asset_to_size(Asset *asset)
{
	if(asset && asset->stream_count(STRDSC_VIDEO) == 1)
	{
		int w, h;

		if(cwindow->stop_playback())
			return;

		int stream = asset->get_stream_ix(STRDSC_VIDEO);
		struct streamdesc *sdsc = &asset->streams[stream];

		w = sdsc->width;
		h = sdsc->height;

		edlsession->output_w = w;
		edlsession->output_h = h;

		if(((w % 4) || (h % 4)) &&
			edlsession->playback_config->vconfig->driver == PLAYBACK_X11_GL)
		{
			errormsg(_("This project's dimensions are not multiples of 4 so\n"
				"it can't be rendered by OpenGL."));
		}

		edlsession->sample_aspect_ratio = sdsc->sample_aspect_ratio;
		AspectRatioSelection::limits(&edlsession->sample_aspect_ratio);
		cwindow->gui->canvas->update_guidelines();
		cwindow->gui->canvas->clear_canvas();
		save_backup();

		undo->update_undo(_("asset to size"), LOAD_ALL);
		sync_parameters();
	}
}

void MWindow::asset_to_rate(Asset *asset)
{
	if(asset && asset->stream_count(STRDSC_VIDEO) == 1)
	{
		int stream = asset->get_stream_ix(STRDSC_VIDEO);
		struct streamdesc *sdsc = &asset->streams[stream];

		if(EQUIV(edlsession->frame_rate, sdsc->frame_rate))
			return;

		if(cwindow->stop_playback())
			return;

		edlsession->frame_rate = sdsc->frame_rate;

		save_backup();

		undo->update_undo(_("asset to rate"), LOAD_ALL);
		update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
		sync_parameters();
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
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::clear(int clear_handle)
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();

	if(cwindow->stop_playback())
		return;

	if(clear_handle || !EQUIV(start, end))
	{
		master_edl->clear(start,
			end, 
			edlsession->labels_follow_edits);
		master_edl->local_session->preview_end = master_edl->total_length();
	}
}

void MWindow::straighten_automation()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->straighten_automation(
		master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	save_backup();
	undo->update_undo(_("straighten curves"), LOAD_AUTOMATION); 

	update_plugin_guis();
	draw_canvas_overlays();
	sync_parameters();
	update_gui(WUPD_PATCHBAY);
	cwindow->update(WUPD_POSITION);
}

void MWindow::clear_automation()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->clear_automation(
		master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend());
	save_backup();
	undo->update_undo(_("clear keyframes"), LOAD_AUTOMATION); 

	update_plugin_guis();
	draw_canvas_overlays();
	sync_parameters();
	update_gui(WUPD_PATCHBAY);
	cwindow->update(WUPD_POSITION);
}

void MWindow::clear_labels()
{
	master_edl->labels->clear(master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend(), 0);
	undo->update_undo(_("clear labels"), LOAD_TIMEBAR);

	update_gui(WUPD_TIMEBAR);
	cwindow->update(WUPD_TIMEBAR);
	save_backup();
}

void MWindow::concatenate_tracks()
{
	if(cwindow->stop_playback())
		return;
	master_edl->tracks->concatenate_tracks();
	save_backup();
	undo->update_undo(_("concatenate tracks"), LOAD_EDITS);

	restart_brender();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
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
	edl.save_xml(&file, 0, EDL_CLIPBRD);

	gui->get_clipboard()->to_clipboard(file.string, strlen(file.string), SECONDARY_SELECTION);
	save_backup();
}

void MWindow::copy_effects()
{
	FileXML file;
	Tracks tracks(0);

	if(master_edl->local_session->get_selectionstart() <
		master_edl->local_session->get_selectionend() - EPSILON)
	{
		tracks.copy(master_edl->tracks, master_edl->local_session->get_selectionstart(),
			master_edl->local_session->get_selectionend());
		tracks.automation_xml(&file);

		gui->get_clipboard()->to_clipboard(file.string,
			strlen(file.string),
			SECONDARY_SELECTION);
	}
}

void MWindow::copy_keyframes(Autos *autos, Auto *keyframe, Plugin *plugin)
{
	FileXML file;

	file.tag.set_title("CLIPBOARD_AUTO");

	if(autos)
	{
		file.tag.set_property("TYPE", "Auto");
		file.tag.set_property("TITLE", Automation::name(autos->autoidx));
	}
	else if(plugin)
	{
		if(!plugin->plugin_server)
			return;
		file.tag.set_property("TYPE", "KeyFrame");
		file.tag.set_property("TITLE", plugin->plugin_server->title);
	}
	file.append_tag();
	file.append_newline();
	keyframe->save_xml(&file);
	file.tag.set_title("/CLIPBOARD_AUTO");
	file.append_tag();
	file.append_newline();
	gui->get_clipboard()->to_clipboard(file.string,
		strlen(file.string),
		SECONDARY_SELECTION);
}

void MWindow::paste_keyframe(Track *track, Plugin *plugin)
{
	int len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);
	ptstime position = master_edl->local_session->get_selectionstart(1);

	if(len)
	{
		FileXML file;
		char *string = new char[len + 1];

		gui->get_clipboard()->from_clipboard(string,
			len, SECONDARY_SELECTION);

		file.read_from_string(string);
		delete string;

		if(!file.read_tag() && file.tag.title_is("CLIPBOARD_AUTO"))
		{
			char *type, *title;
			Auto *new_auto;

			if(cwindow->stop_playback())
				return;

			if(!(type = file.tag.get_property("TYPE")))
				return;

			if(!(title = file.tag.get_property("TITLE")))
				return;

			if(!plugin && !strcmp(type, "Auto"))
			{
				int id;

				if((id = Automation::index(title)) < 0)
					return;

				if(!track->automation || !track->automation->autos[id] ||
						!edlsession->auto_conf->auto_visible[id])
					return;

				if(file.read_tag() || !file.tag.title_is("AUTO"))
					return;

				new_auto = track->automation->autos[id]->insert_auto(position);
			}
			else if(plugin && !strcmp(type, "KeyFrame") &&
					edlsession->keyframes_visible)
			{
				if(!plugin->plugin_server ||
						strcmp(plugin->plugin_server->title, title))
					return;

				new_auto = plugin->keyframes->insert_auto(position);
			}
			if(new_auto)
			{
				new_auto->load(&file);
				save_backup();
				undo->update_undo(_("paste keyframe"), LOAD_ALL);

				update_gui(WUPD_CANVINCR);
				update_plugin_guis();
				sync_parameters();
			}
		}
	}
}

int MWindow::can_paste_keyframe(Track *track, Plugin *plugin)
{
	int len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);
	ptstime position = master_edl->local_session->get_selectionstart(1);

	if(len)
	{
		FileXML file;
		char *string = new char[len + 1];

		gui->get_clipboard()->from_clipboard(string,
			len, SECONDARY_SELECTION);

		file.read_from_string(string);
		delete string;

		if(!file.read_tag() && file.tag.title_is("CLIPBOARD_AUTO"))
		{
			char *type, *title;
			Auto *new_auto;

			if(!(type = file.tag.get_property("TYPE")))
				return 0;

			if(!(title = file.tag.get_property("TITLE")))
				return 0;

			if(!plugin && !strcmp(type, "Auto"))
			{
				int id;

				if(plugin)
					return 0;

				if((id = Automation::index(title)) < 0)
					return 0;

				if(!track->automation || !track->automation->autos[id] ||
						!edlsession->auto_conf->auto_visible[id])
					return 0;
				return 1;
			}
			else if(plugin && !strcmp(type, "KeyFrame"))
			{
				if(!plugin->plugin_server ||
						strcmp(plugin->plugin_server->title, title) ||
						!edlsession->keyframes_visible)
					return 0;
				if(plugin->get_pts() > position || plugin->end_pts() < position)
					return 0;
				return 1;
			}
		}
	}
	return 0;
}

void MWindow::clear_keyframes(Plugin *plugin)
{
	if(edlsession->keyframes_visible)
	{
		if(cwindow->stop_playback())
			return;
		plugin->clear_keyframes();
		save_backup();
		undo->update_undo(_("paste keyframe"), LOAD_ALL);

		update_gui(WUPD_CANVINCR);
		update_plugin_guis();
		sync_parameters();
	}
}

void MWindow::cut()
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();

	copy(start, end);
	master_edl->clear(start,
		end,
		edlsession->labels_follow_edits);

	master_edl->optimize();
	save_backup();
	undo->update_undo(_("cut"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::cut_effects()
{
	if(master_edl->local_session->get_selectionstart() <
		master_edl->local_session->get_selectionend() - EPSILON)
	{
		copy_effects();

		master_edl->tracks->clear_automation(master_edl->local_session->get_selectionstart(),
			master_edl->local_session->get_selectionend());
		save_backup();
		undo->update_undo(_("cut keyframes"), LOAD_AUTOMATION);

		update_plugin_guis();
		draw_canvas_overlays();
		sync_parameters();
		update_gui(WUPD_PATCHBAY);
		cwindow->update(WUPD_POSITION);
	}
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
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::delete_track(Track *track)
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->delete_track(track);
	undo->update_undo(_("delete track"), LOAD_ALL);

	restart_brender();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
	save_backup();
}

void MWindow::detach_transition(Plugin *transition)
{
	if(cwindow->stop_playback())
		return;

	int is_video = (transition->track->data_type == TRACK_VIDEO);
	transition->track->detach_transition(transition);
	save_backup();
	undo->update_undo(_("detach transition"), LOAD_ALL);

	update_gui(WUPD_CANVINCR);
	sync_parameters(is_video);
}


// Insert data from clipboard
void MWindow::insert(ptstime position,
	FileXML *file,
	EDL *parent_edl)
{
	EDL edl(0);

	edl.load_xml(file, 0);

	paste_edl(&edl,
		LOADMODE_PASTE, 
		0, 
		position,
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
	sync_parameters();
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
	sync_parameters();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
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
	PluginClient *client;

	if(cwindow->stop_playback())
		return;

	if(plugin_type == PLUGIN_STANDALONE)
		server = plugindb.get_pluginserver(title, track->data_type, 0);

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
		if(EQUIV(length, 0))
			length = master_edl->duration();
	}
	start = master_edl->align_to_frame(start, 1);
	length = master_edl->align_to_frame(length, 1);

// Insert plugin object
	new_plugin = track->insert_effect(server,
		start, length,
		plugin_type,
		shared_plugin, shared_track);
// Adjust plugin length
	ptstime max_start = track->plugin_max_start(new_plugin);
// Synthetic plugin may change start
	start = new_plugin->get_pts();
	if(start > max_start)
	{
		ptstime new_length = length - (start - max_start);

		if(new_length < EPSILON)
		{
			track->remove_plugin(new_plugin);
			return;
		}
		new_plugin->set_length(new_length);
	}

	if(server && (client = server->open_plugin(0, 0)))
	{
		client->save_data(new_plugin->keyframes->get_first());
		server->close_plugin(client);
	}
	track->tracks->cleanup_plugins();
}

void MWindow::modify_edithandles(void)
{
	master_edl->modify_edithandles(mainsession->drag_start,
		mainsession->drag_position,
		mainsession->drag_handle,
		edlsession->edit_handle_mode[mainsession->drag_button],
		edlsession->labels_follow_edits);

	finish_modify_handles(WUPD_CANVREDRAW);
}

void MWindow::modify_pluginhandles()
{
	master_edl->modify_pluginhandles(mainsession->drag_start,
		mainsession->drag_position,
		mainsession->drag_handle,
		edlsession->edit_handle_mode[mainsession->drag_button]);

	finish_modify_handles();
}

// Common to edithandles and plugin handles
void MWindow::finish_modify_handles(int upd_option)
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
	sync_parameters();
	update_plugin_guis();
	update_gui(upd_option | WUPD_SCROLLBARS | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
}

void MWindow::match_output_size(Track *track)
{
	if(cwindow->stop_playback())
		return;

	track->reset_renderers();
	track->track_w = edlsession->output_w;
	track->track_h = edlsession->output_h;
	save_backup();
	undo->update_undo(_("match output size"), LOAD_ALL);

	sync_parameters();
}

void MWindow::match_asset_size(Track *track)
{
	int w, h;

	w = h = 0;
	track->get_source_dimensions(
		master_edl->local_session->get_selectionstart(1),
		w, h);

	if(w >= MIN_FRAME_WIDTH && h >= MIN_FRAME_WIDTH &&
		w <= MAX_FRAME_WIDTH && h <= MAX_FRAME_HEIGHT)
	{
		if(cwindow->stop_playback())
			return;

		track->reset_renderers();
		track->track_w = w;
		track->track_h = h;

		save_backup();
		undo->update_undo(_("match input size"), LOAD_ALL);

		sync_parameters();
	}
}

void MWindow::move_edits(ArrayList<Edit*> *edits, 
		Track *track,
		ptstime position,
		int behaviour)
{
	EDL edl(0);
	ptstime start, end;
	ArrayList<Track*> tracks;
	ptstime orig_selection = master_edl->local_session->get_selectionstart();

	if(!edits->total)
		return;
	if(cwindow->stop_playback())
		return;

	start = edits->values[0]->get_pts();
	end = edits->values[0]->end_pts();

	for(int i = 0; i < edits->total; i++)
		tracks.append(edits->values[i]->track);
	edl.copy(master_edl, start, end, &tracks);
	master_edl->clear(start, end, edlsession->labels_follow_edits);
	paste_edl(&edl, LOADMODE_PASTE, track, position, behaviour);
	master_edl->local_session->set_selection(orig_selection);
	master_edl->local_session->preview_end = master_edl->total_length();
	save_backup();
	undo->update_undo(_("move edit"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME);

	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR);
}

void MWindow::move_effect(Plugin *plugin,
	Track *dest_track,
	ptstime dest_position)
{

	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_effect(plugin,
		dest_track, 
		dest_position);

	save_backup();
	undo->update_undo(_("move effect"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME);

	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR);
}

void MWindow::move_plugin_up(Plugin *plugin)
{
	if(cwindow->stop_playback())
		return;

	plugin->track->move_plugin_up(plugin);
	save_backup();
	undo->update_undo(_("move effect up"), LOAD_ALL);
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR);
	sync_parameters();
}

void MWindow::move_plugin_down(Plugin *plugin)
{
	if(cwindow->stop_playback())
		return;

	plugin->track->move_plugin_down(plugin);
	save_backup();
	undo->update_undo(_("move effect down"), LOAD_ALL);
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR);
	sync_parameters();
}

void MWindow::move_track_down(Track *track)
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_track_down(track);
	save_backup();
	undo->update_undo(_("move track down"), LOAD_ALL);

	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters();
	save_backup();
}

void MWindow::move_tracks_down()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_tracks_down();
	save_backup();
	undo->update_undo(_("move tracks down"), LOAD_ALL);

	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters();
	save_backup();
}

void MWindow::move_track_up(Track *track)
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_track_up(track);
	save_backup();
	undo->update_undo(_("move track up"), LOAD_ALL);
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters();
	save_backup();
}

void MWindow::move_tracks_up()
{
	if(cwindow->stop_playback())
		return;

	master_edl->tracks->move_tracks_up();
	save_backup();
	undo->update_undo(_("move tracks up"), LOAD_ALL);
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	sync_parameters();
}

void MWindow::mute_selection()
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();

	if(!PTSEQU(start,end))
	{
		if(cwindow->stop_playback())
			return;

		master_edl->clear(start, end, edlsession->labels_follow_edits);
		master_edl->local_session->set_selectionend(end);
		master_edl->local_session->set_selectionstart(start);
		master_edl->paste_silence(start, end, 0);
		save_backup();
		undo->update_undo(_("mute"), LOAD_EDITS);

		restart_brender();
		update_plugin_guis();
		update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
		cwindow->playback_engine->send_command(CURRENT_FRAME);
	}
}

void MWindow::overwrite(EDL *source)
{
	ptstime src_start = source->local_session->get_selectionstart();
	ptstime overwrite_len = source->local_session->get_selectionend() - src_start;
	ptstime dst_start = master_edl->local_session->get_selectionstart();
	ptstime dst_len = master_edl->local_session->get_selectionend() - dst_start;
	EDL edl(0);

	if(cwindow->stop_playback())
		return;
// in/out points or selection present and shorter than overwrite range
// shorten the copy range
	if(!EQUIV(dst_len, 0) && (dst_len < overwrite_len))
		overwrite_len = dst_len;

	edl.copy(source, src_start, src_start + overwrite_len);
	paste_edl(&edl, LOADMODE_PASTE, 0, dst_start, 1);

	master_edl->local_session->set_selection(dst_start + overwrite_len);

	save_backup();
	undo->update_undo(_("overwrite"), LOAD_EDITS);

	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);
	sync_parameters();
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

		if(cwindow->stop_playback())
			return;

		gui->get_clipboard()->from_clipboard(string, 
			len, 
			SECONDARY_SELECTION);
		FileXML file;
		file.read_from_string(string);

		clear(0);

		insert(start, &file);
		master_edl->optimize();
		delete [] string;

		save_backup();
		undo->update_undo(_("paste"), LOAD_EDITS | LOAD_TIMEBAR);

		update_plugin_guis();
		update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_CLOCK);
		awindow->gui->async_update_assets();
		sync_parameters();
	}
}

void MWindow::paste_assets(ptstime position, Track *dest_track, int overwrite)
{
	ptstime cursor_pos = master_edl->local_session->get_selectionstart();

	if(cwindow->stop_playback())
		return;

	if(mainsession->drag_assets->total)
	{
		load_assets(mainsession->drag_assets,
			position, 
			dest_track, 
			overwrite);
	}

	if(mainsession->drag_clips->total)
	{
		paste_edls(mainsession->drag_clips,
			LOADMODE_PASTE, 
			dest_track,
			position, 
			overwrite); // o
	}

	master_edl->local_session->set_selection(cursor_pos);
	save_backup();

	undo->update_undo(_("paste assets"), LOAD_EDITS);
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR | WUPD_CLOCK |
		WUPD_ZOOMBAR);
	sync_parameters();
}

void MWindow::load_assets(ArrayList<Asset*> *new_assets, 
	ptstime position,
	Track *first_track,
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
		master_edl->local_session->preview_end = master_edl->duration();
	}
	save_backup();
}

void MWindow::paste_effects(int operation)
{
	int len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

	if(len)
	{
		FileXML file;
		Tracks tracks(0);

		if(cwindow->stop_playback())
			return;

		char *string = new char[len + 1];
		gui->get_clipboard()->from_clipboard(string, 
			len, 
			SECONDARY_SELECTION);
		file.read_from_string(string);
		tracks.load_effects(&file, operation);
		master_edl->tracks->append_tracks(&tracks,
			master_edl->local_session->get_selectionstart(),
			0, TRACKS_OVERWRITE | TRACKS_EFFECTS);
		save_backup();
		undo->update_undo(_("paste effects"), LOAD_AUTOMATION);
		delete [] string;

		update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_PATCHBAY);
		update_plugin_guis();
		sync_parameters();
		cwindow->update(WUPD_POSITION);
	}
}

ptstime MWindow::paste_edl(EDL *new_edl,
	int load_mode,
	Track *first_track,
	ptstime current_position,
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
			current_position, first_track, overwrite ? TRACKS_OVERWRITE : 0);
		master_edl->local_session->set_selection(current_position + pasted_length);
		break;
	}
// Fix preview range
	if(EQUIV(original_length, original_preview_end))
		master_edl->local_session->preview_end = master_edl->total_length();

// Start examining next batch of index files
	mainindexes->start_build();
	return pasted_length;
}

void MWindow::paste_edls(ArrayList<EDL*> *new_edls,
	int load_mode,
	Track *first_track,
	ptstime current_position,
	int overwrite)
{
	ArrayList<Track*> destination_tracks;
	int need_new_tracks = 0;

	for(int i = 0; i < new_edls->total; i++)
	{
		paste_edl(new_edls->values[i], load_mode,
			first_track, current_position,
			overwrite);

		if(load_mode == LOADMODE_REPLACE ||
				load_mode == LOADMODE_REPLACE_CONCATENATE)
			load_mode = LOADMODE_NEW_TRACKS;
	}
}

void MWindow::paste_silence()
{
	if(cwindow->stop_playback())
		return;

	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();
	master_edl->paste_silence(start,
		end, 
		edlsession->labels_follow_edits);
	master_edl->optimize();
	save_backup();
	undo->update_undo(_("silence"), LOAD_EDITS | LOAD_TIMEBAR);

	update_plugin_guis();
	restart_brender();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::paste_transition()
{
// Only the first transition gets dropped.
	PluginServer *server = mainsession->drag_pluginservers->values[0];

	if(cwindow->stop_playback())
		return;

	if(server->audio)
		strcpy(edlsession->default_atransition, server->title);
	else
		strcpy(edlsession->default_vtransition, server->title);

	insert_transition(server, mainsession->edit_highlighted);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(server->video);
}

void MWindow::insert_transition(PluginServer *server, Edit *dst_edit)
{
	Plugin *transition;
	PluginClient *client;

	transition = dst_edit->insert_transition(server);

	if(server && (client = server->open_plugin(0, 0)))
	{
		client->save_data(transition->keyframes->get_first());
		server->close_plugin(client);
	}
}

void MWindow::paste_transition(int data_type, PluginServer *server, int firstonly)
{
	const char *name = 0;
	ptstime position = master_edl->local_session->get_selectionstart();

	if(!server)
	{
		switch(data_type)
		{
		case TRACK_AUDIO:
			name = edlsession->default_atransition;
			break;
		case TRACK_VIDEO:
			name = edlsession->default_vtransition;
			break;
		}
		if(name)
			server = plugindb.get_pluginserver(name, data_type, 1);

		if(!server)
		{
			show_message(_("No default transition '%s' found."),
				name);
			return;
		}
	}

	if(cwindow->stop_playback())
		return;

	for(Track *track = master_edl->tracks->first; track;
		track = track->next)
	{
		if(track->data_type == data_type &&
			track->record)
		{
			Edit *edit = track->editof(position);

			if(edit)
				insert_transition(server, edit);

			if(firstonly)
				break;
		}
	}
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(data_type == TRACK_VIDEO);

	update_gui(WUPD_CANVINCR);
}

void MWindow::redo_entry()
{
	cwindow->playback_engine->send_command(STOP);
	vwindow->playback_engine->send_command(STOP);

	undo->redo();

	save_backup();
	update_plugin_guis();
	restart_brender();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);
	cwindow->update(WUPD_POSITION | WUPD_OVERLAYS | WUPD_TOOLWIN |
		WUPD_OPERATION | WUPD_TIMEBAR);

	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::resize_track(Track *track, int w, int h)
{
	if(cwindow->stop_playback())
		return;

	track->reset_renderers();
// We have to move all maskpoints so they do not move in relation to image areas
	((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->translate_masks(
		(w - track->track_w) / 2, 
		(h - track->track_h) / 2);
	track->track_w = w;
	track->track_h = h;
	undo->update_undo(_("resize track"), LOAD_ALL);
	save_backup();

	sync_parameters();
}


class InPointUndoItem : public UndoStackItem
{
public:
	InPointUndoItem(ptstime old_position, ptstime new_position, EDL *edl);
	void undo();
	size_t get_size();
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

size_t InPointUndoItem::get_size()
{
	return sizeof(*this);
}

void MWindow::set_inpoint()
{
	InPointUndoItem *undo_item;

	undo_item = new InPointUndoItem(master_edl->local_session->get_inpoint(),
	master_edl->local_session->get_selectionstart(1), master_edl);
	undo->push_undo_item(undo_item);

	master_edl->set_inpoint(master_edl->local_session->get_selectionstart(1));
	save_backup();

	update_gui(WUPD_TIMEBAR);
	cwindow->update(WUPD_TIMEBAR);
}

class OutPointUndoItem : public UndoStackItem
{
public:
	OutPointUndoItem(ptstime old_position, ptstime new_position, EDL *edl);
	void undo();
	size_t get_size();
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

size_t OutPointUndoItem::get_size()
{
	return sizeof(*this);
}

void MWindow::set_outpoint()
{
	OutPointUndoItem *undo_item;

	undo_item = new OutPointUndoItem(master_edl->local_session->get_outpoint(),
		master_edl->local_session->get_selectionend(1), master_edl);
	undo->push_undo_item(undo_item);

	master_edl->set_outpoint(master_edl->local_session->get_selectionend(1));
	save_backup();

	update_gui(WUPD_TIMEBAR);
	cwindow->update(WUPD_TIMEBAR);
}

void MWindow::splice(EDL *source)
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime source_start = source->local_session->get_selectionstart();
	ptstime source_end = source->local_session->get_selectionend();
	EDL edl(0);

	if(cwindow->stop_playback())
		return;

	edl.copy(source, source_start, source_end);
	paste_edl(&edl, LOADMODE_PASTE, 0, start, 0);

	save_backup();
	undo->update_undo(_("splice"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);
	sync_parameters();
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
	clip_edit->create_clip(new_edl);
	save_backup();
}


class LabelUndoItem : public UndoStackItem
{
public:
	LabelUndoItem(ptstime position1, ptstime position2, EDL *edl);
	void undo();
	size_t get_size();
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

size_t LabelUndoItem::get_size()
{
	return sizeof(*this);
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

	update_gui(WUPD_TIMEBAR);
	gui->canvas->activate();
	cwindow->update(WUPD_TIMEBAR);
	awindow->gui->async_update_assets();
}

void MWindow::trim_selection()
{
	if(cwindow->stop_playback())
		return;

	master_edl->trim_selection(master_edl->local_session->get_selectionstart(),
		master_edl->local_session->get_selectionend(),
		edlsession->labels_follow_edits);

	save_backup();
	undo->update_undo(_("trim selection"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	restart_brender();
	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::undo_entry()
{
	cwindow->playback_engine->send_command(STOP);
	vwindow->playback_engine->send_command(STOP);

	undo->undo(); 

	save_backup();
	restart_brender();
	update_plugin_guis();
	update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);
	cwindow->update(WUPD_POSITION | WUPD_OVERLAYS | WUPD_TOOLWIN |
		WUPD_OPERATION | WUPD_TIMEBAR);

	awindow->gui->async_update_assets();
	cwindow->playback_engine->send_command(CURRENT_FRAME);
}

void MWindow::select_point(ptstime position)
{
	master_edl->local_session->set_selection(position);
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	update_plugin_guis();
	update_gui(WUPD_PATCHBAY | WUPD_CURSOR | WUPD_CLOCK | WUPD_ZOOMBAR);
}

void MWindow::map_audio(int pattern)
{
	int current_channel = 0;
	int current_track = 0;

	if(cwindow->stop_playback())
		return;

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
	sync_parameters(0);
	update_gui(WUPD_CANVINCR | WUPD_PATCHBAY);
}
