// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "automation.h"
#include "bcresources.h"
#include "clip.h"
#include "cropautos.h"
#include "cropengine.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "fadeengine.h"
#include "file.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "overlayframe.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "tmpframecache.h"
#include "track.h"
#include "videorender.h"
#include "vtrack.h"
#include "vtrackrender.h"
#include "vframe.h"

VTrackRender::VTrackRender(Track *track, VideoRender *vrender)
 : TrackRender(track)
{
	fader = 0;
	masker = 0;
	cropper = 0;
	overlayer = 0;
	videorender = vrender;
	track_frame = 0;
}

VTrackRender::~VTrackRender()
{
	delete fader;
	delete masker;
	delete cropper;
	delete overlayer;
	BC_Resources::tmpframes.release_frame(track_frame);
}

void VTrackRender::read_vframe(VFrame *vframe, Edit *edit, int filenum)
{
	ptstime pts = vframe->get_pts();
	ptstime src_pts;
	File *file = media_file(edit, filenum);

	if(!file)
	{
		vframe->set_duration(media_track->edl->this_edlsession->frame_duration());
		vframe->clear_frame();
		return;
	}

	src_pts = pts - edit->get_pts() + edit->get_source_pts() - media_track->nudge;
	vframe->set_source_pts(src_pts);
	file->get_frame(vframe);
	pts = align_to_frame(pts);
	vframe->set_pts(pts);
	vframe->set_duration(media_track->edl->this_edlsession->frame_duration());
}

void VTrackRender::process_vframe(ptstime pts, int rstep)
{
	Edit *edit = media_track->editof(pts);

	pts = align_to_frame(pts);
	if(is_playable(pts, edit))
	{
		if(rstep == RSTEP_NORMAL)
		{
			track_frame = BC_Resources::tmpframes.get_tmpframe(
				media_track->track_w, media_track->track_h,
				edlsession->color_model);
			track_frame->set_pts(pts);
			read_vframe(track_frame, edit);
			track_frame = render_transition(track_frame, edit);
			track_frame = render_camera(track_frame);
			render_mask(track_frame, 1);
			render_crop(track_frame, 1);
			track_frame = render_plugins(track_frame, edit);
		}
		else
		{
			if(next_plugin)
				track_frame = render_plugins(track_frame, edit);
		}
		if(!next_plugin)
		{
			render_fade(track_frame);
			render_mask(track_frame, 0);
			render_crop(track_frame, 0);
		}
	}
	else
		next_plugin = 0;
}

void VTrackRender::render_fade(VFrame *frame)
{
	double value = autos_track->automation->get_floatvalue(
		frame->get_pts(), AUTOMATION_VFADE);

	CLAMP(value, 0, 100);

	value /= 100;

	if(!EQUIV(value, 1.0))
	{
		if(!fader)
			fader = new FadeEngine(preferences_global->max_threads);
		fader->do_fade(frame, value);
		frame->set_transparent();
	}
}

void VTrackRender::render_mask(VFrame *frame, int before_plugins)
{
	int total_points = 0;
	MaskAuto *keyframe = (MaskAuto*)autos_track->automation->get_prev_auto(frame->get_pts(), AUTOMATION_MASK);

	if(!keyframe)
		return;

	MaskAutos *keyframe_set =
		(MaskAutos*)autos_track->automation->have_autos(AUTOMATION_MASK);

	for(int i = 0; i < keyframe->masks.total; i++)
	{
		int submask_points = keyframe->get_submask(i)->points.total;

		if(submask_points > 1)
			total_points += submask_points;
	}

// Ignore masks that are not relevant
	if(total_points <= 2 || (keyframe->value == 0 &&
			(keyframe_set->get_mode() == MASK_SUBTRACT_ALPHA ||
			keyframe_set->get_mode() == MASK_SUBTRACT_COLOR)))
		return;

// Fake certain masks
	if(keyframe->value == 0 &&
		keyframe_set->get_mode() == MASK_MULTIPLY_COLOR)
	{
		frame->clear_frame();
		return;
	}
	if(!masker)
		masker = new MaskEngine(preferences_global->max_threads);

	masker->do_mask(frame, keyframe_set, before_plugins);
	frame->set_transparent();
}

