// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "affine.h"
#include "bchash.h"
#include "clip.h"
#include "colorspaces.h"
#include "filexml.h"
#include "guidelines.h"
#include "keyframe.h"
#include "language.h"
#include "motion.h"
#include "motionwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "rotateframe.h"

#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN

MotionConfig::MotionConfig()
{
	global_range_w = 5;
	global_range_h = 5;
	rotation_range = 5;
	block_count = 1;
	global_block_w = MIN_BLOCK;
	global_block_h = MIN_BLOCK;
	rotation_block_w = MIN_BLOCK;
	rotation_block_h = MIN_BLOCK;
	block_x = 50;
	block_y = 50;
	global_positions = 256;
	rotate_positions = 4;
	magnitude = 100;
	return_speed = 0;
	mode1 = STABILIZE;
	global = 1;
	rotate = 1;
	addtrackedframeoffset = 0;
	mode2 = NO_CALCULATE;
	draw_vectors = 1;
	mode3 = MotionConfig::TRACK_SINGLE;
	track_pts = 0;
	horizontal_only = 0;
	vertical_only = 0;
	stab_gain_x = 1;
	stab_gain_y = 1;
}

void MotionConfig::boundaries()
{
	CLAMP(global_range_w, MIN_RADIUS, MAX_RADIUS);
	CLAMP(global_range_h, MIN_RADIUS, MAX_RADIUS);
	CLAMP(rotation_range, MIN_ROTATION, MAX_ROTATION);
	CLAMP(block_count, MIN_BLOCKS, MAX_BLOCKS);
	CLAMP(global_block_w, MIN_BLOCK, MAX_BLOCK);
	CLAMP(global_block_h, MIN_BLOCK, MAX_BLOCK);
	CLAMP(rotation_block_w, MIN_BLOCK, MAX_BLOCK);
	CLAMP(rotation_block_h, MIN_BLOCK, MAX_BLOCK);
}

int MotionConfig::equivalent(MotionConfig &that)
{
	return global_range_w == that.global_range_w &&
		global_range_h == that.global_range_h &&
		rotation_range == that.rotation_range &&
		mode1 == that.mode1 &&
		global == that.global &&
		rotate == that.rotate &&
		addtrackedframeoffset == that.addtrackedframeoffset &&
		draw_vectors == that.draw_vectors &&
		block_count == that.block_count &&
		global_block_w == that.global_block_w &&
		global_block_h == that.global_block_h &&
		rotation_block_w == that.rotation_block_w &&
		rotation_block_h == that.rotation_block_h &&
		EQUIV(block_x, that.block_x) &&
		EQUIV(block_y, that.block_y) &&
		global_positions == that.global_positions &&
		rotate_positions == that.rotate_positions &&
		magnitude == that.magnitude &&
		return_speed == that.return_speed &&
		mode3 == that.mode3 &&
		PTSEQU(track_pts, that.track_pts) &&
		horizontal_only == that.horizontal_only &&
		vertical_only == that.vertical_only &&
		EQUIV(stab_gain_x, that.stab_gain_x) &&
		EQUIV(stab_gain_y, that.stab_gain_y);
}

void MotionConfig::copy_from(MotionConfig &that)
{
	global_range_w = that.global_range_w;
	global_range_h = that.global_range_h;
	rotation_range = that.rotation_range;
	mode1 = that.mode1;
	global = that.global;
	rotate = that.rotate;
	addtrackedframeoffset = that.addtrackedframeoffset;
	mode2 = that.mode2;
	draw_vectors = that.draw_vectors;
	block_count = that.block_count;
	block_x = that.block_x;
	block_y = that.block_y;
	global_positions = that.global_positions;
	rotate_positions = that.rotate_positions;
	global_block_w = that.global_block_w;
	global_block_h = that.global_block_h;
	rotation_block_w = that.rotation_block_w;
	rotation_block_h = that.rotation_block_h;
	magnitude = that.magnitude;
	return_speed = that.return_speed;
	mode3 = that.mode3;
	track_pts = that.track_pts;
	horizontal_only = that.horizontal_only;
	vertical_only = that.vertical_only;
	stab_gain_x = that.stab_gain_x;
	stab_gain_y = that.stab_gain_y;
}

void MotionConfig::interpolate(MotionConfig &prev, 
	MotionConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->block_x = prev.block_x;
	this->block_y = prev.block_y;
	global_range_w = prev.global_range_w;
	global_range_h = prev.global_range_h;
	rotation_range = prev.rotation_range;
	mode1 = prev.mode1;
	global = prev.global;
	rotate = prev.rotate;
	addtrackedframeoffset = prev.addtrackedframeoffset;
	mode2 = prev.mode2;
	draw_vectors = prev.draw_vectors;
	block_count = prev.block_count;
	global_positions = prev.global_positions;
	rotate_positions = prev.rotate_positions;
	global_block_w = prev.global_block_w;
	global_block_h = prev.global_block_h;
	rotation_block_w = prev.rotation_block_w;
	rotation_block_h = prev.rotation_block_h;
	magnitude = prev.magnitude;
	return_speed = prev.return_speed;
	mode3 = prev.mode3;
	track_pts = prev.track_pts;
	horizontal_only = prev.horizontal_only;
	vertical_only = prev.vertical_only;
	stab_gain_x = prev_scale * prev.stab_gain_x + next_scale * next.stab_gain_x;
	stab_gain_y = prev_scale * prev.stab_gain_y + next_scale * next.stab_gain_y;
}


MotionMain::MotionMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	rotate_engine = 0;
	motion_rotate = 0;
	total_dx = 0;
	total_dy = 0;
	total_angle = 0;
	overlayer = 0;
	prev_ref = 0;
	current_ref = 0;
	target_src = 0;
	target_dst = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

MotionMain::~MotionMain()
{
	delete engine;
	delete overlayer;
	delete rotate_engine;
	delete motion_rotate;

	release_vframe(prev_ref);

	PLUGIN_DESTRUCTOR_MACRO
}

void MotionMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
	if(overlayer)
	{
		delete overlayer;
		overlayer = 0;
	}
	if(motion_rotate)
	{
		delete motion_rotate;
		motion_rotate = 0;
	}
	if(rotate_engine)
	{
		delete rotate_engine;
		rotate_engine = 0;
	}
	if(prev_ref)
	{
		release_vframe(prev_ref);
		prev_ref = 0;
	}
}

PLUGIN_CLASS_METHODS

