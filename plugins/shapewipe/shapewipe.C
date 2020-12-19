// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#include "bctitle.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"
#include "shapewipe.h"

#include <png.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

ShapeWipeW2B::ShapeWipeW2B(ShapeWipeMain *plugin, 
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 0, _("White to Black"))
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
	return 1;
}

ShapeWipeB2W::ShapeWipeB2W(ShapeWipeMain *plugin, 
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 1, _("Black to White"))
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
	return 1;
}

ShapeWipeAntiAlias::ShapeWipeAntiAlias(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_CheckBox(x, y, plugin->antialias, _("Anti-aliasing"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeAntiAlias::handle_event()
{
	plugin->antialias = get_value();
	plugin->send_configure_change();
	return 1;
}

ShapeWipePreserveAspectRatio::ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_CheckBox(x,y,plugin->preserve_aspect, _("Preserve shape aspect ratio"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipePreserveAspectRatio::handle_event()
{
	plugin->preserve_aspect = get_value();
	plugin->send_configure_change();
	return 1;
}

ShapeWipeFilename::ShapeWipeFilename(
	ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	char *value,
	int x,
	int y)
 : BC_TextBox(x, y, 180, 1, value)
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
	return 1;
}

ShapeWipeBrowseButton::ShapeWipeBrowseButton(
	ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	ShapeWipeFilename *filename,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Browse..."))
{
	this->plugin = plugin;
	this->window = window;
	this->filename = filename;
}

int ShapeWipeBrowseButton::handle_event()
{
	int result;
	ShapeWipeLoad window(filename, filename->get_text());
	window.update_filter("*.png");
	result = window.run_window();

	if(!result)
	{
		filename->update(window.get_submitted_path());
		strcpy(plugin->filename, window.get_submitted_path());
		plugin->send_configure_change();
		return 1;
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
	set_icon(new VFrame(picon_png));
	this->filename = filename;
}

ShapeWipeWindow::ShapeWipeWindow(ShapeWipeMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	450, 
	125)
{
	x = y = 10;

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
	this->plugin = plugin;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

PLUGIN_THREAD_METHODS

ShapeWipeMain::ShapeWipeMain(PluginServer *server)
 : PluginVClient(server)
{
	direction = 0;
	strcpy(filename, DEFAULT_SHAPE);
	last_read_filename[0] = '\0';
	pattern_image = NULL;
	min_value = 255;
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

void ShapeWipeMain::reset_plugin()
{
	reset_pattern_image();
}

PLUGIN_CLASS_METHODS

void ShapeWipeMain::load_defaults()
{
	defaults = load_defaults_file("shapewipe.rc");

	direction = defaults->get("DIRECTION", direction);
	antialias = defaults->get("ANTIALIAS", antialias);
	preserve_aspect = defaults->get("PRESERVE_ASPECT", preserve_aspect);
	defaults->get("FILENAME", filename);
}

void ShapeWipeMain::save_defaults()
{
	defaults->update("DIRECTION", direction);
	defaults->update("ANTIALIAS", antialias);
	defaults->update("PRESERVE_ASPECT", preserve_aspect);
	defaults->update("FILENAME", filename);
	defaults->save();
}

void ShapeWipeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SHAPEWIPE");
	output.tag.set_property("DIRECTION", direction);
	output.tag.set_property("ANTIALIAS", antialias);
	output.tag.set_property("PRESERVE_ASPECT", preserve_aspect);
	output.tag.set_property("FILENAME", filename);
	output.append_tag();
	output.tag.set_title("/SHAPEWIPE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ShapeWipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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

int ShapeWipeMain::load_configuration()
{
	read_data(get_prev_keyframe(source_pts));
	return 0;
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

	if(!fp)
		return 1;

	if(fread(header, 1, 8, fp) < 8)
		goto erret;

	is_png = !png_sig_cmp(header, 0, 8);

	if(!is_png)
		goto erret;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);

	if(!png_ptr)
		goto erret;

	// Tell libpng we already checked the first 8 bytes
	png_set_sig_bytes(png_ptr, 8);

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		goto erret;
	}

	end_info = png_create_info_struct(png_ptr);
	if(!end_info)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
erret:
		fclose(fp);
		return 1;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	width  = png_get_image_width (png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);

// Skip the alpha channel if present
// stripping alpha currently doesn't work in conjunction with
// converting to grayscale in libpng
	if(color_type & PNG_COLOR_MASK_ALPHA)
		pixel_width = 2;
	else
		pixel_width = 1;

	// Convert 16 bit data to 8 bit
	if(bit_depth == 16)
		png_set_strip_16(png_ptr);

	// Expand to 1 pixel per byte if necessary
	if(bit_depth < 8)
		png_set_packing(png_ptr);

	// Convert to grayscale
	if(color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);

	// Allocate memory to hold the original png image
	image = (png_bytep*)malloc(sizeof(png_bytep)*height);
	for(row = 0; row < height; row++)
	{
		image[row] = (png_byte*)malloc(sizeof(png_byte)*width*pixel_width);
	}

	// Allocate memory for the pattern image that will actually be
	//  used for the wipe
	pattern_image = (unsigned char**)malloc(sizeof(unsigned char*)*frame_height);

	png_read_image(png_ptr, image);
	png_read_end(png_ptr, end_info);

	double row_factor, col_factor;
	double row_offset = 0.5, col_offset = 0.5;         // for rounding
	double sample_aspect_ratio = get_sample_aspect_ratio();

	if(preserve_aspect && sample_aspect_ratio)
	{
		double aspect_ratio = sample_aspect_ratio * frame_width / frame_height;
		row_factor = (height - 1);
		col_factor = (width - 1) / aspect_ratio;
		if (row_factor < col_factor)
			col_factor = row_factor;
		else
			row_factor = col_factor;
		row_factor *= 1.0 / (double)(frame_height - 1);
		col_factor *= aspect_ratio / (double)(frame_width - 1);

		// center the pattern over the frame
		row_offset += (height - 1 - (frame_height - 1) * row_factor) / 2;
		col_offset += (width - 1 - (frame_width - 1) * col_factor) / 2;
	}
	else
	{
		// Stretch (or shrink) the pattern image to fill the frame
		row_factor = (double)(height - 1) / (double)(frame_height - 1);
		col_factor = (double)(width - 1) / (double)(frame_width - 1);
	}

	for(scaled_row = 0; scaled_row < frame_height; scaled_row++)
	{
		row = (int)(row_factor * scaled_row + row_offset);
		pattern_image[scaled_row] =
			(unsigned char*)malloc(frame_width);

		for(scaled_col = 0; scaled_col < frame_width; scaled_col++)
		{
			col = (int)(col_factor * scaled_col + col_offset) * pixel_width;
			value = image[row][col];
			pattern_image[scaled_row][scaled_col] = value;
			if(value < min_value)
				min_value = value;
			if(value > max_value)
				max_value = value;
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fp);
// Deallocate the original image as it is no longer needed
	for(row = 0; row < height; row++)
		free(image[row]);
	free(image);
	return 0;
}

void ShapeWipeMain::reset_pattern_image()
{
	int row;

	if(pattern_image)
	{
		for(row = 0; row < frame_height; row++)
		{
			free(pattern_image[row]);
		}
		free(pattern_image);
		pattern_image = 0;
		min_value = 255, max_value = 0;  // are recalc'd in read_pattern_image
	}
}

#define SHAPEWIPE(type, components) \
{ \
	for(j = 0; j < h; j++) \
	{ \
		type *in_row = (type*)incoming->get_row_ptr(j); \
		type *out_row = (type*)outgoing->get_row_ptr(j); \
		pattern_row = pattern_image[j]; \
 \
		col_offset = 0; \
		for(k = 0; k < w; k++) \
		{ \
			value = pattern_row[k]; \
			if((direction == 0 && value >= threshold) || \
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
	if(pattern_image[x][y] <= threshold) \
		opacity++; \
}

#define COMPARE2(x,y) \
{ \
	if(pattern_image[x][y] >= threshold) \
		opacity++; \
}

/* opacity is defined as opacity of incoming frame */
#define BLEND(x,y,total) \
{ \
	double alpha = (double)opacity / total;; \
	int col; \
	const int bits = sizeof(uint16_t) * 8; \
	uint64_t blend_opacity = (uint64_t)(alpha * ((uint64_t)1 << bits) + 0.5); \
	uint64_t blend_transparency = ((uint64_t)1 << bits) - blend_opacity; \
	uint16_t* in_row = (uint16_t*)incoming->get_row_ptr(x); \
	uint16_t* output = (uint16_t*)outgoing->get_row_ptr(x); \
 \
	if(alpha > 0.0) \
	{ \
		switch(cmodel) \
		{ \
		case BC_RGBA16161616: \
			col = y * 4; \
 \
			output[col] = ((uint64_t)in_row[col] * blend_opacity + \
				output[col] * blend_transparency) >> bits; \
			output[col + 1] = ((uint64_t)in_row[col + 1] * blend_opacity + \
				output[col+1] * blend_transparency) >> bits; \
			output[col + 2] = ((uint64_t)in_row[col + 2] * blend_opacity + \
				output[col+2] * blend_transparency) >> bits; \
			break; \
		case BC_AYUV16161616: \
			col = y * 4; \
 \
			output[col + 1] = ((uint64_t)in_row[col + 1] * blend_opacity + \
				output[col + 1] * blend_transparency) >> bits; \
			output[col + 2] = ((int64_t)in_row[col + 2] * blend_opacity + \
				output[col + 2] * blend_transparency) >> bits; \
			output[col + 3] = ((int64_t)in_row[col + 3] * blend_opacity + \
				output[col + 3] * blend_transparency) >> bits; \
			break; \
		} \
	} \
}

void ShapeWipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	unsigned char *pattern_row;
	int col_offset;
	unsigned char threshold;
	unsigned char value;
	int i, j, k;
	int opacity;
	int w = incoming->get_w();
	int h = incoming->get_h();
	int cmodel = incoming->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return;
	}

	load_configuration();

	if(get_length() < EPSILON)
		return;

	if(strncmp(filename, last_read_filename, BCTEXTLEN)
			|| preserve_aspect != last_preserve_aspect)
		reset_pattern_image();

	if(!pattern_image)
	{
		read_pattern_image(w, h);
		strncpy(last_read_filename, filename, BCTEXTLEN);
		last_preserve_aspect = preserve_aspect;
	}

	if(!pattern_image)
	{
		abort_plugin("Cannot load shape %s", filename);
		return;
	}

	if(direction)
	{
		threshold = (unsigned char)round(source_pts / get_length() *
			(max_value - min_value)) + min_value;
	}
	else
	{
		threshold = (unsigned char)((max_value - min_value) -
				round(source_pts / get_length() *
				(double)(max_value - min_value)))
			+ min_value;
	}

	if(antialias)
	{
		if(direction)
		{
			// Top left corner
			opacity = 0;
			COMPARE1(0, 0);
			COMPARE1(0, 1);
			COMPARE1(1, 0);
			COMPARE1(1, 1);
			BLEND(0, 0, 4.0);

			// Top edge
			for(k = 1; k < w - 1; k++)
			{
				opacity = 0;
				COMPARE1(0, k - 1);
				COMPARE1(0, k);
				COMPARE1(0, k + 1);
				COMPARE1(1, k - 1);
				COMPARE1(1,k);
				COMPARE1(1, k + 1);
				BLEND(0, k, 6.0);
			}

			// Top right corner
			opacity = 0;
			COMPARE1(0, w - 1);
			COMPARE1(0, w - 2);
			COMPARE1(1, w - 1);
			COMPARE1(1, w - 2);
			BLEND(0, w - 1, 4.0);

			// Left edge
			for(j = 1; j < h - 1; j++)
			{
				opacity = 0;
				COMPARE1(j - 1, 0);
				COMPARE1(j, 0);
				COMPARE1(j + 1, 0);
				COMPARE1(j - 1, 1);
				COMPARE1(j, 1);
				COMPARE1(j + 1, 1);
				BLEND(j, 0, 6.0);
			}

			// Middle
			for(j = 1; j < h-1; j++)
			{
				for(k = 1; k < w-1; k++)
				{
					opacity = 0;
					COMPARE1(j - 1,k - 1);
					COMPARE1(j, k -1);
					COMPARE1(j + 1,k - 1);
					COMPARE1(j - 1, k);
					COMPARE1(j, k);
					COMPARE1(j + 1, k);
					COMPARE1(j - 1, k + 1);
					COMPARE1(j, k + 1);
					COMPARE1(j + 1,k + 1);
					BLEND(j, k, 9.0);
				}
			}

			// Right edge
			for(j = 1; j < h - 1; j++)
			{
				opacity = 0;
				COMPARE1(j - 1, w - 1);
				COMPARE1(j, w - 1);
				COMPARE1(j + 1,w - 1);
				COMPARE1(j - 1,w - 2);
				COMPARE1(j, w - 2);
				COMPARE1(j + 1,w - 2);
				BLEND(j, w - 1, 6.0);
			}

			// Bottom left corner
			opacity = 0;
			COMPARE1(h - 1, 0);
			COMPARE1(h - 1, 1);
			COMPARE1(h - 2, 0);
			COMPARE1(h - 2, 1);
			BLEND(h - 1, 0, 4.0);

			// Bottom edge
			for(k = 1; k < w - 1; k++)
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

			// Bottom right corner
			opacity = 0;
			COMPARE1(h - 1, w - 1);
			COMPARE1(h - 1, w - 2);
			COMPARE1(h - 2, w - 1);
			COMPARE1(h - 2, w - 2);
			BLEND(h - 1, w - 1, 4.0);
		}
		else
		{
			// Top left corner */
			opacity = 0;
			COMPARE2(0, 0);
			COMPARE2(0, 1);
			COMPARE2(1, 0);
			COMPARE2(1, 1);
			BLEND(0, 0, 4.0);

			// Top edge
			for(k = 1; k < w - 1; k++)
			{
				opacity = 0;
				COMPARE2(0, k - 1);
				COMPARE2(0, k);
				COMPARE2(0, k + 1);
				COMPARE2(1, k - 1);
				COMPARE2(1, k);
				COMPARE2(1, k + 1);
				BLEND(0, k, 6.0);
			}

			// Top right corner
			opacity = 0;
			COMPARE2(0, w - 1);
			COMPARE2(0, w - 2);
			COMPARE2(1, w - 1);
			COMPARE2(1, w - 2);
			BLEND(0, w - 1, 4.0);

			// Left edge
			for(j = 1; j < h - 1; j++)
			{
				opacity = 0;
				COMPARE2(j - 1, 0);
				COMPARE2(j, 0);
				COMPARE2(j + 1, 0);
				COMPARE2(j - 1, 1);
				COMPARE2(j, 1);
				COMPARE2(j + 1, 1);
				BLEND(j, 0, 6.0);
			}

			// Middle
			for(j = 1; j < h-1; j++)
			{
				for(k = 1; k < w-1; k++)
				{
					opacity = 0;
					COMPARE2(j - 1, k - 1);
					COMPARE2(j, k - 1);
					COMPARE2(j + 1, k - 1);
					COMPARE2(j - 1, k);
					COMPARE2(j, k);
					COMPARE2(j + 1, k);
					COMPARE2(j - 1,k + 1);
					COMPARE2(j, k + 1);
					COMPARE2(j + 1, k + 1);
					BLEND(j, k, 9.0);
				}
			}

			// Right edge
			for(j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE2(j - 1,w - 1);
				COMPARE2(j, w - 1);
				COMPARE2(j + 1, w - 1);
				COMPARE2(j - 1,w - 2);
				COMPARE2(j, w - 2);
				COMPARE2(j + 1,w - 2);
				BLEND(j, w - 1, 6.0);
			}

			// Bottom left corner
			opacity = 0;
			COMPARE2(h - 1, 0);
			COMPARE2(h - 1, 1);
			COMPARE2(h - 2, 0);
			COMPARE2(h - 2, 1);
			BLEND(h - 1, 0, 4.0);

			// Bottom edge
			for(k = 1; k < w - 1; k++)
			{
				opacity = 0;
				COMPARE2(h - 1, k - 1);
				COMPARE2(h - 1, k);
				COMPARE2(h - 1, k + 1);
				COMPARE2(h - 2, k - 1);
				COMPARE2(h - 2, k);
				COMPARE2(h - 2, k + 1);
				BLEND(h - 1, k, 6.0);
			}

			// Bottom right corner
			opacity = 0;
			COMPARE2(h - 1, w - 1);
			COMPARE2(h - 1, w - 2);
			COMPARE2(h - 2, w - 1);
			COMPARE2(h - 2, w - 2);
			BLEND(h - 1, w - 1, 4.0);
		}
	}
	else
	{
		switch(cmodel)
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			for(j = 0; j < h; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);

				pattern_row = pattern_image[j];
				col_offset = 0;

				for(k = 0; k < w; k++)
				{
					value = pattern_row[k];
					if((direction == 0 && value >= threshold) ||
						(direction == 1 && value <= threshold))
					{
						out_row[col_offset] = in_row[col_offset];
						out_row[col_offset + 1] = in_row[col_offset + 1];
						out_row[col_offset + 2] = in_row[col_offset + 2];
						out_row[col_offset + 3] = in_row[col_offset + 3];
					}
					col_offset += 4;
				}
			}
			break;
		}
	}
}
