
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

#include "affine.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "motion.h"
#include "motionwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "rotateframe.h"
#include "transportque.h"


#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN(MotionMain)

//#undef DEBUG

#ifndef DEBUG
#define DEBUG
#endif

static void sort(int *array, int total)
{
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 0; i < total - 1; i++)
		{
			if(array[i] > array[i + 1])
			{
				array[i] ^= array[i + 1];
				array[i + 1] ^= array[i];
				array[i] ^= array[i + 1];
				done = 0;
			}
		}
	}
}



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
	track_frame = 0;
	bottom_is_master = 1;
	horizontal_only = 0;
	vertical_only = 0;
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
		track_frame == that.track_frame &&
		bottom_is_master == that.bottom_is_master &&
		horizontal_only == that.horizontal_only &&
		vertical_only == that.vertical_only;
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
	track_frame = that.track_frame;
	bottom_is_master = that.bottom_is_master;
	horizontal_only = that.horizontal_only;
	vertical_only = that.vertical_only;
}

void MotionConfig::interpolate(MotionConfig &prev, 
	MotionConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
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
	track_frame = prev.track_frame;
	bottom_is_master = prev.bottom_is_master;
	horizontal_only = prev.horizontal_only;
	vertical_only = prev.vertical_only;
}



















MotionMain::MotionMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	rotate_engine = 0;
	motion_rotate = 0;
	total_dx = 0;
	total_dy = 0;
	total_angle = 0;
	overlayer = 0;
	search_area = 0;
	search_size = 0;
	temp_frame = 0;
	previous_frame_number = -1;

	prev_global_ref = 0;
	current_global_ref = 0;
	global_target_src = 0;
	global_target_dst = 0;

	prev_rotate_ref = 0;
	current_rotate_ref = 0;
	rotate_target_src = 0;
	rotate_target_dst = 0;
}

MotionMain::~MotionMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete engine;
	delete overlayer;
	delete [] search_area;
	delete temp_frame;
	delete rotate_engine;
	delete motion_rotate;


	delete prev_global_ref;
	delete current_global_ref;
	delete global_target_src;
	delete global_target_dst;

	delete prev_rotate_ref;
	delete current_rotate_ref;
	delete rotate_target_src;
	delete rotate_target_dst;
}

char* MotionMain::plugin_title() { return N_("Motion"); }
int MotionMain::is_realtime() { return 1; }
int MotionMain::is_multichannel() { return 1; }

NEW_PICON_MACRO(MotionMain)

SHOW_GUI_MACRO(MotionMain, MotionThread)

SET_STRING_MACRO(MotionMain)

RAISE_WINDOW_MACRO(MotionMain)

LOAD_CONFIGURATION_MACRO(MotionMain, MotionConfig)



void MotionMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("MotionMain::update_gui");
			
			char string[BCTEXTLEN];
			sprintf(string, "%d", config.global_positions);
			thread->window->global_search_positions->set_text(string);
			sprintf(string, "%d", config.rotate_positions);
			thread->window->rotation_search_positions->set_text(string);

			thread->window->global_block_w->update(config.global_block_w);
			thread->window->global_block_h->update(config.global_block_h);
			thread->window->rotation_block_w->update(config.rotation_block_w);
			thread->window->rotation_block_h->update(config.rotation_block_h);
			thread->window->block_x->update(config.block_x);
			thread->window->block_y->update(config.block_y);
			thread->window->block_x_text->update((float)config.block_x);
			thread->window->block_y_text->update((float)config.block_y);
			thread->window->magnitude->update(config.magnitude);
			thread->window->return_speed->update(config.return_speed);


			thread->window->track_single->update(config.mode3 == MotionConfig::TRACK_SINGLE);
			thread->window->track_frame_number->update(config.track_frame);
			thread->window->track_previous->update(config.mode3 == MotionConfig::TRACK_PREVIOUS);
			thread->window->previous_same->update(config.mode3 == MotionConfig::PREVIOUS_SAME_BLOCK);
			if(config.mode3 != MotionConfig::TRACK_SINGLE)
				thread->window->track_frame_number->disable();
			else
				thread->window->track_frame_number->enable();

			thread->window->mode1->set_text(
				Mode1::to_text(config.mode1));
			thread->window->mode2->set_text(
				Mode2::to_text(config.mode2));
			thread->window->mode3->set_text(
				Mode3::to_text(config.horizontal_only, config.vertical_only));
			thread->window->master_layer->set_text(
				MasterLayer::to_text(config.bottom_is_master));


			thread->window->update_mode();
			thread->window->unlock_window();
		}
	}
}


int MotionMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%smotion.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

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
	config.track_frame = defaults->get("TRACK_FRAME", config.track_frame);
	config.bottom_is_master = defaults->get("BOTTOM_IS_MASTER", config.bottom_is_master);
	config.horizontal_only = defaults->get("HORIZONTAL_ONLY", config.horizontal_only);
	config.vertical_only = defaults->get("VERTICAL_ONLY", config.vertical_only);
	config.boundaries();
	return 0;
}


int MotionMain::save_defaults()
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
	defaults->update("TRACK_FRAME", config.track_frame);
	defaults->update("BOTTOM_IS_MASTER", config.bottom_is_master);
	defaults->update("HORIZONTAL_ONLY", config.horizontal_only);
	defaults->update("VERTICAL_ONLY", config.vertical_only);
	defaults->save();
	return 0;
}



void MotionMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
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
	output.tag.set_property("TRACK_FRAME", config.track_frame);
	output.tag.set_property("BOTTOM_IS_MASTER", config.bottom_is_master);
	output.tag.set_property("HORIZONTAL_ONLY", config.horizontal_only);
	output.tag.set_property("VERTICAL_ONLY", config.vertical_only);
	output.append_tag();
	output.tag.set_title("/MOTION");
	output.append_tag();
	output.terminate_string();
}

void MotionMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
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
				config.track_frame = input.tag.get_property("TRACK_FRAME", config.track_frame);
				config.bottom_is_master = input.tag.get_property("BOTTOM_IS_MASTER", config.bottom_is_master);
				config.horizontal_only = input.tag.get_property("HORIZONTAL_ONLY", config.horizontal_only);
				config.vertical_only = input.tag.get_property("VERTICAL_ONLY", config.vertical_only);
			}
		}
	}
	config.boundaries();
}