void VTrackRender::render_crop(VFrame *frame, int before_plugins)
{
	int left, right, top, bottom;
	CropAutos *cropautos = (CropAutos *)autos_track->automation->have_autos(AUTOMATION_CROP);

	if(!cropautos || !cropautos->first)
		return;

	cropautos->get_values(frame->get_pts(),
		&left, &right, &top, &bottom);

	if(left == 0 && top == 0 && right >= frame->get_w() && bottom >= frame->get_h())
		return;

	if(!cropper)
		cropper = new CropEngine();

	frame = cropper->do_crop(cropautos, frame, before_plugins);
}

VFrame *VTrackRender::render_projector(VFrame *output, VFrame **input)
{
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	int mode;
	IntAuto *mode_keyframe;

	if(!input)
		input = &track_frame;

	if(autos_track->automation->get_intvalue(output->get_pts(), AUTOMATION_MUTE))
	{
		BC_Resources::tmpframes.release_frame(*input);
		return output;
	}

	if(!*input || *input == output)
	{
		*input = 0;
		return output;
	}

	calculate_output_transfer(output,
		&in_x1, &in_y1, &in_x2, &in_y2,
		&out_x1, &out_y1, &out_x2, &out_y2);

	if(out_x2 > out_x1 && out_y2 > out_y1 &&
		in_x2 > in_x1 && in_y2 > in_y1)
	{
		mode_keyframe = (IntAuto*)autos_track->automation->get_prev_auto(
			(*input)->get_pts(), AUTOMATION_MODE);

		if(mode_keyframe)
			mode = mode_keyframe->value;
		else
			mode = TRANSFER_NORMAL;
		if(!(*input)->is_transparent() &&
				mode == TRANSFER_NORMAL &&
				in_x1 == out_x1 && in_x2 == out_x2 &&
				in_y1 == out_y1 && in_y2 == out_y2)
		{
			BC_Resources::tmpframes.release_frame(output);
			output = *input;
			*input = 0;
			return output;
		}

		if(!overlayer)
			overlayer = new OverlayFrame(preferences_global->max_threads);

		overlayer->overlay(output, *input,
			in_x1, in_y1, in_x2, in_y2,
			out_x1, out_y1, out_x2, out_y2,
			1, mode, BC_Resources::interpolation_method);
		output->merge_status(*input);
		BC_Resources::tmpframes.release_frame(*input);
		*input = 0;
		return output;
	}
	BC_Resources::tmpframes.release_frame(output);
	output = *input;
	*input = 0;
	return output;
}

VFrame *VTrackRender::render_camera(VFrame *frame)
{
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	VFrame *dstframe;

	calculate_input_transfer(frame->get_pts(),
		&in_x1, &in_y1, &in_x2, &in_y2,
		&out_x1, &out_y1, &out_x2, &out_y2);

	if(in_x1 != 0 || in_y1 != 0 ||
		in_x1 != out_x1 || in_y1 != out_y1 ||
		in_x2 != out_x2 || in_y2 != out_y2)
	{
		if(!overlayer)
			overlayer = new OverlayFrame(preferences_global->max_threads);

		dstframe = BC_Resources::tmpframes.clone_frame(frame);
		dstframe->clear_frame();
		overlayer->overlay(dstframe, frame,
			in_x1, in_y1, in_x2, in_y2,
			out_x1, out_y1, out_x2, out_y2,
			1, TRANSFER_REPLACE, BC_Resources::interpolation_method);
		dstframe->copy_pts(frame);
		dstframe->merge_status(frame);
		BC_Resources::tmpframes.release_frame(frame);
		return dstframe;
	}
	return frame;
}

