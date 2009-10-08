
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

#include "assets.h"
#include "autoconf.h"
#include "bezierautos.h"
#include "cache.h"
#include "edit.h"
#include "edits.h"
#include "filexml.h"
#include "floatautos.h"
#include "mwindow.h"
#include "patch.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vtrack.h"
#include "datatype.h"

VTrack::VTrack(MWindow *mwindow, Tracks *tracks) : Track(mwindow, tracks)
{
	data_type = TRACK_VIDEO;
}

VTrack::~VTrack()
{
	delete edits;
	delete camera_autos;
	delete projector_autos;
}

int VTrack::create_derived_objs(int flash)
{
	int i;
	edits = new VEdits(mwindow, this);
	camera_autos = new BezierAutos(this, 
									WHITE, 
									0, 
									0, 
									1, 
									mwindow->track_w,
									mwindow->track_h);
	
	projector_autos = new BezierAutos(this,
									WHITE,
									0,
									0,
									1,
									mwindow->output_w,
									mwindow->output_h);
	
	fade_autos = new FloatAutos(this, LTGREY, -100, 100);
	fade_autos->create_objects();

	if(mwindow->gui)
	{
		if(mwindow->session->tracks_vertical)
		draw(pixel, mwindow->zoom_track, 0, tracks->canvas->h, flash);
		else
		draw(0, tracks->canvas->w, pixel, mwindow->zoom_track, flash);
	}
}


int VTrack::save_derived(FileXML *xml)
{
	edits->save(xml);

	if(camera_autos->total())
	{
		xml->tag.set_title("CAMERAAUTOS");
		xml->append_tag();
		xml->append_newline();
		camera_autos->save(xml);
		xml->tag.set_title("/CAMERAAUTOS");
		xml->append_tag();
		xml->append_newline();
	}

	if(projector_autos->total())
	{
		xml->tag.set_title("PROJECTORAUTOS");
		xml->append_tag();
		xml->append_newline();
		projector_autos->save(xml);
		xml->tag.set_title("/PROJECTORAUTOS");
		xml->append_tag();
		xml->append_newline();
	}
}

int VTrack::load_derived(FileXML *xml, int automation_only, int edits_only, int load_all, int &output_channel)
{
	if(xml->tag.title_is("CAMERAAUTOS"))
	{
		camera_autos->load(xml, "/CAMERAAUTOS");
	}
	else
	if(xml->tag.title_is("PROJECTORAUTOS"))
	{
		projector_autos->load(xml, "/PROJECTORAUTOS");
	}
}

long VTrack::length()
{
	return tosamples(edits->end(), mwindow->sample_rate, mwindow->frame_rate);
}

int VTrack::get_dimensions(float &view_start, float &view_units, float &zoom_units)
{
	view_start = toframes(mwindow->view_start, mwindow->sample_rate, mwindow->frame_rate);
	view_units = toframes(tracks->view_samples(), mwindow->sample_rate, mwindow->frame_rate);
	zoom_units = toframes(mwindow->zoom_sample, mwindow->sample_rate, mwindow->frame_rate);
}

long VTrack::samples_to_units(long &samples)
{
	samples = toframes_round(samples, mwindow->sample_rate, mwindow->frame_rate);
}

int VTrack::copy_derived(long start, long end, FileXML *xml)
{
// automation is copied in the Track::copy
}

int VTrack::copy_automation_derived(AutoConf *auto_conf, long start, long end, FileXML *xml)
{
// used for copying only automation
	if(auto_conf->camera)
	{
		xml->tag.set_title("CAMERAAUTOS");
		xml->append_tag();

		camera_autos->copy(start, end, xml, 1);

		xml->tag.set_title("/CAMERAAUTOS");
		xml->append_tag();
		xml->append_newline();
	}

	if(auto_conf->projector)
	{
		xml->tag.set_title("PROJECTORAUTOS");
		xml->append_tag();

		projector_autos->copy(start, end, xml, 1);

		xml->tag.set_title("/PROJECTORAUTOS");
		xml->append_tag();
		xml->append_newline();
	}
}

int VTrack::paste_derived(long start, long end, long total_length, FileXML *xml, int &current_channel)
{
	if(xml->tag.title_is("CAMERAAUTOS"))
	{
		camera_autos->paste(start, end, total_length, xml, "/CAMERAAUTOS", 1);
	}
	else
	if(xml->tag.title_is("PROJECTORAUTOS"))
	{
		projector_autos->paste(start, end, total_length, xml, "/PROJECTORAUTOS", 1);
	}
}