void MotionMain::allocate_temp(int w, int h, int color_model)
{
	if(temp_frame && 
		(temp_frame->get_w() != w ||
		temp_frame->get_h() != h))
	{
		delete temp_frame;
		temp_frame = 0;
	}
	if(!temp_frame)
		temp_frame = new VFrame(0, w, h, color_model);
}



void MotionMain::process_global()
{
	if(!engine) engine = new MotionScan(this,
		PluginClient::get_project_smp() + 1,
		PluginClient::get_project_smp() + 1);

// Get the current motion vector between the previous and current frame
	engine->scan_frame(current_global_ref, prev_global_ref);
	current_dx = engine->dx_result;
	current_dy = engine->dy_result;

// Add current motion vector to accumulation vector.
	if(config.mode3 != MotionConfig::TRACK_SINGLE)
	{
// Retract over time
		total_dx = (int64_t)total_dx * (100 - config.return_speed) / 100;
		total_dy = (int64_t)total_dy * (100 - config.return_speed) / 100;
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
		int block_w = (int64_t)config.global_block_w * 
				current_global_ref->get_w() / 100;
		int block_h = (int64_t)config.global_block_h * 
				current_global_ref->get_h() / 100;
		int block_x_orig = (int64_t)(config.block_x * 
			current_global_ref->get_w() / 
			100);
		int block_y_orig = (int64_t)(config.block_y *
			current_global_ref->get_h() / 
			100);

		int max_block_x = (int64_t)(current_global_ref->get_w() - block_x_orig) *
			OVERSAMPLE * 
			config.magnitude / 
			100;
		int max_block_y = (int64_t)(current_global_ref->get_h() - block_y_orig) *
			OVERSAMPLE *
			config.magnitude / 
			100;
		int min_block_x = (int64_t)-block_x_orig * 
			OVERSAMPLE * 
			config.magnitude / 
			100;
		int min_block_y = (int64_t)-block_y_orig * 
			OVERSAMPLE * 
			config.magnitude / 
			100;

		CLAMP(total_dx, min_block_x, max_block_x);
		CLAMP(total_dy, min_block_y, max_block_y);
	}

#ifdef DEBUG
printf("MotionMain::process_global 2 total_dx=%.02f total_dy=%.02f\n", 
(float)total_dx / OVERSAMPLE,
(float)total_dy / OVERSAMPLE);
#endif

	if(config.mode3 != MotionConfig::TRACK_SINGLE && !config.rotate)
	{
// Transfer current reference frame to previous reference frame and update
// counter.  Must wait for rotate to compare.
		prev_global_ref->copy_from(current_global_ref);
		previous_frame_number = get_source_position();
	}

// Decide what to do with target based on requested operation
	int interpolation;
	float dx;
	float dy;
	switch(config.mode1)
	{
		case MotionConfig::NOTHING:
			global_target_dst->copy_from(global_target_src);
			break;
		case MotionConfig::TRACK_PIXEL:
			interpolation = NEAREST_NEIGHBOR;
			dx = (int)(total_dx / OVERSAMPLE);
			dy = (int)(total_dy / OVERSAMPLE);
			break;
		case MotionConfig::STABILIZE_PIXEL:
			interpolation = NEAREST_NEIGHBOR;
			dx = -(int)(total_dx / OVERSAMPLE);
			dy = -(int)(total_dy / OVERSAMPLE);
			break;
			break;
		case MotionConfig::TRACK:
			interpolation = CUBIC_LINEAR;
			dx = (float)total_dx / OVERSAMPLE;
			dy = (float)total_dy / OVERSAMPLE;
			break;
		case MotionConfig::STABILIZE:
			interpolation = CUBIC_LINEAR;
			dx = -(float)total_dx / OVERSAMPLE;
			dy = -(float)total_dy / OVERSAMPLE;
			break;
	}


	if(config.mode1 != MotionConfig::NOTHING)
	{
		if(!overlayer) 
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		global_target_dst->clear_frame();
		overlayer->overlay(global_target_dst,
			global_target_src,
			0,
			0,
			global_target_src->get_w(),
			global_target_src->get_h(),
			dx,
			dy,
			(float)global_target_src->get_w() + dx,
			(float)global_target_src->get_h() + dy,
			1,
			TRANSFER_REPLACE,
			interpolation);
	}
}