void VTrackRender::calculate_input_transfer(ptstime position,
	int *in_x1, int *in_y1, int *in_x2, int *in_y2,
	int *out_x1, int *out_y1, int *out_x2, int *out_y2)
{
	double auto_x, auto_y, auto_z;
	double camera_z = 1;
	double camera_x = autos_track->track_w / 2;
	double camera_y = autos_track->track_h / 2;
	double dtrackw = autos_track->track_w;
	double dtrackh = autos_track->track_h;
	double x[4], y[4];

// get camera center in track
	autos_track->automation->get_camera(&auto_x, &auto_y, &auto_z, position);

	camera_z *= auto_z;
	camera_x += auto_x;
	camera_y += auto_y;

// get camera coords
	x[0] = camera_x - dtrackw / 2 / camera_z;
	y[0] = camera_y - dtrackh / 2 / camera_z;
	x[1] = x[0] + dtrackw / camera_z;
	y[1] = y[0] + dtrackh / camera_z;

// get coords on camera
	x[2] = 0;
	y[2] = 0;
	x[3] = dtrackw;
	y[3] = dtrackh;

// crop coords on camera
	if(x[0] < 0)
	{
		x[2] -= x[0] * camera_z;
		x[0] = 0;
	}

	if(y[0] < 0)
	{
		y[2] -= y[0] * camera_z;
		y[0] = 0;
	}
	if(x[1] > dtrackw)
	{
		x[3] -= (x[1] - dtrackw) * camera_z;
		x[1] = dtrackw;
	}
	if(y[1] > dtrackh)
	{
		y[3] -= (y[1] - dtrackh) * camera_z;
		y[1] = dtrackh;
	}

// get output bounding box
	*out_x1 = round(x[2]);
	*out_y1 = round(y[2]);
	*out_x2 = round(x[3]);
	*out_y2 = round(y[3]);

	*in_x1 = round(x[0]);
	*in_y1 = round(y[0]);
	*in_x2 = round(x[1]);
	*in_y2 = round(y[1]);
}

void VTrackRender::calculate_output_transfer(VFrame *output,
	int *in_x1, int *in_y1, int *in_x2, int *in_y2,
	int *out_x1, int *out_y1, int *out_x2, int *out_y2)
{
	double center_x, center_y, center_z;
	double x[4], y[4];
	double outw = output->get_w();
	double outh = output->get_h();
	double trackw = autos_track->track_w;
	double trackh = autos_track->track_h;

	x[0] = 0;
	y[0] = 0;
	x[1] = trackw;
	y[1] = trackh;

	autos_track->automation->get_projector(&center_x, &center_y,
		&center_z, output->get_pts());

	center_x += outw / 2;
	center_y += outh / 2;

	x[2] = center_x - (trackw / 2) * center_z;
	y[2] = center_y - (trackh / 2) * center_z;
	x[3] = x[2] + trackw * center_z;
	y[3] = y[2] + trackh * center_z;

// Clip to boundaries of output
	if(x[2] < 0)
	{
		x[0] -= (x[2] - 0) / center_z;
		x[2] = 0;
	}
	if(y[2] < 0)
	{
		y[0] -= (y[2] - 0) / center_z;
		y[2] = 0;
	}
	if(x[3] > outw)
	{
		x[1] -= (x[3] - outw) / center_z;
		x[3] = outw;
	}
	if(y[3] > outh)
	{
		y[1] -= (y[3] - outh) / center_z;
		y[3] = outh;
	}

	*in_x1 = round(x[0]);
	*in_y1 = round(y[0]);
	*in_x2 = round(x[1]);
	*in_y2 = round(y[1]);
	*out_x1 = round(x[2]);
	*out_y1 = round(y[2]);
	*out_x2 = round(x[3]);
	*out_y2 = round(y[3]);
}

VFrame *VTrackRender::render_plugins(VFrame *input, Edit *edit)
{
	Plugin *plugin;
	ptstime start = input->get_pts();
	ptstime end = start + input->get_duration();
	VFrame *tmpframe;

	for(int i = 0; i < plugins_track->plugins.total; i++)
	{
		plugin = plugins_track->plugins.values[i];

		if(next_plugin)
		{
			if(next_plugin != plugin)
				continue;
		}

		if(plugin->active_in(start, end))
		{
			if(plugin->on)
			{
				input->set_layer(media_track->number_of());
				if(tmpframe = execute_plugin(plugin, input, edit))
					input = tmpframe;
				else
				{
					next_plugin = plugin;
					break;
				}
			}
			else if(plugin->plugin_server &&
					plugin->plugin_server->multichannel &&
					videorender->is_shared_ready(plugin, start))
				videorender->shared_done(plugin);
		}
	}
	return input;
}

