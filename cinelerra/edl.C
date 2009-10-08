
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
#include "guicast.h"
#include "labels.h"
#include "localsession.h"
#include "mutex.h"
#include "panauto.h"
#include "panautos.h"
#include "playbackconfig.h"
#include "plugin.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordlabel.h"
#include "sharedlocation.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.inc"
#include "vtrack.h"




Mutex* EDL::id_lock = 0;



EDL::EDL(EDL *parent_edl)
{
	this->parent_edl = parent_edl;
	tracks = 0;
	labels = 0;
	local_session = 0;
	vwindow_edl = 0;
	vwindow_edl_shared = 0;


	folders.set_array_delete();

	new_folder(CLIP_FOLDER);

	new_folder(MEDIA_FOLDER);

	id = next_id();
	project_path[0] = 0;
}


EDL::~EDL()
{

	if(tracks)
	{
		delete tracks;
	}
	if(labels)
	{
		delete labels;
	}

	if(local_session)
	{
		delete local_session;
	}

	if(vwindow_edl && !vwindow_edl_shared)
		delete vwindow_edl;

	if(!parent_edl)
	{
		delete assets;
		delete session;
	}


	folders.remove_all_objects();
	clips.remove_all_objects();
}


int EDL::create_objects()
{
	tracks = new Tracks(this);
	if(!parent_edl)
	{
		assets = new Assets(this);
		session = new EDLSession(this);
	}
	else
	{
		assets = parent_edl->assets;
		session = parent_edl->session;
	}
	
	local_session = new LocalSession(this);
	labels = new Labels(this, "LABELS");
//	last_playback_position = 0;
	return 0;
}

EDL& EDL::operator=(EDL &edl)
{
printf("EDL::operator= 1\n");
	copy_all(&edl);
	return *this;
}

int EDL::load_defaults(BC_Hash *defaults)
{
	if(!parent_edl)
		session->load_defaults(defaults);

	local_session->load_defaults(defaults);
	return 0;
}

int EDL::save_defaults(BC_Hash *defaults)
{
	if(!parent_edl)
		session->save_defaults(defaults);
	
	local_session->save_defaults(defaults);
	return 0;
}

void EDL::boundaries()
{
	session->boundaries();
	local_session->boundaries();
}

int EDL::create_default_tracks()
{

	for(int i = 0; i < session->video_tracks; i++)
	{
		tracks->add_video_track(0, 0);
	}
	for(int i = 0; i < session->audio_tracks; i++)
	{
		tracks->add_audio_track(0, 0);
	}
	return 0;
}

