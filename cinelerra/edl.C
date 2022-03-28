// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetlist.h"
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "awindowgui.inc"
#include "bcsignals.h"
#include "clip.h"
#include "cliplist.h"
#include "colormodels.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "intauto.h"
#include "intautos.h"
#include "labels.h"
#include "loadmode.inc"
#include "localsession.h"
#include "mutex.h"
#include "panauto.h"
#include "panautos.h"
#include "playbackconfig.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderbase.h"
#include "theme.h"
#include "tracks.h"
#include "vtrack.h"
#include "versioninfo.h"

#include <string.h>

Mutex* EDL::id_lock = 0;


EDL::EDL(int is_master)
{
	this->is_master = is_master;
	id = next_id();
	project_path[0] = 0;

	this_edlsession = 0;

	tracks = new Tracks(this);
	assets = new ArrayList<Asset*>;
	local_session = new LocalSession(this);
	labels = new Labels(this, "LABELS");
}

EDL::~EDL()
{
	delete tracks;
	delete labels;
	delete local_session;
	delete assets;
}

void EDL::reset_instance()
{
	tracks->reset_instance();
	labels->reset_instance();
	assets->remove_all();
	local_session->reset_instance();
}

void EDL::reset_plugins()
{
	tracks->reset_plugins();
}

void EDL::reset_renderers()
{
	tracks->reset_renderers();
}

EDL& EDL::operator=(EDL &edl)
{
	copy_all(&edl);
	return *this;
}

void EDL::load_defaults(BC_Hash *defaults, EDLSession *session)
{
	if(session)
	{
		session->load_defaults(defaults);
		this_edlsession = session;
	}

	local_session->load_defaults(defaults);
}

void EDL::save_defaults(BC_Hash *defaults, EDLSession *session)
{
	if(session)
		session->save_defaults(defaults);

	local_session->save_defaults(defaults);
}

void EDL::boundaries()
{
	if(this_edlsession)
		this_edlsession->boundaries();
	local_session->boundaries();
}

void EDL::create_default_tracks()
{
	for(int i = 0; i < edlsession->video_tracks; i++)
		tracks->add_track(TRACK_VIDEO, 0, 0);

	for(int i = 0; i < edlsession->audio_tracks; i++)
		tracks->add_track(TRACK_AUDIO, 0, 0);
// Set master track
	if(first_track())
		first_track()->master = 1;
}

void EDL::load_xml(FileXML *file, EDLSession *session)
{
	int result = 0;

	if(is_master)
	{
		while(!file->read_tag() &&
			!file->tag.title_is("EDL"));
	}

	if(!result)
	{
// Get path for backups
		project_path[0] = 0;
		file->tag.get_property("PROJECT_PATH", project_path);

// Erase everything
		while(tracks->last) delete tracks->last;

		while(labels->last) delete labels->last;
		local_session->unset_inpoint();
		local_session->unset_outpoint();

		do{
			result = file->read_tag();

			if(!result)
			{
				if(file->tag.title_is("/XML") ||
					file->tag.title_is("/EDL") ||
					file->tag.title_is("/CLIP_EDL") ||
					file->tag.title_is("/VWINDOW_EDL"))
				{
					result = 1;
					break;
				}
				else
				if(file->tag.title_is("CLIPBOARD"))
				{
				}
				else
				if(file->tag.title_is("VIDEO"))
				{
					if(session)
						session->load_video_config(file);
				}
				else
				if(file->tag.title_is("AUDIO"))
				{
					if(session)
						session->load_audio_config(file);
				}
				else
				if(file->tag.title_is("ASSETS"))
					assetlist_global.load_assets(file, assets);
				else
				if(file->tag.title_is(labels->xml_tag))
					labels->load(file);
				else
				if(file->tag.title_is("LOCALSESSION"))
					local_session->load_xml(file);
				else
				if(file->tag.title_is("SESSION"))
				{
					if(session)
					{
						session->load_xml(file);
						this_edlsession = session;
					}
				}
				else
				if(file->tag.title_is("TRACK"))
					tracks->load(file);
				else
// Sub EDL.
				if(file->tag.title_is("CLIP_EDL"))
				{
					if(is_master)
					{
						EDL *new_edl = new EDL(0);

						new_edl->load_xml(file, 0);
						cliplist_global.add_clip(new_edl);
					}
					else
						file->skip_to_tag("/CLIP_EDL");
				}
				else
				if(file->tag.title_is("VWINDOW_EDL"))
				{
					if(is_master)
						vwindow_edl->load_xml(file, 0);
					else
						file->skip_to_tag("/VWINDOW__EDL");
				}
			}
		}while(!result);
	}
	tracks->init_shared_pointers();
	check_master_track();
	boundaries();
	tracks->cleanup_plugins();
}


