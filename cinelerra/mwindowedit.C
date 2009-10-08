
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
#include "bcsignals.h"
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
#include "recordlabel.h"
#include "samplescroll.h"
#include "trackcanvas.h"
#include "track.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
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
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
		CHANGE_EDL,
		edl,
		1);
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
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
	save_backup();
}


int MWindow::add_audio_track(int above, Track *dst)
{
	edl->tracks->add_audio_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
	return 0;
}

int MWindow::add_video_track(int above, Track *dst)
{
	edl->tracks->add_video_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
	return 0;
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
			MainError::show_error(
				_("This project's dimensions are not multiples of 4 so\n"
				"it can't be rendered by OpenGL."));
		}


// Get aspect ratio
		if(defaults->get("AUTOASPECT", 0))
		{
			create_aspect_ratio(edl->session->aspect_w, 
				edl->session->aspect_h, 
				w, 
				h);
		}

		save_backup();

		undo->update_undo(_("asset to size"), LOAD_ALL);
		restart_brender();
		sync_parameters(CHANGE_ALL);
	}
}


void MWindow::asset_to_rate()
{
	if(session->drag_assets->total &&
		session->drag_assets->values[0]->video_data)
	{
		double new_framerate = session->drag_assets->values[0]->frame_rate;
		double old_framerate = edl->session->frame_rate;

		edl->session->frame_rate = new_framerate;
		edl->resample(old_framerate, new_framerate, TRACK_VIDEO);

		save_backup();

		undo->update_undo(_("asset to rate"), LOAD_ALL);
		restart_brender();
		gui->update(1,
			2,
			1,
			1,
			1, 
			1,
			0);
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
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_EDL,
	    		   edl,
	    		   1);
}

void MWindow::clear(int clear_handle)
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if(clear_handle || !EQUIV(start, end))
	{
		edl->clear(start, 
			end, 
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits);
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
	cwindow->update(1, 0, 0);
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
	cwindow->update(1, 0, 0);
}

int MWindow::clear_default_keyframe()
{
	edl->tracks->clear_default_keyframe();
	save_backup();
	undo->update_undo(_("clear default keyframe"), LOAD_AUTOMATION);
	
	restart_brender();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(1, 0, 0);
	
	return 0;
}

void MWindow::clear_labels()
{
	clear_labels(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
	undo->update_undo(_("clear labels"), LOAD_TIMEBAR);
	
	gui->timebar->update();
	cwindow->update(0, 0, 0, 0, 1);
	save_backup();
}

int MWindow::clear_labels(double start, double end)
{
	edl->labels->clear(start, end, 0);
	return 0;
}

void MWindow::concatenate_tracks()
{
	edl->tracks->concatenate_tracks(edl->session->plugins_follow_edits);
	save_backup();
	undo->update_undo(_("concatenate tracks"), LOAD_EDITS);

	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
		CHANGE_EDL,
		edl,
		1);
}


void MWindow::copy()
{
	copy(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend());
}

int MWindow::copy(double start, double end)
{
	if(start == end) return 1;

//printf("MWindow::copy 1\n");
	FileXML file;
//printf("MWindow::copy 1\n");
	edl->copy(start, 
		end, 
		0,
		0,
		0,
		&file, 
		plugindb,
		"",
		1);
//printf("MWindow::copy 1\n");

// File is now terminated and rewound

//printf("MWindow::copy 1\n");
	gui->get_clipboard()->to_clipboard(file.string, strlen(file.string), SECONDARY_SELECTION);
//printf("MWindow::copy\n%s\n", file.string);
//printf("MWindow::copy 2\n");
	save_backup();
	return 0;
}

int MWindow::copy_automation()
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
	return 0;
}