int EDL::load_xml(ArrayList<PluginServer*> *plugindb,
	FileXML *file, 
	uint32_t load_flags)
{
	int result = 0;
// Track numbering offset for replacing undo data.
	int track_offset = 0;


	folders.remove_all_objects();

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
					local_session->clipboard_length = file->tag.get_property("LENGTH", 0);
				}
				else
				if(file->tag.title_is("VIDEO"))
				{
					if((load_flags & LOAD_VCONFIG) &&
						(load_flags & LOAD_SESSION))
						session->load_video_config(file, 0, load_flags);
				}
				else
				if(file->tag.title_is("AUDIO"))
				{
					if((load_flags & LOAD_ACONFIG) &&
						(load_flags & LOAD_SESSION))
						session->load_audio_config(file, 0, load_flags);
				}
				else
				if(file->tag.title_is("FOLDER"))
				{
					char folder[BCTEXTLEN];
					strcpy(folder, file->read_text());
					new_folder(folder);
				}
				else
				if(file->tag.title_is("ASSETS"))
				{
					if(load_flags & LOAD_ASSETS)
						assets->load(plugindb, file, load_flags);
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
						session->load_xml(file, 0, load_flags);
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
					new_edl->create_objects();
					new_edl->load_xml(plugindb, file, LOAD_ALL);

					if((load_flags & LOAD_ALL) == LOAD_ALL)
						clips.append(new_edl);
					else
						delete new_edl;
				}
				else
				if(file->tag.title_is("VWINDOW_EDL") && !parent_edl)
				{
					EDL *new_edl = new EDL(this);
					new_edl->create_objects();
					new_edl->load_xml(plugindb, file, LOAD_ALL);


					if((load_flags & LOAD_ALL) == LOAD_ALL)
					{
						if(vwindow_edl && !vwindow_edl_shared) delete vwindow_edl;
						vwindow_edl = new_edl;
						vwindow_edl_shared = 0;
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
//dump();

	return 0;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.
// Called recursively by copy for clips, thus the string can't be terminated.
// The string is not terminated in this call.
int EDL::save_xml(ArrayList<PluginServer*> *plugindb,
	FileXML *file, 
	char *output_path,
	int is_clip,
	int is_vwindow)
{
	copy(0, 
		tracks->total_length(), 
		1, 
		is_clip,
		is_vwindow,
		file, 
		plugindb, 
		output_path,
		0);
	return 0;
}

int EDL::copy_all(EDL *edl)
{
	copy_session(edl);
	copy_assets(edl);
	copy_clips(edl);
	tracks->copy_from(edl->tracks);
	labels->copy_from(edl->labels);
	return 0;
}

void EDL::copy_clips(EDL *edl)
{
	if(vwindow_edl && !vwindow_edl_shared) delete vwindow_edl;
	vwindow_edl = 0;
	vwindow_edl_shared = 0;
	if(edl->vwindow_edl)
	{
		vwindow_edl = new EDL(this);
		vwindow_edl->create_objects();
		vwindow_edl->copy_all(edl->vwindow_edl);
	}
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
		assets->copy_from(edl->assets);
	}
}

void EDL::copy_session(EDL *edl, int session_only)
{
	if(!session_only)
	{
		strcpy(this->project_path, edl->project_path);

		folders.remove_all_objects();
		for(int i = 0; i < edl->folders.total; i++)
		{
			char *new_folder;
			folders.append(new_folder = new char[strlen(edl->folders.values[i]) + 1]);
			strcpy(new_folder, edl->folders.values[i]);
		}
	}

	if(!parent_edl)
	{
		session->copy(edl->session);
	}

	if(!session_only)
	{
		local_session->copy_from(edl->local_session);
	}
}

int EDL::copy_assets(double start, 
	double end, 
	FileXML *file, 
	int all, 
	ArrayList<PluginServer*> *plugindb,
	char *output_path)
{
	ArrayList<Asset*> asset_list;
	Track* current;

	file->tag.set_title("ASSETS");
	file->append_tag();
	file->append_newline();

// Copy everything for a save
	if(all)
	{
		for(Asset *asset = assets->first;
			asset;
			asset = asset->next)
		{
			asset_list.append(asset);
		}
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
	}

// Paths relativised here
	for(int i = 0; i < asset_list.total; i++)
	{
		asset_list.values[i]->write(file, 
			0, 
			output_path);
	}

	file->tag.set_title("/ASSETS");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	return 0;
}

int EDL::copy(double start, 
	double end, 
	int all, 
	int is_clip,
	int is_vwindow,
	FileXML *file, 
	ArrayList<PluginServer*> *plugindb, 
	char *output_path,
	int rewind_it)
{
//printf("EDL::copy 1\n");
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
		file->tag.set_property("LENGTH", end - start);
		file->append_tag();
		file->tag.set_title("/CLIPBOARD");
		file->append_tag();
		file->append_newline();
		file->append_newline();
	}
//printf("EDL::copy 1\n");

// Sessions
	local_session->save_xml(file, start);

//printf("EDL::copy 1\n");

// Top level stuff.
//	if(!parent_edl)
	{
// Need to copy all this from child EDL if pasting is desired.
// Session
		session->save_xml(file);
		session->save_video_config(file);
		session->save_audio_config(file);

// Folders
		for(int i = 0; i < folders.total; i++)
		{
			file->tag.set_title("FOLDER");
			file->append_tag();
			file->append_text(folders.values[i]);
			file->tag.set_title("/FOLDER");
			file->append_tag();
			file->append_newline();
		}

// Media
// Don't replicate all assets for every clip.
// The assets for the clips are probably in the mane EDL.
		if(!is_clip)
			copy_assets(start, 
				end, 
				file, 
				all, 
				plugindb,
				output_path);

// Clips
// Don't want this if using clipboard
		if(all)
		{
			if(vwindow_edl)
			{
				
				vwindow_edl->save_xml(plugindb, 
					file, 
					output_path,
					0,
					1);
			}

			for(int i = 0; i < clips.total; i++)
				clips.values[i]->save_xml(plugindb, 
					file, 
					output_path,
					1,
					0);
		}

		file->append_newline();
		file->append_newline();
	}


//printf("EDL::copy 1\n");

	labels->copy(start, end, file);
//printf("EDL::copy 1\n");
	tracks->copy(start, end, all, file, output_path);
//printf("EDL::copy 2\n");

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
	return 0;
}