void MotionMain::process_rotation()
{
	int block_x;
	int block_y;

// Convert the previous global reference into the previous rotation reference.
// Convert global target destination into rotation target source.
	if(config.global)
	{
		if(!overlayer) 
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		float dx;
		float dy;
		if(config.mode3 == MotionConfig::TRACK_SINGLE)
		{
			dx = (float)total_dx / OVERSAMPLE;
			dy = (float)total_dy / OVERSAMPLE;
		}
		else
		{
			dx = (float)current_dx / OVERSAMPLE;
			dy = (float)current_dy / OVERSAMPLE;
		}

		prev_rotate_ref->clear_frame();
		overlayer->overlay(prev_rotate_ref,
			prev_global_ref,
			0,
			0,
			prev_global_ref->get_w(),
			prev_global_ref->get_h(),
			dx,
			dy,
			(float)prev_global_ref->get_w() + dx,
			(float)prev_global_ref->get_h() + dy,
			1,
			TRANSFER_REPLACE,
			CUBIC_LINEAR);
// Pivot is destination global position
		block_x = (int)(prev_rotate_ref->get_w() * 
			config.block_x / 
			100 +
			(float)total_dx / 
			OVERSAMPLE);
		block_y = (int)(prev_rotate_ref->get_h() * 
			config.block_y / 
			100 +
			(float)total_dy / 
			OVERSAMPLE);
// Use the global target output as the rotation target input
		rotate_target_src->copy_from(global_target_dst);
// Transfer current reference frame to previous reference frame for global.
		if(config.mode3 != MotionConfig::TRACK_SINGLE)
		{
			prev_global_ref->copy_from(current_global_ref);
			previous_frame_number = get_source_position();
		}
	}
	else
	{
// Pivot is fixed
		block_x = (int)(prev_rotate_ref->get_w() * 
			config.block_x / 
			100);
		block_y = (int)(prev_rotate_ref->get_h() * 
			config.block_y / 
			100);
	}



// Get rotation
	if(!motion_rotate)
		motion_rotate = new RotateScan(this, 
			get_project_smp() + 1, 
			get_project_smp() + 1);

	current_angle = motion_rotate->scan_frame(prev_rotate_ref, 
		current_rotate_ref,
		block_x,
		block_y);



// Add current rotation to accumulation
	if(config.mode3 != MotionConfig::TRACK_SINGLE)
	{
// Retract over time
		total_angle = total_angle * (100 - config.return_speed) / 100;
		total_angle += current_angle;

		if(!config.global)
		{
// Transfer current reference frame to previous reference frame and update
// counter.
			prev_rotate_ref->copy_from(current_rotate_ref);
			previous_frame_number = get_source_position();
		}
	}
	else
	{
		total_angle = current_angle;
	}

#ifdef DEBUG
printf("MotionMain::process_rotation total_angle=%f\n", total_angle);
#endif


// Calculate rotation parameters based on requested operation
	float angle;
	switch(config.mode1)
	{
		case MotionConfig::NOTHING:
			rotate_target_dst->copy_from(rotate_target_src);
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
			rotate_engine = new AffineEngine(PluginClient::get_project_smp() + 1,
				PluginClient::get_project_smp() + 1);

		rotate_target_dst->clear_frame();

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
					rotate_engine->set_pivot((int)(rotate_target_dst->get_w() * 
							config.block_x / 
							100),
						(int)(rotate_target_dst->get_h() * 
							config.block_y / 
							100));
				
				}
				else
				{
// Use origin
					rotate_engine->set_pivot(block_x, block_y);
				}
				break;
		}


		rotate_engine->rotate(rotate_target_dst, rotate_target_src, angle);
// overlayer->overlay(rotate_target_dst,
// 	prev_rotate_ref,
// 	0,
// 	0,
// 	prev_rotate_ref->get_w(),
// 	prev_rotate_ref->get_h(),
// 	0,
// 	0,
// 	prev_rotate_ref->get_w(),
// 	prev_rotate_ref->get_h(),
// 	1,
// 	TRANSFER_NORMAL,
// 	CUBIC_LINEAR);
// overlayer->overlay(rotate_target_dst,
// 	current_rotate_ref,
// 	0,
// 	0,
// 	prev_rotate_ref->get_w(),
// 	prev_rotate_ref->get_h(),
// 	0,
// 	0,
// 	prev_rotate_ref->get_w(),
// 	prev_rotate_ref->get_h(),
// 	1,
// 	TRANSFER_NORMAL,
// 	CUBIC_LINEAR);


	}


}









int MotionMain::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();
	int color_model = frame[0]->get_color_model();
	w = frame[0]->get_w();
	h = frame[0]->get_h();
	

#ifdef DEBUG
printf("MotionMain::process_buffer 1 start_position=%lld\n", start_position);
#endif


// Calculate the source and destination pointers for each of the operations.
// Get the layer to track motion in.
	reference_layer = config.bottom_is_master ?
		PluginClient::total_in_buffers - 1 :
		0;
// Get the layer to apply motion in.
	target_layer = config.bottom_is_master ?
		0 :
		PluginClient::total_in_buffers - 1;


	output_frame = frame[target_layer];


// Get the position of previous reference frame.
	int64_t actual_previous_number;
// Skip if match frame not available
	int skip_current = 0;


	if(config.mode3 == MotionConfig::TRACK_SINGLE)
	{
		actual_previous_number = config.track_frame;
		if(get_direction() == PLAY_REVERSE)
			actual_previous_number++;
		if(actual_previous_number == start_position)
			skip_current = 1;
	}
	else
	{
		actual_previous_number = start_position;
		if(get_direction() == PLAY_FORWARD)
		{
			actual_previous_number--;
			if(actual_previous_number < get_source_start())
				skip_current = 1;
			else
			{
				KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
				if(keyframe->position > 0 &&
					actual_previous_number < keyframe->position)
					skip_current = 1;
			}
		}
		else
		{
			actual_previous_number++;
			if(actual_previous_number >= get_source_start() + get_total_len())
				skip_current = 1;
			else
			{
				KeyFrame *keyframe = get_next_keyframe(start_position, 1);
				if(keyframe->position > 0 &&
					actual_previous_number >= keyframe->position)
					skip_current = 1;
			}
		}

// Only count motion since last keyframe
		

	}


	if(!config.global && !config.rotate) skip_current = 1;




