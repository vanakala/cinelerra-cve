
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "edl.inc"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"
#include "shapewipe.h"

#include <png.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN(ShapeWipeMain)

ShapeWipeW2B::ShapeWipeW2B(ShapeWipeMain *plugin, 
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 0, 
		_("White to Black"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeW2B::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

ShapeWipeB2W::ShapeWipeB2W(ShapeWipeMain *plugin, 
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 1, 
		_("Black to White"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeB2W::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}

ShapeWipeAntiAlias::ShapeWipeAntiAlias(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_CheckBox (x,y,plugin->antialias, _("Anti-aliasing"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeAntiAlias::handle_event()
{
	plugin->antialias = get_value();
	plugin->send_configure_change();
	return 0;
}

ShapeWipePreserveAspectRatio::ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_CheckBox (x,y,plugin->preserve_aspect, _("Preserve shape aspect ratio"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipePreserveAspectRatio::handle_event()
{
	plugin->preserve_aspect = get_value();
	plugin->send_configure_change();
	return 0;
}

ShapeWipeFilename::ShapeWipeFilename(
	ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	char *value,
	int x,
	int y)
 : BC_TextBox(x,y,180,1, value)
{
	this->plugin = plugin;
	this->window = window;
	this->value = value;
}

int ShapeWipeFilename::handle_event()
{
	value = get_text();
	strcpy(plugin->filename, get_text());
	plugin->send_configure_change();
	return 0;
}

ShapeWipeBrowseButton::ShapeWipeBrowseButton(
	ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	ShapeWipeFilename *filename,
	int x,
	int y)
 : BC_GenericButton(x,y,_("Browse..."))
{
	this->plugin = plugin;
	this->window = window;
	this->filename = filename;
}

int ShapeWipeBrowseButton::handle_event()
{
	int result;
	ShapeWipeLoad window(filename, filename->get_text());
	window.create_objects();
	window.update_filter("*.png");
	result = window.run_window();

	if (!result)
	{
		filename->update(window.get_submitted_path());
		strcpy(plugin->filename, window.get_submitted_path());
		plugin->send_configure_change();
	}

	return 0;
}

ShapeWipeLoad::ShapeWipeLoad(
	ShapeWipeFilename *filename, 
	char *init_directory)
 : BC_FileBox(
	1,
	1,
	init_directory, 
	_("Choose Shape"), 
	_("Choose a Wipe Shape"))
{
   this->filename = filename;
}

ShapeWipeWindow::ShapeWipeWindow(ShapeWipeMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x, 
	y, 
	450, 
	125, 
	450, 
	125, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

int ShapeWipeWindow::close_event()
{
	set_done(1);
	return 1;
}

void ShapeWipeWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(left = new ShapeWipeW2B(plugin, 
		this,
		x,
		y));
	x += 200;
	add_subwindow(right = new ShapeWipeB2W(plugin, 
		this,
		x,
		y));
	x = 10; y += 25;
	add_subwindow(new BC_Title(x, y, _("Shape:")));
	x += 100;

	add_subwindow(filename_widget = new 
	ShapeWipeFilename(plugin, 
		this,
		plugin->filename,
		x,
		y));
	x += 200;
	add_subwindow(new ShapeWipeBrowseButton(
		plugin, 
		this,
		filename_widget,
		x,
		y));
	x = 110; y += 25;
	add_subwindow(new ShapeWipeAntiAlias(
		plugin, 
		this,
		x,
		y));
	x = 110; y += 25;
	add_subwindow(new ShapeWipePreserveAspectRatio(
		plugin, 
		this,
		x,
		y));
	show_window();
	flush();
}

PLUGIN_THREAD_OBJECT(ShapeWipeMain, ShapeWipeThread, ShapeWipeWindow)

ShapeWipeMain::ShapeWipeMain(PluginServer *server)
 : PluginVClient(server)
{
	direction = 0;
	strcpy(filename, DEFAULT_SHAPE);	// is defined by a -D compiler instruction
	last_read_filename[0] = '\0';
	pattern_image = NULL;
	min_value = 256;
	max_value = 0;
	antialias = 0;
	preserve_aspect = 0;
	last_preserve_aspect = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ShapeWipeMain::~ShapeWipeMain()
{
	reset_pattern_image();
	PLUGIN_DESTRUCTOR_MACRO
}

char* ShapeWipeMain::plugin_title() { return N_("Shape Wipe"); }
int ShapeWipeMain::is_video() { return 1; }
int ShapeWipeMain::is_transition() { return 1; }
int ShapeWipeMain::uses_gui() { return 1; }

SHOW_GUI_MACRO(ShapeWipeMain, ShapeWipeThread);
SET_STRING_MACRO(ShapeWipeMain)
RAISE_WINDOW_MACRO(ShapeWipeMain)


VFrame* ShapeWipeMain::new_picon()
{
	return new VFrame(picon_png);
}

int ShapeWipeMain::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sshapewipe.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	direction = defaults->get("DIRECTION", direction);
	antialias = defaults->get("ANTIALIAS", antialias);
	preserve_aspect = defaults->get("PRESERVE_ASPECT", preserve_aspect);
	defaults->get("FILENAME", filename);
	return 0;
}

int ShapeWipeMain::save_defaults()
{
	defaults->update("DIRECTION", direction);
	defaults->update("ANTIALIAS", antialias);
	defaults->update("PRESERVE_ASPECT", preserve_aspect);
	defaults->update("FILENAME", filename);
	defaults->save();
	return 0;
}

void ShapeWipeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SHAPEWIPE");
	output.tag.set_property("DIRECTION", direction);
	output.tag.set_property("ANTIALIAS", antialias);
	output.tag.set_property("PRESERVE_ASPECT", preserve_aspect);
	output.tag.set_property("FILENAME", filename);
	output.append_tag();
	output.tag.set_title("/SHAPEWIPE");
	output.append_tag();
	output.terminate_string();
}

void ShapeWipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("SHAPEWIPE"))
		{
			direction = input.tag.get_property("DIRECTION", direction);
			antialias = input.tag.get_property("ANTIALIAS", antialias);
			preserve_aspect = input.tag.get_property("PRESERVE_ASPECT", preserve_aspect);
			input.tag.get_property("FILENAME", filename);
		}
	}
}

void ShapeWipeMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
}

int ShapeWipeMain::read_pattern_image(int new_frame_width, int new_frame_height)
{
	png_byte header[8];
	int is_png;
	int row;
	int col;
	int scaled_row;
	int scaled_col;
	int pixel_width;
	unsigned char value;
	png_uint_32 width;
	png_uint_32 height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	png_bytep *image;
	frame_width = new_frame_width;
	frame_height = new_frame_height;

	FILE *fp = fopen(filename, "rb");
	if (!fp)
	{
		return 1;
	}

	fread(header, 1, 8, fp);
	is_png = !png_sig_cmp(header, 0, 8);

	if (!is_png)
	{
		return 1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL);

	if (!png_ptr)
	{
	return 1;
	}

	/* Tell libpng we already checked the first 8 bytes */
	png_set_sig_bytes(png_ptr, 8);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return 1;
	}

	end_info = png_create_info_struct(png_ptr);
	if (!end_info)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		return 1;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	width  = png_get_image_width (png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);

	/* Skip the alpha channel if present 
	* stripping alpha currently doesn't work in conjunction with 
	* converting to grayscale in libpng */
	if (color_type & PNG_COLOR_MASK_ALPHA)
		pixel_width = 2;
	else
		pixel_width = 1;

	/* Convert 16 bit data to 8 bit */
	if (bit_depth == 16) png_set_strip_16(png_ptr);

	/* Expand to 1 pixel per byte if necessary */
	if (bit_depth < 8) png_set_packing(png_ptr);

	/* Convert to grayscale */
	if (color_type == PNG_COLOR_TYPE_RGB || 
		color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);

	/* Allocate memory to hold the original png image */
	image = (png_bytep*)malloc(sizeof(png_bytep)*height);
	for (row = 0; row < height; row++)
	{
		image[row] = (png_byte*)malloc(sizeof(png_byte)*width*pixel_width);
	}

	/* Allocate memory for the pattern image that will actually be
	* used for the wipe */
	pattern_image = (unsigned char**)malloc(sizeof(unsigned char*)*frame_height);


	png_read_image(png_ptr, image);
	png_read_end(png_ptr, end_info);

	double row_factor, col_factor;
	double row_offset = 0.5, col_offset = 0.5;	// for rounding

	if (preserve_aspect && aspect_w != 0 && aspect_h != 0)
	{
		row_factor = (height-1)/aspect_h;
		col_factor = (width-1)/aspect_w;
		if (row_factor < col_factor)
			col_factor = row_factor;
		else
			row_factor = col_factor;
		row_factor *= aspect_h/(double)(frame_height-1);
		col_factor *= aspect_w/(double)(frame_width-1);

		// center the pattern over the frame
		row_offset += (height-1-(frame_height-1)*row_factor)/2;
		col_offset += (width-1-(frame_width-1)*col_factor)/2;
	}
	else
	{
		// Stretch (or shrink) the pattern image to fill the frame
		row_factor = (double)(height-1)/(double)(frame_height-1);
		col_factor = (double)(width-1)/(double)(frame_width-1);
	}

	for (scaled_row = 0; scaled_row < frame_height; scaled_row++)
	{
		row = (int)(row_factor*scaled_row + row_offset);
		pattern_image[scaled_row] = (unsigned char*)malloc(sizeof(unsigned char)*frame_width);
		for (scaled_col = 0; scaled_col < frame_width; scaled_col++)
		{
			col = (int)(col_factor*scaled_col + col_offset)*pixel_width;
			value = image[row][col];
			pattern_image[scaled_row][scaled_col] = value;
			if (value < min_value) min_value = value;
			if (value > max_value) max_value = value;

		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fp);
	/* Deallocate the original image as it is no longer needed */
	for (row = 0; row < height; row++)
	{
		free(image[row]);
	}
	free (image);
	return 0;
}

void ShapeWipeMain::reset_pattern_image()
{
	int row;
	if (pattern_image != NULL)
	{
		for (row = 0; row < frame_height; row++)
		{
			free (pattern_image[row]);
		}
		free (pattern_image);
		pattern_image = NULL;
		min_value = 256, max_value = 0;	// are recalc'd in read_pattern_image
	}
}

#define SHAPEWIPE(type, components) \
{ \
\
	type  **in_rows = (type**)incoming->get_rows(); \
	type **out_rows = (type**)outgoing->get_rows(); \
	\
	type *in_row; \
	type *out_row; \
	\
	for(j = 0; j < h; j++) \
	{ \
		in_row = (type*) in_rows[j]; \
		out_row = (type*)out_rows[j]; \
		pattern_row = pattern_image[j]; \
		\
		col_offset = 0; \
		for(k = 0; k < w; k++) \
		{ \
			value = pattern_row[k]; \
			if ((direction == 0 && value >= threshold) || \
			(direction == 1 && value <= threshold)) \
			{ \
				out_row[col_offset]     = in_row[col_offset]; \
				out_row[col_offset + 1] = in_row[col_offset + 1]; \
				out_row[col_offset + 2] = in_row[col_offset + 2]; \
				if(components == 4) \
					out_row[col_offset + 3] = in_row[col_offset + 3]; \
			} \
			col_offset += components; \
		} \
	} \
}

#define COMPARE1(x,y) \
{ \
	if (pattern_image[x][y] <= threshold) opacity++; \
}

#define COMPARE2(x,y) \
{ \
	if (pattern_image[x][y] >= threshold) opacity++; \
}

// components is always 4
#define BLEND_ONLY_4_NORMAL(temp_type, type, max, chroma_offset,x,y) \
{ \
	const int bits = sizeof(type) * 8; \
	temp_type blend_opacity = (temp_type)(alpha * ((temp_type)1 << bits) + 0.5); \
	temp_type blend_transparency = ((temp_type)1 << bits) - blend_opacity; \
 \
	col = y * 4; \
	type* in_row = (type*)incoming->get_rows()[x]; \
	type* output = (type*)outgoing->get_rows()[x]; \
 \
	output[col] = ((temp_type)in_row[col] * blend_opacity + output[col] * blend_transparency) >> bits; \
	output[col+1] = ((temp_type)in_row[col+1] * blend_opacity + output[col+1] * blend_transparency) >> bits; \
	output[col+2] = ((temp_type)in_row[col+2] * blend_opacity + output[col+2] * blend_transparency) >> bits; \
}


// components is always 3
#define BLEND_ONLY_3_NORMAL(temp_type, type, max, chroma_offset,x,y) \
{ \
	const int bits = sizeof(type) * 8; \
	temp_type blend_opacity = (temp_type)(alpha * ((temp_type)1 << bits) + 0.5); \
	temp_type blend_transparency = ((temp_type)1 << bits) - blend_opacity; \
 \
	col = y * 3; \
	type* in_row = (type*)incoming->get_rows()[x]; \
	type* output = (type*)outgoing->get_rows()[x]; \
 \
	output[col] = ((temp_type)in_row[col] * blend_opacity + output[col] * blend_transparency) >> bits; \
	output[col+1] = ((temp_type)in_row[col+1] * blend_opacity + output[col+1] * blend_transparency) >> bits; \
	output[col+2] = ((temp_type)in_row[col+2] * blend_opacity + output[col+2] * blend_transparency) >> bits; \
}

/* opacity is defined as opacity of incoming frame */
#define BLEND(x,y,total) \
{ \
	float pixel_opacity = (float)opacity / total; \
	float alpha = pixel_opacity; \
	float pixel_transparency = 1.0 - pixel_opacity; \
	int col; \
 \
	if (pixel_opacity > 0.0) \
	{ \
		switch(incoming->get_color_model()) \
		{ \
		case BC_RGB_FLOAT: \
		{ \
			float  *in_row = (float*)incoming->get_rows()[x]; \
			float *out_row = (float*)outgoing->get_rows()[x]; \
			col = y * 3; \
			out_row[col] = in_row[col] * pixel_opacity + \
				out_row[col] * pixel_transparency; \
			out_row[col+1] = in_row[col+1] * pixel_opacity + \
				out_row[col+1] * pixel_transparency; \
			out_row[col+2] = in_row[col+2] * pixel_opacity + \
				out_row[col+2] * pixel_transparency; \
			break; \
		} \
		case BC_RGBA_FLOAT: \
		{ \
			float  *in_row = (float*)incoming->get_rows()[x]; \
			float *out_row = (float*)outgoing->get_rows()[x]; \
			col = y * 4; \
			out_row[col] = in_row[col] * pixel_opacity + \
				out_row[col] * pixel_transparency; \
			out_row[col+1] = in_row[col+1] * pixel_opacity + \
				out_row[col+1] * pixel_transparency; \
			out_row[col+2] = in_row[col+2] * pixel_opacity + \
				out_row[col+2] * pixel_transparency; \
			break; \
		} \
		case BC_RGB888: \
			BLEND_ONLY_3_NORMAL(uint32_t, unsigned char, 0xff, 0,x,y); \
			break; \
		case BC_YUV888: \
			BLEND_ONLY_3_NORMAL(int32_t, unsigned char, 0xff, 0x80,x,y); \
			break; \
		case BC_RGBA8888: \
			BLEND_ONLY_4_NORMAL(uint32_t, unsigned char, 0xff, 0,x,y); \
			break; \
		case BC_YUVA8888: \
			BLEND_ONLY_4_NORMAL(int32_t, unsigned char, 0xff, 0x80,x,y); \
			break; \
		case BC_RGB161616: \
			BLEND_ONLY_3_NORMAL(uint64_t, uint16_t, 0xffff, 0,x,y); \
			break; \
		case BC_YUV161616: \
			BLEND_ONLY_3_NORMAL(int64_t, uint16_t, 0xffff, 0x8000,x,y); \
			break; \
		case BC_RGBA16161616: \
			BLEND_ONLY_4_NORMAL(uint64_t, uint16_t, 0xffff, 0,x,y); \
			break; \
		case BC_YUVA16161616: \
			BLEND_ONLY_4_NORMAL(int64_t, uint16_t, 0xffff, 0x8000,x,y); \
			break; \
      } \
   } \
}

int ShapeWipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	unsigned char *pattern_row;
	int col_offset;
	unsigned char threshold;
	unsigned char value;
	int i,j,k;
	int opacity;

	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();

	if (strncmp(filename,last_read_filename,BCTEXTLEN)
		|| preserve_aspect != last_preserve_aspect)
	{
		reset_pattern_image();
	}

	if (!pattern_image) {
		read_pattern_image(w, h);
		strncpy(last_read_filename, filename, BCTEXTLEN);
		last_preserve_aspect = preserve_aspect;
	}

	if (!pattern_image)
	{
		fprintf(stderr, "Shape Wipe: cannot load shape %s\n", filename);
		return 0;
	}

	if (direction)
	{
		threshold = (unsigned char)(
				(float)PluginClient::get_source_position() /
				(float)PluginClient::get_total_len() * 
				(float)(max_value - min_value))
			+ min_value;
	}
	else
	{
		threshold = (unsigned char)((max_value - min_value) - ( 
				(float)PluginClient::get_source_position() /
				(float)PluginClient::get_total_len() * 
				(float)(max_value - min_value)))
			+ min_value;
	}

	if (antialias)
	{
		if (direction)
		{
			/* Top left corner */
			opacity = 0;
			COMPARE1(0,0);
			COMPARE1(0,1);
			COMPARE1(1,0);
			COMPARE1(1,1);
			BLEND(0,0,4.0);

			/* Top edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE1(0,k-1);
				COMPARE1(0,k);
				COMPARE1(0,k+1);
				COMPARE1(1,k-1);
				COMPARE1(1,k);
				COMPARE1(1,k+1);
				BLEND(0,k,6.0);
			}

			/* Top right corner */
			opacity = 0;
			COMPARE1(0,w-1);
			COMPARE1(0,w-2);
			COMPARE1(1,w-1);
			COMPARE1(1,w-2);
			BLEND(0,w-1,4.0);

			/* Left edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE1(j-1,0);
				COMPARE1(j,0);
				COMPARE1(j+1,0);
				COMPARE1(j-1,1);
				COMPARE1(j,1);
				COMPARE1(j+1,1);
				BLEND(j,0,6.0);
			}

			/* Middle */
			for (j = 1; j < h-1; j++)
			{
				for (k = 1; k < w-1; k++)
				{
					opacity = 0;
					COMPARE1(j-1,k-1);
					COMPARE1(j,k-1);
					COMPARE1(j+1,k-1);
					COMPARE1(j-1,k);
					COMPARE1(j,k);
					COMPARE1(j+1,k);
					COMPARE1(j-1,k+1);
					COMPARE1(j,k+1);
					COMPARE1(j+1,k+1);
					BLEND(j,k,9.0);
				}
			}

			/* Right edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE1(j-1,w-1);
				COMPARE1(j,w-1);
				COMPARE1(j+1,w-1);
				COMPARE1(j-1,w-2);
				COMPARE1(j,w-2);
				COMPARE1(j+1,w-2);
				BLEND(j,w-1,6.0);
			}

			/* Bottom left corner */
			opacity = 0;
			COMPARE1(h-1,0);
			COMPARE1(h-1,1);
			COMPARE1(h-2,0);
			COMPARE1(h-2,1);
			BLEND(h-1,0,4.0);

			/* Bottom edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE1(h-1,k-1);
				COMPARE1(h-1,k);
				COMPARE1(h-1,k+1);
				COMPARE1(h-2,k-1);
				COMPARE1(h-2,k);
				COMPARE1(h-2,k+1);
				BLEND(h-1,k,6.0);
			}

			/* Bottom right corner */
			opacity = 0;
			COMPARE1(h-1,w-1);
			COMPARE1(h-1,w-2);
			COMPARE1(h-2,w-1);
			COMPARE1(h-2,w-2);
			BLEND(h-1,w-1,4.0);
		}
		else
		{
			/* Top left corner */
			opacity = 0;
			COMPARE2(0,0);
			COMPARE2(0,1);
			COMPARE2(1,0);
			COMPARE2(1,1);
			BLEND(0,0,4.0);

			/* Top edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE2(0,k-1);
				COMPARE2(0,k);
				COMPARE2(0,k+1);
				COMPARE2(1,k-1);
				COMPARE2(1,k);
				COMPARE2(1,k+1);
				BLEND(0,k,6.0);
			}

			/* Top right corner */
			opacity = 0;
			COMPARE2(0,w-1);
			COMPARE2(0,w-2);
			COMPARE2(1,w-1);
			COMPARE2(1,w-2);
			BLEND(0,w-1,4.0);

			/* Left edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE2(j-1,0);
				COMPARE2(j,0);
				COMPARE2(j+1,0);
				COMPARE2(j-1,1);
				COMPARE2(j,1);
				COMPARE2(j+1,1);
				BLEND(j,0,6.0);
			}

			/* Middle */
			for (j = 1; j < h-1; j++)
			{
				for (k = 1; k < w-1; k++)
				{
					opacity = 0;
					COMPARE2(j-1,k-1);
					COMPARE2(j,k-1);
					COMPARE2(j+1,k-1);
					COMPARE2(j-1,k);
					COMPARE2(j,k);
					COMPARE2(j+1,k);
					COMPARE2(j-1,k+1);
					COMPARE2(j,k+1);
					COMPARE2(j+1,k+1);
					BLEND(j,k,9.0);
				}
			}

			/* Right edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE2(j-1,w-1);
				COMPARE2(j,w-1);
				COMPARE2(j+1,w-1);
				COMPARE2(j-1,w-2);
				COMPARE2(j,w-2);
				COMPARE2(j+1,w-2);
				BLEND(j,w-1,6.0);
			}

			/* Bottom left corner */
			opacity = 0;
			COMPARE2(h-1,0);
			COMPARE2(h-1,1);
			COMPARE2(h-2,0);
			COMPARE2(h-2,1);
			BLEND(h-1,0,4.0);

			/* Bottom edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE2(h-1,k-1);
				COMPARE2(h-1,k);
				COMPARE2(h-1,k+1);
				COMPARE2(h-2,k-1);
				COMPARE2(h-2,k);
				COMPARE2(h-2,k+1);
				BLEND(h-1,k,6.0);
			}

			/* Bottom right corner */
			opacity = 0;
			COMPARE2(h-1,w-1);
			COMPARE2(h-1,w-2);
			COMPARE2(h-2,w-1);
			COMPARE2(h-2,w-2);
			BLEND(h-1,w-1,4.0);
		}
	}
	else
	{
		switch(incoming->get_color_model())
		{
		case BC_RGB_FLOAT:
			SHAPEWIPE(float, 3)
			break;
		case BC_RGB888:
		case BC_YUV888:
			SHAPEWIPE(unsigned char, 3)
			break;
		case BC_RGBA_FLOAT:
			SHAPEWIPE(float, 4)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			SHAPEWIPE(unsigned char, 4)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			SHAPEWIPE(uint16_t, 3)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			SHAPEWIPE(uint16_t, 4)
			break;
		}
	}
	return 0;
}