int MWindow::copy_default_keyframe()
{
	FileXML file;
	edl->tracks->copy_default_keyframe(&file);
	gui->get_clipboard()->to_clipboard(file.string,
		strlen(file.string),
		SECONDARY_SELECTION);
	return 0;
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

// Recalculate aspect ratio
	if(defaults->get("AUTOASPECT", 0))
	{
		create_aspect_ratio(edl->session->aspect_w, 
			edl->session->aspect_h, 
			edl->session->output_w, 
			edl->session->output_h);
	}

	undo->update_undo(_("crop"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME,
		CHANGE_ALL,
		edl,
		1);
	save_backup();
}

void MWindow::cut()
{

	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	copy(start, end);
	edl->clear(start, 
		end,
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);


	edl->optimize();
	save_backup();
	undo->update_undo(_("cut"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
}

int MWindow::cut_automation()
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
	cwindow->update(1, 0, 0);
	return 0;
}

int MWindow::cut_default_keyframe()
{

	copy_default_keyframe();
	edl->tracks->clear_default_keyframe();
	undo->update_undo(_("cut default keyframe"), LOAD_AUTOMATION);

	restart_brender();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(1, 0, 0);
	save_backup();


	return 0;
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
	gui->update(1, 1, 1, 0, 1, 0, 0);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_EDL,
	    		   edl,
	    		   1);
}

void MWindow::delete_track(Track *track)
{
	edl->tracks->delete_track(track);
	undo->update_undo(_("delete track"), LOAD_ALL);

	restart_brender();
	update_plugin_states();
	gui->update(1, 1, 1, 0, 1, 0, 0);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_EDL,
	    		   edl,
	    		   1);
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
	gui->update(0,
		1,
		0,
		0,
		0, 
		0,
		0);
	sync_parameters(CHANGE_EDL);
}





// Insert data from clipboard
void MWindow::insert(double position, 
	FileXML *file,
	int edit_labels,
	int edit_plugins,
	EDL *parent_edl)
{
// For clipboard pasting make the new edl use a separate session 
// from the master EDL.  Then it can be resampled to the master rates.
// For splice, overwrite, and dragging need same session to get the assets.
	EDL edl(parent_edl);
	ArrayList<EDL*> new_edls;
	uint32_t load_flags = LOAD_ALL;


	new_edls.append(&edl);
	edl.create_objects();




	if(parent_edl) load_flags &= ~LOAD_SESSION;
	if(!edl.session->autos_follow_edits) load_flags &= ~LOAD_AUTOMATION;
	if(!edl.session->labels_follow_edits) load_flags &= ~LOAD_TIMEBAR;

	edl.load_xml(plugindb, file, load_flags);






	paste_edls(&new_edls, 
		LOAD_PASTE, 
		0, 
		position,
		edit_labels,
		edit_plugins,
		0); // overwrite
// if(vwindow->edl)
// printf("MWindow::insert 5 %f %f\n", 
// vwindow->edl->local_session->in_point,
// vwindow->edl->local_session->out_point);
	new_edls.remove_all();
//printf("MWindow::insert 6 %p\n", vwindow->get_edl());
}

void MWindow::insert_effects_canvas(double start,
	double length)
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


	double start = 0;
	double length = dest_track->get_length();

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
	gui->update(1,
		1,
		0,
		0,
		1,
		0,
		0);
}



