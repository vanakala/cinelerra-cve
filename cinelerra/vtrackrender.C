
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "pluginserver.h"
#include "preferences.h"
#include "tmpframecache.h"
#include "track.h"
#include "vtrack.h"
#include "vtrackrender.h"
#include "vframe.h"

VTrackRender::VTrackRender(Track *track)
 : TrackRender(track)
{
	fader = 0;
	masker = 0;
	cropper = 0;
	overlayer = 0;
	current_edit = 0;
}

VTrackRender::~VTrackRender()
{
	delete fader;
	delete masker;
	delete cropper;
	delete overlayer;
}

VFrame *VTrackRender::read_vframe(VFrame *vframe, Edit *edit, int filenum)
{
	ptstime pts = vframe->get_pts();
	ptstime src_pts;
	File *file = media_file(edit, filenum);

	if(!file)
	{
		vframe->set_duration(track->edl->this_edlsession->frame_duration());
		vframe->clear_frame();
		return 0;
	}

	src_pts = pts - edit->get_pts() + edit->get_source_pts();
	vframe->set_source_pts(src_pts);
	vframe->set_layer(edit->channel);
	file->get_frame(vframe);
	pts = align_to_frame(pts);
	vframe->set_pts(pts);
	vframe->set_duration(track->edl->this_edlsession->frame_duration());
	return vframe;
}

VFrame *VTrackRender::get_frame(VFrame *output)
{
	ptstime pts = align_to_frame(output->get_pts());
	Edit *edit = track->editof(pts);

	if(is_playable(pts, edit))
	{
		VFrame *frame = BC_Resources::tmpframes.get_tmpframe(
			track->track_w, track->track_h,
			edlsession->color_model);
		frame->set_pts(pts);
		read_vframe(frame, edit);
		frame = render_transition(frame, edit);
		frame = render_camera(frame);
		render_mask(frame, 1);
		render_crop(frame, 1);
		frame = render_plugins(frame, edit);
		render_fade(frame);
		render_mask(frame, 0);
		render_crop(frame, 0);
		frame = render_projector(output, frame);
		return frame;
	}
	return output;
}

void VTrackRender::render_fade(VFrame *frame)
{
	double value;

	value = ((FloatAutos*)track->automation->autos[AUTOMATION_FADE])->get_value(
		frame->get_pts());

	CLAMP(value, 0, 100);

	value /= 100;

	if(!EQUIV(value, 1.0))
	{
		if(!fader)
			fader = new FadeEngine(preferences_global->processors);
		fader->do_fade(frame, value);
	}
}

void VTrackRender::render_mask(VFrame *frame, int before_plugins)
{
	int total_points = 0;
	MaskAutos *keyframe_set =
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK];
	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(frame->get_pts());

	if(!keyframe)
		return;

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
		masker = new MaskEngine(preferences_global->processors);

	masker->do_mask(frame, keyframe_set, before_plugins);
}

void VTrackRender::render_crop(VFrame *frame, int before_plugins)
{
	int left, right, top, bottom;
	CropAutos *cropautos = (CropAutos *)track->automation->autos[AUTOMATION_CROP];

	if(!cropautos->first)
		return;

	cropautos->get_values(frame->get_pts(),
		&left, &right, &top, &bottom);

	if(left == 0 && top == 0 && right >= frame->get_w() && bottom >= frame->get_h())
		return;

	if(!cropper)
		cropper = new CropEngine();

	frame = cropper->do_crop(cropautos, frame, before_plugins);
}

