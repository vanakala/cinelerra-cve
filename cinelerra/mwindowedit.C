#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
#include "cache.h"
#include "clip.h"
#include "clipedit.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "labels.h"
#include "levelwindow.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
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
#include "vplayback.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zoombar.h"



#include <string.h>






void MWindow::add_audio_track_entry()
{
	undo->update_undo_before("add track", LOAD_ALL);
	add_audio_track();
	save_backup();
	undo->update_undo_after();

	restart_brender();
	gui->get_scrollbars();
	gui->canvas->draw();
	gui->patchbay->update();
	gui->cursor->draw();
	gui->canvas->flash();
	gui->canvas->activate();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
}

void MWindow::add_video_track_entry()
{
	undo->update_undo_before("add track", LOAD_ALL);
	add_video_track();
	undo->update_undo_after();

	restart_brender();
	gui->get_scrollbars();
	gui->canvas->draw();
	gui->patchbay->update();
	gui->cursor->draw();
	gui->canvas->flash();
	gui->canvas->activate();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
	save_backup();
}


int MWindow::add_audio_track()
{
	edl->tracks->add_audio_track(1);
	edl->tracks->update_y_pixels(theme);
	save_backup();
	return 0;
}

int MWindow::add_video_track()
{
	edl->tracks->add_video_track(0);
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

		w = session->drag_assets->values[0]->width;
		h = session->drag_assets->values[0]->height;

		undo->update_undo_before("asset to size", LOAD_ALL);
		edl->session->output_w = w;
		edl->session->output_h = h;
		save_backup();

		undo->update_undo_after();
		restart_brender();
		sync_parameters(CHANGE_ALL);
	}
}





void MWindow::clear()
{
	undo->update_undo_before("clear", LOAD_EDITS | LOAD_TIMEBAR);
	edl->clear(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend(), 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);

	edl->optimize();
	save_backup();
	undo->update_undo_after();

	restart_brender();
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_EDL,
	    		   edl,
	    		   1);

}

void MWindow::clear_automation()
{
	undo->update_undo_before("clear keyframes", LOAD_AUTOMATION); 
	edl->tracks->clear_automation(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
	save_backup();
	undo->update_undo_after(); 

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
	undo->update_undo_before("clear default keyframe", LOAD_AUTOMATION); 
	edl->tracks->clear_default_keyframe();
	save_backup();
	undo->update_undo_after();
	
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
	undo->update_undo_before("clear", LOAD_TIMEBAR);
	clear_labels(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
	undo->update_undo_after(); 
	
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
	undo->update_undo_before("concatenate tracks", LOAD_EDITS);
	edl->tracks->concatenate_tracks(edl->session->plugins_follow_edits);
	save_backup();
	undo->update_undo_after();

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
	undo->update_undo_before("crop", LOAD_ALL);


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
	undo->update_undo_after();

	restart_brender();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME,
		CHANGE_ALL,
		edl,
		1);
	save_backup();
}

void MWindow::cut()
{
	undo->update_undo_before("cut", LOAD_EDITS | LOAD_TIMEBAR);

	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	copy(start, end);
	edl->clear(start, 
		end,
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);


	edl->optimize();
	save_backup();
	undo->update_undo_after();

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
	undo->update_undo_before("cut keyframes", LOAD_AUTOMATION);
	
	copy_automation();

	edl->tracks->clear_automation(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
	save_backup();
	undo->update_undo_after(); 


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
	undo->update_undo_before("cut default keyframe", LOAD_AUTOMATION);

	copy_default_keyframe();
	edl->tracks->clear_default_keyframe();
	undo->update_undo_after();

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
	edl->local_session->in_point = -1;
	save_backup();
}

void MWindow::delete_outpoint()
{
	edl->local_session->out_point = -1;
	save_backup();
}

void MWindow::delete_track()
{
	undo->update_undo_before("delete track", LOAD_ALL);
	edl->tracks->delete_track();
	undo->update_undo_after();
	save_backup();

	restart_brender();
	update_plugin_states();
	gui->update(1, 1, 1, 0, 1, 0, 0);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_EDL,
	    		   edl,
	    		   1);
}

void MWindow::delete_tracks()
{
	undo->update_undo_before("delete tracks", LOAD_ALL);
	edl->tracks->delete_tracks();
	undo->update_undo_after();
	save_backup();

	restart_brender();
	update_plugin_states();
	gui->update(1, 1, 1, 0, 1, 0, 0);
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_EDL,
	    		   edl,
	    		   1);
//printf("MWindow::delete_tracks 5\n");
}