void MotionMain::load_defaults()
{
	defaults = load_defaults_file("motion.rc");

	config.block_count = defaults->get("BLOCK_COUNT", config.block_count);
	config.global_positions = defaults->get("GLOBAL_POSITIONS", config.global_positions);
	config.rotate_positions = defaults->get("ROTATE_POSITIONS", config.rotate_positions);
	config.global_block_w = defaults->get("GLOBAL_BLOCK_W", config.global_block_w);
	config.global_block_h = defaults->get("GLOBAL_BLOCK_H", config.global_block_h);
	config.rotation_block_w = defaults->get("ROTATION_BLOCK_W", config.rotation_block_w);
	config.rotation_block_h = defaults->get("ROTATION_BLOCK_H", config.rotation_block_h);
	config.block_x = defaults->get("BLOCK_X", config.block_x);
	config.block_y = defaults->get("BLOCK_Y", config.block_y);
	config.global_range_w = defaults->get("GLOBAL_RANGE_W", config.global_range_w);
	config.global_range_h = defaults->get("GLOBAL_RANGE_H", config.global_range_h);
	config.rotation_range = defaults->get("ROTATION_RANGE", config.rotation_range);
	config.magnitude = defaults->get("MAGNITUDE", config.magnitude);
	config.return_speed = defaults->get("RETURN_SPEED", config.return_speed);
	config.mode1 = defaults->get("MODE1", config.mode1);
	config.global = defaults->get("GLOBAL", config.global);
	config.rotate = defaults->get("ROTATE", config.rotate);
	config.mode2 = defaults->get("MODE2", config.mode2);
	config.draw_vectors = defaults->get("DRAW_VECTORS", config.draw_vectors);
	config.mode3 = defaults->get("MODE3", config.mode3);
	framenum track_frame = defaults->get("TRACK_FRAME", -1);
	if(track_frame >= 0)
		config.track_pts = track_frame / get_project_framerate();
	config.track_pts = defaults->get("TRACK_PTS", config.track_pts);
	config.horizontal_only = defaults->get("HORIZONTAL_ONLY", config.horizontal_only);
	config.vertical_only = defaults->get("VERTICAL_ONLY", config.vertical_only);
	config.boundaries();
}

void MotionMain::save_defaults()
{
	defaults->update("BLOCK_COUNT", config.block_count);
	defaults->update("GLOBAL_POSITIONS", config.global_positions);
	defaults->update("ROTATE_POSITIONS", config.rotate_positions);
	defaults->update("GLOBAL_BLOCK_W", config.global_block_w);
	defaults->update("GLOBAL_BLOCK_H", config.global_block_h);
	defaults->update("ROTATION_BLOCK_W", config.rotation_block_w);
	defaults->update("ROTATION_BLOCK_H", config.rotation_block_h);
	defaults->update("BLOCK_X", config.block_x);
	defaults->update("BLOCK_Y", config.block_y);
	defaults->update("GLOBAL_RANGE_W", config.global_range_w);
	defaults->update("GLOBAL_RANGE_H", config.global_range_h);
	defaults->update("ROTATION_RANGE", config.rotation_range);
	defaults->update("MAGNITUDE", config.magnitude);
	defaults->update("RETURN_SPEED", config.return_speed);
	defaults->update("MODE1", config.mode1);
	defaults->update("GLOBAL", config.global);
	defaults->update("ROTATE", config.rotate);
	defaults->update("MODE2", config.mode2);
	defaults->update("DRAW_VECTORS", config.draw_vectors);
	defaults->update("MODE3", config.mode3);
	defaults->delete_key("TRACK_FRAME");
	defaults->update("TRACK_PTS", config.track_pts);
	defaults->delete_key("BOTTOM_IS_MASTER");
	defaults->update("HORIZONTAL_ONLY", config.horizontal_only);
	defaults->update("VERTICAL_ONLY", config.vertical_only);
	defaults->save();
}

void MotionMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("MOTION");
	output.tag.set_property("BLOCK_COUNT", config.block_count);
	output.tag.set_property("GLOBAL_POSITIONS", config.global_positions);
	output.tag.set_property("ROTATE_POSITIONS", config.rotate_positions);
	output.tag.set_property("GLOBAL_BLOCK_W", config.global_block_w);
	output.tag.set_property("GLOBAL_BLOCK_H", config.global_block_h);
	output.tag.set_property("ROTATION_BLOCK_W", config.rotation_block_w);
	output.tag.set_property("ROTATION_BLOCK_H", config.rotation_block_h);
	output.tag.set_property("BLOCK_X", config.block_x);
	output.tag.set_property("BLOCK_Y", config.block_y);
	output.tag.set_property("STABILIZE_GAIN_X", config.stab_gain_x);
	output.tag.set_property("STABILIZE_GAIN_Y", config.stab_gain_y);
	output.tag.set_property("GLOBAL_RANGE_W", config.global_range_w);
	output.tag.set_property("GLOBAL_RANGE_H", config.global_range_h);
	output.tag.set_property("ROTATION_RANGE", config.rotation_range);
	output.tag.set_property("MAGNITUDE", config.magnitude);
	output.tag.set_property("RETURN_SPEED", config.return_speed);
	output.tag.set_property("MODE1", config.mode1);
	output.tag.set_property("GLOBAL", config.global);
	output.tag.set_property("ROTATE", config.rotate);
	output.tag.set_property("ADDTRACKEDFRAMEOFFSET", config.addtrackedframeoffset);
	output.tag.set_property("MODE2", config.mode2);
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("MODE3", config.mode3);
	output.tag.set_property("TRACK_PTS", config.track_pts);
	output.tag.set_property("HORIZONTAL_ONLY", config.horizontal_only);
	output.tag.set_property("VERTICAL_ONLY", config.vertical_only);
	output.append_tag();
	output.tag.set_title("/MOTION");
	output.append_tag();
	keyframe->set_data(output.string);
}

void MotionMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("MOTION"))
		{
			config.block_count = input.tag.get_property("BLOCK_COUNT", config.block_count);
			config.global_positions = input.tag.get_property("GLOBAL_POSITIONS", config.global_positions);
			config.rotate_positions = input.tag.get_property("ROTATE_POSITIONS", config.rotate_positions);
			config.global_block_w = input.tag.get_property("GLOBAL_BLOCK_W", config.global_block_w);
			config.global_block_h = input.tag.get_property("GLOBAL_BLOCK_H", config.global_block_h);
			config.rotation_block_w = input.tag.get_property("ROTATION_BLOCK_W", config.rotation_block_w);
			config.rotation_block_h = input.tag.get_property("ROTATION_BLOCK_H", config.rotation_block_h);
			config.block_x = input.tag.get_property("BLOCK_X", config.block_x);
			config.block_y = input.tag.get_property("BLOCK_Y", config.block_y);
			config.stab_gain_x = input.tag.get_property("STABILIZE_GAIN_X", config.stab_gain_x);
			config.stab_gain_y = input.tag.get_property("STABILIZE_GAIN_Y", config.stab_gain_y);
			config.global_range_w = input.tag.get_property("GLOBAL_RANGE_W", config.global_range_w);
			config.global_range_h = input.tag.get_property("GLOBAL_RANGE_H", config.global_range_h);
			config.rotation_range = input.tag.get_property("ROTATION_RANGE", config.rotation_range);
			config.magnitude = input.tag.get_property("MAGNITUDE", config.magnitude);
			config.return_speed = input.tag.get_property("RETURN_SPEED", config.return_speed);
			config.mode1 = input.tag.get_property("MODE1", config.mode1);
			config.global = input.tag.get_property("GLOBAL", config.global);
			config.rotate = input.tag.get_property("ROTATE", config.rotate);
			config.addtrackedframeoffset = input.tag.get_property("ADDTRACKEDFRAMEOFFSET", config.addtrackedframeoffset);
			config.mode2 = input.tag.get_property("MODE2", config.mode2);
			config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
			config.mode3 = input.tag.get_property("MODE3", config.mode3);
			framenum track_frame = input.tag.get_property("TRACK_FRAME", -1);
			if(track_frame > 0)
				config.track_pts = track_frame / get_project_framerate();
			config.track_pts = input.tag.get_property("TRACK_PTS", config.track_pts);
			config.horizontal_only = input.tag.get_property("HORIZONTAL_ONLY", config.horizontal_only);
			config.vertical_only = input.tag.get_property("VERTICAL_ONLY", config.vertical_only);
		}
	}
	config.boundaries();
}