void EDL::copy_all(EDL *edl)
{
	update_assets(edl);
	tracks->copy_from(edl->tracks);
	labels->copy_from(edl->labels);
}

void EDL::copy_session(EDL *edl, EDLSession *session)
{
	strcpy(this->project_path, edl->project_path);
	local_session->copy_from(edl->local_session);
	if(session)
		this_edlsession = session;
}

void EDL::copy_assets(ptstime start, 
	ptstime end, 
	FileXML *file, 
	int all, 
	const char *output_path)
{
	ArrayList<Asset*> asset_list;
	ArrayList<Asset*> *listp;
	Track* current;

	file->tag.set_title("ASSETS");
	file->append_tag();
	file->append_newline();

// Copy everything for a save
	if(all)
	{
		listp = assets;
	}
	else
// Copy just the ones being used.
	{
		for(current = tracks->first; 
			current; 
			current = NEXT)
		{
			if(current->record)
			{
				current->copy_assets(start, 
					end, 
					&asset_list);
			}
		}
		listp = &asset_list;
	}

// Paths relativised here
	for(int i = 0; i < listp->total; i++)
		listp->values[i]->write(file, 0, -1, output_path);

	file->tag.set_title("/ASSETS");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.
void EDL::save_xml(FileXML *file, const char *output_path,
	int save_flags)
{
	ptstime start = 0;
	ptstime end = total_length();
// begin file
	if(!(save_flags & EDL_CLIP))    // Cliplist writes tag itself
	{
		if(save_flags & EDL_VWINDOW)
			file->tag.set_title("VWINDOW_EDL");
		else
		{
			file->tag.set_title("EDL");
			file->tag.set_property("VERSION", CINELERRA_VERSION);
// Save path for restoration of the project title from a backup.
			if(this->project_path[0])
			{
				file->tag.set_property("PROJECT_PATH", project_path);
			}
		}

		file->append_tag();
		file->append_newline();
	}

// Sessions
	local_session->save_xml(file);

// Top level stuff.
// Need to copy all this from child EDL if pasting is desired.
// Session
	if(this_edlsession)
	{
		this_edlsession->save_xml(file);
		this_edlsession->save_video_config(file);
		this_edlsession->save_audio_config(file);
	}

// Media
// Don't replicate all assets for every clip.
	if((save_flags & (EDL_CLIP | EDL_VWINDOW)) == 0)
		copy_assets(start, end, file, 1, output_path);

// Clips
// Don't want this if using clipboard
	if(!(save_flags & (EDL_CLIPBRD | EDL_UNDO)) && vwindow_edl->total_tracks() &&
		this != vwindow_edl)
	{
		vwindow_edl->save_xml(file,
			output_path, EDL_VWINDOW);
	}
	if(this == master_edl && !(save_flags & EDL_UNDO))
		cliplist_global.save_xml(file, output_path);

	file->append_newline();

	labels->save_xml(file);
	tracks->save_xml(file, output_path);

// terminate file
	if(!(save_flags & EDL_CLIP))
	{
		if(save_flags & EDL_VWINDOW)
			file->tag.set_title("/VWINDOW_EDL");
		else
			file->tag.set_title("/EDL");
		file->append_tag();
		file->append_newline();
	}
}

void EDL::copy(EDL *edl, ptstime start, ptstime end, ArrayList<Track*> *src_tracks)
{
	if(PTSEQU(start, end))
		return;
	local_session->copy(edl->local_session, start, end);
	labels->copy(edl->labels, start, end);
	tracks->copy(edl->tracks, start, end, src_tracks);
}

void EDL::rechannel()
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO)
		{
			PanAutos *autos = (PanAutos*)current->automation->have_autos(AUTOMATION_PAN);

			if(autos)
			{
				for(PanAuto *keyframe = (PanAuto*)autos->first;
						keyframe;
						keyframe = (PanAuto*)keyframe->next)
					keyframe->rechannel();
			}
		}
	}
}

