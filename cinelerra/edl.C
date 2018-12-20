
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
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "awindowgui.inc"
#include "bcsignals.h"
#include "clip.h"
#include "colormodels.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "labels.h"
#include "loadmode.inc"
#include "localsession.h"
#include "mutex.h"
#include "panauto.h"
#include "panautos.h"
#include "playbackconfig.h"
#include "plugin.h"
#include "sharedlocation.h"
#include "theme.h"
#include "tracks.h"
#include "vtrack.h"
#include "versioninfo.h"

Mutex* EDL::id_lock = 0;


EDL::EDL(EDL *parent_edl)
{
	this->parent_edl = parent_edl;

	id = next_id();
	project_path[0] = 0;

	if(!edlsession)
		edlsession = new EDLSession();

	tracks = new Tracks(this);
	if(!parent_edl)
		assets = new ArrayList<Asset*>;
	else
		assets = parent_edl->assets;

	local_session = new LocalSession(this);
	labels = new Labels(this, "LABELS");
}


EDL::~EDL()
{
	if(tracks)
		delete tracks;

	if(labels)
		delete labels;

	if(local_session)
		delete local_session;

	if(!parent_edl)
		delete assets;

	clips.remove_all_objects();
}

EDL& EDL::operator=(EDL &edl)
{
	copy_all(&edl);
	return *this;
}

void EDL::load_defaults(BC_Hash *defaults)
{
	if(!parent_edl)
		edlsession->load_defaults(defaults);

	local_session->load_defaults(defaults);
}

void EDL::save_defaults(BC_Hash *defaults)
{
	if(!parent_edl)
		edlsession->save_defaults(defaults);

	local_session->save_defaults(defaults);
}

void EDL::boundaries()
{
	edlsession->boundaries();
	local_session->boundaries();
}

void EDL::create_default_tracks()
{
	for(int i = 0; i < edlsession->video_tracks; i++)
		tracks->add_video_track(0, 0);

	for(int i = 0; i < edlsession->audio_tracks; i++)
		tracks->add_audio_track(0, 0);
}

void EDL::load_xml(FileXML *file, uint32_t load_flags)
{
	int result = 0;
// Track numbering offset for replacing undo data.
	int track_offset = 0;

// Search for start of master EDL.

// The parent_edl test caused clip creation to fail since those XML files
// contained an EDL tag.

// The parent_edl test is required to make EDL loading work because
// when loading an EDL the EDL tag is already read by the parent.

	if(!parent_edl)
	{
		do{
			result = file->read_tag();
		}while(!result && 
			!file->tag.title_is("XML") && 
			!file->tag.title_is("EDL"));
	}

	if(!result)
	{
// Get path for backups
		project_path[0] = 0;
		file->tag.get_property("PROJECT_PATH", project_path);

// Erase everything
		if((load_flags & LOAD_ALL) == LOAD_ALL ||
			(load_flags & LOAD_EDITS) == LOAD_EDITS)
		{
			while(tracks->last) delete tracks->last;
		}

		if((load_flags & LOAD_ALL) == LOAD_ALL)
		{
			clips.remove_all_objects();
		}

		if(load_flags & LOAD_TIMEBAR)
		{
			while(labels->last) delete labels->last;
			local_session->unset_inpoint();
			local_session->unset_outpoint();
		}

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
				}
				else
				if(file->tag.title_is("CLIPBOARD"))
				{
				}
				else
				if(file->tag.title_is("VIDEO"))
				{
					if((load_flags & LOAD_VCONFIG) &&
						(load_flags & LOAD_SESSION))
						edlsession->load_video_config(file, 0, load_flags);
				}
				else
				if(file->tag.title_is("AUDIO"))
				{
					if((load_flags & LOAD_ACONFIG) &&
						(load_flags & LOAD_SESSION))
						edlsession->load_audio_config(file, 0, load_flags);
				}
				else
				if(file->tag.title_is("ASSETS"))
				{
					if(load_flags & LOAD_ASSETS)
						assetlist_global.load_assets(file, assets);
				}
				else
				if(file->tag.title_is(labels->xml_tag))
				{
					if(load_flags & LOAD_TIMEBAR)
						labels->load(file, load_flags);
				}
				else
				if(file->tag.title_is("LOCALSESSION"))
				{
					if((load_flags & LOAD_SESSION) ||
						(load_flags & LOAD_TIMEBAR))
						local_session->load_xml(file, load_flags);
				}
				else
				if(file->tag.title_is("SESSION"))
				{
					if((load_flags & LOAD_SESSION) &&
						!parent_edl)
						edlsession->load_xml(file, 0, load_flags);
				}
				else
				if(file->tag.title_is("TRACK"))
				{
					tracks->load(file, track_offset, load_flags);
				}
				else
// Sub EDL.
// Causes clip creation to fail because that involves an opening EDL tag.
				if(file->tag.title_is("CLIP_EDL") && !parent_edl)
				{
					EDL *new_edl = new EDL(this);
					new_edl->load_xml(file, LOAD_ALL);

					if((load_flags & LOAD_ALL) == LOAD_ALL)
						clips.append(new_edl);
					else
						delete new_edl;
				}
				else
				if(file->tag.title_is("VWINDOW_EDL") && !parent_edl)
				{
					EDL *new_edl = new EDL(this);
					new_edl->load_xml(file, LOAD_ALL);

					if((load_flags & LOAD_ALL) == LOAD_ALL)
					{
						if(vwindow_edl) delete vwindow_edl;
						vwindow_edl = new_edl;
					}
					else
					{
						delete new_edl;
						new_edl = 0;
					}
				}
			}
		}while(!result);
	}
	boundaries();
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.
// Called recursively by copy for clips, thus the string can't be terminated.
// The string is not terminated in this call.
void EDL::save_xml(FileXML *file, const char *output_path,
	int is_clip, int is_vwindow)
{
	copy(0, tracks->total_length(), 1, is_clip,
		is_vwindow, file, output_path, 0);
}