void MWindow::delete_track(Track *track)
{
	undo->update_undo_before("delete track", LOAD_ALL);
	edl->tracks->delete_track(track);
	undo->update_undo_after();

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
	undo->update_undo_before("detach transition", LOAD_ALL);
	hide_plugin(transition, 1);
	int is_video = (transition->edit->track->data_type == TRACK_VIDEO);
	transition->edit->detach_transition();
	save_backup();
	undo->update_undo_after();

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
	unsigned long load_flags = LOAD_ALL;

	new_edls.append(&edl);
	edl.create_objects();


//printf("MWindow::insert 1 %p\n", edl.local_session);

	if(parent_edl) load_flags &= ~LOAD_SESSION;
	if(!edl.session->autos_follow_edits) load_flags &= ~LOAD_AUTOMATION;
//printf("MWindow::insert 2 %p\n", vwindow->get_edl());
	if(!edl.session->labels_follow_edits) load_flags &= ~LOAD_TIMEBAR;
//printf("MWindow::insert 3 %d\n", this->edl->session->sample_rate);
	edl.load_xml(plugindb, file, load_flags);
//printf("MWindow::insert 4 %d\n", this->edl->session->sample_rate);




//file->dump();
//edl.dump();
// if(vwindow->edl)
// printf("MWindow::insert 4 %f %f\n", 
// vwindow->edl->local_session->in_point,
// vwindow->edl->local_session->out_point);

	paste_edls(&new_edls, 
		LOAD_PASTE, 
		0, 
		position,
		edit_labels,
		edit_plugins);
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

	undo->update_undo_before("insert effect", LOAD_EDITS | LOAD_PATCHES);

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
	undo->update_undo_after();
	restart_brender();
	sync_parameters(CHANGE_EDL);
// GUI updated in TrackCanvas, after current_operations are reset
}

void MWindow::insert_effects_cwindow(Track *dest_track)
{
	if(!dest_track) return;

	undo->update_undo_before("insert effect", LOAD_EDITS | LOAD_PATCHES);

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
	undo->update_undo_after();
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
		server = new PluginServer(*scan_plugindb(title));

		server->open_plugin(0, edl, 0);
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
	undo->update_undo_before("drag handle", LOAD_EDITS | LOAD_TIMEBAR);





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
	undo->update_undo_before("drag handle", LOAD_EDITS | LOAD_TIMEBAR);

	edl->modify_pluginhandles(session->drag_start, 
		session->drag_position, 
		session->drag_handle, 
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->labels_follow_edits);

	finish_modify_handles();

	return 0;
}


// Common to edithandles and plugin handles
void MWindow::finish_modify_handles()
{


//printf("TrackCanvas::end_handle_selection 1\n");
	if(session->drag_handle == 1) 
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 
			session->drag_position;
	else
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 
			session->drag_start;

//printf("TrackCanvas::end_handle_selection 1\n");
	if(edl->local_session->selectionstart < 0) 
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 0;
//printf("TrackCanvas::end_handle_selection 1\n");

	save_backup();
	undo->update_undo_after();
	restart_brender();
	sync_parameters(CHANGE_EDL);
//printf("TrackCanvas::end_handle_selection 1\n");
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
//printf("TrackCanvas::end_handle_selection 2\n");


}

void MWindow::match_output_size(Track *track)
{
    undo->update_undo_before("match output size", LOAD_ALL);
	track->track_w = edl->session->output_w;
	track->track_h = edl->session->output_h;
	save_backup();
	undo->update_undo_after();

	restart_brender();
	sync_parameters(CHANGE_EDL);
}