void EDL::trim_selection(ptstime start, 
	ptstime end,
	int edit_labels)
{
	if(start != end)
	{
// clear the data
		clear(0, start, edit_labels);
		clear(end - start, total_length(),
			edit_labels);
	}
}

int EDL::equivalent(ptstime position1, ptstime position2)
{
	ptstime threshold;

	if(edlsession->cursor_on_frames)
		threshold = (double).5 / edlsession->frame_rate;
	else
		threshold = (double)1 / edlsession->sample_rate;

	if(fabs(position2 - position1) < threshold)
		return 1;
	else
		return 0;
}

double EDL::equivalent_output(EDL *edl)
{
	double result = -1;
	tracks->equivalent_output(edl->tracks, &result);
	return result;
}

void EDL::set_project_path(const char *path)
{
	strcpy(this->project_path, path);
}

void EDL::set_inpoint(ptstime position)
{
	if(equivalent(local_session->get_inpoint(), position) && 
		local_session->get_inpoint() >= 0)
	{
		local_session->unset_inpoint();
	}
	else
	{
		ptstime total_len = total_length();

		position = align_to_frame(position);
		if(position < 0)
			position = 0;
		if(position > total_len)
			position = total_len;
		local_session->set_inpoint(position);
		if(local_session->get_outpoint() <= local_session->get_inpoint()) 
			local_session->unset_outpoint();
	}
}

void EDL::set_outpoint(ptstime position)
{
	if(equivalent(local_session->get_outpoint(), position) && 
		local_session->get_outpoint() >= 0)
	{
		local_session->unset_outpoint();
	}
	else
	{
		ptstime total_len = total_length();

		position = align_to_frame(position);
		if(position < 0)
			position = 0;
		if(position > total_len)
			position = total_len;
		local_session->set_outpoint(position);
		if(local_session->get_inpoint() >= local_session->get_outpoint()) 
			local_session->unset_inpoint();
	}
}

void EDL::clear(ptstime start,
	ptstime end,
	int edit_labels)
{
	if(PTSEQU(start, end))
	{
		ptstime distance = 0;

		tracks->clear_handle(start, 
			end,
			distance, 
			edit_labels);
		if(edit_labels && distance > 0)
			labels->paste_silence(start, 
				start + distance);
	}
	else
	{
		tracks->clear(start, end);
		if(edit_labels)
			labels->clear(start, 
				end, 
				1);
	}

// Need to put at beginning so a subsequent paste operation starts at the
// right position.
	local_session->set_selection(local_session->get_selectionstart());
}

ptstime EDL::adjust_position(ptstime oldposition,
	ptstime newposition,
	int currentend,
	int handle_mode)
{
	if(tracks && tracks->total())
		return tracks->adjust_position(oldposition, newposition,
			currentend, handle_mode);
	return newposition;
}

void EDL::modify_edithandles(ptstime oldposition,
	ptstime newposition,
	int currentend, // handle
	int handle_mode, // button mode
	int edit_labels)
{
	ptstime newpos = adjust_position(oldposition, newposition, currentend,
		handle_mode);
	tracks->modify_edithandles(oldposition,
		newpos,
		currentend,
		handle_mode);
	labels->modify_handles(oldposition,
		newpos,
		currentend,
		handle_mode,
		edit_labels);
}

void EDL::modify_pluginhandles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode)
{
	adjust_position(oldposition, newposition, currentend,
		handle_mode);
	optimize();
}

void EDL::paste_silence(ptstime start,
	ptstime end,
	int edit_labels)
{
	if(edit_labels)
		labels->paste_silence(start, end);
	tracks->paste_silence(start, end);
}

void EDL::remove_from_project(ArrayList<Asset*> *assets)
{
	for(int i = 0; i < assets->total; i++)
	{
// Remove from tracks
		for(Track *track = tracks->first; track; track = track->next)
			track->remove_asset(assets->values[i]);

// Remove from assets
		this->assets->remove(assets->values[i]);
	}
}