// printf("process_realtime %d %lld %lld\n", 
// skip_current, 
// previous_frame_number, 
// actual_previous_number);
// Load match frame and reset vectors
	int need_reload = !skip_current && 
		(previous_frame_number != actual_previous_number ||
		need_reconfigure);
	if(need_reload)
	{
		total_dx = 0;
		total_dy = 0;
		total_angle = 0;
		previous_frame_number = actual_previous_number;
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




// Get the global pointers.  Here we walk through the sequence of events.
	if(config.global)
	{
// Assume global only.  Global reads previous frame and compares
// with current frame to get the current translation.
// The center of the search area is fixed in compensate mode or
// the user value + the accumulation vector in track mode.
		if(!prev_global_ref)
			prev_global_ref = new VFrame(0, w, h, color_model);
		if(!current_global_ref)
			current_global_ref = new VFrame(0, w, h, color_model);

// Global loads the current target frame into the src and 
// writes it to the dst frame with desired translation.
		if(!global_target_src)
			global_target_src = new VFrame(0, w, h, color_model);
		if(!global_target_dst)
			global_target_dst = new VFrame(0, w, h, color_model);


// Load the global frames
		if(need_reload)
		{
			read_frame(prev_global_ref, 
				reference_layer, 
				previous_frame_number, 
				frame_rate);
		}

		read_frame(current_global_ref, 
			reference_layer, 
			start_position, 
			frame_rate);
		read_frame(global_target_src,
			target_layer,
			start_position,
			frame_rate);



// Global followed by rotate
		if(config.rotate)
		{
// Must translate the previous global reference by the current global
// accumulation vector to match the current global reference.
// The center of the search area is always the user value + the accumulation
// vector.
			if(!prev_rotate_ref)
				prev_rotate_ref = new VFrame(0, w, h, color_model);
// The current global reference is the current rotation reference.
			if(!current_rotate_ref)
				current_rotate_ref = new VFrame(0, w, h, color_model);
			current_rotate_ref->copy_from(current_global_ref);

// The global target destination is copied to the rotation target source
// then written to the rotation output with rotation.
// The pivot for the rotation is the center of the search area 
// if we're tracking.
// The pivot is fixed to the user position if we're compensating.
			if(!rotate_target_src)
				rotate_target_src = new VFrame(0, w, h, color_model);
			if(!rotate_target_dst)
				rotate_target_dst = new VFrame(0, w,h , color_model);
		}
	}
	else
// Rotation only
	if(config.rotate)
	{
// Rotation reads the previous reference frame and compares it with current 
// reference frame.
		if(!prev_rotate_ref)
			prev_rotate_ref = new VFrame(0, w, h, color_model);
		if(!current_rotate_ref)
			current_rotate_ref = new VFrame(0, w, h, color_model);

// Rotation loads target frame to temporary, rotates it, and writes it to the
// target frame.  The pivot is always fixed.
		if(!rotate_target_src)
			rotate_target_src = new VFrame(0, w, h, color_model);
		if(!rotate_target_dst)
			rotate_target_dst = new VFrame(0, w,h , color_model);


// Load the rotate frames
		if(need_reload)
		{
			read_frame(prev_rotate_ref, 
				reference_layer, 
				previous_frame_number, 
				frame_rate);
		}
		read_frame(current_rotate_ref, 
			reference_layer, 
			start_position, 
			frame_rate);
		read_frame(rotate_target_src,
			target_layer,
			start_position,
			frame_rate);
	}










	if(!skip_current)
	{
// Get position change from previous frame to current frame
		if(config.global) process_global();
// Get rotation change from previous frame to current frame
		if(config.rotate) process_rotation();
//frame[target_layer]->copy_from(prev_rotate_ref);
//frame[target_layer]->copy_from(current_rotate_ref);
	}






// Transfer the relevant target frame to the output
	if(!skip_current)
	{
		if(config.rotate)
		{
			frame[target_layer]->copy_from(rotate_target_dst);
		}
		else
		{
			frame[target_layer]->copy_from(global_target_dst);
		}
	}
	else
// Read the target destination directly
	{
		read_frame(frame[target_layer],
			target_layer,
			start_position,
			frame_rate);
	}

	if(config.draw_vectors)
	{
		draw_vectors(frame[target_layer]);
	}

#ifdef DEBUG
printf("MotionMain::process_buffer 100\n");
#endif
	return 0;
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
// printf("MotionMain::clamp_scan 1 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w,
// h,
// *block_x1,
// *block_y1,
// *block_x2,
// *block_y2,
// *scan_x1,
// *scan_y1,
// *scan_x2,
// *scan_y2,
// use_absolute);

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

// 		CLAMP(*scan_x1, 0, w - (*block_x2 - *block_x1));
// 		CLAMP(*scan_y1, 0, h - (*block_y2 - *block_y1));
// 		CLAMP(*scan_x2, 0, w - (*block_x2 - *block_x1));
// 		CLAMP(*scan_y2, 0, h - (*block_y2 - *block_y1));
	}

// Sanity checks which break the calculation but should never happen if the
// center of the block is inside the frame.
	CLAMP(*block_x1, 0, w);
	CLAMP(*block_x2, 0, w);
	CLAMP(*block_y1, 0, h);
	CLAMP(*block_y2, 0, h);

// printf("MotionMain::clamp_scan 2 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w,
// h,
// *block_x1,
// *block_y1,
// *block_x2,
// *block_y2,
// *scan_x1,
// *scan_y1,
// *scan_x2,
// *scan_y2,
// use_absolute);
}



