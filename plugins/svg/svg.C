#include "clip.h"
#include "filexml.h"
#include "picon_png.h"
#include "svg.h"
#include "svgwin.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#include "empty_svg.h"

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
	force_png_render = 1;
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

char* SvgMain::plugin_title() { return _("SVG via Sodipodi"); }
int SvgMain::is_realtime() { return 1; }

NEW_PICON_MACRO(SvgMain)

int SvgMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%ssvg.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
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
	int fh_lockfile;
	char filename_png[1024], filename_lock[1024];
	struct stat st_svg, st_png;
	int result_stat_png;
	VFrame *input, *output;
	input = input_ptr;
	output = output_ptr;

	
	need_reconfigure |= load_configuration();

	if (config.svg_file[0] == 0) {
		output->copy_from(input);
		return(0);
	}

	
	strcpy(filename_png, config.svg_file);
	strcat(filename_png, ".png");
	// get the time of the last PNG change
	result_stat_png = stat (filename_png, &st_png);
	


//	printf("PNg mtime: %li, last_load: %li\n", st_png.st_mtime, config.last_load);
	if (need_reconfigure || result_stat_png || (st_png.st_mtime > config.last_load)) {
		if (temp_frame)
			delete temp_frame;
		temp_frame = 0;
	}
	need_reconfigure = 0;
	
	if(!temp_frame) 
	{
		int result;
		VFrame *tmp2;
//		printf("PROCESSING: %s %li\n", filename_png, config.last_load);

		if (result = stat (config.svg_file, &st_svg)) 
		{
			printf(_("Error calling stat() on svg file: %s\n"), config.svg_file); 
		}
		if (force_png_render || result_stat_png || 
			st_png.st_mtime < st_svg.st_mtime) 
		{
			char command[1024];
			sprintf(command,
				"sodipodi --export-png=%s --export-width=%i --export-height=%i %s",
				filename_png, (int)config.in_w, (int)config.in_h, config.svg_file);
			printf(_("Running command %s\n"), command);
			system(command);
			stat(filename_png, &st_png);
			force_png_render = 0;
		}

		// advisory lock, so we wait for sodipodi to finish
		strcpy(filename_lock, filename_png);
		strcat(filename_lock, ".lock");
//		printf("Cinelerra locking %s\n", filename_lock);
		fh_lockfile = open (filename_lock, O_CREAT | O_RDWR);
		int res = lockf(fh_lockfile, F_LOCK, 10);    // Blocking call - will wait for sodipodi to finish!
//		printf("Cinelerra: filehandle: %i, cineres: %i, errno: %i\n", fh_lockfile, res, errno);
//		perror("Cineerror");
		int fh = open(filename_png, O_RDONLY);
		unsigned char *pngdata;
		// get the size again
		result_stat_png = fstat (fh, &st_png);

		pngdata = (unsigned char*) malloc(st_png.st_size + 4);
		*((int32_t *)pngdata) = st_png.st_size; 
//		printf("PNG size: %i\n", st_png.st_size);
		result = read(fh, pngdata+4, st_png.st_size);
		close(fh);
		// unlock the file
		lockf(fh_lockfile, F_ULOCK, 0);
		close(fh_lockfile);
//		printf("Cinelerra unlocking\n");

		config.last_load = st_png.st_mtime; // we just updated
		
		tmp2 = new VFrame(pngdata);
		temp_frame = new VFrame(0, 
				        tmp2->get_w(),
					tmp2->get_h(),
					output_ptr->get_color_model());
		free (pngdata);
	        cmodel_transfer(temp_frame->get_rows(),
	                tmp2->get_rows(),
	                0,
	                0,
	                0,
	                0,
	                0,
	                0,
	                0,
	                0,
	                tmp2->get_w(),
	                tmp2->get_h(),
	                0,
	                0,
	                temp_frame->get_w(),
	                temp_frame->get_h(),
	               	tmp2->get_color_model(),
	                temp_frame->get_color_model(),
	                0,
	                tmp2->get_w(),
	                temp_frame->get_w());

		delete tmp2;

	}

//printf("SvgMain::process_realtime 2 %p\n", input);


	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}

//	output->clear_frame();


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
			config.in_x, 
			config.in_y, 
			config.in_x + config.in_w,
			config.in_y + config.in_h,
			config.out_x, 
			config.out_y, 
			config.out_x + config.out_w,
			config.out_y + config.out_h,
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
		thread->window->out_w->update(config.out_w);
		thread->window->out_h->update(config.out_h);
		thread->window->svg_file_title->update(config.svg_file);
		thread->window->unlock_window();
	}
}