void MWindow::insert_effect(char *title, 
	SharedLocation *shared_location, 
	Track *track,
	PluginSet *plugin_set,
	double start,
	double length,
	int plugin_type)
{
	KeyFrame *default_keyframe = 0;
	PluginServer *server = 0;






// Get default keyframe
	if(plugin_type == PLUGIN_STANDALONE)
	{
		default_keyframe = new KeyFrame;
		server = new PluginServer(*scan_plugindb(title, track->data_type));

		server->open_plugin(0, preferences, edl, 0, -1);
		server->save_data(default_keyframe);
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


	if(plugin_type == PLUGIN_STANDALONE)
	{
		server->close_plugin();
		delete server;
		delete default_keyframe;
	}
}

int MWindow::modify_edithandles()
{

	edl->modify_edithandles(session->drag_start, 
		session->drag_position, 
		session->drag_handle, 
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);

	finish_modify_handles();


//printf("MWindow::modify_handles 1\n");
	return 0;
}

int MWindow::modify_pluginhandles()
{

	edl->modify_pluginhandles(session->drag_start, 
		session->drag_position, 
		session->drag_handle, 
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->labels_follow_edits,
		session->trim_edits);

	finish_modify_handles();

	return 0;
}


// Common to edithandles and plugin handles
void MWindow::finish_modify_handles()
{
	int edit_mode = edl->session->edit_handle_mode[session->drag_button];

	if((session->drag_handle == 1 && edit_mode != MOVE_NO_EDITS) ||
		(session->drag_handle == 0 && edit_mode == MOVE_ONE_EDIT))
	{
		edl->local_session->set_selectionstart(session->drag_position);
		edl->local_session->set_selectionend(session->drag_position);
	}
	else
	if(edit_mode != MOVE_NO_EDITS)
	{
		edl->local_session->set_selectionstart(session->drag_start);
		edl->local_session->set_selectionend(session->drag_start);
	}

	if(edl->local_session->get_selectionstart(1) < 0)
	{
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(0);
	}

	save_backup();
	undo->update_undo(_("drag handle"), LOAD_EDITS | LOAD_TIMEBAR);
	restart_brender();
	sync_parameters(CHANGE_EDL);
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
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
		double position,
		int behaviour)
{

	edl->tracks->move_edits(edits, 
		track, 
		position,
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits,
		behaviour);

	save_backup();
	undo->update_undo(_("move edit"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
		CHANGE_EDL,
		edl,
		1);

	update_plugin_guis();
	gui->update(1,
		1,      // 1 for incremental drawing.  2 for full refresh
		1,
		0,
		0, 
		0,
		0);
}

void MWindow::move_effect(Plugin *plugin,
	PluginSet *dest_plugin_set,
	Track *dest_track,
	int64_t dest_position)
{

	edl->tracks->move_effect(plugin, 
		dest_plugin_set, 
		dest_track, 
		dest_position);

	save_backup();
	undo->update_undo(_("move effect"), LOAD_ALL);

	restart_brender();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
		CHANGE_EDL,
		edl,
		1);

	update_plugin_guis();
	gui->update(1,
		1,      // 1 for incremental drawing.  2 for full refresh
		0,
		0,
		0, 
		0,
		0);
}

void MWindow::move_plugins_up(PluginSet *plugin_set)
{

	plugin_set->track->move_plugins_up(plugin_set);

	save_backup();
	undo->update_undo(_("move effect up"), LOAD_ALL);
	restart_brender();
	gui->update(1,
		1,      // 1 for incremental drawing.  2 for full refresh
		0,
		0,
		0, 
		0,
		0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_plugins_down(PluginSet *plugin_set)
{

	plugin_set->track->move_plugins_down(plugin_set);

	save_backup();
	undo->update_undo(_("move effect down"), LOAD_ALL);
	restart_brender();
	gui->update(1,
		1,      // 1 for incremental drawing.  2 for full refresh
		0,
		0,
		0, 
		0,
		0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_track_down(Track *track)
{
	edl->tracks->move_track_down(track);
	save_backup();
	undo->update_undo(_("move track down"), LOAD_ALL);

	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_down()
{
	edl->tracks->move_tracks_down();
	save_backup();
	undo->update_undo(_("move tracks down"), LOAD_ALL);

	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_track_up(Track *track)
{
	edl->tracks->move_track_up(track);
	save_backup();
	undo->update_undo(_("move track up"), LOAD_ALL);
	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_up()
{
	edl->tracks->move_tracks_up();
	save_backup();
	undo->update_undo(_("move tracks up"), LOAD_ALL);
	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
}


void MWindow::mute_selection()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if(start != end)
	{
		edl->clear(start, 
			end, 
			0, 
			edl->session->plugins_follow_edits);
		edl->local_session->set_selectionend(end);
		edl->local_session->set_selectionstart(start);
		edl->paste_silence(start, end, 0, edl->session->plugins_follow_edits);
		save_backup();
		undo->update_undo(_("mute"), LOAD_EDITS);

		restart_brender();
		update_plugin_guis();
		gui->update(1, 2, 1, 1, 1, 1, 0);
		cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
								CHANGE_EDL,
								edl,
								1);
	}
}



void MWindow::overwrite(EDL *source)
{
	FileXML file;

	double src_start = source->local_session->get_selectionstart();
	double overwrite_len = source->local_session->get_selectionend() - src_start;
	double dst_start = edl->local_session->get_selectionstart();
	double dst_len = edl->local_session->get_selectionend() - dst_start;

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
			0, 
			0);

	paste(dst_start, 
		dst_start + overwrite_len, 
		&file,
		0,
		0);

	edl->local_session->set_selectionstart(dst_start + overwrite_len);
	edl->local_session->set_selectionend(dst_start + overwrite_len);

	save_backup();
	undo->update_undo(_("overwrite"), LOAD_EDITS);

	restart_brender();
	update_plugin_guis();
	gui->update(1, 1, 1, 1, 0, 1, 0);
	sync_parameters(CHANGE_EDL);
}

// For splice and overwrite
int MWindow::paste(double start, 
	double end, 
	FileXML *file,
	int edit_labels,
	int edit_plugins)
{
	clear(0);

// Want to insert with assets shared with the master EDL.
	insert(start, 
			file,
			edit_labels,
			edit_plugins,
			edl);

	return 0;
}

// For editing use insertion point position
void MWindow::paste()
{

	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	int64_t len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

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
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits);

		edl->optimize();


		delete [] string;



		save_backup();


		undo->update_undo(_("paste"), LOAD_EDITS | LOAD_TIMEBAR);
		restart_brender();
		update_plugin_guis();
		gui->update(1, 2, 1, 1, 0, 1, 0);
		awindow->gui->async_update_assets();
		sync_parameters(CHANGE_EDL);
	}

}

int MWindow::paste_assets(double position, Track *dest_track, int overwrite)
{
	int result = 0;




	if(session->drag_assets->total)
	{
		load_assets(session->drag_assets, 
			position, 
			LOAD_PASTE,
			dest_track, 
			0,
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits,
			overwrite);
		result = 1;
	}


	if(session->drag_clips->total)
	{
		paste_edls(session->drag_clips, 
			LOAD_PASTE, 
			dest_track,
			position, 
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits,
			overwrite); // o
		result = 1;
	}


	save_backup();

	undo->update_undo(_("paste assets"), LOAD_EDITS);
	restart_brender();
	gui->update(1, 
		2,
		1,
		0,
		0,
		1,
		0);
	sync_parameters(CHANGE_EDL);
	return result;
}

void MWindow::load_assets(ArrayList<Asset*> *new_assets, 
	double position, 
	int load_mode,
	Track *first_track,
	RecordLabels *labels,
	int edit_labels,
	int edit_plugins,
	int overwrite)
{
//printf("MWindow::load_assets 1\n");
	if(position < 0) position = edl->local_session->get_selectionstart();

	ArrayList<EDL*> new_edls;
	for(int i = 0; i < new_assets->total; i++)
	{
		remove_asset_from_caches(new_assets->values[i]);
		EDL *new_edl = new EDL;
		new_edl->create_objects();
		new_edl->copy_session(edl);
		new_edls.append(new_edl);


//printf("MWindow::load_assets 2 %d %d\n", new_assets->values[i]->audio_length, new_assets->values[i]->video_length);
		asset_to_edl(new_edl, new_assets->values[i]);


		if(labels)
			for(RecordLabel *label = labels->first; label; label = label->next)
			{
				new_edl->labels->toggle_label(label->position, label->position);
			}
	}
//printf("MWindow::load_assets 3\n");

	paste_edls(&new_edls, 
		load_mode, 
		first_track,
		position,
		edit_labels,
		edit_plugins,
		overwrite);
//printf("MWindow::load_assets 4\n");


	save_backup();
	new_edls.remove_all_objects();

}

int MWindow::paste_automation()
{
	int64_t len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

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
		cwindow->update(1, 0, 0);
	}

	return 0;
}

int MWindow::paste_default_keyframe()
{
	int64_t len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

	if(len)
	{
		char *string = new char[len + 1];
		gui->get_clipboard()->from_clipboard(string, 
			len, 
			SECONDARY_SELECTION);
		FileXML file;
		file.read_from_string(string);
		edl->tracks->paste_default_keyframe(&file); 
		undo->update_undo(_("paste default keyframe"), LOAD_AUTOMATION);


		restart_brender();
		update_plugin_guis();
		gui->canvas->draw_overlays();
		gui->canvas->flash();
		sync_parameters(CHANGE_PARAMS);
		gui->patchbay->update();
		cwindow->update(1, 0, 0);
		delete [] string;
		save_backup();
	}

	return 0;
}


// Insert edls with project deletion and index file generation.
int MWindow::paste_edls(ArrayList<EDL*> *new_edls, 
	int load_mode, 
	Track *first_track,
	double current_position,
	int edit_labels,
	int edit_plugins,
	int overwrite)
{

	ArrayList<Track*> destination_tracks;
	int need_new_tracks = 0;

	if(!new_edls->total) return 0;


SET_TRACE
	double original_length = edl->tracks->total_playable_length();
SET_TRACE
	double original_preview_end = edl->local_session->preview_end;
SET_TRACE

// Delete current project
	if(load_mode == LOAD_REPLACE ||
		load_mode == LOAD_REPLACE_CONCATENATE)
	{
		reset_caches();

		edl->save_defaults(defaults);

		hide_plugins();

		delete edl;

		edl = new EDL;

		edl->create_objects();

		edl->copy_session(new_edls->values[0]);

		gui->mainmenu->update_toggles(0);


		gui->unlock_window();

		gwindow->gui->update_toggles(1);

		gui->lock_window("MWindow::paste_edls");


// Insert labels for certain modes constitutively
		edit_labels = 1;
		edit_plugins = 1;
// Force reset of preview
		original_length = 0;
		original_preview_end = -1;
	}

SET_TRACE


SET_TRACE
// Create new tracks in master EDL
	if(load_mode == LOAD_REPLACE || 
		load_mode == LOAD_REPLACE_CONCATENATE ||
		load_mode == LOAD_NEW_TRACKS)
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
			if(load_mode == LOAD_REPLACE_CONCATENATE) break;
		}

	}
	else
// Recycle existing tracks of master EDL
	if(load_mode == LOAD_CONCATENATE || load_mode == LOAD_PASTE)
	{

// The point of this is to shift forward labels after the selection so they can
// then be shifted back to their original locations without recursively
// shifting back every paste.
		if(load_mode == LOAD_PASTE && edl->session->labels_follow_edits)
			edl->labels->clear(edl->local_session->get_selectionstart(),
						edl->local_session->get_selectionend(),
						1);
	
		Track *current = first_track ? first_track : edl->tracks->first;
		for( ; current; current = NEXT)
		{
			if(current->record)
			{
				destination_tracks.append(current);

// This should be done in the caller so we don't get recursive clear disease.
// 				if(load_mode == LOAD_PASTE)
// 					current->clear(edl->local_session->get_selectionstart(),
// 						edl->local_session->get_selectionend(),
// 						1,
// 						edl->session->labels_follow_edits, 
// 						edl->session->plugins_follow_edits,
// 						1);
			}
		}

	}




	int destination_track = 0;
	double *paste_position = new double[destination_tracks.total];



SET_TRACE


// Iterate through the edls
	for(int i = 0; i < new_edls->total; i++)
	{

		EDL *new_edl = new_edls->values[i];
SET_TRACE
		double edl_length = new_edl->local_session->clipboard_length ?
			new_edl->local_session->clipboard_length :
			new_edl->tracks->total_length();
// printf("MWindow::paste_edls 2\n");
// new_edl->dump();

SET_TRACE



// Resample EDL to master rates
		new_edl->resample(new_edl->session->sample_rate, 
			edl->session->sample_rate, 
			TRACK_AUDIO);
		new_edl->resample(new_edl->session->frame_rate, 
			edl->session->frame_rate, 
			TRACK_VIDEO);

SET_TRACE



// Add assets and prepare index files
		for(Asset *new_asset = new_edl->assets->first;
			new_asset;
			new_asset = new_asset->next)
		{
			mainindexes->add_next_asset(0, new_asset);
		}
SET_TRACE
// Capture index file status from mainindex test
		edl->update_assets(new_edl);

SET_TRACE


// Get starting point of insertion.  Need this to paste labels.
		switch(load_mode)
		{
			case LOAD_REPLACE:
			case LOAD_NEW_TRACKS:
		  	 	current_position = 0;
				break;

			case LOAD_CONCATENATE:
			case LOAD_REPLACE_CONCATENATE:
				destination_track = 0;
		  	 	if(destination_tracks.total)
					current_position = destination_tracks.values[0]->get_length();
				else
					current_position = 0;
				break;

			case LOAD_PASTE:
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

			case LOAD_RESOURCESONLY:
				edl->add_clip(new_edl);
				break;
		}




SET_TRACE

// Insert edl
		if(load_mode != LOAD_RESOURCESONLY)
		{
// Insert labels
//printf("MWindow::paste_edls %f %f\n", current_position, edl_length);
			if(load_mode == LOAD_PASTE)
				edl->labels->insert_labels(new_edl->labels, 
					destination_tracks.total ? paste_position[0] : 0.0,
					edl_length,
					edit_labels);
			else
				edl->labels->insert_labels(new_edl->labels, 
					current_position,
					edl_length,
					edit_labels);

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

//printf("MWindow::paste_edls 1 %d\n", replace_default);
// Insert new track at current position
					switch(load_mode)
					{
						case LOAD_REPLACE_CONCATENATE:
						case LOAD_CONCATENATE:
							current_position = track->get_length();
							break;

						case LOAD_PASTE:
							current_position = paste_position[destination_track];
							paste_position[destination_track] += new_track->get_length();
							break;
					}
					if (overwrite)
						track->clear(current_position, 
								current_position + new_track->get_length(), 
								1, // edit edits
								edit_labels,
								edit_plugins, 
								1, // convert units
								0); // trim edits


					track->insert_track(new_track, 
						current_position, 
						replace_default,
						edit_plugins);
				}

// Get next destination track
				destination_track++;
				if(destination_track >= destination_tracks.total)
					destination_track = 0;
			}
		}