void MotionMain::process_global()
{
	if(!engine)
		engine = new MotionScan(this,
			PluginClient::get_project_smp(),
			PluginClient::get_project_smp());

// Get the current motion vector between the previous and current frame
	engine->scan_frame(current_ref, prev_ref);
	current_dx = engine->dx_result;
	current_dy = engine->dy_result;
// Add current motion vector to accumulation vector.
	if(config.mode3 != MotionConfig::TRACK_SINGLE)
	{
// Retract over time
		total_dx = total_dx * (100 - config.return_speed) / 100;
		total_dy = total_dy * (100 - config.return_speed) / 100;
		total_dx += engine->dx_result;
		total_dy += engine->dy_result;
	}
	else
// Make accumulation vector current
	{
		total_dx = engine->dx_result;
		total_dy = engine->dy_result;
	}

// Clamp accumulation vector
	if(config.magnitude < 100)
	{
		int block_w = config.global_block_w *
				current_ref->get_w() / 100;
		int block_h = config.global_block_h *
				current_ref->get_h() / 100;
		int block_x_orig = config.block_x *
			current_ref->get_w() / 100;
		int block_y_orig = config.block_y *
			current_ref->get_h() / 100;

		int max_block_x = (int64_t)(current_ref->get_w() - block_x_orig) *
			OVERSAMPLE * config.magnitude / 100;
		int max_block_y = (int64_t)(current_ref->get_h() - block_y_orig) *
			OVERSAMPLE * config.magnitude / 100;
		int min_block_x = (int64_t)-block_x_orig * OVERSAMPLE *
			config.magnitude / 100;
		int min_block_y = (int64_t)-block_y_orig *
			OVERSAMPLE * config.magnitude / 100;

		CLAMP(total_dx, min_block_x, max_block_x);
		CLAMP(total_dy, min_block_y, max_block_y);
	}

	total_dx = round(config.stab_gain_x * total_dx);
	total_dy = round(config.stab_gain_y * total_dy);

// Decide what to do with target based on requested operation
	int interpolation;
	double dx;
	double dy;

	switch(config.mode1)
	{
	case MotionConfig::NOTHING:
		break;
	case MotionConfig::TRACK_PIXEL:
		interpolation = NEAREST_NEIGHBOR;
		dx = total_dx / OVERSAMPLE;
		dy = total_dy / OVERSAMPLE;
		break;
	case MotionConfig::STABILIZE_PIXEL:
		interpolation = NEAREST_NEIGHBOR;
		dx = -total_dx / OVERSAMPLE;
		dy = -total_dy / OVERSAMPLE;
		break;
	case MotionConfig::TRACK:
		interpolation = CUBIC_LINEAR;
		dx = (double)total_dx / OVERSAMPLE;
		dy = (double)total_dy / OVERSAMPLE;
		break;
	case MotionConfig::STABILIZE:
		interpolation = CUBIC_LINEAR;
		dx = -(double)total_dx / OVERSAMPLE;
		dy = -(double)total_dy / OVERSAMPLE;
		break;
	}

	dx = config.stab_gain_x * dx;
	dy = config.stab_gain_y * dy;

	if(config.mode1 != MotionConfig::NOTHING)
	{
		if(!overlayer) 
			overlayer = new OverlayFrame(PluginClient::get_project_smp());

		target_dst->clear_frame();

		overlayer->overlay(target_dst,
			target_src,
			0,
			0,
			target_src->get_w(),
			target_src->get_h(),
			dx,
			dy,
			target_src->get_w() + dx,
			target_src->get_h() + dy,
			1,
			TRANSFER_REPLACE,
			interpolation);
	}
}

void MotionMain::process_rotation()
{
	int block_x;
	int block_y;

	block_x = round(prev_ref->get_w() * config.block_x / 100);
	block_y = round(prev_ref->get_h() * config.block_y / 100);

// Get rotation
	if(!motion_rotate)
		motion_rotate = new RotateScan(this, get_project_smp(),
			get_project_smp());

	current_angle = motion_rotate->scan_frame(prev_ref,
		current_ref, block_x, block_y);

// Add current rotation to accumulation
	if(config.mode3 != MotionConfig::TRACK_SINGLE)
	{
// Retract over time
		total_angle = total_angle * (100 - config.return_speed) / 100;
		total_angle += current_angle;
	}
	else
	{
		total_angle = current_angle;
	}

// Calculate rotation parameters based on requested operation
	double angle;

	switch(config.mode1)
	{
	case MotionConfig::NOTHING:
		break;
	case MotionConfig::TRACK:
	case MotionConfig::TRACK_PIXEL:
		angle = total_angle;
		break;
	case MotionConfig::STABILIZE:
	case MotionConfig::STABILIZE_PIXEL:
		angle = -total_angle;
		break;
	}

	if(config.mode1 != MotionConfig::NOTHING)
	{
		if(!rotate_engine)
			rotate_engine = new AffineEngine(PluginClient::get_project_smp(),
				PluginClient::get_project_smp());

		target_dst->clear_frame();

// Determine pivot based on a number of factors.
		switch(config.mode1)
		{
		case MotionConfig::TRACK:
		case MotionConfig::TRACK_PIXEL:
// Use destination of global tracking.
			rotate_engine->set_pivot(block_x, block_y);
			break;

		case MotionConfig::STABILIZE:
		case MotionConfig::STABILIZE_PIXEL:
			if(config.global)
			{
// Use origin of global stabilize operation
				rotate_engine->set_pivot(round(target_dst->get_w() *
						config.block_x / 100),
					round(target_dst->get_h() *
						config.block_y / 100));
			}
			else
				rotate_engine->set_pivot(block_x, block_y);
			break;
		}
		rotate_engine->rotate(target_dst, target_src, angle);
	}
}


void MotionMain::process_tmpframes(VFrame **frame)
{
	int color_model = frame[0]->get_color_model();
	int do_reconfigure = 0;

	width = frame[0]->get_w();
	height = frame[0]->get_h();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return;
	}

	if(load_configuration())
	{
		update_gui();
		do_reconfigure = 1;
	}

	if(!config.global && !config.rotate)
		return;