void EDL::copy_all(EDL *edl)
{
	copy_assets(edl);
	copy_clips(edl);
	tracks->copy_from(edl->tracks);
	labels->copy_from(edl->labels);
}

void EDL::copy_clips(EDL *edl)
{
	clips.remove_all_objects();
	for(int i = 0; i < edl->clips.total; i++)
	{
		add_clip(edl->clips.values[i]);
	}
}

void EDL::copy_assets(EDL *edl)
{
	if(!parent_edl)
	{
		for(int i = 0; i < edl->assets->total; i++)
			assets->append(edl->assets->values[i]);
	}
}

void EDL::copy_session(EDL *edl, int session_only)
{
	if(!session_only)
	{
		strcpy(this->project_path, edl->project_path);
	}

	if(!session_only)
	{
		local_session->copy_from(edl->local_session);
	}
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
		listp->values[i]->write(file, 0, output_path);

	file->tag.set_title("/ASSETS");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void EDL::copy(ptstime start, 
	ptstime end, 
	int all, 
	int is_clip,
	int is_vwindow,
	FileXML *file, 
	const char *output_path,
	int rewind_it)
{
// begin file
	if(is_clip)
		file->tag.set_title("CLIP_EDL");
	else
	if(is_vwindow)
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

// Set clipboard samples only if copying to clipboard
	if(!all)
	{
		file->tag.set_title("CLIPBOARD");
		file->tag.set_title("/CLIPBOARD");
		file->append_tag();
		file->append_newline();
		file->append_newline();
	}

// Sessions
	local_session->save_xml(file, start);

// Top level stuff.
// Need to copy all this from child EDL if pasting is desired.
// Session
	edlsession->save_xml(file);
	edlsession->save_video_config(file);
	edlsession->save_audio_config(file);

// Media
// Don't replicate all assets for every clip.
// The assets for the clips are probably in the mane EDL.
	if(!is_clip)
		copy_assets(start, end, file, all, output_path);

// Clips
// Don't want this if using clipboard
	if(all)
	{
		if(vwindow_edl)
		{
			vwindow_edl->save_xml(file,
				output_path,
				0,
				1);
		}

		for(int i = 0; i < clips.total; i++)
			clips.values[i]->save_xml(file,
				output_path,
				1,
				0);
	}

	file->append_newline();
	file->append_newline();

	labels->copy(start, end, file);
	tracks->copy(start, end, all, file, output_path);

// terminate file
	if(is_clip)
		file->tag.set_title("/CLIP_EDL");
	else
	if(is_vwindow)
		file->tag.set_title("/VWINDOW_EDL");
	else
		file->tag.set_title("/EDL");
	file->append_tag();
	file->append_newline();

// For editing operations we want to rewind it for immediate pasting.
// For clips and saving to disk leave it alone.
	if(rewind_it)
	{
		file->terminate_string();
		file->rewind();
	}
}

void EDL::rechannel()
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO)
		{
			PanAutos *autos = (PanAutos*)current->automation->autos[AUTOMATION_PAN];

			for(PanAuto *keyframe = (PanAuto*)autos->first;
				keyframe;
				keyframe = (PanAuto*)keyframe->next)
			{
				keyframe->rechannel();
			}
		}
	}
}