void MWindow::move_edits(ArrayList<Edit*> *edits, 
		Track *track,
		double position)
{
	undo->update_undo_before("move edit", LOAD_ALL);

	edl->tracks->move_edits(edits, 
		track, 
		position,
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);

	save_backup();
	undo->update_undo_after();

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
	long dest_position)
{
	undo->update_undo_before("move effect", LOAD_ALL);

	edl->tracks->move_effect(plugin, 
		dest_plugin_set, 
		dest_track, 
		dest_position);

	save_backup();
	undo->update_undo_after();

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
	undo->update_undo_before("move effect up", LOAD_ALL);

	plugin_set->track->move_plugins_up(plugin_set);

	save_backup();
	undo->update_undo_after();
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
	undo->update_undo_before("move effect down", LOAD_ALL);

	plugin_set->track->move_plugins_down(plugin_set);

	save_backup();
	undo->update_undo_after();
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
	undo->update_undo_before("move track down", LOAD_ALL);
	edl->tracks->move_track_down(track);
	save_backup();
	undo->update_undo_after();

	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_down()
{
	undo->update_undo_before("move tracks down", LOAD_ALL);
	edl->tracks->move_tracks_down();
	save_backup();
	undo->update_undo_after();

	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_track_up(Track *track)
{
	undo->update_undo_before("move track up", LOAD_ALL);
	edl->tracks->move_track_up(track);
	save_backup();
	undo->update_undo_after();
	restart_brender();
	gui->update(1, 1, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_up()
{
	undo->update_undo_before("move tracks up", LOAD_ALL);
	edl->tracks->move_tracks_up();
	save_backup();
	undo->update_undo_after();
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
		undo->update_undo_before("mute", LOAD_EDITS);


		edl->clear(start, 
			end, 
			0, 
			edl->session->plugins_follow_edits);
		edl->local_session->selectionend = end;
		edl->local_session->selectionstart = start;
		edl->paste_silence(start, end, 0, edl->session->plugins_follow_edits);
		save_backup();
		undo->update_undo_after();

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
	source->copy(source->local_session->in_point, 
		source->local_session->out_point, 
		1,
		0,
		0,
		&file,
		plugindb,
		"",
		1);
	undo->update_undo_before("overwrite", LOAD_EDITS);

	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
// Calculate overwrite length from length of source
	if(EQUIV(end, start))
	{
		end = start + 
			source->local_session->out_point - 
			source->local_session->in_point;
	}


	paste(start, 
		end, 
		&file,
		0,
		0);
	edl->local_session->selectionstart = 
		edl->local_session->selectionend =
		start + 
		source->local_session->out_point - 
		source->local_session->in_point;
	save_backup();
	undo->update_undo_after();

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
//printf("MWindow::paste 1\n");
	if(!EQUIV(start, end))
	{
		edl->clear(start, 
			end,
			edit_labels,
			edit_plugins);
	}

// Want to insert with assets shared with the master EDL.
	insert(start, 
			file,
			edit_labels,
			edit_plugins,
			edl);

//printf("MWindow::paste 2\n");
	return 0;
}

// For editing use insertion point position
void MWindow::paste()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	long len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);
	char *string = new char[len + 1];

	gui->get_clipboard()->from_clipboard(string, 
		len, 
		SECONDARY_SELECTION);
	FileXML file;
	file.read_from_string(string);


	undo->update_undo_before("paste", LOAD_EDITS | LOAD_TIMEBAR);


	if(!EQUIV(start, end))
	{
		edl->clear(start, 
			end, 
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits);
	}

	insert(start, 
		&file, 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);
	edl->optimize();

	delete [] string;


	save_backup();


	undo->update_undo_after();
	restart_brender();
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 0, 1, 0);
	awindow->gui->update_assets();
	sync_parameters(CHANGE_EDL);
}

int MWindow::paste_assets(double position, Track *dest_track)
{
	int result = 0;

//printf("MWindow::paste_assets 1\n");
	undo->update_undo_before("paste assets", LOAD_EDITS);
//printf("MWindow::paste_assets 2\n");



	if(session->drag_assets->total)
	{
//printf("MWindow::paste_assets 3\n");
		load_assets(session->drag_assets, 
			position, 
			LOAD_PASTE,
			dest_track, 
			0,
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits);
		result = 1;
	}



//printf("TrackCanvas::drag_stop 4 %d\n", session->drag_clips->total);
//printf("MWindow::paste_assets 4\n");

	if(session->drag_clips->total)
	{
		paste_edls(session->drag_clips, 
			LOAD_PASTE, 
			dest_track,
			position, 
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits);
		result = 1;
	}


	save_backup();

//printf("MWindow::paste_assets 5\n");
	undo->update_undo_after();
	restart_brender();
//printf("MWindow::paste_assets 6\n");
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
	int edit_plugins)
{
//printf("MWindow::load_assets 1\n");
	if(position < 0) position = edl->local_session->get_selectionstart();

	ArrayList<EDL*> new_edls;
	for(int i = 0; i < new_assets->total; i++)
	{
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
		edit_plugins);
//printf("MWindow::load_assets 4\n");


	save_backup();
	new_edls.remove_all_objects();

}

int MWindow::paste_automation()
{
	long len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);

	char *string = new char[len + 1];
	gui->get_clipboard()->from_clipboard(string, 
		len, 
		SECONDARY_SELECTION);
	FileXML file;
	file.read_from_string(string);

	undo->update_undo_before("paste keyframes", LOAD_AUTOMATION); 
	edl->tracks->clear_automation(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend()); 
	edl->tracks->paste_automation(edl->local_session->get_selectionstart(), 
		&file,
		0); 
	save_backup();
	undo->update_undo_after(); 
	delete [] string;


	restart_brender();
	update_plugin_guis();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(1, 0, 0);

	return 0;
}