// Calculate the source and destination pointers for each of the operations.
	reference_layer = total_in_buffers - 1;
	ptstime actual_previous_pts;

// Skip if match frame not available
	int skip_current = 0;

	if(config.mode3 == MotionConfig::TRACK_SINGLE)
	{
		actual_previous_pts = config.track_pts + get_start();
		if(frame[0]->pts_in_frame(actual_previous_pts) ||
				actual_previous_pts >= get_end())
			skip_current = 1;
	}
	else
	{
		actual_previous_pts = source_pts - frame[0]->get_duration();

		if(actual_previous_pts < get_start())
			skip_current = 1;
		else
		{
			KeyFrame *keyframe = get_prev_keyframe(source_pts);
			if(keyframe->pos_time > 0 &&
				actual_previous_pts < keyframe->pos_time)
				skip_current = 1;
		}
// Only count motion since last keyframe
	}

	if(skip_current)
		return;

	int need_reload = (!prev_ref ||
		!prev_ref->pts_in_frame(actual_previous_pts) ||
		do_reconfigure);

	if(need_reload)
	{
		total_dx = 0;
		total_dy = 0;
		total_angle = 0;
	}

	if(skip_current)
	{
		total_dx = 0;
		total_dy = 0;
		current_dx = 0;
		current_dy = 0;
		total_angle = 0;
		current_angle = 0;
	}
// The center of the search area is fixed in compensate mode or
// the user value + the accumulation vector in track mode.
	if(!prev_ref)
		prev_ref = clone_vframe(frame[0]);

// Load the global frames
	if(need_reload)
	{
		prev_ref->set_layer(frame[reference_layer]->get_layer());
		prev_ref->set_pts(actual_previous_pts);
		prev_ref = get_frame(prev_ref);
	}
	current_ref = frame[reference_layer];

	target_src = frame[0];
	target_dst = frame[0] = clone_vframe(target_src);
	frame[0]->copy_pts(target_src);

// Get position change from previous frame to current frame
	if(config.global)
		process_global();
// Get rotation change from previous frame to current frame
	if(config.rotate)
	{
		if(config.global)
		{
			target_dst = target_src;
			target_src = frame[0];
			frame[0] = target_dst;
		}
		process_rotation();
	}

	if(config.mode1 == MotionConfig::NOTHING)
	{
		release_vframe(frame[0]);
		frame[0] = target_src;
	}
	else
		release_vframe(target_src);

	if(config.mode3 != MotionConfig::TRACK_SINGLE)
	{
// Transfer current reference frame to previous reference frame
		prev_ref->copy_from(current_ref);
	}

	draw_vectors();
}

void MotionMain::clamp_scan(int w, 
	int h, 
	int *block_x1,
	int *block_y1,
	int *block_x2,
	int *block_y2,
	int *scan_x1,
	int *scan_y1,
	int *scan_x2,
	int *scan_y2,
	int use_absolute)
{
	if(use_absolute)
	{
// scan is always out of range before block.
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
			*block_x1 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
			*block_y1 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 > w)
		{
			int difference = *scan_x2 - w;
			*block_x2 -= difference;
			*scan_x2 -= difference;
		}

		if(*scan_y2 > h)
		{
			int difference = *scan_y2 - h;
			*block_y2 -= difference;
			*scan_y2 -= difference;
		}

		CLAMP(*scan_x1, 0, w);
		CLAMP(*scan_y1, 0, h);
		CLAMP(*scan_x2, 0, w);
		CLAMP(*scan_y2, 0, h);
	}
	else
	{
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
			*block_x1 += difference;
			*scan_x2 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
			*block_y1 += difference;
			*scan_y2 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 - *block_x1 + *block_x2 > w)
		{
			int difference = *scan_x2 - *block_x1 + *block_x2 - w;
			*block_x2 -= difference;
		}

		if(*scan_y2 - *block_y1 + *block_y2 > h)
		{
			int difference = *scan_y2 - *block_y1 + *block_y2 - h;
			*block_y2 -= difference;
		}
	}

// Sanity checks which break the calculation but should never happen if the
// center of the block is inside the frame.
	CLAMP(*block_x1, 0, w);
	CLAMP(*block_x2, 0, w);
	CLAMP(*block_y1, 0, h);
	CLAMP(*block_y2, 0, h);
}

void MotionMain::draw_vectors()
{
	int global_x1, global_y1;
	int global_x2, global_y2;
	int block_x, block_y;
	int block_w, block_h;
	int block_x1, block_y1;
	int block_x2, block_y2;
	int block_x3, block_y3;
	int block_x4, block_y4;
	int search_w, search_h;
	int search_x1, search_y1;
	int search_x2, search_y2;
	int search_x3, search_y3;
	int search_x4, search_y4;

	GuideFrame *gf = get_guides();
	gf->clear();

	if(!config.draw_vectors)
		return;

	if(config.global)
	{
// Get vector
// Start of vector is center of previous block.
// End of vector is total accumulation.
		if(config.mode3 == MotionConfig::TRACK_SINGLE)
		{
			global_x1 = round(config.block_x * width / 100);
			global_y1 = round(config.block_y * height / 100);
			global_x2 = global_x1 + total_dx / OVERSAMPLE;
			global_y2 = global_y1 + total_dy / OVERSAMPLE;
		}
		else
// Start of vector is center of previous block.
// End of vector is current change.
		if(config.mode3 == MotionConfig::PREVIOUS_SAME_BLOCK)
		{
			global_x1 = round(config.block_x * width / 100);
			global_y1 = round(config.block_y * height / 100);
			global_x2 = global_x1 + current_dx / OVERSAMPLE;
			global_y2 = global_y1 + current_dy / OVERSAMPLE;
		}
		else
		{
			global_x1 = round(config.block_x * width / 100 +
				(total_dx - current_dx) / OVERSAMPLE);
			global_y1 = round(config.block_y * height / 100 +
				(total_dy - current_dy) / OVERSAMPLE);
			global_x2 = round(config.block_x * width / 100 +
				total_dx / OVERSAMPLE);
			global_y2 = round(config.block_y * height / 100 +
				total_dy / OVERSAMPLE);
		}

		block_x = global_x1;
		block_y = global_y1;
		block_w = config.global_block_w * width / 100;
		block_h = config.global_block_h * height / 100;
		block_x1 = block_x - block_w / 2;
		block_y1 = block_y - block_h / 2;
		block_x2 = block_x + block_w / 2;
		block_y2 = block_y + block_h / 2;
		search_w = config.global_range_w * width / 100;
		search_h = config.global_range_h * height / 100;
		search_x1 = block_x1 - search_w / 2;
		search_y1 = block_y1 - search_h / 2;
		search_x2 = block_x2 + search_w / 2;
		search_y2 = block_y2 + search_h / 2;

		clamp_scan(width, height,
			&block_x1,
			&block_y1,
			&block_x2,
			&block_y2,
			&search_x1,
			&search_y1,
			&search_x2,
			&search_y2,
			1);

// Vector
		draw_arrow(gf, global_x1, global_y1, global_x2, global_y2);
// Macroblock
		gf->add_rectangle(block_x1, block_y1,
			block_x2 - block_x1,
			block_y2 - block_y1);
// Search area
		gf->add_rectangle(search_x1, search_y1,
			search_x2 - search_x1,
			search_y2 - search_y1);
// Block should be endpoint of motion
		if(config.rotate)
		{
			block_x = global_x2;
			block_y = global_y2;
		}
	}
	else
	{
		block_x = round(config.block_x * width / 100);
		block_y = round(config.block_y * height / 100);
	}

	block_w = config.rotation_block_w * width / 100;
	block_h = config.rotation_block_h * height / 100;

	if(config.rotate)
	{
		gf->add_rectangle(block_x - block_w / 2, block_y - block_h / 2,
			block_w, block_h);
// Center
		if(!config.global)
			gf->add_circle(block_x - 3, block_y - 3, 6, 6);
	}
}

