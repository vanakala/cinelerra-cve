
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
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "transportcommand.h"
#include "units.h"
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
	overlayer = new OverlayFrame(preferences_global->processors);
}

VRender::~VRender()
{
	if(overlayer) delete overlayer;
}


VirtualConsole* VRender::new_vconsole_object() 
{
	return new VirtualVConsole(renderengine, this);
}

int VRender::get_total_tracks()
{
	return renderengine->edl->total_tracks_of(TRACK_VIDEO);
}

Module* VRender::new_module(Track *track)
{
	return new VModule(renderengine, this, 0, track);
}

int VRender::flash_output()
{
	if(PTSEQU(video_out->get_pts(), flashed_pts))
		return 0;
	frame_count++;
	flashed_pts = video_out->get_pts();
	return renderengine->video->write_buffer(video_out, renderengine->edl);
}

VFrame *VRender::process_buffer(VFrame *buffer)
{
// process buffer for non realtime
	ptstime render_len = edlsession->frame_duration();

	video_out = buffer;

	current_postime = video_out->get_pts();

// test for automation configuration and shorten the fragment len if necessary
	if(vconsole->test_reconfigure(render_len, last_playback))
		restart_playback();
	process_buffer(current_postime);
	return video_out;
}


void VRender::process_buffer(ptstime input_postime)
{
// Get output buffer from device
	if(renderengine->command.realtime)
		video_out = renderengine->video->new_output_buffer(
			edlsession->color_model);
	input_postime = round(input_postime *
		edlsession->frame_rate) /
		edlsession->frame_rate;
	if(renderengine->brender_available(current_postime))
	{
		Asset *asset = preferences_global->brender_asset;
		File *file = renderengine->get_vcache()->check_out(asset);

		if(file)
		{
// Cache single frames only
			video_out->set_source_pts(current_postime);
			file->get_frame(video_out);
			video_out->set_pts(video_out->get_source_pts());
			renderengine->get_vcache()->check_in(asset);
		}
	}
	else
// Read into virtual console
	{
		((VirtualVConsole*)vconsole)->process_buffer(input_postime);
	}
}

void VRender::run()
{
	int reconfigure;
	ptstime current_pts, start_pts, end_pts;
	ptstime init_pos = current_postime;
// Number of frames before next reconfigure
	ptstime current_input_duration;
	ptstime len_pts = edlsession->frame_duration();
	int direction = renderengine->command.get_direction();
// Statistics
	frame_count = 0;
	sum_dly = 0;
	late_frame = 0;

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
				|| renderengine->command.single_frame())
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
				renderengine->command.get_speed();
// earliest time by which the frame needs to be shown.
			start_pts = current_postime;
			if((len_pts = video_out->get_duration()) < EPSILON)
// We have no current frame
				len_pts = edlsession->frame_duration();

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
				late_frame++;

				if(edlsession->video_every_frame)
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
					Timer::delay(delay_time);
					sum_dly += delay_time;
				}
// Flash frame now.
				flash_output();
			}
			first_frame = 0;
		}
// advance position in project
		if(!last_playback)
		{
			ptstime project_len_pts = edlsession->frame_duration();

			get_boundaries(current_input_duration, project_len_pts / 2.0);
			first_frame = advance_position(video_out->get_pts(),
				project_len_pts);
		}

		if(renderengine->command.realtime &&
			!renderengine->video->interrupt)
		{
// Calculate the framerate counter
			framerate_counter++;
			if(framerate_counter >= edlsession->frame_rate)
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

	if(frame_count)
		renderengine->update_playstatistics(frame_count, late_frame,
			(int)(sum_dly / frame_count));
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
	if(renderengine->command.realtime)
	{
		start();
	}
}