VFrame *VTrackRender::execute_plugin(Plugin *plugin, VFrame *frame, Edit *edit)
{
	PluginServer *server = plugin->plugin_server;
	int layer;
	VFrame *output;

	switch(plugin->plugin_type)
	{
	case PLUGIN_NONE:
		break;

	case PLUGIN_SHAREDMODULE:
		if(!plugin->shared_track)
			break;
		set_effects_track(plugin->shared_track);
		frame = render_camera(frame);
		render_mask(frame, 1);
		render_crop(frame, 1);
		frame = render_plugins(frame, edit);
		render_fade(frame);
		render_mask(frame, 0);
		render_crop(frame, 0);
		frame = render_projector(track_frame, &frame);
		set_effects_track(media_track);
		return frame;

	case PLUGIN_SHAREDPLUGIN:
		if(!server && plugin->shared_plugin)
		{
			if(!plugin->shared_plugin->plugin_server->multichannel)
				server = plugin->shared_plugin->plugin_server;
			else
				return 0;
		}
		// Fall through

	case PLUGIN_STANDALONE:
		if(server)
		{
			if(server->multichannel && !plugin->shared_plugin)
			{
				if(!videorender->is_shared_ready(plugin, frame->get_pts()))
					return 0;
				if(!plugin->client)
					plugin->plugin_server->open_plugin(plugin, this);
				plugin->client->set_renderer(this);

				videorender->pass_vframes(plugin, frame, this, edit);
				plugin->client->process_buffer(vframes.values);
				frame = videorender->take_vframes(plugin, this, edit);

				videorender->shared_done(plugin);
			}
			else
			{
				if(!plugin->client)
				{
					server->open_plugin(plugin, this);
					plugin->client->plugin_init(1);
				}
				plugin->client->set_renderer(this);
				plugin->client->process_buffer(&frame);
				next_plugin = 0;
			}
		}
		break;
	}
	return frame;
}

VFrame *VTrackRender::get_vtmpframe(VFrame *buffer, PluginClient *client)
{
// Called by tmpframe aware plugin
	ptstime buffer_pts = buffer->get_pts();
	Plugin *current = client->plugin;

	if(buffer_pts > current->end_pts())
		buffer_pts = current->end_pts() - buffer->get_duration();
	if(buffer_pts < current->get_pts())
		buffer_pts = current->get_pts();

	Edit *edit = media_track->editof(buffer_pts);

	if(edit)
	{
		read_vframe(buffer, edit, 2);

		buffer = render_transition(buffer, edit);
		buffer = render_camera(buffer);
		render_mask(buffer, 1);
		render_crop(buffer, 1);
		// render all standalone plugns before the current
		for(int i = 0; i < plugins_track->plugins.total; i++)
		{
			Plugin *plugin = plugins_track->plugins.values[i];

			if(plugin == current)
				break;
			if(!plugin->plugin_server)
				continue;
			if(plugin->plugin_type != PLUGIN_STANDALONE ||
					plugin->plugin_server->multichannel)
				continue;
			if(plugin->on && plugin->active_in(buffer->get_pts(), buffer->next_pts()))
				buffer = execute_plugin(plugin, buffer, edit);
		}
	}
	return buffer;
}

VFrame *VTrackRender::render_transition(VFrame *frame, Edit *edit)
{
	VFrame *tmpframe;
	Edit *prev;
	Plugin *transition;

	if(!edit || !(transition = edit->transition) ||
			!transition->plugin_server || !transition->on ||
			transition->duration() < frame->get_pts() - edit->get_pts())
		return frame;

	if(!transition->client)
	{
		transition->plugin_server->open_plugin(transition, this);
		transition->client->plugin_init(1);
	}
	tmpframe = BC_Resources::tmpframes.clone_frame(frame);
	tmpframe->copy_pts(frame);
	if(prev = edit->previous)
		read_vframe(tmpframe, prev, 1);
	else
		tmpframe->clear_frame();
	transition->client->process_transition(frame, tmpframe,
		frame->get_pts() - edit->get_pts());
	BC_Resources::tmpframes.release_frame(frame);
	return tmpframe;
}

int VTrackRender::need_camera(ptstime pts)
{
	double auto_x, auto_y, auto_z;

	autos_track->automation->get_camera(&auto_x, &auto_y, &auto_z, pts);

	return (!EQUIV(auto_x, 0) || !EQUIV(auto_y, 0) || !EQUIV(auto_z, 1));
}

VFrame *VTrackRender::handover_trackframe()
{
	VFrame *tmp = track_frame;

	track_frame = 0;
	return tmp;
}

void VTrackRender::take_vframe(VFrame *frame)
{
	track_frame = frame;
}

void VTrackRender::dump(int indent)
{
	printf("%*sVTrackRender %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*strack_frame %p videorender %p\n", indent, "",
		track_frame, videorender);
	TrackRender::dump(indent);
}