#define ARROW_SIZE 10
void MotionMain::draw_arrow(GuideFrame *gf, int x1, int y1, int x2, int y2)
{
	double angle;
	double angle1;
	double angle2;
	int x3;
	int y3;
	int x4;
	int y4;

	if(x2 == x1)
	{
		gf->add_pixel(x1, y1);
		return;
	}

	angle = atan((double)(y2 - y1) / (double)(x2 - x1));
	angle1 = angle + (double)145 / 360 * 2 * M_PI;
	angle2 = angle - (double)145 / 360 * 2 * M_PI;

	if(x2 < x1)
	{
		x3 = x2 - round(ARROW_SIZE * cos(angle1));
		y3 = y2 - round(ARROW_SIZE * sin(angle1));
		x4 = x2 - round(ARROW_SIZE * cos(angle2));
		y4 = y2 - round(ARROW_SIZE * sin(angle2));
	}
	else
	{
		x3 = x2 + round(ARROW_SIZE * cos(angle1));
		y3 = y2 + round(ARROW_SIZE * sin(angle1));
		x4 = x2 + round(ARROW_SIZE * cos(angle2));
		y4 = y2 + round(ARROW_SIZE * sin(angle2));
	}

// Main vector
	gf->add_line(x1, y1, x2, y2);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1))
		gf->add_line(x2, y2, x3, y3);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1))
		gf->add_line(x2, y2, x4, y4);
}

int MotionMain::abs_diff(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int row_bytes,
	int w,
	int h,
	int color_model)
{
	int result = 0;

	switch(color_model)
	{
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *prev_row = (uint16_t*)prev_ptr;
			uint16_t *current_row = (uint16_t*)current_ptr;

			for(int j = 0; j < w; j++)
			{
				result += abs(ColorSpaces::rgb_to_y_16(prev_row[0],
						prev_row[1], prev_row[2]) -
					ColorSpaces::rgb_to_y_16(current_row[0],
						current_row[1], current_row[2]));

				prev_row += 4;
				current_row += 4;
			}
			prev_ptr += row_bytes;
			current_ptr += row_bytes;
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *prev_row = (uint16_t*)prev_ptr;
			uint16_t *current_row = (uint16_t*)current_ptr;

			for(int j = 0; j < w; j++)
			{
				result += abs(prev_row[1] - current_row[1]);
				prev_row += 4;
				current_row += 4;
			}
			prev_ptr += row_bytes;
			current_ptr += row_bytes;
		}
		break;
	}
	return result;
}

int MotionMain::abs_diff_sub(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int row_bytes,
	int w,
	int h,
	int color_model,
	int sub_x,
	int sub_y)
{
	int h_sub = h - 1;
	int w_sub = w - 1;
	int result = 0;

	int y2_fraction = sub_y * 0x100 / OVERSAMPLE;
	int y1_fraction = 0x100 - y2_fraction;
	int x2_fraction = sub_x * 0x100 / OVERSAMPLE;
	int x1_fraction = 0x100 - x2_fraction;

	switch(color_model)
	{
	case BC_RGBA16161616:
		for(int i = 0; i < h_sub; i++)
		{
			uint16_t *prev_row1 = (uint16_t*)prev_ptr;
			uint16_t *prev_row2 = (uint16_t*)prev_ptr + 4;
			uint16_t *prev_row3 = (uint16_t*)(prev_ptr + row_bytes);
			uint16_t *prev_row4 = (uint16_t*)(prev_ptr + row_bytes) + 4;
			uint16_t *current_row = (uint16_t*)current_ptr;
			for(int j = 0; j < w_sub; j++)
			{
				int y_row1 = ColorSpaces::rgb_to_y_16(prev_row1[0],
					prev_row1[1], prev_row1[2]);
				int y_row2 = ColorSpaces::rgb_to_y_16(prev_row2[0],
					prev_row2[1], prev_row2[2]);
				int y_row3 = ColorSpaces::rgb_to_y_16(prev_row3[0],
					prev_row3[1], prev_row3[2]);
				int y_row4 = ColorSpaces::rgb_to_y_16(prev_row4[0],
					prev_row4[1], prev_row4[2]);
				int y_cur = ColorSpaces::rgb_to_y_16(current_row[0],
					current_row[1], current_row[2]);
				int prev_value =
					((int64_t)y_row1 * x1_fraction * y1_fraction +
					y_row2 * x2_fraction * y1_fraction +
					y_row3 * x1_fraction * y2_fraction +
					y_row4 * x2_fraction * y2_fraction) /
					0x100 / 0x100;
				result += abs(prev_value - y_cur);
				prev_row1 += 4;
				prev_row2 += 4;
				prev_row3 += 4;
				prev_row4 += 4;
				current_row += 4;
			}
			prev_ptr += row_bytes;
			current_ptr += row_bytes;
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h_sub; i++)
		{
			uint16_t *prev_row1 = (uint16_t*)prev_ptr;
			uint16_t *prev_row2 = (uint16_t*)prev_ptr + 4;
			uint16_t *prev_row3 = (uint16_t*)(prev_ptr + row_bytes);
			uint16_t *prev_row4 = (uint16_t*)(prev_ptr + row_bytes) + 4;
			uint16_t *current_row = (uint16_t*)current_ptr;
			for(int j = 0; j < w_sub; j++)
			{
				prev_row1++;
				prev_row2++;
				prev_row3++;
				prev_row4++;
				current_row++;

				int prev_value =
					((int64_t)*prev_row1 * x1_fraction * y1_fraction +
					*prev_row2 * x2_fraction * y1_fraction +
					*prev_row3 * x1_fraction * y2_fraction +
					*prev_row4 * x2_fraction * y2_fraction) /
					0x100 / 0x100;
				result += abs(prev_value - *current_row);
				prev_row1 += 3;
				prev_row2 += 3;
				prev_row3 += 3;
				prev_row4 += 3;
				current_row += 3;
			}
			prev_ptr += row_bytes;
			current_ptr += row_bytes;
		}
		break;
	}
	return result;
}