int MWindow::paste_default_keyframe()
{
	long len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);
	char *string = new char[len + 1];
	gui->get_clipboard()->from_clipboard(string, 
		len, 
		SECONDARY_SELECTION);
	FileXML file;
	file.read_from_string(string);
	undo->update_undo_before("paste default keyframe", LOAD_AUTOMATION); 
	edl->tracks->paste_default_keyframe(&file); 
	undo->update_undo_after(); 


	restart_brender();
	update_plugin_guis();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
	gui->patchbay->update();
	cwindow->update(1, 0, 0);
	delete [] string;
	save_backup();

	return 0;
}


// Insert edls with project deletion and index file generation.
int MWindow::paste_edls(ArrayList<EDL*> *new_edls, 
	int load_mode, 
	Track *first_track,
	double current_position,
	int edit_labels,
	int edit_plugins)
{
	ArrayList<Track*> destination_tracks;
	int need_new_tracks = 0;

	if(!new_edls->total) return 0;
//printf("MWindow::paste_edls 1\n");

// Delete current project
	if(load_mode == LOAD_REPLACE ||
		load_mode == LOAD_REPLACE_CONCATENATE)
	{
		edl->save_defaults(defaults);
		hide_plugins();
		delete edl;
		edl = new EDL;
		edl->create_objects();
		edl->copy_session(new_edls->values[0]);
		gui->mainmenu->update_toggles();

// Insert labels for certain modes constitutively
		edit_labels = 1;
		edit_plugins = 1;
	}
//printf("MWindow::paste_edls 2\n");

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
//printf("MWindow::paste_edls 2\n");

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
					edl->tracks->add_video_track(1);
					if(current->draw) edl->tracks->last->draw = 1;
					destination_tracks.append(edl->tracks->last);
				}
				else
				if(current->data_type == TRACK_AUDIO)
				{
					edl->tracks->add_audio_track(1);
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

				if(load_mode == LOAD_PASTE)
					current->clear(edl->local_session->get_selectionstart(),
						edl->local_session->get_selectionend(),
						1,
						edl->session->labels_follow_edits, 
						edl->session->plugins_follow_edits,
						1);
			}
		}
	}


//printf("MWindow::paste_edls 2\n");

	int destination_track = 0;
	double *paste_position = new double[destination_tracks.total];





//printf("MWindow::paste_edls 2\n");
// Iterate through the edls
	for(int i = 0; i < new_edls->total; i++)
	{
		EDL *new_edl = new_edls->values[i];
		double edl_length = new_edl->local_session->clipboard_length ?
			new_edl->local_session->clipboard_length :
			new_edl->tracks->total_length();

// Resample EDL to master rates
		new_edl->resample(new_edl->session->sample_rate, 
			edl->session->sample_rate, 
			TRACK_AUDIO);
		new_edl->resample(new_edl->session->frame_rate, 
			edl->session->frame_rate, 
			TRACK_VIDEO);



//printf("MWindow::paste_edls 2 %d\n", new_edl->assets->total());
// Add assets and prepare index files
		edl->update_assets(new_edl);
		for(Asset *new_asset = edl->assets->first;
			new_asset;
			new_asset = new_asset->next)
		{
			mainindexes->add_next_asset(new_asset);
		}


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




// Insert edl
		if(load_mode != LOAD_RESOURCESONLY)
		{
// Insert labels
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
// Replace default keyframes if first EDL and new tracks were created
					int replace_default = (i == 0) && need_new_tracks;

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

		if(load_mode == LOAD_PASTE)
			current_position += edl_length;
	}
//printf("MWindow::paste_edls 3\n");

	delete [] paste_position;

// This is already done in load_filenames and everything else that uses paste_edls
//	update_project(load_mode);

//printf("MWindow::paste_edls 5\n");

// Start examining next batch of index files
	mainindexes->start_build();
//printf("MWindow::paste_edls 6\n");

// Don't save a backup after loading since the loaded file is on disk already.

	return 0;
}

