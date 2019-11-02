
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "preferences.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"
#include "videodevice.h"
#include "videorender.h"
#include "vframe.h"
#include "vtrackrender.h"

VideoRender::VideoRender(RenderEngine *renderengine, EDL *edl)
 : RenderBase(renderengine, edl)
{
	brender_file = 0;
	frame = 0;
	flashed_pts = -1;
	flashed_duration = 0;
}

VideoRender::~VideoRender()
{
	delete brender_file;
}

void VideoRender::run()
{
	int first_frame = 1;
	ptstime init_pts = render_pts;
	ptstime current_pts;
	ptstime start_pts, end_pts;
	ptstime current_input_duration;
	ptstime duration = edl->this_edlsession->frame_duration();

// Statistics
	frame_count = 0;
	sum_delay = 0;
	late_frame = 0;
	framerate_counter = 0;
	framerate_timer.update();
	flashed_pts = -1;

	start_lock->unlock();
	while(1)
	{
		get_frame(render_pts);

		if(renderengine->video->interrupt || last_playback ||
			render_single)
		{
			flash_output();
			break;
		}
		current_pts = renderengine->sync_postime() *
			renderengine->command.get_speed();
		if(first_frame)
		{
			flash_output();
			renderengine->wait_another("VideoRender::run", TRACK_VIDEO);
			init_pts = render_pts - current_pts;
		}
// earliest time when the frame can be shown
		start_pts = render_pts;
		if((duration = frame->get_duration()) < EPSILON)
			duration = edl->this_edlsession->frame_duration();
// latest time when the frame can be shown
		end_pts = start_pts + duration;
		if(render_direction == PLAY_REVERSE)
		{
			ptstime t = start_pts;
			start_pts = -end_pts;
			end_pts = -t;
			current_pts -= init_pts;
			duration = edl->this_edlsession->frame_duration();
		} else
			current_pts += init_pts;

		if(end_pts < current_pts)
		{
// Frame rendered late. Flash it now
			if(!first_frame)
				flash_output();
			late_frame++;

			if(edl->this_edlsession->video_every_frame)
				current_input_duration = duration;
			else
// Duration to skip
				current_input_duration = current_pts - end_pts + duration;
		}
		else
		{
// Frame rendered early or just in time.
			current_input_duration = duration;
			if(start_pts > current_pts)
			{
				int64_t delay_time =
					(int64_t)((start_pts - current_pts) * 1000);

				if(delay_time > (int64_t)(FRAME_ACCURACY * 1000))
				{
					Timer::delay(delay_time);
					sum_delay += delay_time;
				}
			}
			flash_output();
		}
		first_frame = advance_position(current_input_duration);

		if(!renderengine->video->interrupt &&
			framerate_counter >= edl->this_edlsession->frame_rate)
		{
			renderengine->update_framerate((float)framerate_counter /
				((float)framerate_timer.get_difference() / 1000));
			framerate_counter = 0;
			framerate_timer.update();
		}
	}
	renderengine->stop_tracking(flashed_pts, TRACK_VIDEO);
	renderengine->render_start_lock->unlock();

	if(frame_count)
		renderengine->update_playstatistics(frame_count, late_frame,
			(int)(sum_delay / frame_count));
}

void VideoRender::get_frame(ptstime pts)
{
	frame = renderengine->video->new_output_buffer(
		edl->this_edlsession->color_model);

	if(renderengine->brender_available(pts))
	{
		if(!brender_file)
		{
			brender_file = new File;
			if(brender_file->open_file(preferences_global->brender_asset,
					FILE_OPEN_READ | FILE_OPEN_VIDEO))
			{
				delete brender_file;
				brender_file = 0;
			}
		}
		if(brender_file)
		{
			frame->set_source_pts(pts);
			brender_file->get_frame(frame);
			frame->set_pts(frame->get_source_pts());
			return;
		}
	}
	process_frame(pts);
}

void VideoRender::process_frame(ptstime pts)
{
	VTrackRender *trender;

	frame->set_pts(pts);

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_VIDEO)
			continue;
		trender = (VTrackRender *)track->renderer;
		if(!trender)
		{
			trender = new VTrackRender(track);
			track->renderer = trender;
		}
		frame = trender->get_frame(frame);
	}
}

VFrame *VideoRender::process_buffer(VFrame *buffer)
{
	frame = buffer;
	process_frame(buffer->get_pts());
	return frame;
}

void VideoRender::flash_output()
{
	ptstime pts = frame->get_pts();
	ptstime duration = frame->get_duration();

// Do not flash frames that are too short
	if(!last_playback && !render_single && duration < FRAME_ACCURACY)
		return;
// Do not flash the same frame
	if(flashed_pts <= pts && pts < flashed_pts + flashed_duration)
		return;
	frame_count++;
	framerate_counter++;
	flashed_pts = pts;
	flashed_duration = duration;
	renderengine->video->write_buffer(frame, edl);
	renderengine->set_tracking_position(flashed_pts, TRACK_VIDEO);
}