MotionScanPackage::MotionScanPackage()
 : LoadPackage()
{
	valid = 1;
}


MotionScanUnit::MotionScanUnit(MotionScan *server, 
	MotionMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	cache_lock = new Mutex("MotionScanUnit::cache_lock");
}

MotionScanUnit::~MotionScanUnit()
{
	delete cache_lock;
}

void MotionScanUnit::process_package(LoadPackage *package)
{
	MotionScanPackage *pkg = (MotionScanPackage*)package;
	int w = server->current_frame->get_w();
	int h = server->current_frame->get_h();
	int color_model = server->current_frame->get_color_model();
	int pixel_size = ColorModels::calculate_pixelsize(color_model);
	int row_bytes = server->current_frame->get_bytes_per_line();

// Single pixel
	if(!server->subpixel)
	{
		int search_x = pkg->scan_x1 + (pkg->pixel % (pkg->scan_x2 - pkg->scan_x1));
		int search_y = pkg->scan_y1 + (pkg->pixel / (pkg->scan_x2 - pkg->scan_x1));
// Try cache
		pkg->difference1 = server->get_cache(search_x, search_y);

		if(pkg->difference1 == INT_MAX)
		{
// Pointers to first pixel in each block
			unsigned char *prev_ptr = server->previous_frame->get_row_ptr(search_y) +
				search_x * pixel_size;
			unsigned char *current_ptr = server->current_frame->get_row_ptr(pkg->block_y1) +
				pkg->block_x1 * pixel_size;
// Scan block
			pkg->difference1 = plugin->abs_diff(prev_ptr,
				current_ptr,
				row_bytes,
				pkg->block_x2 - pkg->block_x1,
				pkg->block_y2 - pkg->block_y1,
				color_model);
			server->put_cache(search_x, search_y, pkg->difference1);
		}
	}
	else
// Sub pixel
	{
		int sub_x = pkg->pixel % (OVERSAMPLE * 2 - 1) + 1;
		int sub_y = pkg->pixel / (OVERSAMPLE * 2 - 1) + 1;

		if(plugin->config.horizontal_only)
		{
			sub_y = 0;
		}

		if(plugin->config.vertical_only)
		{
			sub_x = 0;
		}

		int search_x = pkg->scan_x1 + sub_x / OVERSAMPLE;
		int search_y = pkg->scan_y1 + sub_y / OVERSAMPLE;
		sub_x %= OVERSAMPLE;
		sub_y %= OVERSAMPLE;

		unsigned char *prev_ptr = server->previous_frame->get_row_ptr(search_y) +
			search_x * pixel_size;
		unsigned char *current_ptr = server->current_frame->get_row_ptr(pkg->block_y1) +
			pkg->block_x1 * pixel_size;

// With subpixel, there are two ways to compare each position, one by shifting
// the previous frame and two by shifting the current frame.
		pkg->difference1 = plugin->abs_diff_sub(prev_ptr,
			current_ptr,
			row_bytes,
			pkg->block_x2 - pkg->block_x1,
			pkg->block_y2 - pkg->block_y1,
			color_model,
			sub_x,
			sub_y);
		pkg->difference2 = plugin->abs_diff_sub(current_ptr,
			prev_ptr,
			row_bytes,
			pkg->block_x2 - pkg->block_x1,
			pkg->block_y2 - pkg->block_y1,
			color_model,
			sub_x,
			sub_y);
	}
}

