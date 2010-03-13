
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

#include "clip.h"
#include "filexml.h"
#include "picon_png.h"
#include "svg.h"
#include "svgwin.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#include "empty_svg.h"

struct raw_struct {
	char rawc[5];        // Null terminated "RAWC" string
	int32_t struct_version;  // currently 1 (bumped at each destructive change) 
	int32_t struct_size;     // size of this struct in bytes
	int32_t width;               // logical width of image
	int32_t height;
	int32_t pitch;           // physical width of image in memory
	int32_t color_model;      // as BC_ constant, currently only BC_RGBA8888 is supported
	int64_t time_of_creation; // in milliseconds - calculated as (tv_sec * 1000 + tv_usec / 1000);
				// we can't trust date on the file, due to different reasons
};	


REGISTER_PLUGIN(SvgMain)

SvgConfig::SvgConfig()
{
	in_x = 0;
	in_y = 0;
	in_w = 720;
	in_h = 480;
	out_x = 0;
	out_y = 0;
	out_w = 720;
	out_h = 480;
	last_load = 0;
	strcpy(svg_file, "");
}

int SvgConfig::equivalent(SvgConfig &that)
{
	return EQUIV(in_x, that.in_x) && 
		EQUIV(in_y, that.in_y) && 
		EQUIV(in_w, that.in_w) && 
		EQUIV(in_h, that.in_h) &&
		EQUIV(out_x, that.out_x) && 
		EQUIV(out_y, that.out_y) && 
		EQUIV(out_w, that.out_w) &&
		EQUIV(out_h, that.out_h) &&
		!strcmp(svg_file, that.svg_file);
}

void SvgConfig::copy_from(SvgConfig &that)
{
	in_x = that.in_x;
	in_y = that.in_y;
	in_w = that.in_w;
	in_h = that.in_h;
	out_x = that.out_x;
	out_y = that.out_y;
	out_w = that.out_w;
	out_h = that.out_h;
	last_load = that.last_load;
	strcpy(svg_file, that.svg_file);
}

void SvgConfig::interpolate(SvgConfig &prev, 
	SvgConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->in_x = prev.in_x * prev_scale + next.in_x * next_scale;
	this->in_y = prev.in_y * prev_scale + next.in_y * next_scale;
	this->in_w = prev.in_w * prev_scale + next.in_w * next_scale;
	this->in_h = prev.in_h * prev_scale + next.in_h * next_scale;
	this->out_x = prev.out_x * prev_scale + next.out_x * next_scale;
	this->out_y = prev.out_y * prev_scale + next.out_y * next_scale;
	this->out_w = prev.out_w * prev_scale + next.out_w * next_scale;
	this->out_h = prev.out_h * prev_scale + next.out_h * next_scale;
	strcpy(this->svg_file, prev.svg_file);
}








SvgMain::SvgMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	overlayer = 0;
	need_reconfigure = 0;
	force_raw_render = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SvgMain::~SvgMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
}

const char* SvgMain::plugin_title() { return N_("SVG via Inkscape"); }
int SvgMain::is_realtime() { return 1; }
int SvgMain::is_synthesis() { return 1; }

NEW_PICON_MACRO(SvgMain)

int SvgMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%ssvg.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();


	config.in_x = defaults->get("IN_X", config.in_x);
	config.in_y = defaults->get("IN_Y", config.in_y);
	config.in_w = defaults->get("IN_W", config.in_w);
	config.in_h = defaults->get("IN_H", config.in_h);
	config.out_x = defaults->get("OUT_X", config.out_x);
	config.out_y = defaults->get("OUT_Y", config.out_y);
	config.out_w = defaults->get("OUT_W", config.out_w);
	config.out_h = defaults->get("OUT_H", config.out_h);
	strcpy(config.svg_file, "");
//	defaults->get("SVG_FILE", config.svg_file);
}