SET_TRACE
		if(load_mode == LOAD_PASTE)
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
			edl->vwindow_edl->create_objects();
			edl->vwindow_edl->copy_all(new_edl->vwindow_edl);
		}
	}


SET_TRACE
	if(paste_position) delete [] paste_position;


SET_TRACE
// This is already done in load_filenames and everything else that uses paste_edls
//	update_project(load_mode);

// Fix preview range
	if(EQUIV(original_length, original_preview_end))
	{
		edl->local_session->preview_end = edl->tracks->total_playable_length();
	}


SET_TRACE
// Start examining next batch of index files
	mainindexes->start_build();
SET_TRACE


// Don't save a backup after loading since the loaded file is on disk already.


	return 0;
}

void MWindow::paste_silence()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	edl->paste_silence(start, 
		end, 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);
	edl->optimize();
	save_backup();
	undo->update_undo(_("silence"), LOAD_EDITS | LOAD_TIMEBAR);

	update_plugin_guis();
	restart_brender();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
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
	gui->update(0, 1, 0, 0, 0, 0, 0);
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_audio_transition()
{
 	PluginServer *server = scan_plugindb(edl->session->default_atransition,
		TRACK_AUDIO);
	if(!server)
	{
		char string[BCTEXTLEN];
		sprintf(string, _("No default transition %s found."), edl->session->default_atransition);
		gui->show_message(string);
		return;
	}

	edl->tracks->paste_audio_transition(server);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_ALL);
	gui->update(0, 1, 0, 0, 0, 0, 0);
}