void EDL::synchronize_params(EDL *edl)
{
	local_session->synchronize_params(edl->local_session);
	for(Track *this_track = tracks->first, *that_track = edl->tracks->first; 
		this_track && that_track; 
		this_track = this_track->next,
		that_track = that_track->next)
	{
		this_track->synchronize_params(that_track);
	}
}

void EDL::trim_selection(ptstime start, 
	ptstime end,
	int actions)
{
	if(start != end)
	{
// clear the data
		clear(0, 
			start,
			actions);
		clear(end - start, 
			tracks->total_length(),
			actions);
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
		local_session->set_inpoint(align_to_frame(position));
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
		local_session->set_outpoint(align_to_frame(position));
		if(local_session->get_inpoint() >= local_session->get_outpoint()) 
			local_session->unset_inpoint();
	}
}

void EDL::clear(ptstime start,
	ptstime end,
	int actions)
{
	if(PTSEQU(start, end))
	{
		double distance = 0;
		tracks->clear_handle(start, 
			end,
			distance, 
			actions);
		if((actions & EDIT_LABELS) && distance > 0)
			labels->paste_silence(start, 
				start + distance);
	}
	else
	{
		tracks->clear(start, 
			end,
			actions & EDIT_PLUGINS);
		if(actions & EDIT_LABELS)
			labels->clear(start, 
				end, 
				1);
	}

// Need to put at beginning so a subsequent paste operation starts at the
// right position.
	double position = local_session->get_selectionstart();
	local_session->set_selectionend(position);
	local_session->set_selectionstart(position);
}

void EDL::modify_edithandles(ptstime oldposition,
	ptstime newposition,
	int currentend,
	int handle_mode,
	int actions)
{
	tracks->modify_edithandles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		actions);
	labels->modify_handles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		actions & EDIT_LABELS);
}

void EDL::modify_pluginhandles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	tracks->modify_pluginhandles(oldposition, 
		newposition, 
		currentend, 
		handle_mode,
		edit_labels);
	optimize();
}

void EDL::paste_silence(ptstime start,
	ptstime end,
	int edit_labels, 
	int edit_plugins)
{
	if(edit_labels) 
		labels->paste_silence(start, end);
	tracks->paste_silence(start, 
		end, 
		edit_plugins);
}

void EDL::remove_from_project(ArrayList<EDL*> *clips)
{
	for(int i = 0; i < clips->total; i++)
	{
		for(int j = 0; j < this->clips.total; j++)
		{
			if(this->clips.values[j] == clips->values[i])
			{
				this->clips.remove_object(clips->values[i]);
			}
		}
	}
}

void EDL::remove_from_project(ArrayList<Asset*> *assets)
{
// Remove from clips
	if(!parent_edl)
		for(int j = 0; j < clips.total; j++)
		{
			clips.values[j]->remove_from_project(assets);
		}

// Remove from VWindow
	if(vwindow_edl)
		vwindow_edl->remove_from_project(assets);

	for(int i = 0; i < assets->total; i++)
	{
// Remove from tracks
		for(Track *track = tracks->first; track; track = track->next)
		{
			track->remove_asset(assets->values[i]);
		}

// Remove from assets
		if(!parent_edl)
		{
			this->assets->remove(assets->values[i]);
		}
	}
// Remove from global list
	if(!parent_edl)
		assetlist_global.remove_assets(assets);
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
	w = edlsession->output_w * edlsession->sample_aspect_ratio;
	h = edlsession->output_h;
}

double EDL::get_sample_aspect_ratio()
{
	return edlsession->sample_aspect_ratio;
}