int VTrack::paste_output(long startproject, long endproject, long startsource, long endsource, int layer, Asset *asset)
{
	int result = 0;

//printf("VTrack::paste_output startproject %ld endproject %ld\n", startproject, endproject);
	//Track::samples_to_units(startproject, endproject);

	result = ((VEdits*)edits)->paste_edit(startproject, 
						endproject, 
						startsource, 
						endsource - startsource, 
						layer, 
						0,
						0,
						1,
						asset);

	//if(!result && mwindow->autos_follow_edits)
	//{
	//	paste_auto_silence(startproject, endproject);
	//}
	return result;
}

int VTrack::clear_derived(long start, long end)
{
	if(mwindow->autos_follow_edits)
	{
		camera_autos->clear(start, end, mwindow->autos_follow_edits, 1);
		projector_autos->clear(start, end, mwindow->autos_follow_edits, 1);
	}
}

int VTrack::paste_automation_derived(long start, long end, long total_length, FileXML *xml, int shift_autos, int &current_pan)
{
// only used for pasting automation
	camera_autos->paste(start, end, total_length, xml, "/CAMERAUTOS", 1, shift_autos);
	projector_autos->paste(start, end, total_length, xml, "/PROJECTORAUTOS", 1, shift_autos);
}

int VTrack::clear_automation_derived(AutoConf *auto_conf, long start, long end, int shift_autos)
{
	if(auto_conf->camera)
		camera_autos->clear(start, end, mwindow->autos_follow_edits, shift_autos);

	if(auto_conf->projector)
		projector_autos->clear(start, end, mwindow->autos_follow_edits, shift_autos);
}

int VTrack::paste_auto_silence_derived(long start, long end)
{
	camera_autos->paste_silence(start, end);
	projector_autos->paste_silence(start, end);
}

int VTrack::draw_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf)
{
		if(auto_conf->camera) 
			camera_autos->draw(tracks->canvas, 
							pixel,
							mwindow->zoom_track, 
							zoom_units, 
							view_start, 
							mwindow->session->tracks_vertical);

		if(auto_conf->projector)
			projector_autos->draw(tracks->canvas, 
							pixel,
							mwindow->zoom_track, 
							zoom_units, 
							view_start, 
							mwindow->session->tracks_vertical);
}

int VTrack::select_translation(int cursor_x, int cursor_y)
{
// cursor position is relative to time
	int result = 0;
	float view_start, view_units, zoom_units;
	get_dimensions(view_start, view_units, zoom_units);

	if(cursor_y > pixel && cursor_y < pixel + mwindow->zoom_track)
	{
		for(Edit* current = edits->first; current && !result; current = NEXT)
		{
			result = ((VEdit*)current)->select_translation(cursor_x, cursor_y, view_start, zoom_units);
		}
	}
	return result;
}

int VTrack::update_translation(int cursor_x, int cursor_y, int shift_down)
{
	int result = 0;
	float view_start, view_units, zoom_units;
	get_dimensions(view_start, view_units, zoom_units);

	for(Edit* current = edits->first; current && !result; current = NEXT)
	{
		result = ((VEdit*)current)->update_translation(cursor_x, cursor_y, shift_down, view_start, zoom_units);
	}
	return result;
}

int VTrack::end_translation()
{
	int result = 0;
	for(Edit* current = edits->first; current && !result; current = NEXT)
	{
		result = ((VEdit*)current)->end_translation();
	}
	return result;
}

int VTrack::reset_translation(long start, long end)
{
	int result = 0;
	Track::samples_to_units(start, end);
	for(Edit* current = edits->first; current && !result; current = NEXT)
	{
		result = ((VEdit*)current)->reset_translation(start, end);
	}
	return result;
}


int VTrack::select_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y)
{
	int result = 0;

	if(auto_conf->camera) 
		result = camera_autos->select_auto(tracks->canvas, 
										pixel, 
										mwindow->zoom_track, 
										zoom_units, 
										view_start, 
										cursor_x, 
										cursor_y, 
										tracks->canvas->shift_down(),
										tracks->canvas->ctrl_down(),
										tracks->canvas->get_buttonpress(),
										mwindow->session->tracks_vertical);

	if(auto_conf->projector && !result) 
		result = projector_autos->select_auto(tracks->canvas, 
										pixel, 
										mwindow->zoom_track, 
										zoom_units, 
										view_start, 
										cursor_x, 
										cursor_y, 
										tracks->canvas->shift_down(),
										tracks->canvas->ctrl_down(),
										tracks->canvas->get_buttonpress(),
										mwindow->session->tracks_vertical);

	return result;
}