void MWindow::paste_video_transition()
{
 	PluginServer *server = scan_plugindb(edl->session->default_vtransition,
		TRACK_VIDEO);
	if(!server)
	{
		char string[BCTEXTLEN];
		sprintf(string, _("No default transition %s found."), edl->session->default_vtransition);
		gui->show_message(string);
		return;
	}


	edl->tracks->paste_video_transition(server);
	save_backup();
	undo->update_undo(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_ALL);
	restart_brender();
	gui->update(0, 1, 0, 0, 0, 0, 0);
}


void MWindow::redo_entry(BC_WindowBase *calling_window_gui)
{

	calling_window_gui->unlock_window();

	cwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	vwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	cwindow->playback_engine->interrupt_playback(0);
	vwindow->playback_engine->interrupt_playback(0);


	cwindow->gui->lock_window("MWindow::redo_entry");
	vwindow->gui->lock_window("MWindow::undo_entry 2");
	gui->lock_window();

	undo->redo(); 

	save_backup();
	update_plugin_states();
	update_plugin_guis();
	restart_brender();
	gui->update(1, 2, 1, 1, 1, 1, 1);
	cwindow->update(1, 1, 1, 1, 1);

	if (calling_window_gui != cwindow->gui) 
		cwindow->gui->unlock_window();
	if (calling_window_gui != gui)
		gui->unlock_window();
	if (calling_window_gui != vwindow->gui)
		vwindow->gui->unlock_window();

	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_ALL,
	    		   edl,
	    		   1);
	
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
	InPointUndoItem(double old_position, double new_position, EDL *edl);
	void undo();
	int get_size();