void EDL::dump(int indent)
{
	if(parent_edl)
		printf("%*sCLIP %p dump: (parent %p)\n", indent, "", this, parent_edl);
	else
		printf("%*sEDL %p dump:\n", indent, "", this);
	local_session->dump(indent + 2);
	indent += 2;
	edlsession->dump(indent + 1);

	printf("%*sEDLS (total %d)\n", indent + 1, "", clips.total);

	for(int i = 0; i < clips.total; i++)
		clips.values[i]->dump(indent + 2);

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

EDL* EDL::add_clip(EDL *edl)
{
// Copy argument.  New edls are deleted from MWindow::load_filenames.
	EDL *new_edl = new EDL(this);
	new_edl->copy_all(edl);
	clips.append(new_edl);
	return new_edl;
}

void EDL::insert_asset(Asset *asset, 
	ptstime position, 
	Track *first_track)
{
// Insert asset into asset table
	update_assets(asset);

// Paste video
	int vtrack = 0;
	Track *current = first_track ? first_track : tracks->first;

// Fix length of single frame
	ptstime length;

	if(asset->single_image)
	{
		if(edlsession->si_useduration)
			length = edlsession->si_duration;
		else
			length = 1.0 / edlsession->frame_rate;
	}
	else
		if(asset->frame_rate > 0)
			length = ((double)asset->video_length / asset->frame_rate);
		else
			length = 1.0 / edlsession->frame_rate;

	for(; current && vtrack < asset->layers;
		current = NEXT)
	{
		if(!current->record || 
			current->data_type != TRACK_VIDEO)
			continue;

		current->insert_asset(asset,
			length, 
			position, 
			vtrack);

		vtrack++;
	}

	int atrack = 0;
	for(; current && atrack < asset->channels;
		current = NEXT)
	{
		if(!current->record ||
			current->data_type != TRACK_AUDIO)
			continue;

		current->insert_asset(asset,
			(double)asset->audio_length /
				asset->sample_rate,
			position, 
			atrack);

		atrack++;
	}
}

void EDL::optimize()
{
	ptstime length = tracks->total_length();

	if(local_session->preview_end > length) local_session->preview_end = length;
	if(local_session->preview_start > length ||
		local_session->preview_start < 0) local_session->preview_start = 0;
	for(Track *current = tracks->first; current; current = NEXT)
		current->optimize();
}

int EDL::next_id()
{
	id_lock->lock("EDL::next_id");
	int result = EDLSession::current_id++;
	id_lock->unlock();
	return result;
}

void EDL::get_shared_plugins(Track *source, 
	ArrayList<SharedLocation*> *plugin_locations)
{
	for(Track *track = tracks->first; track; track = track->next)
	{
		if(track != source && 
			track->data_type == source->data_type)
		{
			for(int i = 0; i < track->plugin_set.total; i++)
			{
				Plugin *plugin = track->get_current_plugin(
					local_session->get_selectionstart(1), 
					i, 
					0);
				if(plugin && plugin->plugin_type == PLUGIN_STANDALONE)
				{
					plugin_locations->append(new SharedLocation(tracks->number_of(track), i));
				}
			}
		}
	}
}

void EDL::get_shared_tracks(Track *track, ArrayList<SharedLocation*> *module_locations)
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current != track && 
			current->data_type == track->data_type)
		{
			module_locations->append(new SharedLocation(tracks->number_of(current), 0));
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

void EDL::finalize_edl(int load_mode)
{
	Track *first, *current;
	ptstime track_length;
	int no_track = 0;

// There are no tracks created
	edlsession->video_tracks = 0;
	edlsession->audio_tracks = 0;

	for(int i = 0; i < assets->total; i++)
	{
		Asset *new_asset = assets->values[i];
		first = 0;

		if(i == 0)
		{
			// Use name of the first asset as clip name
			char string[BCTEXTLEN];
			FileSystem fs;

			fs.extract_name(string, new_asset->path);
			strcpy(local_session->clip_title, string);
		}

		if(load_mode == LOADMODE_REPLACE_CONCATENATE ||
			load_mode == LOADMODE_CONCATENATE)
		{
			track_length = tracks->total_length_framealigned(edlsession->frame_rate);
			no_track = i;
			if(i)
			{
				if(edlsession->cursor_on_frames)
					tracks->clear(track_length, tracks->total_length() + 100, 1);
			}
		}
		else
			track_length = 0;

		if(!no_track && new_asset->video_data)
		{
			edlsession->video_tracks += new_asset->layers;

			for(int k = 0; k < new_asset->layers; k++)
			{
				current = tracks->add_video_track(0, 0);
				if(!first)
					first = current;
			}
		}

		if(!no_track && new_asset->audio_data)
		{
			edlsession->audio_tracks += new_asset->channels;

			for(int k = 0; k < new_asset->channels; k++)
			{
				current = tracks->add_audio_track(0, 0);
				if(!first)
					first = current;
			}
		}
		insert_asset(new_asset, track_length, first);
	}

// Align cursor on frames:: clip the new_edl to the minimum of the last joint frame.
	if(edlsession->cursor_on_frames)
	{
		track_length = tracks->total_length_framealigned(edlsession->frame_rate);
		tracks->clear(track_length, tracks->total_length() + 100, 1);
	}
}