void EDL::update_assets(EDL *src)
{
	Asset *current;
	int found;

	for(int i = 0; i < src->assets->total; i++)
	{
		current = src->assets->values[i];
		found = 0;

		for(int k = 0; k < assets->total; k++)
		{
			if(assets->values[k] == current)
			{
				found = 1;
				break;
			}
		}
		if(!found)
			assets->append(current);
	}
}

void EDL::update_assets(Asset *asset)
{
	if(!asset)
		return;

	for(int k = 0; k < assets->total; k++)
	{
		if(assets->values[k] == asset)
			return;
	}
	assets->append(asset);
}

int EDL::get_tracks_height(Theme *theme)
{
	int total_pixels = 0;
	for(Track *current = tracks->first;
		current;
		current = NEXT)
	{
		total_pixels += current->vertical_span(theme);
	}
	return total_pixels;
}

// Get the total output size scaled to aspect ratio
void EDL::calculate_conformed_dimensions(double &w, double &h)
{
	if(this_edlsession)
	{
		w = this_edlsession->output_w * this_edlsession->sample_aspect_ratio;
		h = this_edlsession->output_h;
	}
	else
	{
		w = edlsession->output_w * edlsession->sample_aspect_ratio;
		h = edlsession->output_h;
	}
}

double EDL::get_sample_aspect_ratio()
{
	if(this_edlsession)
		return this_edlsession->sample_aspect_ratio;
	return edlsession->sample_aspect_ratio;
}

void EDL::dump(int indent)
{
	if(!is_master)
		printf("%*sCLIP %p dump: id %d\n", indent, "", this, id);
	else
		printf("%*sEDL %p dump: id %d\n", indent, "", this, id);
	local_session->dump(indent + 2);
	indent += 2;
	if(this_edlsession)
		this_edlsession->dump(indent + 1);
	else
		printf("%*sNo EDLSession\n", indent + 1, "");
	if(this == master_edl)
	{
		printf("%*sClips (total %d)\n", indent + 1, "", cliplist_global.total);
		cliplist_global.dump(indent + 2);
	}
	dump_assets(indent + 1);

	labels->dump(indent + 1);
	tracks->dump(indent + 1);
}

void EDL::dump_assets(int indent)
{
	printf("%*sAssets %p dump(%d):\n", indent, "", this, assets->total);
	indent += 1;
	for(int i = 0; i < assets->total; i++)
		assets->values[i]->dump(indent);
}

void EDL::insert_asset(Asset *asset, 
	ptstime position, 
	Track *first_track)
{
	Track *current = first_track ? first_track : tracks->first;
	ptstime length;
	int stream;

// Insert asset into asset table
	update_assets(asset);

// Fix length of single frame
	if(asset->single_image)
	{
		if(edlsession->si_useduration)
			length = edlsession->si_duration;
		else
			length = 1.0 / edlsession->frame_rate;
	}
	else
		length = asset->duration();

// Paste video
	stream = -1;
	while((stream = asset->get_stream_ix(STRDSC_VIDEO, stream)) >= 0)
	{
		int layers = asset->streams[stream].channels;

		for(int vtrack = 0; current && vtrack < layers;
			current = NEXT)
		{
			if(!current->record ||
					current->data_type != TRACK_VIDEO)
				continue;

			current->insert_asset(asset, stream, vtrack,
				length, position);
			vtrack++;
		}
	}

	stream = -1;
	while((stream = asset->get_stream_ix(STRDSC_AUDIO, stream)) >= 0)
	{
		int channels = asset->streams[stream].channels;

		for(int atrack = 0; current && atrack < channels;
			current = NEXT)
		{
			if(!current->record ||
					current->data_type != TRACK_AUDIO)
				continue;

			current->insert_asset(asset, stream, atrack,
				length, position);
			atrack++;
		}
	}
}

void EDL::optimize()
{
	ptstime length = total_length();

	if(local_session->preview_end > length) local_session->preview_end = length;
	if(local_session->preview_start > length ||
		local_session->preview_start < 0) local_session->preview_start = 0;
}

int EDL::next_id()
{
	id_lock->lock("EDL::next_id");
	int result = EDLSession::current_id++;
	id_lock->unlock();
	return result;
}

