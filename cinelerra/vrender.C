
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
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "condition.h"
#include "datatype.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "interlacemodes.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "strategies.inc"
#include "tracks.h"
#include "transportcommand.h"
#include "units.h"
#include "vedit.h"
#include "vframe.h"
#include "videodevice.h"
#include "virtualconsole.h"
#include "virtualvconsole.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"





VRender::VRender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
	data_type = TRACK_VIDEO;
	transition_temp = 0;
	overlayer = new OverlayFrame(renderengine->preferences->processors);
	input_temp = 0;
}

VRender::~VRender()
{
	if(input_temp) delete input_temp;
	if(transition_temp) delete transition_temp;
	if(overlayer) delete overlayer;
}


VirtualConsole* VRender::new_vconsole_object() 
{
	return new VirtualVConsole(renderengine, this);
}

int VRender::get_total_tracks()
{
	return renderengine->edl->tracks->total_video_tracks();
}

Module* VRender::new_module(Track *track)
{
	return new VModule(renderengine, this, 0, track);
}

int VRender::flash_output()
{
	flashed_pts = video_out->get_pts();

	return renderengine->video->write_buffer(video_out, renderengine->edl);
}

int VRender::process_buffer(VFrame *video_out)
{
// process buffer for non realtime
	int i, j;
	ptstime render_len = fromunits(1);
	int reconfigure = 0;

	this->video_out = video_out;

	current_postime = video_out->get_pts();

// test for automation configuration and shorten the fragment len if necessary
	reconfigure = vconsole->test_reconfigure(render_len,
		last_playback);

	if(reconfigure) restart_playback();
	return process_buffer(current_postime);
}


int VRender::process_buffer(ptstime input_postime)
{
	Edit *playable_edit = 0;
	int colormodel;
	int use_vconsole = 1;
	int use_brender = 0;
	int result = 0;
	int use_cache = renderengine->command->single_frame();
	int use_asynchronous = 
		renderengine->command->realtime && 
		renderengine->edl->session->video_asynchronous;
// Determine the rendering strategy for this frame.
	use_vconsole = get_use_vconsole(playable_edit, 
		input_postime,
		use_brender);
// Negotiate color model
	colormodel = get_colormodel(playable_edit, use_vconsole, use_brender);

// Get output buffer from device
	if(renderengine->command->realtime)
		renderengine->video->new_output_buffer(&video_out, colormodel);

// Read directly from file to video_out
	if(!use_vconsole)
	{

		if(use_brender)
		{
			Asset *asset = renderengine->preferences->brender_asset;
			File *file = renderengine->get_vcache()->check_out(asset,
				renderengine->edl);
			if(file)
			{
// Cache single frames only
				if(use_asynchronous)
					file->start_video_decode_thread();
				else
					file->stop_video_thread();

				if(use_cache) file->set_cache_frames(1);
				video_out->set_source_pts(current_postime);
				file->get_frame(video_out);
				if(use_cache) file->set_cache_frames(0);
				video_out->set_pts(video_out->get_source_pts());
				renderengine->get_vcache()->check_in(asset);
			}
		}
		else
		if(playable_edit)
		{
			result = ((VEdit*)playable_edit)->read_frame(video_out, 
				current_postime,
				renderengine->get_vcache(),
				1,
				use_cache,
				use_asynchronous);
		}
	}
	else
// Read into virtual console
	{
// process this buffer now in the virtual console
		((VirtualVConsole*)vconsole)->process_buffer(input_postime);
	}
	return result;
}

// Determine if virtual console is needed
int VRender::get_use_vconsole(Edit* &playable_edit, 
	ptstime position,
	int &use_brender)
{
	Track *playable_track;

// Background rendering completed
	if((use_brender = renderengine->brender_available(position)) != 0) 
		return 0;

	return 1;
}


int VRender::get_colormodel(Edit* &playable_edit, 
	int use_vconsole,
	int use_brender)
{
	int colormodel = renderengine->edl->session->color_model;

	if(!use_vconsole && !renderengine->command->single_frame())
	{
// Get best colormodel supported by the file
		int driver = renderengine->config->vconfig->driver;
		File *file;
		Asset *asset;

		if(use_brender)
		{
			asset = renderengine->preferences->brender_asset;
		}
		else
		{
			asset = playable_edit->asset;
		}

		file = renderengine->get_vcache()->check_out(asset,
			renderengine->edl);

		if(file)
		{
			colormodel = file->get_best_colormodel(driver);
			renderengine->get_vcache()->check_in(asset);
		}
	}
	return colormodel;
}