void MotionMain::draw_vectors(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
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

	if(config.global)
	{
// Get vector
// Start of vector is center of previous block.
// End of vector is total accumulation.
		if(config.mode3 == MotionConfig::TRACK_SINGLE)
		{
			global_x1 = (int64_t)(config.block_x * 
				w / 
				100);
			global_y1 = (int64_t)(config.block_y *
				h / 
				100);
			global_x2 = global_x1 + total_dx / OVERSAMPLE;
			global_y2 = global_y1 + total_dy / OVERSAMPLE;
//printf("MotionMain::draw_vectors %d %d %d %d %d %d\n", total_dx, total_dy, global_x1, global_y1, global_x2, global_y2);
		}
		else
// Start of vector is center of previous block.
// End of vector is current change.
		if(config.mode3 == MotionConfig::PREVIOUS_SAME_BLOCK)
		{
			global_x1 = (int64_t)(config.block_x * 
				w / 
				100);
			global_y1 = (int64_t)(config.block_y *
				h / 
				100);
			global_x2 = global_x1 + current_dx / OVERSAMPLE;
			global_y2 = global_y1 + current_dy / OVERSAMPLE;
		}
		else
		{
			global_x1 = (int64_t)(config.block_x * 
				w / 
				100 + 
				(total_dx - current_dx) / 
				OVERSAMPLE);
			global_y1 = (int64_t)(config.block_y *
				h / 
				100 +
				(total_dy - current_dy) /
				OVERSAMPLE);
			global_x2 = (int64_t)(config.block_x * 
				w / 
				100 + 
				total_dx / 
				OVERSAMPLE);
			global_y2 = (int64_t)(config.block_y *
				h / 
				100 +
				total_dy /
				OVERSAMPLE);
		}

		block_x = global_x1;
		block_y = global_y1;
		block_w = config.global_block_w * w / 100;
		block_h = config.global_block_h * h / 100;
		block_x1 = block_x - block_w / 2;
		block_y1 = block_y - block_h / 2;
		block_x2 = block_x + block_w / 2;
		block_y2 = block_y + block_h / 2;
		search_w = config.global_range_w * w / 100;
		search_h = config.global_range_h * h / 100;
		search_x1 = block_x1 - search_w / 2;
		search_y1 = block_y1 - search_h / 2;
		search_x2 = block_x2 + search_w / 2;
		search_y2 = block_y2 + search_h / 2;

// printf("MotionMain::draw_vectors %d %d %d %d %d %d %d %d %d %d %d %d\n",
// global_x1,
// global_y1,
// block_w,
// block_h,
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// search_x1,
// search_y1,
// search_x2,
// search_y2);

		clamp_scan(w, 
			h, 
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
		draw_arrow(frame, global_x1, global_y1, global_x2, global_y2);

// Macroblock
		draw_line(frame, block_x1, block_y1, block_x2, block_y1);
		draw_line(frame, block_x2, block_y1, block_x2, block_y2);
		draw_line(frame, block_x2, block_y2, block_x1, block_y2);
		draw_line(frame, block_x1, block_y2, block_x1, block_y1);


// Search area
		draw_line(frame, search_x1, search_y1, search_x2, search_y1);
		draw_line(frame, search_x2, search_y1, search_x2, search_y2);
		draw_line(frame, search_x2, search_y2, search_x1, search_y2);
		draw_line(frame, search_x1, search_y2, search_x1, search_y1);

// Block should be endpoint of motion
		if(config.rotate)
		{
			block_x = global_x2;
			block_y = global_y2;
		}
	}
	else
	{
		block_x = (int64_t)(config.block_x * w / 100);
		block_y = (int64_t)(config.block_y * h / 100);
	}

	block_w = config.rotation_block_w * w / 100;
	block_h = config.rotation_block_h * h / 100;
	if(config.rotate)
	{
		float angle = total_angle * 2 * M_PI / 360;
		double base_angle1 = atan((float)block_h / block_w);
		double base_angle2 = atan((float)block_w / block_h);
		double target_angle1 = base_angle1 + angle;
		double target_angle2 = base_angle2 + angle;
		double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
		block_x1 = (int)(block_x - cos(target_angle1) * radius);
		block_y1 = (int)(block_y - sin(target_angle1) * radius);
		block_x2 = (int)(block_x + sin(target_angle2) * radius);
		block_y2 = (int)(block_y - cos(target_angle2) * radius);
		block_x3 = (int)(block_x - sin(target_angle2) * radius);
		block_y3 = (int)(block_y + cos(target_angle2) * radius);
		block_x4 = (int)(block_x + cos(target_angle1) * radius);
		block_y4 = (int)(block_y + sin(target_angle1) * radius);

		draw_line(frame, block_x1, block_y1, block_x2, block_y2);
		draw_line(frame, block_x2, block_y2, block_x4, block_y4);
		draw_line(frame, block_x4, block_y4, block_x3, block_y3);
		draw_line(frame, block_x3, block_y3, block_x1, block_y1);


// Center
		if(!config.global)
		{
			draw_line(frame, block_x, block_y - 5, block_x, block_y + 6);
			draw_line(frame, block_x - 5, block_y, block_x + 6, block_y);
		}
	}
}



void MotionMain::draw_pixel(VFrame *frame, int x, int y)
{
	if(!(x >= 0 && y >= 0 && x < frame->get_w() && y < frame->get_h())) return;

#define DRAW_PIXEL(x, y, components, do_yuv, max, type) \
{ \
	type **rows = (type**)frame->get_rows(); \
	rows[y][x * components] = max - rows[y][x * components]; \
	if(!do_yuv) \
	{ \
		rows[y][x * components + 1] = max - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = max - rows[y][x * components + 2]; \
	} \
	else \
	{ \
		rows[y][x * components + 1] = (max / 2 + 1) - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = (max / 2 + 1) - rows[y][x * components + 2]; \
	} \
	if(components == 4) \
		rows[y][x * components + 3] = max; \
}


	switch(frame->get_color_model())
	{
		case BC_RGB888:
			DRAW_PIXEL(x, y, 3, 0, 0xff, unsigned char);
			break;
		case BC_RGBA8888:
			DRAW_PIXEL(x, y, 4, 0, 0xff, unsigned char);
			break;
		case BC_RGB_FLOAT:
			DRAW_PIXEL(x, y, 3, 0, 1.0, float);
			break;
		case BC_RGBA_FLOAT:
			DRAW_PIXEL(x, y, 4, 0, 1.0, float);
			break;
		case BC_YUV888:
			DRAW_PIXEL(x, y, 3, 1, 0xff, unsigned char);
			break;
		case BC_YUVA8888:
			DRAW_PIXEL(x, y, 4, 1, 0xff, unsigned char);
			break;
		case BC_RGB161616:
			DRAW_PIXEL(x, y, 3, 0, 0xffff, uint16_t);
			break;
		case BC_YUV161616:
			DRAW_PIXEL(x, y, 3, 1, 0xffff, uint16_t);
			break;
		case BC_RGBA16161616:
			DRAW_PIXEL(x, y, 4, 0, 0xffff, uint16_t);
			break;
		case BC_YUVA16161616:
			DRAW_PIXEL(x, y, 4, 1, 0xffff, uint16_t);
			break;
	}
}


void MotionMain::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int w = labs(x2 - x1);
	int h = labs(y2 - y1);
//printf("MotionMain::draw_line 1 %d %d %d %d\n", x1, y1, x2, y2);

	if(!w && !h)
	{
		draw_pixel(frame, x1, y1);
	}
	else
	if(w > h)
	{
// Flip coordinates so x1 < x2
		if(x2 < x1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = y2 - y1;
		int denominator = x2 - x1;
		for(int i = x1; i < x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, i, y);
		}
	}
	else
	{
// Flip coordinates so y1 < y2
		if(y2 < y1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = x2 - x1;
		int denominator = y2 - y1;
		for(int i = y1; i < y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, x, i);
		}
	}
//printf("MotionMain::draw_line 2\n");
}