void EDL::get_shared_plugins(Track *source, ptstime startpos, ptstime endpos,
	ArrayList<Plugin*> *plugin_locations)
{
	for(Track *track = tracks->first; track; track = track->next)
	{
		if(track != source && 
			track->data_type == source->data_type)
		{
			for(int i = 0; i < track->plugins.total; i++)
			{
				Plugin *plugin = track->plugins.values[i];

				if(plugin->get_pts() > endpos ||
						plugin->end_pts() < startpos)
					continue;

				if(plugin && plugin->plugin_type == PLUGIN_STANDALONE)
				{
					ptstime plugin_start = plugin->get_pts();
					ptstime plugin_end = plugin->end_pts();

					if(!plugin->plugin_server)
						continue;

					if(plugin->plugin_server->multichannel &&
							!plugin->shared_slots())
						continue;

					if(!RenderBase::multichannel_possible(source,
							source->plugins.total,
							plugin_start, plugin_end,
							plugin))
						continue;

					for(int j = 0; j < source->plugins.total; j++)
					{
						Plugin *current = source->plugins.values[j];

						// Plugin already shared?
						if(current->plugin_type == PLUGIN_SHAREDPLUGIN &&
							current->shared_plugin == plugin)
						{
							plugin = 0;
							break;
						}
					}
					if(plugin)
						plugin_locations->append(plugin);
				}
			}
		}
	}
}

void EDL::get_shared_tracks(Track *track, ptstime start, ptstime end,
	ArrayList<Track*> *module_locations)
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		int found = 0;

		if(current != track &&
			current->data_type == track->data_type)
		{
			for(int i = 0; i < current->plugins.total; i++)
			{
				Plugin *plugin = current->plugins.values[i];

				if(plugin->get_pts() > end ||
						plugin->end_pts() < start)
					continue;

				if(plugin->shared_track == track ||
					(plugin->shared_plugin &&
					plugin->shared_plugin->track == track) ||
					(plugin->plugin_server &&
					plugin->plugin_server->multichannel &&
					plugin->shared_with(track)))
				{
					found = 1;
					break;
				}
			}
			if(!found)
				module_locations->append(current);
		}
	}
}

// Convert position to frames if cursor alignment is enabled
ptstime EDL::align_to_frame(ptstime position, int roundit)
{
	ptstime temp;
// Seconds -> Frames/samples
	if(edlsession->cursor_on_frames)
		temp = position * edlsession->frame_rate;
	else
		temp = position * edlsession->sample_rate;

	if(roundit)
		temp = round(temp);
	else
		temp = nearbyint(temp);

// Frames/samples -> Seconds
	if(edlsession->cursor_on_frames)
		temp /= edlsession->frame_rate;
	else
		temp /= edlsession->sample_rate;

	return temp;
}

void EDL::update_plugin_guis()
{
	for(Track *track = tracks->first; track; track = track->next)
		track->update_plugin_guis();
}

void EDL::update_plugin_titles()
{
	for(Track *track = tracks->first; track; track = track->next)
		track->update_plugin_titles();
}

void EDL::init_edl()
{
	Asset *new_asset;
	int stream;

	if(tracks && tracks->total())
		return;
	if(!assets->total)
		return;

	edlsession->video_tracks = 0;
	edlsession->audio_tracks = 0;

// Edl has only one asset here
	new_asset = assets->values[0];

	stream = -1;
	while((stream = new_asset->get_stream_ix(STRDSC_VIDEO, stream)) >= 0)
	{
		int layers = new_asset->streams[stream].channels;
		edlsession->video_tracks += layers;

		for(int k = 0; k < layers; k++)
			tracks->add_track(TRACK_VIDEO, 0, 0);
	}

	stream = -1;

	while((stream = new_asset->get_stream_ix(STRDSC_AUDIO, stream)) >= 0)
	{
		int channels = new_asset->streams[stream].channels;

		edlsession->audio_tracks += channels;

		for(int k = 0; k < channels; k++)
			tracks->add_track(TRACK_AUDIO, 0, 0);
	}
	insert_asset(new_asset, 0);

	check_master_track();
}

ptstime EDL::total_length()
{
	if(tracks && tracks->total())
		return tracks->total_length();
	return 0;
}