int MotionScanUnit::get_cache(int x, int y)
{
	int result = INT_MAX;

	cache_lock->lock("MotionScanUnit::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		MotionScanCache *ptr = cache.values[i];
		if(ptr->x == x && ptr->y == y)
		{
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void MotionScanUnit::put_cache(int x, int y, int difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, difference);
	cache_lock->lock("MotionScanUnit::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}

MotionScan::MotionScan(MotionMain *plugin, 
	int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
	cache_lock = new Mutex("MotionScan::cache_lock");
}

MotionScan::~MotionScan()
{
	delete cache_lock;
}

void MotionScan::init_packages()
{
// Set package coords
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);
		pkg->block_x1 = block_x1;
		pkg->block_x2 = block_x2;
		pkg->block_y1 = block_y1;
		pkg->block_y2 = block_y2;
		pkg->scan_x1 = scan_x1;
		pkg->scan_x2 = scan_x2;
		pkg->scan_y1 = scan_y1;
		pkg->scan_y2 = scan_y2;
		pkg->pixel = i * total_pixels / total_steps;
		pkg->difference1 = 0;
		pkg->difference2 = 0;
		pkg->dx = 0;
		pkg->dy = 0;
		pkg->valid = 1;
	}
}

LoadClient* MotionScan::new_client()
{
	return new MotionScanUnit(this, plugin);
}

LoadPackage* MotionScan::new_package()
{
	return new MotionScanPackage;
}

void MotionScan::scan_frame(VFrame *previous_frame,
	VFrame *current_frame)
{
	this->previous_frame = previous_frame;
	this->current_frame = current_frame;
	subpixel = 0;

	cache.remove_all_objects();

	int w = current_frame->get_w();
	int h = current_frame->get_h();

// Initial search parameters
	int scan_w = w * plugin->config.global_range_w / 100;
	int scan_h = h * plugin->config.global_range_h / 100;
	int block_w = w * plugin->config.global_block_w / 100;
	int block_h = h * plugin->config.global_block_h / 100;

// Location of block in previous frame
	block_x1 = round(w * plugin->config.block_x / 100 - block_w / 2);
	block_y1 = round(h * plugin->config.block_y / 100 - block_h / 2);
	block_x2 = round(w * plugin->config.block_x / 100 + block_w / 2);
	block_y2 = round(h * plugin->config.block_y / 100 + block_h / 2);

// Offset to location of previous block.  This offset needn't be very accurate
// since it's the offset of the previous image and current image we want.
	if(plugin->config.mode3 == MotionConfig::TRACK_PREVIOUS)
	{
		block_x1 += plugin->total_dx / OVERSAMPLE;
		block_y1 += plugin->total_dy / OVERSAMPLE;
		block_x2 += plugin->total_dx / OVERSAMPLE;
		block_y2 += plugin->total_dy / OVERSAMPLE;
	}

	skip = 0;

	switch(plugin->config.mode2)
	{
// Don't calculate
	case MotionConfig::NO_CALCULATE:
		dx_result = 0;
		dy_result = 0;
		skip = 1;
		break;

	case MotionConfig::LOAD:
	{
// Load result from disk
		char string[BCTEXTLEN];
		sprintf(string, "%s%08d", MOTION_FILE,
			(int)round(plugin->source_pts * 100));
		FILE *input = fopen(string, "r");
		if(input)
		{
			if(fscanf(input, "%d %d", &dx_result, &dy_result) != 2)
			{
				dx_result = 0;
				dy_result = 0;
			}
			fclose(input);
			skip = 1;
		}
		break;
	}

// Scan from scratch
	default:
		skip = 0;
		break;
	}

// Perform scan
	if(!skip)
	{
// Location of block in current frame
		int x_result = block_x1;
		int y_result = block_y1;

		while(1)
		{
			scan_x1 = x_result - scan_w / 2;
			scan_y1 = y_result - scan_h / 2;
			scan_x2 = x_result + scan_w / 2;
			scan_y2 = y_result + scan_h / 2;

// Zero out requested values
			if(plugin->config.horizontal_only)
			{
				scan_y1 = block_y1;
				scan_y2 = block_y1 + 1;
			}
			if(plugin->config.vertical_only)
			{
				scan_x1 = block_x1;
				scan_x2 = block_x1 + 1;
			}

			MotionMain::clamp_scan(w, 
				h, 
				&block_x1,
				&block_y1,
				&block_x2,
				&block_y2,
				&scan_x1,
				&scan_y1,
				&scan_x2,
				&scan_y2,
				0);

// Give up if invalid coords.
			if(scan_y2 <= scan_y1 ||
				scan_x2 <= scan_x1 ||
				block_x2 <= block_x1 ||
				block_y2 <= block_y1)
				break;

// For subpixel, the top row and left column are skipped
			if(subpixel)
			{
				if(plugin->config.horizontal_only ||
					plugin->config.vertical_only)
				{
					total_pixels = 4 * OVERSAMPLE * OVERSAMPLE - 4 * OVERSAMPLE;
				}
				else
				{
					total_pixels = 4 * OVERSAMPLE;
				}

				total_steps = total_pixels;

				set_package_count(total_steps);
				process_packages();

// Get least difference
				int min_difference = INT_MAX;

				for(int i = 0; i < get_total_packages(); i++)
				{
					MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);

					if(pkg->difference1 < min_difference)
					{
						min_difference = pkg->difference1;

						if(plugin->config.vertical_only)
							x_result = scan_x1 * OVERSAMPLE;
						else
							x_result = scan_x1 * OVERSAMPLE + 
								(pkg->pixel % (OVERSAMPLE * 2 - 1)) + 1;

						if(plugin->config.horizontal_only)
							y_result = scan_y1 * OVERSAMPLE;
						else
							y_result = scan_y1 * OVERSAMPLE + 
								(pkg->pixel / (OVERSAMPLE * 2 - 1)) + 1;

// Fill in results
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
					}

					if(pkg->difference2 < min_difference)
					{
						min_difference = pkg->difference2;

						if(plugin->config.vertical_only)
							x_result = scan_x1 * OVERSAMPLE;
						else
							x_result = scan_x2 * OVERSAMPLE -
								((pkg->pixel % (OVERSAMPLE * 2 - 1)) + 1);

						if(plugin->config.horizontal_only)
							y_result = scan_y1 * OVERSAMPLE;
						else
							y_result = scan_y2 * OVERSAMPLE -
								((pkg->pixel / (OVERSAMPLE * 2 - 1)) + 1);

						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
					}
				}
				break;
			}
			else
			{
				total_pixels = (scan_x2 - scan_x1) * (scan_y2 - scan_y1);
				total_steps = MIN(plugin->config.global_positions, total_pixels);

				set_package_count(total_steps);
				process_packages();

// Get least difference
				int min_difference = INT_MAX;

				for(int i = 0; i < get_total_packages(); i++)
				{
					MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);

					if(pkg->difference1 < min_difference || min_difference == -1)
					{
						min_difference = pkg->difference1;
						x_result = scan_x1 + (pkg->pixel % (scan_x2 - scan_x1));
						y_result = scan_y1 + (pkg->pixel / (scan_x2 - scan_x1));
						x_result *= OVERSAMPLE;
						y_result *= OVERSAMPLE;
					}
				}

// If a new search is required, rescale results back to pixels.
				if(total_steps >= total_pixels)
				{
// Single pixel accuracy reached.  Now do exhaustive subpixel search.
					if(plugin->config.mode1 == MotionConfig::STABILIZE ||
						plugin->config.mode1 == MotionConfig::TRACK ||
						plugin->config.mode1 == MotionConfig::NOTHING)
					{
						x_result /= OVERSAMPLE;
						y_result /= OVERSAMPLE;
						scan_w = 2;
						scan_h = 2;
						subpixel = 1;
					}
					else
					{
// Fill in results and quit
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
						break;
					}
				}
				else
// Reduce scan area and try again
				{
					scan_w = (scan_x2 - scan_x1) / 2;
					scan_h = (scan_y2 - scan_y1) / 2;
					x_result /= OVERSAMPLE;
					y_result /= OVERSAMPLE;
				}
			}
		}

		dx_result *= -1;
		dy_result *= -1;

		// Add offsets from the "tracked single frame"
		if(plugin->config.addtrackedframeoffset)
		{
			int tf_dx_result, tf_dy_result;
			char string[BCTEXTLEN];
			sprintf(string, "%s%08d", MOTION_FILE,
				(int)round(plugin->config.track_pts * 100));
			FILE *input = fopen(string, "r");
			if(input)
			{
				if(fscanf(input, "%d %d", &tf_dx_result, &tf_dy_result) != 2)
					tf_dx_result = tf_dy_result = 0;
				dx_result += tf_dx_result;
				dy_result += tf_dy_result;
				fclose(input);
			}
			else
			{
				plugin->abort_plugin(_("Failed to load coords %m"));
			}
		}
	}

// Write results
	if(plugin->config.mode2 == MotionConfig::SAVE)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s%08d", MOTION_FILE,
			(int)round(plugin->source_pts * 100));

		FILE *output = fopen(string, "w");
		if(output)
		{
			fprintf(output, "%d %d\n",
				dx_result, dy_result);
			fclose(output);
		}
		else
		{
			plugin->abort_plugin(_("Failed to save coords: %m"));
		}
	}
}