void MWindow::paste_silence()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
    undo->update_undo_before("silence", LOAD_EDITS | LOAD_TIMEBAR);
	edl->paste_silence(start, 
		end, 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);
	edl->optimize();
	save_backup();
	undo->update_undo_after();

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
    undo->update_undo_before("transition", LOAD_EDITS);
// Only the first transition gets dropped.
 	PluginServer *server = session->drag_pluginservers->values[0];
	if(server->audio)
		strcpy(edl->session->default_atransition, server->title);
	else
		strcpy(edl->session->default_vtransition, server->title);

	edl->tracks->paste_transition(server, session->edit_highlighted);
	save_backup();
	undo->update_undo_after();

	if(server->video) restart_brender();
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_transition_cwindow(Track *dest_track)
{
	undo->update_undo_before("transition", LOAD_EDITS);
	PluginServer *server = session->drag_pluginservers->values[0];
	edl->tracks->paste_video_transition(server, 1);
	save_backup();
	restart_brender();
	gui->update(0, 1, 0, 0, 0, 0, 0);
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_audio_transition()
{
 	PluginServer *server = scan_plugindb(edl->session->default_atransition);
	if(!server)
	{
		char string[BCTEXTLEN];
		sprintf(string, "No default transition %s found.", edl->session->default_atransition);
		gui->show_message(string);
		return;
	}

    undo->update_undo_before("paste transition", LOAD_EDITS);
	edl->tracks->paste_audio_transition(server);
	save_backup();
	undo->update_undo_after();

	sync_parameters(CHANGE_ALL);
	gui->update(0, 1, 0, 0, 0, 0, 0);
}

void MWindow::paste_video_transition()
{
 	PluginServer *server = scan_plugindb(edl->session->default_vtransition);
	if(!server)
	{
		char string[BCTEXTLEN];
		sprintf(string, "No default transition %s found.", edl->session->default_vtransition);
		gui->show_message(string);
		return;
	}


    undo->update_undo_before("paste transition", LOAD_EDITS);
	edl->tracks->paste_video_transition(server);
	save_backup();
	undo->update_undo_after();

	sync_parameters(CHANGE_ALL);
	restart_brender();
	gui->update(0, 1, 0, 0, 0, 0, 0);
}

void MWindow::redo_entry(int is_mwindow)
{
	if(is_mwindow)
		gui->unlock_window();
	else
		cwindow->gui->unlock_window();


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


	cwindow->gui->lock_window();
	gui->lock_window();

	undo->redo(); 

	save_backup();
	update_plugin_states();
	update_plugin_guis();
	restart_brender();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 1, 1, 1, 1);

	if(is_mwindow)
		cwindow->gui->unlock_window();
	else
		gui->unlock_window();

	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_ALL,
	    		   edl,
	    		   1);
	
}


void MWindow::resize_track(Track *track, int w, int h)
{
	undo->update_undo_before("resize track", LOAD_ALL);
	track->track_w = w;
	track->track_h = h;
	undo->update_undo_after();
	save_backup();

	restart_brender();
	sync_parameters(CHANGE_EDL);
}


void MWindow::set_inpoint(int is_mwindow)
{
	undo->update_undo_before("in point", LOAD_TIMEBAR);
	edl->set_inpoint(edl->local_session->selectionstart);
	save_backup();
	undo->update_undo_after();


	if(!is_mwindow)
	{
		gui->lock_window();
	}
	gui->timebar->update();
	gui->flush();
	if(!is_mwindow)
	{
		gui->unlock_window();
	}

	if(is_mwindow)
	{
		cwindow->gui->lock_window();
	}
	cwindow->gui->timebar->update();
	cwindow->gui->flush();
	if(is_mwindow)
	{
		cwindow->gui->unlock_window();
	}
}