VFrame *VTrackRender::render_projector(VFrame *output, VFrame *frame)
{
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	int mode;
	IntAuto *mode_keyframe;

	calculate_output_transfer(output,
		&in_x1, &in_y1, &in_x2, &in_y2,
		&out_x1, &out_y1, &out_x2, &out_y2);

	if(out_x2 > out_x1 && out_y2 > out_y1 &&
		in_x2 > in_x1 && in_y2 > in_y1)
	{
		mode_keyframe =
			(IntAuto*)track->automation->autos[AUTOMATION_MODE]->get_prev_auto(
				frame->get_pts());

		if(mode_keyframe)
			mode = mode_keyframe->value;
		else
			mode = TRANSFER_NORMAL;

		if(mode == TRANSFER_NORMAL && in_x1 == out_x1 && in_x2 == out_x2 &&
				in_y1 == out_y1 && in_y2 == out_y2)
		{
			BC_Resources::tmpframes.release_frame(output);
			return frame;
		}

		if(!overlayer)
			overlayer = new OverlayFrame(preferences_global->processors);

		overlayer->overlay(output, frame,
			in_x1, in_y1, in_x2, in_y2,
			out_x1, out_y1, out_x2, out_y2,
			1, mode, BC_Resources::interpolation_method);
		BC_Resources::tmpframes.release_frame(frame);
		return output;
	}
	return frame;
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
			overlayer = new OverlayFrame(preferences_global->processors);

		dstframe = BC_Resources::tmpframes.clone_frame(frame);
		dstframe->clear_frame();
		overlayer->overlay(dstframe, frame,
			in_x1, in_y1, in_x2, in_y2,
			out_x1, out_y1, out_x2, out_y2,
			1, TRANSFER_REPLACE, BC_Resources::interpolation_method);
		dstframe->copy_pts(frame);
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
	double camera_x = track->track_w / 2;
	double camera_y = track->track_h / 2;
	double dtrackw = track->track_w;
	double dtrackh = track->track_h;
	double x[4], y[4];

// get camera center in track
	track->automation->get_camera(&auto_x, &auto_y, &auto_z, position);
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
	double trackw = track->track_w;
	double trackh = track->track_h;

	x[0] = 0;
	y[0] = 0;
	x[1] = trackw;
	y[1] = trackh;

	track->automation->get_projector(&center_x, &center_y,
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

	current_frame = input;
	current_edit = edit;
	for(int i = 0; i < track->plugins.total; i++)
	{
		plugin = track->plugins.values[i];

		if(plugin->on && plugin->active_in(start, end))
		{
			if(tmpframe = execute_plugin(plugin, current_frame))
				current_frame = tmpframe;
		}
	}
	return current_frame;
}

VFrame *VTrackRender::execute_plugin(Plugin *plugin, VFrame *frame)
{
	if(plugin->plugin_server)
	{
		if(plugin->plugin_server->multichannel)
			puts("Multchannel plugin is not ready");
		else
		{
			if(!plugin->active_server)
			{
				plugin->active_server = new PluginServer(*plugin->plugin_server);
				plugin->active_server->open_plugin(0, plugin, this);
				plugin->active_server->init_realtime(1);
			}
			plugin->active_server->process_buffer(&frame, plugin->get_length());
			return frame;
		}
	}
	return 0;
}

VFrame *VTrackRender::get_vframe(VFrame *buffer)
{
// This is called by plugin
// We have to put frame into buffer, because plugins to
// not accept tmpframes yet
	ptstime current_pts = current_frame->get_pts();
	ptstime buffer_pts = buffer->get_pts();

	if(PTSEQU(current_pts, buffer_pts))
		buffer->copy_from(current_frame, 1);
	else
	{
		Edit *edit = track->editof(buffer_pts);

		read_vframe(buffer, edit, 2);
		if(edit->transition && edit->transition->plugin_server &&
			edit->transition->on || need_camera(buffer_pts))
		{
			VFrame *frame = BC_Resources::tmpframes.clone_frame(buffer);
			frame->copy_from(buffer, 1);
			frame = render_transition(frame, edit);
			frame = render_camera(frame);
			buffer->copy_from(frame, 1);
			BC_Resources::tmpframes.release_frame(frame);
		}
		render_mask(buffer, 1);
		render_crop(buffer, 1);
	}
	return buffer;
}

VFrame *VTrackRender::render_transition(VFrame *frame, Edit *edit)
{
	VFrame *tmpframe;
	Edit *prev;
	Plugin *transition = edit->transition;

	if(!transition || !transition->plugin_server || !transition->on ||
			transition->get_length() < frame->get_pts() - edit->get_pts())
		return frame;

	if(!transition->active_server)
	{
		transition->active_server = new PluginServer(*transition->plugin_server);
		transition->active_server->open_plugin(0, transition, this);
		transition->active_server->init_realtime(1);
	}
	tmpframe = BC_Resources::tmpframes.clone_frame(frame);
	tmpframe->copy_pts(frame);
	prev = edit->previous;
	read_vframe(tmpframe, prev, 1);
	transition->active_server->process_transition(frame, tmpframe,
		frame->get_pts() - edit->get_pts(), transition->get_length());
	BC_Resources::tmpframes.release_frame(frame);
	return tmpframe;
}

int VTrackRender::need_camera(ptstime pts)
{
	double auto_x, auto_y, auto_z;

	track->automation->get_camera(&auto_x, &auto_y, &auto_z, pts);

	return (!EQUIV(auto_x, 0) || !EQUIV(auto_y, 0) || !EQUIV(auto_z, 1));
}