int SvgMain::save_defaults()
{
	defaults->update("IN_X", config.in_x);
	defaults->update("IN_Y", config.in_y);
	defaults->update("IN_W", config.in_w);
	defaults->update("IN_H", config.in_h);
	defaults->update("OUT_X", config.out_x);
	defaults->update("OUT_Y", config.out_y);
	defaults->update("OUT_W", config.out_w);
	defaults->update("OUT_H", config.out_h);
	defaults->update("SVG_FILE", config.svg_file);
	defaults->save();
}

LOAD_CONFIGURATION_MACRO(SvgMain, SvgConfig)

void SvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

// Store data
	output.tag.set_title("SVG");
	output.tag.set_property("IN_X", config.in_x);
	output.tag.set_property("IN_Y", config.in_y);
	output.tag.set_property("IN_W", config.in_w);
	output.tag.set_property("IN_H", config.in_h);
	output.tag.set_property("OUT_X", config.out_x);
	output.tag.set_property("OUT_Y", config.out_y);
	output.tag.set_property("OUT_W", config.out_w);
	output.tag.set_property("OUT_H", config.out_h);
	output.tag.set_property("SVG_FILE", config.svg_file);
	output.append_tag();
	output.tag.set_title("/SVG");
	output.append_tag();

	output.terminate_string();
// data is now in *text
}

void SvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SVG"))
			{
 				config.in_x = input.tag.get_property("IN_X", config.in_x);
				config.in_y = input.tag.get_property("IN_Y", config.in_y);
				config.in_w = input.tag.get_property("IN_W", config.in_w);
				config.in_h = input.tag.get_property("IN_H", config.in_h);
				config.out_x =	input.tag.get_property("OUT_X", config.out_x);
				config.out_y =	input.tag.get_property("OUT_Y", config.out_y);
				config.out_w =	input.tag.get_property("OUT_W", config.out_w);
				config.out_h =	input.tag.get_property("OUT_H", config.out_h);
				input.tag.get_property("SVG_FILE", config.svg_file);
			}
		}
	}
}








int SvgMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	char filename_raw[1024];
	int fh_raw;
	struct stat st_raw;
	VFrame *input, *output;
	input = input_ptr;
	output = output_ptr;
	unsigned char * raw_buffer;
	struct raw_struct *raw_data;

	need_reconfigure |= load_configuration();

	if (config.svg_file[0] == 0) {
		output->copy_from(input);
		return(0);
	}

	strcpy(filename_raw, config.svg_file);
	strcat(filename_raw, ".raw");
	fh_raw = open(filename_raw, O_RDWR); // in order for lockf to work it has to be open for writing

	if (fh_raw == -1 || force_raw_render) // file does not exist, export it
	{
		need_reconfigure = 1;
		char command[1024];
		sprintf(command,
			"inkscape --without-gui --cinelerra-export-file=%s %s",
			filename_raw, config.svg_file);
		printf(_("Running command %s\n"), command);
		if(system(command) < 0) {
			printf("Failed to execute command\n");
			return 0;
		}
		stat(filename_raw, &st_raw);
		force_raw_render = 0;
		fh_raw = open(filename_raw, O_RDWR); // in order for lockf to work it has to be open for writing
		if (!fh_raw) {
			printf(_("Export of %s to %s failed\n"), config.svg_file, filename_raw);
			return 0;
		}
	}


	// file exists, ... lock it, mmap it and check time_of_creation
	if(lockf(fh_raw, F_LOCK, 0) < 0){    // Blocking call - will wait for inkscape to finish!
		perror("SvgMain::process_realtime - lock");
		return 0;
	}
	fstat (fh_raw, &st_raw);
	raw_buffer = (unsigned char *)mmap (NULL, st_raw.st_size, PROT_READ, MAP_SHARED, fh_raw, 0); 
	raw_data = (struct raw_struct *) raw_buffer;

	if (strcmp(raw_data->rawc, "RAWC")) 
	{
		printf (_("The file %s that was generated from %s is not in RAWC format. Try to delete all *.raw files.\n"), filename_raw, config.svg_file);	
		if(lockf(fh_raw, F_ULOCK, 0))
			perror("SvgMain::process_realtime - unlock");
		close(fh_raw);
		return (0);
	}
	if (raw_data->struct_version > 1) 
	{
		printf (_("Unsupported version of RAWC file %s. This means your Inkscape uses newer RAWC format than Cinelerra. Please upgrade Cinelerra.\n"), filename_raw);
		if(lockf(fh_raw, F_ULOCK, 0))
			perror("SvgMain::process_realtime - unlock");
		close(fh_raw);
		return (0);
	}
	// Ok, we can now be sure we have valid RAWC file on our hands
	if (need_reconfigure || config.last_load < raw_data->time_of_creation) {    // the file was updated or is new (then last_load is zero)

		if (temp_frame && 
		  !temp_frame->params_match(raw_data->width, raw_data->height, output_ptr->get_color_model()))
		{
			// parameters don't match
			delete temp_frame;
			temp_frame = 0;
		}
		if (!temp_frame)			
			temp_frame = new VFrame(0, 
				        raw_data->width,
					raw_data->height,
					output_ptr->get_color_model());

		// temp_frame is ready by now, we can do the loading
		unsigned char ** raw_rows;
		raw_rows = new unsigned char*[raw_data->height]; // this could be optimized, so new isn't used every time
		for (int i = 0; i < raw_data->height; i++) {
			raw_rows[i] = raw_buffer + raw_data->struct_size + raw_data->pitch * i * 4;
		}
	        cmodel_transfer(temp_frame->get_rows(),
	                raw_rows,
	                0,
	                0,
	                0,
	                0,
	                0,
	                0,
	                0,
	                0,
	                raw_data->width,
	                raw_data->height,
	                0,
	                0,
	                temp_frame->get_w(),
	                temp_frame->get_h(),
	               	BC_RGBA8888,
	                temp_frame->get_color_model(),
	                0,
	                raw_data->pitch,
	                temp_frame->get_w());
		delete [] raw_rows;
		munmap(raw_buffer, st_raw.st_size);
		if(lockf(fh_raw, F_ULOCK, 0))
			perror("SvgMain::process_realtime - unlock");
		close(fh_raw);


	}	
	// by now we have temp_frame ready, we just need to overylay it

	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}



// printf("SvgMain::process_realtime 3 output=%p input=%p config.w=%f config.h=%f"
// 	"%f %f %f %f -> %f %f %f %f\n", 
// 	output,
// 	input,
// 	config.w, 
// 	config.h,
// 	in_x1, 
// 	in_y1, 
// 	in_x2, 
// 	in_y2,
// 	out_x1, 
// 	out_y1, 
// 	out_x2, 
// 	out_y2);
		output->copy_from(input);
		overlayer->overlay(output, 
			temp_frame,
			0, 
			0, 
			temp_frame->get_w(),
			temp_frame->get_h(),
			config.out_x, 
			config.out_y, 
			config.out_x + temp_frame->get_w(),
			config.out_y + temp_frame->get_h(),
			1,
			TRANSFER_NORMAL,
			get_interpolation_type());

	return(0);
}




SHOW_GUI_MACRO(SvgMain, SvgThread)
                                                              
RAISE_WINDOW_MACRO(SvgMain)

SET_STRING_MACRO(SvgMain)

void SvgMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
//		thread->window->in_x->update(config.in_x);
//		thread->window->in_y->update(config.in_y);
//		thread->window->in_w->update(config.in_w);
//		thread->window->in_h->update(config.in_h);
		thread->window->out_x->update(config.out_x);
		thread->window->out_y->update(config.out_y);
//		thread->window->out_w->update(config.out_w);
//		thread->window->out_h->update(config.out_h);
		thread->window->svg_file_title->update(config.svg_file);
		thread->window->unlock_window();
	}
}