void MWindow::set_outpoint(int is_mwindow)
{
	undo->update_undo_before("out point", LOAD_TIMEBAR);
	edl->set_outpoint(edl->local_session->selectionend);
	save_backup();
	undo->update_undo_after();


	if(!is_mwindow)
	{
		gui->lock_window();
	}
	gui->timebar->update();
	gui->flush();
	if(!is_mwindow)
	{
		gui->unlock_window();
	}

	if(is_mwindow)
	{
		cwindow->gui->lock_window();
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

	source->copy(source->local_session->in_point, 
		source->local_session->out_point, 
		1,
		0,
		0,
		&file,
		plugindb,
		"",
		1);

	undo->update_undo_before("splice", LOAD_EDITS | LOAD_TIMEBAR);


//file.dump();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	paste(start, 
		start, 
		&file,
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits);

	edl->local_session->selectionstart = 
		edl->local_session->selectionend =
		start + 
		source->local_session->out_point - 
		source->local_session->in_point;

	save_backup();
	undo->update_undo_after();
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

//printf("MWindow::to_clip 1 %s\n", edl->local_session->clip_title);

//file.dump();


	EDL *new_edl = new EDL(edl);
	new_edl->create_objects();
	new_edl->load_xml(plugindb, &file, LOAD_ALL);
	sprintf(new_edl->local_session->clip_title, "Clip %d\n", session->clip_number++);

//printf("VWindowEditing::to_clip 2 %d\n", edl->assets->total());
	awindow->clip_edit->create_clip(new_edl);
	save_backup();
//printf("VWindowEditing::to_clip 3 %d\n", edl->assets->total());
}




int MWindow::toggle_label(int is_mwindow)
{
	double position1, position2;

//printf("MWindow::toggle_label 1\n");
	undo->update_undo_before("label", LOAD_TIMEBAR);
//printf("MWindow::toggle_label 1\n");

	if(cwindow->playback_engine->is_playing_back)
	{
		position1 = position2 = 
			cwindow->playback_engine->get_tracking_position();
	}
	else
	{
		position1 = edl->local_session->selectionstart;
		position2 = edl->local_session->selectionend;
	}

//printf("MWindow::toggle_label 1 %f %f\n", position1,position2 );
	position1 = edl->align_to_frame(position1, 0);
	position2 = edl->align_to_frame(position2, 0);

//printf("MWindow::toggle_label 1\n");
	edl->labels->toggle_label(position1, position2);
	save_backup();

//printf("MWindow::toggle_label 1\n");
	if(!is_mwindow)
	{
		gui->lock_window();
	}
	gui->timebar->update();
	gui->canvas->activate();
	gui->flush();
	if(!is_mwindow)
	{
		gui->unlock_window();
	}

//printf("MWindow::toggle_label 1\n");
	if(is_mwindow)
	{
		cwindow->gui->lock_window();
	}
	cwindow->gui->timebar->update();
	cwindow->gui->flush();
	if(is_mwindow)
	{
		cwindow->gui->unlock_window();
	}

//printf("MWindow::toggle_label 1\n");
	undo->update_undo_after();
//printf("MWindow::toggle_label 2\n");
	return 0;
}

void MWindow::trim_selection()
{
	undo->update_undo_before("trim selection", LOAD_EDITS);


	edl->trim_selection(edl->local_session->get_selectionstart(), 
		edl->local_session->get_selectionend(), 
		edl->session->labels_follow_edits, 
		edl->session->plugins_follow_edits);

	save_backup();
	undo->update_undo_after();
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	restart_brender();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							CHANGE_EDL,
							edl,
							1);
}



void MWindow::undo_entry(int is_mwindow)
{
	if(is_mwindow)
		gui->unlock_window();
	else
		cwindow->gui->unlock_window();

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

	cwindow->gui->lock_window();
	gui->lock_window();

	undo->undo(); 

	save_backup();
	restart_brender();
	update_plugin_states();
	update_plugin_guis();
	gui->update(1, 2, 1, 1, 1, 1, 0);
	cwindow->update(1, 1, 1, 1, 1);

	if(is_mwindow)
		cwindow->gui->unlock_window();
	else
		gui->unlock_window();

	awindow->gui->lock_window();
	awindow->gui->update_assets();
	awindow->gui->flush();
	awindow->gui->unlock_window();
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
	    		   CHANGE_ALL,
	    		   edl,
	    		   1);
}



void MWindow::new_folder(char *new_folder)
{
	undo->update_undo_before("new folder", LOAD_ALL);
	edl->new_folder(new_folder);
	undo->update_undo_after();
	awindow->gui->lock_window();
	awindow->gui->update_assets();
	awindow->gui->unlock_window();
}

void MWindow::delete_folder(char *folder)
{
	undo->update_undo_before("new folder", LOAD_ALL);
	undo->update_undo_after();
}

void MWindow::select_point(double position)
{
	edl->local_session->selectionstart = 
		edl->local_session->selectionend = position;
// Que the CWindow
	cwindow->update(1, 0, 0, 0, 1);
	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->hide();
	gui->cursor->draw();
	gui->mainclock->update(edl->local_session->selectionstart);
	gui->zoombar->update();
	gui->canvas->flash();
	gui->flush();
}



