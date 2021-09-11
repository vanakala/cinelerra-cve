// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>


#include "bcsignals.h"
#include "bcresources.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "plugin.h"
#include "pluginclient.h"
#include "preferences.h"
#include "renderengine.h"
#include "tmpframecache.h"
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
	frame = BC_Resources::tmpframes.get_tmpframe(
		edl->this_edlsession->output_w,
		edl->this_edlsession->output_h,
		edl->this_edlsession->color_model);
	frame->set_duration(edl->this_edlsession->frame_duration());

	if(renderengine->brender_available(pts))
	{
		if(!brender_file)
		{
			brender_file = new File;
			if(brender_file->open_file(preferences_global->brender_asset,
					FILE_OPEN_READ | FILE_OPEN_VIDEO, 0))
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
	int found;
	int count = 0;

	frame->clear_frame();
	frame->set_pts(pts);

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_VIDEO || track->renderer)
			continue;
		track->renderer = new VTrackRender(track, this);
	}

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_VIDEO)
			continue;
		((VTrackRender*)track->renderer)->process_vframe(pts, RSTEP_NORMAL);
	}

	for(;;)
	{
		found = 0;
		for(Track *track = edl->tracks->last; track; track = track->previous)
		{
			if(track->data_type != TRACK_VIDEO || track->renderer->track_ready())
				continue;
			((VTrackRender*)track->renderer)->process_vframe(pts, RSTEP_SHARED);
			found = 1;
		}
		if(!found)
			break;
		if(++count > 3)
			break;
	}

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_VIDEO)
			continue;
		frame = ((VTrackRender *)track->renderer)->render_projector(frame);
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
	{
		BC_Resources::tmpframes.release_frame(frame);
		return;
	}
// Do not flash the same frame
	if(flashed_pts <= pts && pts < flashed_pts + flashed_duration)
	{
		BC_Resources::tmpframes.release_frame(frame);
		return;
	}
	frame_count++;
	framerate_counter++;
	flashed_pts = pts;
	flashed_duration = duration;
	renderengine->video->write_buffer(frame, edl);
	renderengine->set_tracking_position(flashed_pts, TRACK_VIDEO);
}

void VideoRender::pass_vframes(Plugin *plugin, VFrame *current_frame,
	VTrackRender *current_renderer)
{
	VFrame *vframe;

	current_renderer->vframes.remove_all();
	current_renderer->vframes.append(current_frame);

	// Add frames for other tracks starting from the first
	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != TRACK_VIDEO)
			continue;
		for(int i = 0; i < track->plugins.total; i++)
		{
			if(track->plugins.values[i]->shared_plugin == plugin &&
				track->plugins.values[i]->on)
			{
				if(vframe = ((VTrackRender*)track->renderer)->handover_trackframe())
					current_renderer->vframes.append(vframe);
			}
		}
	}
	if(current_renderer->initialized_buffers != current_renderer->vframes.total)
	{
		plugin->client->plugin_init(current_renderer->vframes.total);
		current_renderer->initialized_buffers = current_renderer->vframes.total;
	}
}

VFrame *VideoRender::take_vframes(Plugin *plugin, VTrackRender *current_renderer)
{
	int k = 1;
	VFrame *current_frame = current_renderer->vframes.values[0];

	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != TRACK_VIDEO)
			continue;
		for(int i = 0; i < track->plugins.total; i++)
		{
			if(track->plugins.values[i]->shared_plugin == plugin)
			{
				if(k >= current_renderer->vframes.total)
					break;
				VFrame *frame = current_renderer->vframes.values[k];
				if(frame->get_layer() == track->number_of())
				{
					((VTrackRender*)track->renderer)->take_vframe(frame);
					k++;
				}
			}
		}
	}
	return current_frame;
}

void VideoRender::release_asset(Asset *asset)
{
	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != TRACK_VIDEO || !track->renderer)
			continue;
		track->renderer->release_asset(asset);
	}
}