private:
	double old_position;
	double new_position;
	EDL *edl;
};

InPointUndoItem::InPointUndoItem(
      double old_position, double new_position, EDL *edl)
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
	double tmp = new_position;
	new_position = old_position;
	old_position = tmp;
}

int InPointUndoItem::get_size()
{
   return 20;
}

void MWindow::set_inpoint(int is_mwindow)
{
   InPointUndoItem *undo_item;

   undo_item = new InPointUndoItem(edl->local_session->get_inpoint(),
         edl->local_session->get_selectionstart(1), edl);
   undo->push_undo_item(undo_item);

	edl->set_inpoint(edl->local_session->get_selectionstart(1));
	save_backup();

	if(!is_mwindow)
	{
		gui->lock_window("MWindow::set_inpoint 1");
	}
	gui->timebar->update();
	gui->flush();
	if(!is_mwindow)
	{
		gui->unlock_window();
	}

	if(is_mwindow)
	{
		cwindow->gui->lock_window("MWindow::set_inpoint 2");
	}
	cwindow->gui->timebar->update();
	cwindow->gui->flush();
	if(is_mwindow)
	{
		cwindow->gui->unlock_window();
	}
}

class OutPointUndoItem : public UndoStackItem
{
public:
	OutPointUndoItem(double old_position, double new_position, EDL *edl);
	void undo();
	int get_size();
private:
	double old_position;
	double new_position;
	EDL *edl;
};