#define ARROW_SIZE 10
void MotionMain::draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2)
{
	double angle = atan((float)(y2 - y1) / (float)(x2 - x1));
	double angle1 = angle + (float)145 / 360 * 2 * 3.14159265;
	double angle2 = angle - (float)145 / 360 * 2 * 3.14159265;
	int x3;
	int y3;
	int x4;
	int y4;
	if(x2 < x1)
	{
		x3 = x2 - (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 - (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 - (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 - (int)(ARROW_SIZE * sin(angle2));
	}
	else
	{
		x3 = x2 + (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 + (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 + (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 + (int)(ARROW_SIZE * sin(angle2));
	}

// Main vector
	draw_line(frame, x1, y1, x2, y2);
//	draw_line(frame, x1, y1 + 1, x2, y2 + 1);

// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(frame, x2, y2, x3, y3);
//	draw_line(frame, x2, y2 + 1, x3, y3 + 1);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(frame, x2, y2, x4, y4);
//	draw_line(frame, x2, y2 + 1, x4, y4 + 1);
}




#define ABS_DIFF(type, temp_type, multiplier, components) \
{ \
	temp_type result_temp = 0; \
	for(int i = 0; i < h; i++) \
	{ \
		type *prev_row = (type*)prev_ptr; \
		type *current_row = (type*)current_ptr; \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				temp_type difference; \
				difference = *prev_row++ - *current_row++; \
				if(difference < 0) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
			if(components == 4) \
			{ \
				prev_row++; \
				current_row++; \
			} \
		} \
		prev_ptr += row_bytes; \
		current_ptr += row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
}

int64_t MotionMain::abs_diff(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int row_bytes,
	int w,
	int h,
	int color_model)
{
	int64_t result = 0;
	switch(color_model)
	{
		case BC_RGB888:
			ABS_DIFF(unsigned char, int64_t, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF(unsigned char, int64_t, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF(float, double, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF(float, double, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF(unsigned char, int64_t, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF(unsigned char, int64_t, 1, 4)
			break;
		case BC_YUV161616:
			ABS_DIFF(uint16_t, int64_t, 1, 3)
			break;
		case BC_YUVA16161616:
			ABS_DIFF(uint16_t, int64_t, 1, 4)
			break;
	}
	return result;
}



#define ABS_DIFF_SUB(type, temp_type, multiplier, components) \
{ \
	temp_type result_temp = 0; \
	temp_type y2_fraction = sub_y * 0x100 / OVERSAMPLE; \
	temp_type y1_fraction = 0x100 - y2_fraction; \
	temp_type x2_fraction = sub_x * 0x100 / OVERSAMPLE; \
	temp_type x1_fraction = 0x100 - x2_fraction; \
	for(int i = 0; i < h_sub; i++) \
	{ \
		type *prev_row1 = (type*)prev_ptr; \
		type *prev_row2 = (type*)prev_ptr + components; \
		type *prev_row3 = (type*)(prev_ptr + row_bytes); \
		type *prev_row4 = (type*)(prev_ptr + row_bytes) + components; \
		type *current_row = (type*)current_ptr; \
		for(int j = 0; j < w_sub; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				temp_type difference; \
				temp_type prev_value = \
					(*prev_row1++ * x1_fraction * y1_fraction + \
					*prev_row2++ * x2_fraction * y1_fraction + \
					*prev_row3++ * x1_fraction * y2_fraction + \
					*prev_row4++ * x2_fraction * y2_fraction) / \
					0x100 / 0x100; \
				temp_type current_value = *current_row++; \
				difference = prev_value - current_value; \
				if(difference < 0) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
 \
			if(components == 4) \
			{ \
				prev_row1++; \
				prev_row2++; \
				prev_row3++; \
				prev_row4++; \
				current_row++; \
			} \
		} \
		prev_ptr += row_bytes; \
		current_ptr += row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
}




int64_t MotionMain::abs_diff_sub(unsigned char *prev_ptr,
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
	int64_t result = 0;

	switch(color_model)
	{
		case BC_RGB888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF_SUB(float, double, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF_SUB(float, double, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 4)
			break;
		case BC_YUV161616:
			ABS_DIFF_SUB(uint16_t, int64_t, 1, 3)
			break;
		case BC_YUVA16161616:
			ABS_DIFF_SUB(uint16_t, int64_t, 1, 4)
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
	int pixel_size = cmodel_calculate_pixelsize(color_model);
	int row_bytes = server->current_frame->get_bytes_per_line();












// Single pixel
	if(!server->subpixel)
	{
		int search_x = pkg->scan_x1 + (pkg->pixel % (pkg->scan_x2 - pkg->scan_x1));
		int search_y = pkg->scan_y1 + (pkg->pixel / (pkg->scan_x2 - pkg->scan_x1));

// Try cache
		pkg->difference1 = server->get_cache(search_x, search_y);
		if(pkg->difference1 < 0)
		{
//printf("MotionScanUnit::process_package 1 %d %d\n", 
//search_x, search_y, pkg->block_x2 - pkg->block_x1, pkg->block_y2 - pkg->block_y1);
// Pointers to first pixel in each block
			unsigned char *prev_ptr = server->previous_frame->get_rows()[
				search_y] +	
				search_x * pixel_size;
			unsigned char *current_ptr = server->current_frame->get_rows()[
				pkg->block_y1] +
				pkg->block_x1 * pixel_size;
// Scan block
			pkg->difference1 = plugin->abs_diff(prev_ptr,
				current_ptr,
				row_bytes,
				pkg->block_x2 - pkg->block_x1,
				pkg->block_y2 - pkg->block_y1,
				color_model);
//printf("MotionScanUnit::process_package 2\n");
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


		unsigned char *prev_ptr = server->previous_frame->get_rows()[
			search_y] +
			search_x * pixel_size;
		unsigned char *current_ptr = server->current_frame->get_rows()[
			pkg->block_y1] +
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
// printf("MotionScanUnit::process_package sub_x=%d sub_y=%d search_x=%d search_y=%d diff1=%lld diff2=%lld\n",
// sub_x,
// sub_y,
// search_x,
// search_y,
// pkg->difference1,
// pkg->difference2);
	}




}










int64_t MotionScanUnit::get_cache(int x, int y)
{
	int64_t result = -1;
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

void MotionScanUnit::put_cache(int x, int y, int64_t difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, difference);
	cache_lock->lock("MotionScanUnit::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}











MotionScan::MotionScan(MotionMain *plugin, 
	int total_clients,
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
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
		pkg->pixel = (int64_t)i * (int64_t)total_pixels / (int64_t)total_steps;
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


// Single macroblock
	int w = current_frame->get_w();
	int h = current_frame->get_h();

// Initial search parameters
	int scan_w = w * plugin->config.global_range_w / 100;
	int scan_h = h * plugin->config.global_range_h / 100;
	int block_w = w * plugin->config.global_block_w / 100;
	int block_h = h * plugin->config.global_block_h / 100;

// Location of block in previous frame
	block_x1 = (int)(w * plugin->config.block_x / 100 - block_w / 2);
	block_y1 = (int)(h * plugin->config.block_y / 100 - block_h / 2);
	block_x2 = (int)(w * plugin->config.block_x / 100 + block_w / 2);
	block_y2 = (int)(h * plugin->config.block_y / 100 + block_h / 2);

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
			sprintf(string, "%s%06d", MOTION_FILE, plugin->get_source_position());
			FILE *input = fopen(string, "r");
			if(input)
			{
				fscanf(input, 
					"%d %d", 
					&dx_result,
					&dy_result);
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

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1 + block_w / 2,
// block_y1 + block_h / 2,
// block_w,
// block_h,
// block_x1,
// block_y1,
// block_x2,
// block_y2);

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

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);
// Clamp the block coords before the scan so we get useful scan coords.
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
// printf("MotionScan::scan_frame 1\n    block_x1=%d block_y1=%d block_x2=%d block_y2=%d\n    scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n    x_result=%d y_result=%d\n", 
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1, 
// scan_y1, 
// scan_x2, 
// scan_y2, 
// x_result, 
// y_result);


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
				int64_t min_difference = -1;
				for(int i = 0; i < get_total_packages(); i++)
				{
					MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);
					if(pkg->difference1 < min_difference || min_difference == -1)
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

//printf("MotionScan::scan_frame 1 %d %d %d %d\n", block_x1, block_y1, x_result, y_result);
				break;
			}
			else
			{
				total_pixels = (scan_x2 - scan_x1) * (scan_y2 - scan_y1);
				total_steps = MIN(plugin->config.global_positions, total_pixels);

				set_package_count(total_steps);
				process_packages();

// Get least difference
				int64_t min_difference = -1;
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

// printf("MotionScan::scan_frame 10 total_steps=%d total_pixels=%d subpixel=%d\n",
// total_steps, 
// total_pixels,
// subpixel);
// 
// printf("	scan w=%d h=%d scan x1=%d y1=%d x2=%d y2=%d\n",
// scan_w,
// scan_h, 
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);
// 
// printf("MotionScan::scan_frame 2 block x1=%d y1=%d x2=%d y2=%d result x=%.2f y=%.2f\n", 
// block_x1, 
// block_y1, 
// block_x2,
// block_y2,
// (float)x_result / 4, 
// (float)y_result / 4);


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
		if (plugin->config.addtrackedframeoffset) {
		  int tf_dx_result, tf_dy_result;
		  char string[BCTEXTLEN];
		  sprintf(string, "%s%06d", MOTION_FILE, plugin->config.track_frame);
		  FILE *input = fopen(string, "r");
		  if(input)
		    {
		      fscanf(input, 
			     "%d %d", 
			     &tf_dx_result,
			     &tf_dy_result);
		      dx_result += tf_dx_result;
		      dy_result += tf_dy_result;
		      fclose(input);
		    }
		}

	}






// Write results
	if(plugin->config.mode2 == MotionConfig::SAVE)
	{
		char string[BCTEXTLEN];
		sprintf(string, 
			"%s%06d", 
			MOTION_FILE, 
			plugin->get_source_position());
		FILE *output = fopen(string, "w");
		if(output)
		{
			fprintf(output, 
				"%d %d\n",
				dx_result,
				dy_result);
			fclose(output);
		}
		else
		{
			perror("MotionScan::scan_frame SAVE 1");
		}
	}

#ifdef DEBUG
printf("MotionScan::scan_frame 10 dx=%.2f dy=%.2f\n", 
(float)this->dx_result / OVERSAMPLE,
(float)this->dy_result / OVERSAMPLE);
#endif
}

















int64_t MotionScan::get_cache(int x, int y)
{
	int64_t result = -1;
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

void MotionScan::put_cache(int x, int y, int64_t difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, difference);
	cache_lock->lock("MotionScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}





MotionScanCache::MotionScanCache(int x, int y, int64_t difference)
{
	this->x = x;
	this->y = y;
	this->difference = difference;
}














RotateScanPackage::RotateScanPackage()
{
}


RotateScanUnit::RotateScanUnit(RotateScan *server, MotionMain *plugin)
 : LoadClient(server)
{
	this->server = server;
	this->plugin = plugin;
	rotater = 0;
	temp = 0;
}

RotateScanUnit::~RotateScanUnit()
{
	delete rotater;
	delete temp;
}

void RotateScanUnit::process_package(LoadPackage *package)
{
	if(server->skip) return;
	RotateScanPackage *pkg = (RotateScanPackage*)package;

	if((pkg->difference = server->get_cache(pkg->angle)) < 0)
	{
//printf("RotateScanUnit::process_package 1\n");
		int color_model = server->previous_frame->get_color_model();
		int pixel_size = cmodel_calculate_pixelsize(color_model);
		int row_bytes = server->previous_frame->get_bytes_per_line();

		if(!rotater)
			rotater = new AffineEngine(1, 1);
		if(!temp) temp = new VFrame(0,
			server->previous_frame->get_w(),
			server->previous_frame->get_h(),
			color_model);


// Rotate original block size
		rotater->set_viewport(server->block_x1, 
			server->block_y1,
			server->block_x2 - server->block_x1,
			server->block_y2 - server->block_y1);
		rotater->set_pivot(server->block_x, server->block_y);
//pkg->angle = 2;
		rotater->rotate(temp,
			server->previous_frame,
			pkg->angle);

// Scan reduced block size
//plugin->output_frame->copy_from(server->current_frame);
//plugin->output_frame->copy_from(temp);
		pkg->difference = plugin->abs_diff(
			temp->get_rows()[server->scan_y] + server->scan_x * pixel_size,
			server->current_frame->get_rows()[server->scan_y] + server->scan_x * pixel_size,
			row_bytes,
			server->scan_w,
			server->scan_h,
			color_model);
		server->put_cache(pkg->angle, pkg->difference);

// printf("RotateScanUnit::process_package 10 x=%d y=%d w=%d h=%d block_x=%d block_y=%d angle=%f scan_w=%d scan_h=%d diff=%lld\n", 
// server->block_x1, 
// server->block_y1,
// server->block_x2 - server->block_x1,
// server->block_y2 - server->block_y1,
// server->block_x,
// server->block_y,
// pkg->angle, 
// server->scan_w,
// server->scan_h,
// pkg->difference);
	}
}






















RotateScan::RotateScan(MotionMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
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


float RotateScan::scan_frame(VFrame *previous_frame,
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
			sprintf(string, "%s%06d", ROTATION_FILE, plugin->get_source_position());
			FILE *input = fopen(string, "r");
			if(input)
			{
				fscanf(input, "%f", &result);
				fclose(input);
				skip = 1;
			}
			else
			{
				perror("RotateScan::scan_frame LOAD");
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

	if(this->block_x - block_w / 2 < 0) block_w = this->block_x * 2;
	if(this->block_y - block_h / 2 < 0) block_h = this->block_y * 2;
	if(this->block_x + block_w / 2 > w) block_w = (w - this->block_x) * 2;
	if(this->block_y + block_h / 2 > h) block_h = (h - this->block_y) * 2;

	block_x1 = this->block_x - block_w / 2;
	block_x2 = this->block_x + block_w / 2;
	block_y1 = this->block_y - block_h / 2;
	block_y2 = this->block_y + block_h / 2;


// Calculate the maximum area available to scan after rotation.
// Must be calculated from the starting range because of cache.
// Get coords of rectangle after rotation.
	double center_x = this->block_x;
	double center_y = this->block_y;
	double max_angle = plugin->config.rotation_range;
	double base_angle1 = atan((float)block_h / block_w);
	double base_angle2 = atan((float)block_w / block_h);
	double target_angle1 = base_angle1 + max_angle * 2 * M_PI / 360;
	double target_angle2 = base_angle2 + max_angle * 2 * M_PI / 360;
	double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
	double x1 = center_x - cos(target_angle1) * radius;
	double y1 = center_y - sin(target_angle1) * radius;
	double x2 = center_x + sin(target_angle2) * radius;
	double y2 = center_y - cos(target_angle2) * radius;
	double x3 = center_x - sin(target_angle2) * radius;
	double y3 = center_y + cos(target_angle2) * radius;

// Track top edge to find greatest area.
	double max_area1 = 0;
	double max_x1 = 0;
	double max_y1 = 0;
	for(double x = x1; x < x2; x++)
	{
		double y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
		if(x >= center_x && x < block_x2 && y >= block_y1 && y < center_y)
		{
			double area = fabs(x - center_x) * fabs(y - center_y);
			if(area > max_area1)
			{
				max_area1 = area;
				max_x1 = x;
				max_y1 = y;
			}
		}
	}

// Track left edge to find greatest area.
	double max_area2 = 0;
	double max_x2 = 0;
	double max_y2 = 0;
	for(double y = y1; y < y3; y++)
	{
		double x = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
		if(x >= block_x1 && x < center_x && y >= block_y1 && y < center_y)
		{
			double area = fabs(x - center_x) * fabs(y - center_y);
			if(area > max_area2)
			{
				max_area2 = area;
				max_x2 = x;
				max_y2 = y;
			}
		}
	}

	double max_x, max_y;
	max_x = max_x2;
	max_y = max_y1;

// Get reduced scan coords
	scan_w = (int)(fabs(max_x - center_x) * 2);
	scan_h = (int)(fabs(max_y - center_y) * 2);
	scan_x = (int)(center_x - scan_w / 2);
	scan_y = (int)(center_y - scan_h / 2);
// printf("RotateScan::scan_frame center=%d,%d scan=%d,%d %dx%d\n", 
// this->block_x, this->block_y, scan_x, scan_y, scan_w, scan_h);
// printf("    angle_range=%f block= %d,%d,%d,%d\n", max_angle, block_x1, block_y1, block_x2, block_y2);

// Determine min angle from size of block
	double angle1 = atan((double)block_h / block_w);
	double angle2 = atan((double)(block_h - 1) / (block_w + 1));
	double min_angle = fabs(angle2 - angle1) / OVERSAMPLE;
	min_angle = MAX(min_angle, MIN_ANGLE);

#ifdef DEBUG
printf("RotateScan::scan_frame min_angle=%f\n", min_angle * 360 / 2 / M_PI);
#endif

	cache.remove_all_objects();
	if(!skip)
	{
// Initial search range
		float angle_range = (float)plugin->config.rotation_range;
		result = 0;
		total_steps = plugin->config.rotate_positions;


		while(angle_range >= min_angle * total_steps)
		{
			scan_angle1 = result - angle_range;
			scan_angle2 = result + angle_range;


			set_package_count(total_steps);
//set_package_count(1);
			process_packages();

			int64_t min_difference = -1;
			for(int i = 0; i < get_total_packages(); i++)
			{
				RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);
				if(pkg->difference < min_difference || min_difference == -1)
				{
					min_difference = pkg->difference;
					result = pkg->angle;
				}
//break;
			}

			angle_range /= 2;

//break;
		}
	}


	if(!skip && plugin->config.mode2 == MotionConfig::SAVE)
	{
		char string[BCTEXTLEN];
		sprintf(string, 
			"%s%06d", 
			ROTATION_FILE, 
			plugin->get_source_position());
		FILE *output = fopen(string, "w");
		if(output)
		{
			fprintf(output, "%f\n", result);
			fclose(output);
		}
		else
		{
			perror("RotateScan::scan_frame SAVE");
		}
	}

#ifdef DEBUG
printf("RotateScan::scan_frame 10 angle=%f\n", result);
#endif
	


	return result;
}

int64_t RotateScan::get_cache(float angle)
{
	int64_t result = -1;
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

void RotateScan::put_cache(float angle, int64_t difference)
{
	RotateScanCache *ptr = new RotateScanCache(angle, difference);
	cache_lock->lock("RotateScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}









RotateScanCache::RotateScanCache(float angle, int64_t difference)
{
	this->angle = angle;
	this->difference = difference;
}