void VRender::run()
{
	int reconfigure;
	ptstime current_pts, start_pts, end_pts;
	ptstime init_pos = current_postime;
// Number of frames before next reconfigure
	ptstime current_input_duration;
	ptstime len_pts = fromunits(1);
	int direction = renderengine->command->get_direction();
	first_frame = 1;

	framerate_counter = 0;
	framerate_timer.update();
	start_lock->unlock();
	flashed_pts = -1;

	while(!done)
	{
// Perform the most time consuming part of frame decompression now.
// Want the condition before, since only 1 frame is rendered 
// and the number of frames skipped after this frame varies
		reconfigure = vconsole->test_reconfigure(len_pts,
			last_playback);

		if(reconfigure) restart_playback();
		process_buffer(current_postime);
		current_postime = video_out->get_pts();

		if(last_playback || renderengine->video->interrupt 
				|| renderengine->command->single_frame())
		{
			flash_output();
			current_input_duration = video_out->get_duration();
			done = 1;
		}
		else
// Perform synchronization
		{
			if(first_frame)
			{
				flash_output();
				renderengine->wait_another("VRender::run", TRACK_VIDEO);
				init_pos = current_postime;
			}
// Determine the delay until the frame needs to be shown.
			current_pts = renderengine->sync_postime() *
				renderengine->command->get_speed();
// earliest time by which the frame needs to be shown.
			start_pts = current_postime;
			if((len_pts = video_out->get_duration()) < EPSILON)
// We have no current frame
				len_pts = fromunits(1);

// latest time at which the frame can be shown.
			end_pts = start_pts + len_pts;

			if(direction == PLAY_REVERSE)
			{
				ptstime t = start_pts;
				start_pts = -end_pts;
				end_pts = -t;
				current_pts -=  init_pos;
			} else
				current_pts += init_pos;

			if(end_pts < current_pts)
			{
// Frame rendered late. Flash it now.
				if(!first_frame)
					flash_output();

				if(renderengine->edl->session->video_every_frame)
				{
// User wants every frame.
					current_input_duration = len_pts;
				}
				else
				{
// Get the frames to skip.
					current_input_duration = current_pts - end_pts + len_pts;
				}
			}
			else
			{
// Frame rendered early or just in time.
				current_input_duration = len_pts;
				if(start_pts > current_pts)
				{
					int64_t delay_time = (int64_t)((start_pts - current_pts) * 1000);
					timer.delay(delay_time);
				}
// Flash frame now.
				flash_output();
			}
			first_frame = 0;
		}
// advance position in project
		if(!last_playback)
		{
			get_boundaries(current_input_duration, video_out->get_duration());
			first_frame = advance_position(video_out->get_pts(), current_input_duration);
		}

		if(renderengine->command->realtime &&
			!renderengine->video->interrupt)
		{
// Calculate the framerate counter
			framerate_counter++;
			if(framerate_counter >= renderengine->edl->session->frame_rate)
			{
				renderengine->update_framerate((float)framerate_counter / 
					((float)framerate_timer.get_difference() / 1000));
				framerate_counter = 0;
				framerate_timer.update();
			}
		}
	}
	renderengine->stop_tracking(flashed_pts, TRACK_VIDEO);
	renderengine->render_start_lock->unlock();
	stop_plugins();
}





VRender::VRender(MWindow *mwindow, RenderEngine *renderengine)
 : CommonRender(mwindow, renderengine)
{
	framerate_counter = 0;
	video_out = 0;
}

int VRender::get_datatype()
{
	return TRACK_VIDEO;
}


int VRender::start_playback()
{
// start reading input and sending to vrenderthread
// use a thread only if there's a video device
	if(renderengine->command->realtime)
	{
		start();
	}
}

posnum VRender::tounits(ptstime position, int round)
{
	if(round)
		return Units::round(position * renderengine->edl->session->frame_rate);
	else
		return Units::to_int64(position * renderengine->edl->session->frame_rate);
}

ptstime VRender::fromunits(posnum position)
{
	return (ptstime)position / renderengine->edl->session->frame_rate;
}