void EDL::rechannel()
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO)
		{
			PanAutos *autos = (PanAutos*)current->automation->autos[AUTOMATION_PAN];
			((PanAuto*)autos->default_auto)->rechannel();
			for(PanAuto *keyframe = (PanAuto*)autos->first;
				keyframe;
				keyframe = (PanAuto*)keyframe->next)
			{
				keyframe->rechannel();
			}
		}
	}
}

void EDL::resample(double old_rate, double new_rate, int data_type)
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current->data_type == data_type)
		{
			current->resample(old_rate, new_rate);
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

int EDL::trim_selection(double start, 
	double end,
	int edit_labels,
	int edit_plugins)
{
	if(start != end)
	{
// clear the data
		clear(0, 
			start,
			edit_labels,
			edit_plugins);
		clear(end - start, 
			tracks->total_length(),
			edit_labels,
			edit_plugins);
	}
	return 0;
}


int EDL::equivalent(double position1, double position2)
{
	double threshold = (double).5 / session->frame_rate;
	if(session->cursor_on_frames) 
		threshold = (double).5 / session->frame_rate;
	else
		threshold = (double)1 / session->sample_rate;

	if(fabs(position2 - position1) < threshold)
    	return 1;
    else
        return 0;
}

double EDL::equivalent_output(EDL *edl)
{
	double result = -1;
	session->equivalent_output(edl->session, &result);
	tracks->equivalent_output(edl->tracks, &result);
	return result;
}


void EDL::set_project_path(char *path)
{
	strcpy(this->project_path, path);
}

void EDL::set_inpoint(double position)
{
	if(equivalent(local_session->get_inpoint(), position) && 
		local_session->get_inpoint() >= 0)
	{
		local_session->unset_inpoint();
	}
	else
	{
		local_session->set_inpoint(align_to_frame(position, 0));
		if(local_session->get_outpoint() <= local_session->get_inpoint()) 
			local_session->unset_outpoint();
	}
}

void EDL::set_outpoint(double position)
{
	if(equivalent(local_session->get_outpoint(), position) && 
		local_session->get_outpoint() >= 0)
	{
		local_session->unset_outpoint();
	}
	else
	{
		local_session->set_outpoint(align_to_frame(position, 0));
		if(local_session->get_inpoint() >= local_session->get_outpoint()) 
			local_session->unset_inpoint();
	}
}


int EDL::clear(double start, 
	double end, 
	int clear_labels,
	int clear_plugins)
{
	if(start == end)
	{
		double distance = 0;
		tracks->clear_handle(start, 
			end,
			distance, 
			clear_labels,
			clear_plugins);
		if(clear_labels && distance > 0)
			labels->paste_silence(start, 
				start + distance);
	}
	else
	{
		tracks->clear(start, 
			end,
			clear_plugins);
		if(clear_labels) 
			labels->clear(start, 
				end, 
				1);
	}

// Need to put at beginning so a subsequent paste operation starts at the
// right position.
	double position = local_session->get_selectionstart();
	local_session->set_selectionend(position);
	local_session->set_selectionstart(position);
	return 0;
}

void EDL::modify_edithandles(double oldposition, 
	double newposition, 
	int currentend,
	int handle_mode,
	int edit_labels,
	int edit_plugins)
{
	tracks->modify_edithandles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		edit_labels, 
		edit_plugins);
	labels->modify_handles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		edit_labels);
}