ptstime EDL::duration()
{
	if(tracks && tracks->total())
	{
		ptstime len = tracks->total_length();

		if(edlsession->cursor_on_frames && len > 0)
			len = floor(len * edlsession->frame_rate) / edlsession->frame_rate;
		return len;
	}
	return 0;
}

ptstime EDL::total_length_of(int type)
{
	if(tracks && tracks->total())
		return tracks->total_length_of(type);
	return 0;
}

int EDL::total_tracks()
{
	if(tracks)
		return tracks->total();
	return 0;
}

int EDL::total_tracks_of(int type)
{
	if(tracks && tracks->total())
		return tracks->total_tracks_of(type);
	return 0;
}

int EDL::playable_tracks_of(int type)
{
	if(tracks && tracks->total())
		return tracks->playable_tracks_of(type);
	return 0;
}

int EDL::recordable_tracks_of(int type)
{
	if(tracks && tracks->total())
		return tracks->recordable_tracks_of(type);
	return 0;
}

ptstime EDL::total_length_framealigned()
{
	if(tracks && tracks->total())
		return tracks->total_length_framealigned(edlsession->frame_rate);
	return 0;
}

Track *EDL::first_track()
{
	if(tracks && tracks->total())
		return tracks->first;
	return 0;
}

int EDL::number_of(Track *track)
{
	int i = 0;

	if(tracks && tracks->total())
		return tracks->number_of(track);
	return -1;
}

Track *EDL::number(int number)
{
	Track *current = 0;
	int i = 0;

	if(tracks && tracks->total())
	{
		for(current = tracks->first; current && i < number; current = NEXT)
			i++;
	}
	return current;
}


void EDL::set_all_toggles(int toggle_type, int value)
{
	if(tracks && tracks->total())
	{
		for(Track* current = tracks->first; current; current = NEXT)
		{
			ptstime position = local_session->get_selectionstart(1);

			switch(toggle_type)
			{
			case Tracks::PLAY:
				current->play = value;
				break;
			case Tracks::RECORD:
				current->record = value;
				break;
			case Tracks::GANG:
				current->gang = value;
				break;
			case Tracks::DRAW:
				current->draw = value;
				break;
			case Tracks::MUTE:
				((IntAuto*)current->automation->get_auto_for_editing(position, AUTOMATION_MUTE))->value = value;
				break;
			case Tracks::EXPAND:
				current->expand_view = value;
				break;
			case Tracks::MASTER:
				current->master = value;
				break;
			}
		}
	}
}

int EDL::total_toggled(int toggle_type)
{
	ptstime start;
	int result = 0;

	if(tracks && tracks->total())
	{
		for(Track *current = tracks->first; current; current = NEXT)
		{
			switch(toggle_type)
			{
			case Tracks::MUTE:
				result += current->automation->get_intvalue(
					local_session->get_selectionstart(1),
					AUTOMATION_MUTE);
				break;
			case Tracks::PLAY:
				result += !!current->play;
				break;
			case Tracks::RECORD:
				result += !!current->record;
				break;
			case Tracks::GANG:
				result += !!current->gang;
				break;
			case Tracks::DRAW:
				result += !!current->draw;
				break;
			case Tracks::EXPAND:
				result += !!current->expand_view;
				break;
			case Tracks::MASTER:
				result += !!current->master;
				break;
			}
		}
	}
	return result;
}

void EDL::check_master_track()
{
	if(!tracks || !tracks->total())
		return;

	if(total_toggled(Tracks::MASTER) != 1 || tracks->length() < EPSILON)
	{
		set_all_toggles(Tracks::MASTER, 0);
		for(Track *current = tracks->first; current; current = NEXT)
		{
			if(current->get_length() > EPSILON)
			{
				current->master = 1;
				break;
			}
		}
	}
	// All tracks are empty
	if(!total_toggled(Tracks::MASTER))
		tracks->first->master = 1;
}

const char *EDL::handle_name(int handle)
{
	switch(handle)
	{
	case HANDLE_MAIN:
		return "Main handle";
	case HANDLE_LEFT:
		return "Left handle";
	case HANDLE_RIGHT:
		return "Right handle";
	}
	return "Unknown handlle";
}

size_t EDL::get_size()
{
	size_t size = sizeof(*this);

	if(tracks)
		size += tracks->get_size();
	size += labels->get_size();
	size += local_session->get_size();
	return size;
}