int MotionScan::get_cache(int x, int y)
{
	int result = INT_MAX;

	cache_lock->lock("MotionScan::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		MotionScanCache *ptr = cache.values[i];
		if(ptr->x == x && ptr->y == y)
		{
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void MotionScan::put_cache(int x, int y, int difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, difference);
	cache_lock->lock("MotionScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}

MotionScanCache::MotionScanCache(int x, int y, int difference)
{
	this->x = x;
	this->y = y;
	this->difference = difference;
}


RotateScanPackage::RotateScanPackage()
{
	angle = MAX_ROTATION;
	difference = INT_MAX;
}


RotateScanUnit::RotateScanUnit(RotateScan *server, MotionMain *plugin)
 : LoadClient(server)
{
	this->server = server;
	this->plugin = plugin;
	rotater = 0;
}

RotateScanUnit::~RotateScanUnit()
{
	delete rotater;
}

void RotateScanUnit::process_package(LoadPackage *package)
{
	if(server->skip)
		return;

	RotateScanPackage *pkg = (RotateScanPackage*)package;

	if((pkg->difference = server->get_cache(pkg->angle)) == INT_MAX)
	{
		int color_model = server->previous_frame->get_color_model();
		int pixel_size = ColorModels::calculate_pixelsize(color_model);
		int row_bytes = server->previous_frame->get_bytes_per_line();

		if(!rotater)
			rotater = new AffineEngine(1, 1);

		VFrame *temp = plugin->clone_vframe(server->previous_frame, 1);

		rotater->set_viewport(server->scan_x,
			server->scan_y,
			server->scan_w,
			server->scan_h);
		rotater->set_pivot(server->block_x, server->block_y);

		rotater->rotate(temp,
			server->previous_frame,
			pkg->angle);

		int x1 = server->block_x1;
		int y1 = server->block_y1;
		int x2 = server->block_x2;
		int y2 = server->block_y2;
		x2 = MIN(temp->get_w(), x2);
		y2 = MIN(temp->get_h(), y2);
		x2 = MIN(server->current_frame->get_w(), x2);
		y2 = MIN(server->current_frame->get_h(), y2);
		x1 = MAX(0, x1);
		y1 = MAX(0, y1);

		if(x2 > x1 && y2 > y1)
		{
// Scan reduced block size
			pkg->difference = plugin->abs_diff(
				temp->get_row_ptr(y1) + x1 * pixel_size,
				server->current_frame->get_row_ptr(y1) + x1 * pixel_size,
				row_bytes,
				x2 - x1,
				y2 - y1,
				color_model);
			server->put_cache(pkg->angle, pkg->difference);
		}
		plugin->release_vframe(temp);
	}
}


RotateScan::RotateScan(MotionMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
	cache_lock = new Mutex("RotateScan::cache_lock");
}

RotateScan::~RotateScan()
{
	delete cache_lock;
}

void RotateScan::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);
		pkg->angle = i * 
			(scan_angle2 - scan_angle1) / 
			(total_steps - 1) + 
			scan_angle1;
	}
}

LoadClient* RotateScan::new_client()
{
	return new RotateScanUnit(this, plugin);
}

LoadPackage* RotateScan::new_package()
{
	return new RotateScanPackage;
}

double RotateScan::scan_frame(VFrame *previous_frame,
	VFrame *current_frame,
	int block_x,
	int block_y)
{
	skip = 0;
	this->block_x = block_x;
	this->block_y = block_y;

	switch(plugin->config.mode2)
	{
	case MotionConfig::NO_CALCULATE:
		result = 0;
		skip = 1;
		break;

	case MotionConfig::LOAD:
		{
			char string[BCTEXTLEN];

			sprintf(string, "%s%06d", ROTATION_FILE,
				(int)round(current_frame->get_pts() * 100));
			FILE *input = fopen(string, "r");
			if(input)
			{
				if(fscanf(input, "%lf", &result) != 1)
					result = 0;
				fclose(input);
				skip = 1;
			}
		break;
		}
	}

	this->previous_frame = previous_frame;
	this->current_frame = current_frame;
	int w = current_frame->get_w();
	int h = current_frame->get_h();
	int block_w = w * plugin->config.rotation_block_w / 100;
	int block_h = h * plugin->config.rotation_block_h / 100;

	if(this->block_x - block_w / 2 < 0)
		block_w = this->block_x * 2;
	if(this->block_y - block_h / 2 < 0)
		block_h = this->block_y * 2;
	if(this->block_x + block_w / 2 > w)
		block_w = (w - this->block_x) * 2;
	if(this->block_y + block_h / 2 > h)
		block_h = (h - this->block_y) * 2;

	block_x1 = this->block_x - block_w / 2;
	block_x2 = this->block_x + block_w / 2;
	block_y1 = this->block_y - block_h / 2;
	block_y2 = this->block_y + block_h / 2;

// Get coords of rectangle after rotation.
	double center_x = this->block_x;
	double center_y = this->block_y;
	double max_angle = plugin->config.rotation_range  * 2 * M_PI / 360;
	double target_angle1 = atan((double)block_h / block_w) + max_angle;
	double target_angle2 = atan((double)block_w / block_h) + max_angle;
	double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
	double wx[4], wy[4];

	wx[0] = center_x - cos(target_angle1) * radius;
	wy[0] = center_y - sin(target_angle1) * radius;
	wx[1] = center_x + sin(target_angle2) * radius;
	wy[1] = center_y - cos(target_angle2) * radius;
	wx[2] = center_x - sin(target_angle2) * radius;
	wy[2] = center_y + cos(target_angle2) * radius;
	wx[3] = center_x + sin(target_angle2) * radius;
	wy[3] = center_y + cos(target_angle2) * radius;

	double min_x, min_y, max_x, max_y;
	min_x = w;
	min_y = h;
	max_x = 0;
	max_y = 0;

	for(int i = 0; i < 4; i++)
	{
		if(min_x > wx[i])
			min_x = wx[i];
		if(max_x < wx[i])
			max_x = wx[i];
		if(min_y > wy[i])
			min_y = wy[i];
		if(max_y < wy[i])
			max_y = wy[i];
	}

// Get reduced scan coords
	scan_w = round(max_x - min_x);
	scan_h = round(max_y - min_y);
	scan_x = round(min_x);
	scan_y = round(min_y);

	cache.remove_all_objects();

	if(!skip)
	{
// Initial search range
		double angle_range = plugin->config.rotation_range;
		result = 0;
		total_steps = plugin->config.rotate_positions;
		double min_range = total_steps * MIN_ANGLE;

		while(angle_range >= min_range)
		{
			scan_angle1 = result - angle_range;
			scan_angle2 = result + angle_range;

			set_package_count(total_steps);
			process_packages();

			int min_difference = INT_MAX;

			for(int i = 0; i < get_total_packages(); i++)
			{
				RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);

				if(pkg->difference < min_difference)
				{
					min_difference = pkg->difference;
					result = pkg->angle;
				}
			}
			angle_range /= 2;
		}
	}

	if(!skip && plugin->config.mode2 == MotionConfig::SAVE)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s%06d", ROTATION_FILE,
			(int)round(current_frame->get_pts() * 100));

		FILE *output = fopen(string, "w");

		if(output)
		{
			fprintf(output, "%f\n", result);
			fclose(output);
		}
		else
		{
			plugin->abort_plugin(_("Failed to save coords: %m"));
		}
	}
	return result;
}

int RotateScan::get_cache(double angle)
{
	int result = INT_MAX;

	cache_lock->lock("RotateScan::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		RotateScanCache *ptr = cache.values[i];

		if(fabs(ptr->angle - angle) <= MIN_ANGLE)
		{
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void RotateScan::put_cache(double angle, int difference)
{
	RotateScanCache *ptr = new RotateScanCache(angle, difference);
	cache_lock->lock("RotateScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}


RotateScanCache::RotateScanCache(double angle, int difference)
{
	this->angle = angle;
	this->difference = difference;
}