int VTrack::move_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down)
{
	int result;
	result = 0;

	if(auto_conf->camera)
		result = camera_autos->move_auto(tracks->canvas, 
									pixel, 
									mwindow->zoom_track, 
									zoom_units, 
									view_start, 
									cursor_x, 
									cursor_y, 
									shift_down, 
									mwindow->session->tracks_vertical);

	if(auto_conf->projector && !result)
		result = projector_autos->move_auto(tracks->canvas, 
									pixel, 
									mwindow->zoom_track, 
									zoom_units, 
									view_start, 
									cursor_x, 
									cursor_y, 
									shift_down, 
									mwindow->session->tracks_vertical);

	if(result)
	{
		mwindow->tracks->hide_overlays(0);
		draw_clear(0, tracks->canvas->w, 0, tracks->canvas->h, 0);
		draw(0, tracks->canvas->w, 0, tracks->canvas->h, 0);
		mwindow->tracks->show_overlays(0);
	}

	return result;
}

int VTrack::draw_floating_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf, int flash)
{
	if(auto_conf->camera) 
		camera_autos->draw_floating_autos(tracks->canvas, 
								pixel, 
								mwindow->zoom_track, 
								zoom_units, 
								view_start, 
								mwindow->session->tracks_vertical, flash);

	if(auto_conf->projector) 
		projector_autos->draw_floating_autos(tracks->canvas, 
								pixel, 
								mwindow->zoom_track, 
								zoom_units, 
								view_start, 
								mwindow->session->tracks_vertical, flash);
}

int VTrack::release_auto_derived()
{
	int result;
	result = 0;

	result = camera_autos->release_auto();
	if(!result) result = projector_autos->release_auto();
		
	return result;
}

int VTrack::scale_video(float camera_scale, float projector_scale, int *offsets)
{
// Fix the camera.
	for(VEdit *current = (VEdit*)edits->first; current; current = (VEdit*)NEXT)
	{
		current->center_z *= camera_scale;
		current->center_x = -offsets[0];
		current->center_y = -offsets[1];
	}

// Fix the projector.
	projector_autos->scale_video(projector_scale, &offsets[2]);
}

int VTrack::render(VFrame **output, long input_len, long input_position, float step)
{
	BezierAuto *before[4], *after[4];     // for bounding box
	int i;
	for(i = 0; i < 4; i++) { before[i] = 0;  after[i] = 0; }

// clear output buffer
//	for(i = 0; i < input_len; i++)
//	{
//		output[i]->clear_frame();
//	}
	output[0]->clear_frame();

// Render from the last edit to the first edit to accomidate feathering
	for(VEdit *current = (VEdit*)edits->last; current; current = (VEdit*)PREVIOUS)
	{
//if(current == (VEdit*)edits->first || current == (VEdit*)edits->first->next)
//	printf("%d %d %d\n", input_position, current->startproject, current->startproject + current->length);
		if(input_position < current->startproject + current->length + current->feather_right &&
			input_position >= current->startproject)
			current->render(output, 
				input_len, 
				input_position, 
				step, 
				before, 
				after, 
				get_patch_of()->automate);
	}
}

int VTrack::get_projection(float &in_x1, float &in_y1, float &in_x2, float &in_y2,
					float &out_x1, float &out_y1, float &out_x2, float &out_y2,
					int frame_w, int frame_h, long real_position,
					BezierAuto **before, BezierAuto **after)
{
	static float center_x, center_y, center_z;
	static float x[4], y[4];

	projector_autos->get_center(center_x, center_y, center_z, (float)real_position, 0, before, after);
	x[0] = y[0] = 0;
	x[1] = frame_w;
	y[1] = frame_h;

	center_x += mwindow->output_w / 2;
	center_y += mwindow->output_h / 2;

	x[2] = center_x - (frame_w / 2) * center_z;
	y[2] = center_y - (frame_h / 2) * center_z;
	x[3] = x[2] + frame_w * center_z;
	y[3] = y[2] + frame_h * center_z;

	if(x[2] < 0)
	{
		x[0] -= x[2] / center_z;
		x[2] = 0;
	}
	if(y[2] < 0)
	{
		y[0] -= y[2] / center_z;
		y[2] = 0;
	}
	if(x[3] > mwindow->output_w)
	{
		x[1] -= (x[3] - mwindow->output_w) / center_z;
		x[3] = mwindow->output_w;
	}
	if(y[3] > mwindow->output_h)
	{
		y[1] -= (y[3] - mwindow->output_h) / center_z;
		y[3] = mwindow->output_h;
	}
	
	in_x1 = x[0];
	in_y1 = y[0];
	in_x2 = x[1];
	in_y2 = y[1];
	out_x1 = x[2];
	out_y1 = y[2];
	out_x2 = x[3];
	out_y2 = y[3];
}

int VTrack::scale_time_derived(float rate_scale, int scale_edits, int scale_autos, long start, long end)
{
	camera_autos->scale_time(rate_scale, scale_edits, scale_autos, start, end);
	projector_autos->scale_time(rate_scale, scale_edits, scale_autos, start, end);
}