void EDL::modify_pluginhandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	Edits *trim_edits)
{
	tracks->modify_pluginhandles(oldposition, 
		newposition, 
		currentend, 
		handle_mode,
		edit_labels,
		trim_edits);
	optimize();
}

void EDL::paste_silence(double start, 
	double end, 
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
			this->assets->remove_asset(assets->values[i]);
		}
	}
}

void EDL::update_assets(EDL *src)
{
	for(Asset *current = src->assets->first;
		current;
		current = NEXT)
	{
		assets->update(current);
	}
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

int64_t EDL::get_tracks_width()
{
	int64_t total_pixels = 0;
	for(Track *current = tracks->first;
		current;
		current = NEXT)
	{
		int64_t pixels = current->horizontal_span();
		if(pixels > total_pixels) total_pixels = pixels;
	}
//printf("EDL::get_tracks_width %d\n", total_pixels);
	return total_pixels;
}

// int EDL::calculate_output_w(int single_channel)
// {
// 	if(single_channel) return session->output_w;
// 
// 	int widest = 0;
// 	for(int i = 0; i < session->video_channels; i++)
// 	{
// 		if(session->vchannel_x[i] + session->output_w > widest) widest = session->vchannel_x[i] + session->output_w;
// 	}
// 	return widest;
// }
// 
// int EDL::calculate_output_h(int single_channel)
// {
// 	if(single_channel) return session->output_h;
// 
// 	int tallest = 0;
// 	for(int i = 0; i < session->video_channels; i++)
// 	{
// 		if(session->vchannel_y[i] + session->output_h > tallest) tallest = session->vchannel_y[i] + session->output_h;
// 	}
// 	return tallest;
// }

// Get the total output size scaled to aspect ratio
void EDL::calculate_conformed_dimensions(int single_channel, float &w, float &h)
{
	w = session->output_w;
	h = session->output_h;

	if((float)session->output_w / session->output_h > get_aspect_ratio())
	{
		h = (float)h * 
			(session->output_w / get_aspect_ratio() / session->output_h);
	}
	else
	{
		w = (float)w * 
			(h * get_aspect_ratio() / session->output_w);
	}
}

float EDL::get_aspect_ratio()
{
	return session->aspect_w / session->aspect_h;
}

int EDL::dump()
{
	if(parent_edl)
		printf("CLIP\n");
	else
		printf("EDL\n");
	printf("clip_title: %s parent_edl: %p\n", local_session->clip_title, parent_edl);
	printf("selectionstart %f selectionend %f loop_start %f loop_end %f\n", 
		local_session->get_selectionstart(1), 
		local_session->get_selectionend(1),
		local_session->loop_start,
		local_session->loop_end);

	if(!parent_edl)
	{
		printf("audio_channels: %d "
			"audio_tracks: %d \n"
			"sample_rate: %d\n",
			session->audio_channels,
			session->audio_tracks,
			session->sample_rate);
		printf("video_channels: %d "
			"video_tracks: %d "
			"frame_rate: %.2f "
			"frames_per_foot: %.2f\n"
    		"output_w: %d "
    		"output_h: %d "
    		"aspect_w: %f "
    		"aspect_h %f "
			"color_model %d\n",
				session->video_channels,
				session->video_tracks,
				session->frame_rate,
				session->frames_per_foot,
    			session->output_w,
    			session->output_h,
    			session->aspect_w,
    			session->aspect_h,
				session->color_model);

		printf(" EDLS\n");
		printf("  total: %d\n", clips.total);
	
		for(int i = 0; i < clips.total; i++)
		{
			printf("\n\n");
			clips.values[i]->dump();
			printf("\n\n");
		}

		printf(" ASSETS\n");
		assets->dump();
	}
	printf(" LABELS\n");
	labels->dump();
	printf(" TRACKS\n");
	tracks->dump();
//printf("EDL::dump 2\n");
	return 0;
}

EDL* EDL::add_clip(EDL *edl)
{
// Copy argument.  New edls are deleted from MWindow::load_filenames.
	EDL *new_edl = new EDL(this);
	new_edl->create_objects();
	new_edl->copy_all(edl);
	clips.append(new_edl);
	return new_edl;
}

void EDL::insert_asset(Asset *asset, 
	double position, 
	Track *first_track, 
	RecordLabels *labels)
{
// Insert asset into asset table
	Asset *new_asset = assets->update(asset);


// Paste video
	int vtrack = 0;
	Track *current = first_track ? first_track : tracks->first;


// Fix length of single frame
	double length;


	if(new_asset->video_length < 0) 
	{
	if(session->si_useduration)
		length = session->si_duration;
	else	
		length = 1.0 / session->frame_rate; 
	}
	else
	if(new_asset->frame_rate > 0)
		length = ((double)new_asset->video_length / new_asset->frame_rate);
	else
		length = 1.0 / session->frame_rate;

	for( ;
		current && vtrack < new_asset->layers;
		current = NEXT)
	{
		if(!current->record || 
			current->data_type != TRACK_VIDEO)
			continue;

		current->insert_asset(new_asset, 
			length, 
			position, 
			vtrack);

		vtrack++;
	}

	int atrack = 0;
	for(current = tracks->first;
		current && atrack < new_asset->channels;
		current = NEXT)
	{
		if(!current->record ||
			current->data_type != TRACK_AUDIO)
			continue;

		current->insert_asset(new_asset, 
			(double)new_asset->audio_length / 
				new_asset->sample_rate, 
			position, 
			atrack);


		atrack++;
	}

	if(labels)
	{
		for(RecordLabel *label = labels->first; label; label = label->next)
		{
			this->labels->toggle_label(label->position, label->position);
		}
	}
}



void EDL::set_index_file(Asset *asset)
{
	assets->update_index(asset);
}

void EDL::optimize()
{
//printf("EDL::optimize 1\n");
	double length = tracks->total_length();
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
					PLAY_FORWARD, 
					1,
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
double EDL::align_to_frame(double position, int round)
{
//printf("EDL::align_to_frame 1 %f\n", position);
	if(session->cursor_on_frames)
	{
// Seconds -> Frames
		double temp = (double)position * session->frame_rate;
//printf("EDL::align_to_frame 2 %f\n", temp);

// Assert some things
		if(session->sample_rate == 0)
			printf("EDL::align_to_frame: sample_rate == 0\n");

		if(session->frame_rate == 0)
			printf("EDL::align_to_frame: frame_rate == 0\n");

// Round frames
// Always round down negative numbers
// but round up only if requested
		if(round) 
		{
			temp = Units::round(temp);
		}
		else
		{
// 			if(temp < 0)
// 			{
// 				temp -= 0.5;
// 			}
// 			else
				temp = Units::to_int64(temp);
		}
//printf("EDL::align_to_frame 3 %f\n", temp);

// Frames -> Seconds
		temp /= session->frame_rate;

//printf("EDL::align_to_frame 5 %f\n", temp);

		return temp;
	}
//printf("EDL::align_to_frame 3 %d\n", position);


	return position;
}


void EDL::new_folder(char *folder)
{
	for(int i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(folders.values[i], folder)) return;
	}

	char *new_folder;
	folders.append(new_folder = new char[strlen(folder) + 1]);
	strcpy(new_folder, folder);
}

void EDL::delete_folder(char *folder)
{
	int i;
	for(i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(folders.values[i], folder))
		{
			break;
		}
	}

	if(i < folders.total) delete folders.values[i];

	for( ; i < folders.total - 1; i++)
	{
		folders.values[i] = folders.values[i + 1];
	}
}