OutPointUndoItem::OutPointUndoItem(
      double old_position, double new_position, EDL *edl)
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
	double tmp = new_position;
	new_position = old_position;
	old_position = tmp;
}

int OutPointUndoItem::get_size()
{
   return 20;
}

void MWindow::set_outpoint(int is_mwindow)
{
   OutPointUndoItem *undo_item;

   undo_item = new OutPointUndoItem(edl->local_session->get_outpoint(),
         edl->local_session->get_selectionend(1), edl);
   undo->push_undo_item(undo_item);

	edl->set_outpoint(edl->local_session->get_selectionend(1));
	save_backup();

	if(!is_mwindow)
	{
		gui->lock_window("MWindow::set_outpoint 1");
	}
	gui->timebar->update();
	gui->flush();
	if(!is_mwindow)
	{
		gui->unlock_window();
	}

	if(is_mwindow)
	{
		cwindow->gui->lock_window("MWindow::set_outpoint 2");
	}
	cwindow->gui->timebar->update();
	cwindow->gui->flush();
	if(is_mwindow)
	{
		cwindow->gui->unlock_window();
	}
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



//file.dump();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	double source_start = source->local_session->get_selectionstart();
	double source_end = source->local_session->get_selectionend();

	paste(start, 
		start, 
		&file,
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits);

// Position at end of clip
	edl->local_session->set_selectionstart(start + 
		source_end - 
		source_start);
	edl->local_session->set_selectionend(start + 
		source_end - 
		source_start);

	save_backup();
	undo->update_undo(_("splice"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	restart_brender();
	gui->update(1, 1, 1, 1, 0, 1, 0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::to_clip()
{
	FileXML file;
	double start, end;
	
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
	new_edl->create_objects();
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

	new_edl->local_session->set_selectionstart(0);
	new_edl->local_session->set_selectionend(0);

	awindow->clip_edit->create_clip(new_edl);
	save_backup();
}

class LabelUndoItem : public UndoStackItem
{
public:
      LabelUndoItem(double position1, double position2, EDL *edl);
      void undo();
      int get_size();
private:
      double position1;
      double position2;
      EDL *edl;
};

LabelUndoItem::LabelUndoItem(
      double position1, double position2, EDL *edl)
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

int MWindow::toggle_label(int is_mwindow)
{
   LabelUndoItem *undo_item;
	double position1, position2;

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

	position1 = edl->align_to_frame(position1, 0);
	position2 = edl->align_to_frame(position2, 0);

//printf("MWindow::toggle_label 1\n");
   undo_item = new LabelUndoItem(position1, position2, edl);
   undo->push_undo_item(undo_item);

	edl->labels->toggle_label(position1, position2);
	save_backup();

	if(!is_mwindow)
	{
		gui->lock_window("MWindow::toggle_label 1");
	}
	gui->timebar->update();
	gui->canvas->activate();
	gui->flush();
	if(!is_mwindow)
	{
		gui->unlock_window();
	}

	if(is_mwindow)
	{
		cwindow->gui->lock_window("MWindow::toggle_label 2");
	}
	cwindow->gui->timebar->update();
	cwindow->gui->flush();
	if(is_mwindow)
	{
		cwindow->gui->unlock_window();
	}

	return 0;
}

void MWindow::trim_selection()
{


	edl->trim_selection(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend(), 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);

	save_backup();
	undo->update_undo(_("trim selection"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	restart_brender();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
}



void MWindow::undo_entry(BC_WindowBase *calling_window_gui)
{
//	if(is_mwindow)
//		gui->unlock_window();
//	else
//		cwindow->gui->unlock_window();
	calling_window_gui->unlock_window();

	cwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	vwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	cwindow->playback_engine->interrupt_playback(0);
	vwindow->playback_engine->interrupt_playback(0);

	cwindow->gui->lock_window("MWindow::undo_entry 1");
	vwindow->gui->lock_window("MWindow::undo_entry 4");
	gui->lock_window("MWindow::undo_entry 2");

	undo->undo(); 

	save_backup();
	restart_brender();
	update_plugin_states();
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 1);
	cwindow->update(1, 1, 1, 1, 1);

//	if(is_mwindow)
//		cwindow->gui->unlock_window();
//	else
//		gui->unlock_window();
	if (calling_window_gui != cwindow->gui) 
		cwindow->gui->unlock_window();
	if (calling_window_gui != gui)
		gui->unlock_window();
	if (calling_window_gui != vwindow->gui)
		vwindow->gui->unlock_window();
	

	awindow->gui->async_update_assets();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_ALL,
	    		   edl,
	    		   1);
}



void MWindow::new_folder(char *new_folder)
{
	edl->new_folder(new_folder);
	undo->update_undo(_("new folder"), LOAD_ALL);
	awindow->gui->async_update_assets();
}

void MWindow::delete_folder(char *folder)
{
//	undo->update_undo(_("delete folder"), LOAD_ALL);
}

void MWindow::select_point(double position)
{
	edl->local_session->set_selectionstart(position);
	edl->local_session->set_selectionend(position);

// Que the CWindow
	cwindow->update(1, 0, 0, 0, 1);
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
	undo->update_undo(_("map 1:1"), LOAD_AUTOMATION, 0);
	sync_parameters(CHANGE_PARAMS);
	gui->update(0,
		1,
		0,
		0,
		1,
		0,
		0);
}

