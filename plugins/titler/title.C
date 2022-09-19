// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

// Originally developed by Heroine Virtual Ltd.
// Support for multiple encodings, outline (stroke) by 
// Andraz Tori <Andraz.tori1@guest.arnes.si>


#include "clip.h"
#include "colormodels.inc"
#include "colorspaces.h"
#include "filexml.h"
#include "filesystem.h"
#include "ft2build.h"
#include FT_GLYPH_H
#include FT_BBOX_H
#include FT_OUTLINE_H
#include FT_STROKER_H

#include "mwindow.inc"
#include "mainerror.h"
#include "picon_png.h"
#include "title.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <sys/stat.h>

#define ZERO (1.0 / 64.0)

#define FONT_SEARCHPATH "fonts"
#define DEFAULT_ENCODING "UTF-8"
#define DEFAULT_TIMECODEFORMAT "h:mm:ss:ff"


REGISTER_PLUGIN


TitleConfig::TitleConfig()
{
	style = 0;
	color_red = 0;
	color_green = 0;
	color_blue = 0;
	color_stroke_red = 0xffff;
	color_stroke_green = 0;
	color_stroke_blue = 0;
	size = 24;
	motion_strategy = NO_MOTION;
	loop = 0;
	hjustification = JUSTIFY_CENTER;
	vjustification = JUSTIFY_MID;
	fade_in = 0.0;
	fade_out = 0.0;
	x = 0.0;
	y = 0.0;
	dropshadow = 10;
	strcpy(font, "fixed");
	strcpy(text, _("hello world"));
	strcpy(encoding, DEFAULT_ENCODING);
	strcpy(timecodeformat, DEFAULT_TIMECODEFORMAT);
	pixels_per_second = 1.0;
	timecode = 0;
	stroke_width = 1.0;
	wtext_length = 0;
	wtext[0] = 0;
}

// Does not test equivalency but determines if redrawing text is necessary.
int TitleConfig::equivalent(TitleConfig &that)
{
	return dropshadow == that.dropshadow &&
		style == that.style &&
		size == that.size &&
		color_red == that.color_red &&
		color_green == that.color_green &&
		color_blue == that.color_blue &&
		color_stroke_red == that.color_stroke_red &&
		color_stroke_green == that.color_stroke_green &&
		color_stroke_blue == that.color_stroke_blue &&
		stroke_width == that.stroke_width &&
		timecode == that.timecode && 
		!strcasecmp(timecodeformat, that.timecodeformat) &&
		hjustification == that.hjustification &&
		vjustification == that.vjustification &&
		EQUIV(pixels_per_second, that.pixels_per_second) &&
		!strcasecmp(font, that.font) &&
		!strcasecmp(encoding, that.encoding) &&
		wtext_length == that.wtext_length &&
		!memcmp(wtext, that.wtext, wtext_length * sizeof(wchar_t));
}

void TitleConfig::copy_from(TitleConfig &that)
{
	strcpy(font, that.font);
	style = that.style;
	size = that.size;
	color_red = that.color_red;
	color_green = that.color_green;
	color_blue = that.color_blue;
	color_stroke_red = that.color_stroke_red;
	color_stroke_green = that.color_stroke_green;
	color_stroke_blue = that.color_stroke_blue;
	stroke_width = that.stroke_width;
	pixels_per_second = that.pixels_per_second;
	motion_strategy = that.motion_strategy;
	loop = that.loop;
	hjustification = that.hjustification;
	vjustification = that.vjustification;
	fade_in = that.fade_in;
	fade_out = that.fade_out;
	x = that.x;
	y = that.y;
	dropshadow = that.dropshadow;
	timecode = that.timecode;
	strcpy(timecodeformat, that.timecodeformat);
	strcpy(encoding, that.encoding);
	memcpy(wtext, that.wtext, that.wtext_length * sizeof(wchar_t));
	wtext_length = that.wtext_length;
}

void TitleConfig::interpolate(TitleConfig &prev, 
	TitleConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	strcpy(font, prev.font);
	strcpy(encoding, prev.encoding);
	style = prev.style;
	size = prev.size;
	color_red = prev.color_red;
	color_green = prev.color_green;
	color_blue = prev.color_blue;
	color_stroke_red = prev.color_stroke_red;
	color_stroke_green = prev.color_stroke_green;
	color_stroke_blue = prev.color_stroke_blue;
	stroke_width = prev.stroke_width;
	motion_strategy = prev.motion_strategy;
	loop = prev.loop;
	hjustification = prev.hjustification;
	vjustification = prev.vjustification;
	fade_in = prev.fade_in;
	fade_out = prev.fade_out;
	pixels_per_second = prev.pixels_per_second;
	memcpy(wtext, prev.wtext, prev.wtext_length * sizeof(wchar_t));
	wtext_length = prev.wtext_length;
	this->x = prev.x;
	this->y = prev.y;
	timecode = prev.timecode;
	strcpy(timecodeformat, prev.timecodeformat);
	this->dropshadow = prev.dropshadow;
}

void TitleConfig::text_to_ucs4(const char *from_enc)
{
	wtext_length = BC_Resources::encode(from_enc, BC_Resources::wide_encoding,
		text, (char *)wtext, sizeof(wtext)) / sizeof(wchar_t);
}

int TitleConfig::color()
{
	return ((color_red << 8) & 0xff0000) | (color_green & 0xff00) | color_blue >> 8;
}

int TitleConfig::color_stroke()
{
	return ((color_stroke_red << 8) & 0xff0000) | (color_stroke_green & 0xff00) |
		color_stroke_blue >> 8;
}

TitleGlyph::TitleGlyph()
{
	char_code = 0;
	data = 0;
	data_stroke = 0;
	width = 0;
	height = 0;
	pitch = 0;
	advance_w = 0;
	left = 0;
	top = 0;
	freetype_index = 0;
}

TitleGlyph::~TitleGlyph()
{
	delete data;
	delete data_stroke;
}

void TitleGlyph::dump(int indent, int dumpdata)
{
	printf("%*sTitleGlyph %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*schar code %#lx\n", indent, "", char_code);
	printf("%*s(%d,%d) [%d,%d] pitch %d adwance_w %d idx %d\n", indent, "",
		left, top, width, height, pitch, advance_w, freetype_index);
	printf("%*sdata %p data_stroke %p\n", indent, "", data, data_stroke);
	if(dumpdata)
	{
		indent++;
		if(data)
			data->dump(indent, 1);
		if(data_stroke)
			data_stroke->dump(indent, 1);
	}
}

GlyphPackage::GlyphPackage() : LoadPackage()
{
}


GlyphUnit::GlyphUnit(TitleMain *plugin, GlyphEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	current_font = 0;
	freetype_library = 0;
	freetype_face = 0;
}

GlyphUnit::~GlyphUnit()
{
	if(freetype_library)
		FT_Done_FreeType(freetype_library);
}

void GlyphUnit::process_package(LoadPackage *package)
{
	GlyphPackage *pkg = (GlyphPackage*)package;
	TitleGlyph *glyph = pkg->glyph;
	int result = 0;
	int altfont = 0;
	char new_path[BCTEXTLEN];

	current_font = plugin->get_font();

	if(plugin->load_freetype_face(freetype_library,
			freetype_face, current_font->path))
		result = 1;

	if(!result)
	{
		int gindex = FT_Get_Char_Index(freetype_face, glyph->char_code);

// Char not found
		if(gindex == 0)
		{
			// Search replacement font
			if(plugin->find_font_by_char(glyph->char_code, new_path, freetype_face))
			{
				plugin->load_freetype_face(freetype_library,
					freetype_face, new_path);
				gindex = FT_Get_Char_Index(freetype_face, glyph->char_code);
			}
		}
		FT_Set_Pixel_Sizes(freetype_face, plugin->config.size, 0);
		if(gindex == 0)
		{
// carrige return
			if(glyph->char_code != '\n')
				errormsg(_("Titler can't find glyph for code: %li.\n"),
					glyph->char_code);
// Prevent a crash here
			glyph->width = 8;
			glyph->height = 8;
			glyph->pitch = 8;
			glyph->left = 9;
			glyph->top = 9;
			glyph->freetype_index = 0;
			glyph->advance_w = 8;
		}
		else
// char found and no outline desired
		if(plugin->config.stroke_width < ZERO ||
			!(plugin->config.style & FONT_OUTLINE))
		{
			FT_Glyph glyph_image;
			FT_BBox bbox;
			FT_Bitmap bm;
			FT_Load_Glyph(freetype_face, gindex, FT_LOAD_DEFAULT);
			FT_Get_Glyph(freetype_face->glyph, &glyph_image);
			FT_Outline_Get_BBox(&((FT_OutlineGlyph) glyph_image)->outline, &bbox);

			FT_Outline_Translate(&((FT_OutlineGlyph) glyph_image)->outline,
				- bbox.xMin,
				- bbox.yMin);
			glyph->width = bm.width = ((bbox.xMax - bbox.xMin + 63) >> 6);
			glyph->height = bm.rows = ((bbox.yMax - bbox.yMin + 63) >> 6);
			glyph->pitch = bm.pitch = bm.width;
			bm.pixel_mode = FT_PIXEL_MODE_GRAY;
			bm.num_grays = 256;
			glyph->left = (bbox.xMin + 31) >> 6;
			if(glyph->left < 0)
				glyph->left = 0;
			glyph->top = (bbox.yMax + 31) >> 6;
			glyph->freetype_index = gindex;
			glyph->advance_w = ((freetype_face->glyph->advance.x + 31) >> 6);
			glyph->data = new VFrame(0,
				glyph->width,
				glyph->height,
				BC_A8,
				glyph->pitch);
			glyph->data->clear_frame();
			bm.buffer = glyph->data->get_data();
			FT_Outline_Get_Bitmap(freetype_library,
				&((FT_OutlineGlyph)glyph_image)->outline,
				&bm);
			FT_Done_Glyph(glyph_image);
		}
		else 
// Outline desired and glyph found
		{
			FT_Glyph glyph_image;
			int no_outline = 0;
			FT_Stroker stroker;
			FT_Outline outline;
			FT_Bitmap bm;
			FT_BBox bbox;
			FT_UInt npoints, ncontours;

			FT_Load_Glyph(freetype_face, gindex, FT_LOAD_DEFAULT);
			FT_Get_Glyph(freetype_face->glyph, &glyph_image);

// check if the outline is ok (non-empty);
			FT_Outline_Get_BBox(&((FT_OutlineGlyph)glyph_image)->outline, &bbox);
			if(bbox.xMin == 0 && bbox.xMax == 0 && bbox.yMin ==0 && bbox.yMax == 0)
			{
				FT_Done_Glyph(glyph_image);
				glyph->advance_w = ((int)(freetype_face->glyph->advance.x +
					plugin->config.stroke_width * 64)) >> 6;
				return;
			}
			FT_Stroker_New(freetype_library, &stroker);
			FT_Stroker_Set(stroker, (int)(plugin->config.stroke_width * 64), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
			FT_Stroker_ParseOutline(stroker, &((FT_OutlineGlyph)glyph_image)->outline, 1);
			FT_Stroker_GetCounts(stroker, &npoints, &ncontours);
			if(npoints == 0 && ncontours == 0)
			{
// this never happens, but FreeType has a bug regarding Linotype's Palatino font
				FT_Stroker_Done(stroker);
				FT_Done_Glyph(glyph_image);
				glyph->advance_w = ((int)(freetype_face->glyph->advance.x +
					plugin->config.stroke_width * 64)) >> 6;
				return;
			};

			FT_Outline_New(freetype_library, npoints, ncontours, &outline);
			outline.n_points = 0;
			outline.n_contours = 0;
			FT_Stroker_Export(stroker, &outline);
			FT_Outline_Get_BBox(&outline, &bbox);
			FT_Outline_Translate(&outline,
					- bbox.xMin,
					- bbox.yMin);
			FT_Outline_Translate(&((FT_OutlineGlyph)glyph_image)->outline,
					- bbox.xMin,
					- bbox.yMin + (int)(plugin->config.stroke_width * 32));
			glyph->width = bm.width = ((bbox.xMax - bbox.xMin) >> 6)+1;
			glyph->height = bm.rows = ((bbox.yMax - bbox.yMin) >> 6) +1;
			glyph->pitch = bm.pitch = bm.width;
			bm.pixel_mode = FT_PIXEL_MODE_GRAY;
			bm.num_grays = 256;
			glyph->left = (bbox.xMin + 31) >> 6;
			if(glyph->left < 0)
				glyph->left = 0;
			glyph->top = (bbox.yMax + 31) >> 6;
			glyph->freetype_index = gindex;
			int real_advance = ((int)ceil((double)freetype_face->glyph->advance.x +
				plugin->config.stroke_width * 64) >> 6);
			glyph->advance_w = glyph->width + glyph->left;
			if(real_advance > glyph->advance_w)
				glyph->advance_w = real_advance;
			glyph->data = new VFrame(0,
				glyph->width,
				glyph->height,
				BC_A8,
				glyph->pitch);
			glyph->data_stroke = new VFrame(0,
				glyph->width,
				glyph->height,
				BC_A8,
				glyph->pitch);
			glyph->data->clear_frame();
			glyph->data_stroke->clear_frame();
			bm.buffer = glyph->data->get_data();
			FT_Outline_Get_Bitmap(freetype_library,
				&((FT_OutlineGlyph)glyph_image)->outline,
				&bm);
			bm.buffer = glyph->data_stroke->get_data();
			FT_Outline_Get_Bitmap(freetype_library,
				&outline, &bm);
			FT_Outline_Done(freetype_library, &outline);
			FT_Stroker_Done(stroker);
			FT_Done_Glyph(glyph_image);
		}
	}
}

GlyphEngine::GlyphEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void GlyphEngine::init_packages()
{
	int current_package = 0;

	for(int i = 0; i < plugin->glyphs.total; i++)
	{
		if(!plugin->glyphs.values[i]->data)
		{
			GlyphPackage *pkg = (GlyphPackage*)get_package(current_package++);
			pkg->glyph = plugin->glyphs.values[i];
		}
	}
}

LoadClient* GlyphEngine::new_client()
{
	return new GlyphUnit(plugin, this);
}

LoadPackage* GlyphEngine::new_package()
{
	return new GlyphPackage;
}


// Copy a single character to the text mask
TitlePackage::TitlePackage()
 : LoadPackage()
{
}


TitleUnit::TitleUnit(TitleMain *plugin, TitleEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

void TitleUnit::draw_glyph(VFrame *output, TitleGlyph *glyph, int x, int y)
{
	int glyph_w = glyph->data->get_w();
	int glyph_h = glyph->data->get_h();
	int output_w = output->get_w();
	int output_h = output->get_h();

	for(int in_y = 0; in_y < glyph_h; in_y++)
	{
		int y_out = y + plugin->get_char_height() + in_y - glyph->top;

		if(y_out >= 0 && y_out < output_h)
		{
			unsigned char *out_row = output->get_row_ptr(y_out);
			unsigned char *in_row = glyph->data->get_row_ptr(in_y);
			for(int in_x = 0; in_x < glyph_w; in_x++)
			{
				int x_out = x + glyph->left + in_x;
				if(x_out >= 0 && x_out < output_w)
				{
					if(in_row[in_x] > 0)
						out_row[x_out] = in_row[in_x];
				}
			}
		}
	}
}

void TitleUnit::process_package(LoadPackage *package)
{
	TitlePackage *pkg = (TitlePackage*)package;

	if(pkg->char_code != '\n')
	{
		for(int i = 0; i < plugin->glyphs.total; i++)
		{
			TitleGlyph *glyph = plugin->glyphs.values[i];

			if(glyph->char_code == pkg->char_code)
			{
				if(glyph->data)
					draw_glyph(plugin->text_mask, glyph, pkg->x, pkg->y);

				if(plugin->config.stroke_width >= ZERO &&
					(plugin->config.style & FONT_OUTLINE) &&
					glyph->data_stroke)
				{
					VFrame *tmp = glyph->data;
					glyph->data = glyph->data_stroke;
					draw_glyph(plugin->text_mask_stroke, glyph, pkg->x, pkg->y);
					glyph->data = tmp;
				}
				break;
			}
		}
	}
}


TitleEngine::TitleEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void TitleEngine::init_packages()
{
	int visible_y1 = plugin->visible_row1 * plugin->get_char_height();
	int current_package = 0;

	for(int i = plugin->visible_char1; i < plugin->visible_char2; i++)
	{
		title_char_position_t *char_position = plugin->char_positions + i;
		TitlePackage *pkg = (TitlePackage*)get_package(current_package);
		pkg->x = char_position->x;
		pkg->y = char_position->y - visible_y1;
		pkg->char_code = plugin->config.wtext[i];
		current_package++;
	}
}

LoadClient* TitleEngine::new_client()
{
	return new TitleUnit(plugin, this);
}

LoadPackage* TitleEngine::new_package()
{
	return new TitlePackage;
}


TitleTranslatePackage::TitleTranslatePackage()
 : LoadPackage()
{
}


TitleTranslateUnit::TitleTranslateUnit(TitleMain *plugin, TitleTranslate *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

void TitleTranslateUnit::translation_array_f(transfer_table_f* &table,
	double out_x1,
	double out_x2,
	double in_x1,
	double in_x2,
	int in_total,
	int out_total,
	int &out_x1_int,
	int &out_x2_int)
{
	int out_w_int;
	double offset = out_x1 - in_x1;

	out_x1_int = round(out_x1);
	out_x2_int = MIN((int)ceil(out_x2), out_total);
	out_w_int = out_x2_int - out_x1_int;
	table = new transfer_table_f[out_w_int];
	memset(table, 0, sizeof(transfer_table_f) * out_w_int);

	double in_x = in_x1;

	for(int out_x = out_x1_int; out_x < out_x2_int; out_x++)
	{
		transfer_table_f *entry = &table[out_x - out_x1_int];

		entry->in_x1 = (int)in_x;
		entry->in_x2 = (int)in_x + 1;

// Get fraction of output pixel to fill
		entry->output_fraction = 1;

		if(out_x1 > out_x)
			entry->output_fraction -= out_x1 - out_x;

		if(out_x2 < out_x + 1)
			entry->output_fraction = (out_x2 - out_x);

// Advance in_x until out_x_fraction is filled
		double out_x_fraction = entry->output_fraction;
		double in_x_fraction = floor(in_x + 1) - in_x;

		if(out_x_fraction <= in_x_fraction)
		{
			entry->in_fraction1 = out_x_fraction;
			entry->in_fraction2 = 0.0;
			in_x += out_x_fraction;
		}
		else
		{
			entry->in_fraction1 = in_x_fraction;
			in_x += out_x_fraction;
			entry->in_fraction2 = in_x - floor(in_x);
		}

// Clip in_x and zero out fraction.  This doesn't work for YUV.
		if(entry->in_x2 >= in_total)
		{
			entry->in_x2 = in_total - 1;
			entry->in_fraction2 = 0.0;
		}

		if(entry->in_x1 >= in_total)
		{
			entry->in_x1 = in_total - 1;
			entry->in_fraction1 = 0.0;
		}
	}
}

void TitleTranslateUnit::process_package(LoadPackage *package)
{
	TitleTranslatePackage *pkg = (TitleTranslatePackage*)package;
	TitleTranslate *server = (TitleTranslate*)this->server;
	int r_in, g_in, b_in;
	int y, u, v;

	r_in = plugin->config.color_red;
	g_in = plugin->config.color_green;
	b_in = plugin->config.color_blue;

	switch(plugin->output->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			if(i + server->out_y1_int >= 0 &&
				i + server->out_y1_int < server->output_h)
			{
				int in_y1, in_y2;
				double y_fraction1, y_fraction2;
				double y_output_fraction;

				in_y1 = server->y_table[i].in_x1;
				in_y2 = server->y_table[i].in_x2;
				y_fraction1 = server->y_table[i].in_fraction1;
				y_fraction2 = server->y_table[i].in_fraction2;
				y_output_fraction = server->y_table[i].output_fraction;
				unsigned char *in_row1 = plugin->text_mask->get_row_ptr(in_y1);
				unsigned char *in_row2 = plugin->text_mask->get_row_ptr(in_y2);
				uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(
					i + server->out_y1_int);

				for(int j = server->out_x1_int; j < server->out_x2_int; j++)
				{
					if(j >= 0 && j < server->output_w)
					{
						int in_x1;
						int in_x2;
						double x_fraction1;
						double x_fraction2;
						double x_output_fraction;
						in_x1 =
							server->x_table[j - server->out_x1_int].in_x1;
						in_x2 =
							server->x_table[j - server->out_x1_int].in_x2;
						x_fraction1 =
							server->x_table[j - server->out_x1_int].in_fraction1;
						x_fraction2 =
							server->x_table[j - server->out_x1_int].in_fraction2;
						x_output_fraction =
							server->x_table[j - server->out_x1_int].output_fraction;

						double fraction1 = x_fraction1 * y_fraction1;
						double fraction2 = x_fraction2 * y_fraction1;
						double fraction3 = x_fraction1 * y_fraction2;
						double fraction4 = x_fraction2 * y_fraction2;
						int input = in_row1[in_x1] * fraction1 +
							in_row1[in_x2] * fraction2 +
							in_row2[in_x1] * fraction3 +
							in_row2[in_x2] * fraction4 + 0.5;
						input *= plugin->alpha;
						input = input >> 8 | input >> 16;
						int anti_input = 0xffff - input;

						out_row[j * 4] =
							(r_in * input + out_row[j * 4] *
							anti_input) >> 16;
						out_row[j * 4 + 1] =
							(g_in * input + out_row[j * 4 + 1] *
							anti_input) >> 16;
						out_row[j * 4 + 2] =
							(b_in * input + out_row[j * 4 + 2] *
							anti_input) >> 16;
						out_row[j * 4 + 3] =
							MAX(input, out_row[j * 4 + 3]);
					}
				}
			}
		}
		break;
	case BC_AYUV16161616:
		{
			ColorSpaces::rgb_to_yuv_16(r_in, g_in, b_in, y, u, v);

			for(int i = pkg->y1; i < pkg->y2; i++)
			{
				if(i + server->out_y1_int >= 0 &&
					i + server->out_y1_int < server->output_h)
				{
					int in_y1, in_y2;
					double y_fraction1, y_fraction2;
					double y_output_fraction;

					in_y1 = server->y_table[i].in_x1;
					in_y2 = server->y_table[i].in_x2;
					y_fraction1 = server->y_table[i].in_fraction1;
					y_fraction2 = server->y_table[i].in_fraction2;
					y_output_fraction = server->y_table[i].output_fraction;
					unsigned char *in_row1 = plugin->text_mask->get_row_ptr(in_y1);
					unsigned char *in_row2 = plugin->text_mask->get_row_ptr(in_y2);
					uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(
						i + server->out_y1_int);

					for(int j = server->out_x1_int; j < server->out_x2_int; j++)
					{
						if(j >= 0 && j < server->output_w)
						{
							int in_x1, in_x2;
							double x_fraction1, x_fraction2;

							in_x1 = server->x_table[j - server->out_x1_int].in_x1;
							in_x2 = server->x_table[j - server->out_x1_int].in_x2;
							x_fraction1 = server->x_table[j - server->out_x1_int].in_fraction1;
							x_fraction2 = server->x_table[j - server->out_x1_int].in_fraction2;

							double fraction1 = x_fraction1 * y_fraction1;
							double fraction2 = x_fraction2 * y_fraction1;
							double fraction3 = x_fraction1 * y_fraction2;
							double fraction4 = x_fraction2 * y_fraction2;
							int input = (in_row1[in_x1] * fraction1 +
								in_row1[in_x2] * fraction2 +
								in_row2[in_x1] * fraction3 +
								in_row2[in_x2] * fraction4 + 0.5);

							input *= plugin->alpha;
							input = input >> 8 | input >> 16;
							int anti_input = 0xffff - input;

							out_row[j * 4] =
								MAX(input, out_row[j * 4]);
							out_row[j * 4 + 1] =
								(y * input + out_row[j * 4 + 1] * anti_input) >> 16;
							out_row[j * 4 + 2] =
								(u * input + out_row[j * 4 + 2] * anti_input) >> 16;
							out_row[j * 4 + 3] =
								(v * input + out_row[j * 4 + 3] * anti_input) >> 16;
						}
					}
				}
			}
		}
		break;
	}
}

TitleTranslate::TitleTranslate(TitleMain *plugin, int cpus)
 : LoadServer(1, 1)
{
	this->plugin = plugin;
	x_table = y_table = 0;
}

TitleTranslate::~TitleTranslate()
{
	if(x_table) delete [] x_table;
	if(y_table) delete [] y_table;
}

void TitleTranslate::init_packages()
{
// Generate scaling tables
	if(x_table) delete [] x_table;
	if(y_table) delete [] y_table;

	output_w = plugin->output->get_w();
	output_h = plugin->output->get_h();

	TitleTranslateUnit::translation_array_f(x_table,
		plugin->text_x1, 
		plugin->text_x1 + plugin->text_w,
		0,
		plugin->text_w,
		plugin->text_w, 
		output_w, 
		out_x1_int,
		out_x2_int);

	TitleTranslateUnit::translation_array_f(y_table,
		plugin->mask_y1, 
		plugin->mask_y1 + plugin->text_mask->get_h(),
		0,
		plugin->text_mask->get_h(),
		plugin->text_mask->get_h(), 
		output_h, 
		out_y1_int,
		out_y2_int);

	out_y1 = out_y1_int;
	out_y2 = out_y2_int;
	out_x1 = out_x1_int;
	out_x2 = out_x2_int;
	int increment = (out_y2 - out_y1) / get_total_packages() + 1;

	for(int i = 0; i < get_total_packages(); i++)
	{
		TitleTranslatePackage *pkg = (TitleTranslatePackage*)get_package(i);

		pkg->y1 = i * increment;
		pkg->y2 = i * increment + increment;
		if(pkg->y1 > out_y2 - out_y1)
			pkg->y1 = out_y2 - out_y1;
		if(pkg->y2 > out_y2 - out_y1)
			pkg->y2 = out_y2 - out_y1;
	}
}

LoadClient* TitleTranslate::new_client()
{
	return new TitleTranslateUnit(plugin, this);
}

LoadPackage* TitleTranslate::new_package()
{
	return new TitleTranslatePackage;
}


TitleMain::TitleMain(PluginServer *server)
 : PluginVClient(server)
{
	text_mask = 0;
	text_mask_stroke = 0;
	glyph_engine = 0;
	title_engine = 0;
	freetype_library = 0;
	freetype_face = 0;
	char_positions = 0;
	rows_bottom = 0;
	translate = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

TitleMain::~TitleMain()
{
	delete text_mask;
	delete text_mask_stroke;
	delete [] char_positions;
	delete [] rows_bottom;
	clear_glyphs();
	delete glyph_engine;
	delete title_engine;
	if(freetype_face) FT_Done_Face(freetype_face);
	if(freetype_library) FT_Done_FreeType(freetype_library);
	delete translate;
	PLUGIN_DESTRUCTOR_MACRO
}

void TitleMain::reset_plugin()
{
	if(text_mask)
	{
		delete text_mask;
		delete text_mask_stroke;
		text_mask = 0;
		text_mask_stroke = 0;
	}
	if(char_positions)
	{
		delete [] char_positions;
		char_positions = 0;
	}
	if(rows_bottom)
	{
		delete [] rows_bottom;
		rows_bottom = 0;
	}
	clear_glyphs();
	if(glyph_engine)
	{
		delete glyph_engine;
		glyph_engine = 0;
	}
	if(title_engine)
	{
		delete title_engine;
		title_engine = 0;
	}
	if(freetype_face)
	{
		FT_Done_Face(freetype_face);
		freetype_face = 0;
	}
	if(freetype_library)
	{
		FT_Done_FreeType(freetype_library);
		freetype_library = 0;
	}
	if(translate)
	{
		delete translate;
		translate = 0;
	}
}

PLUGIN_CLASS_METHODS

int TitleMain::load_freetype_face(FT_Library &freetype_library,
	FT_Face &freetype_face,
	const char *path)
{
	if(!freetype_library) FT_Init_FreeType(&freetype_library);
	if(freetype_face) FT_Done_Face(freetype_face);
	freetype_face = 0;

// Use freetype's internal function for loading font
	if(FT_New_Face(freetype_library, 
		path, 
		0,
		&freetype_face))
	{
		errormsg(_("TitleMain::load_freetype_face %s failed."), path);
		FT_Done_FreeType(freetype_library);
		freetype_face = 0;
		freetype_library = 0;
		return 1;
	}
	return 0;
}

BC_FontEntry* TitleMain::get_font()
{
	int style = 0;
	int mask, pref;

	style |= (config.style & FONT_ITALIC) ?
		FL_SLANT_ITALIC | FL_SLANT_OBLIQUE : FL_SLANT_ROMAN;
	style |= (config.style & FONT_BOLD) ?
		FL_WEIGHT_BOLD | FL_WEIGHT_DEMIBOLD |
		FL_WEIGHT_EXTRABOLD| FL_WEIGHT_BLACK | FL_WEIGHT_EXTRABLACK :
		FL_WEIGHT_BOOK | FL_WEIGHT_NORMAL | FL_WEIGHT_MEDIUM |
		FL_WEIGHT_LIGHT | FL_WEIGHT_EXTRALIGHT | FL_WEIGHT_THIN;

	pref = style & (FL_SLANT_ITALIC | FL_WEIGHT_BOLD | FL_WEIGHT_NORMAL);
	mask = FL_WEIGHT_MASK | FL_SLANT_MASK;

	return find_fontentry(config.font, style, mask, pref);
}


int TitleMain::get_char_height()
{
// this is height above the zero line, but does not include characters that go below
	int result = config.size;

	if((config.style & FONT_OUTLINE))
		result += (int)ceil(config.stroke_width * 2);
	return result;
}

int TitleMain::get_char_advance(FT_ULong current, FT_ULong next)
{
	FT_Vector kerning;
	int result = 0;
	TitleGlyph *current_glyph = 0;
	TitleGlyph *next_glyph = 0;

	if(current == '\n')
		return 0;

	for(int i = 0; i < glyphs.total; i++)
	{
		if(glyphs.values[i]->char_code == current)
		{
			current_glyph = glyphs.values[i];
			break;
		}
	}

	for(int i = 0; i < glyphs.total; i++)
	{
		if(glyphs.values[i]->char_code == next)
		{
			next_glyph = glyphs.values[i];
			break;
		}
	}

	if(current_glyph)
		result = current_glyph->advance_w;

	if(next_glyph)
		FT_Get_Kerning(freetype_face, 
				current_glyph->freetype_index,
				next_glyph->freetype_index,
				ft_kerning_default,
				&kerning);
	else
		kerning.x = 0;

	return result + (kerning.x >> 6);
}


void TitleMain::draw_glyphs()
{
// Build table of all glyphs needed
	int total_packages = 0;

	for(int i = 0; i < config.wtext_length; i++)
	{
		wchar_t char_code;
		int exists = 0;
		char_code = config.wtext[i];

		for(int j = 0; j < glyphs.total; j++)
		{
			if(glyphs.values[j]->char_code == char_code)
			{
				exists = 1;
				break;
			}
		}

		if(!exists)
		{
			total_packages++;
			TitleGlyph *glyph = new TitleGlyph;
			glyphs.append(glyph);
			glyph->char_code = char_code;
		}
	}

	if(!glyph_engine)
		glyph_engine = new GlyphEngine(this, get_project_smp());

	glyph_engine->set_package_count(total_packages);
	glyph_engine->process_packages();
}

void TitleMain::get_total_extents()
{
// Determine extents of total text
	int current_w = 0;
	int row_start = 0;
	text_len = config.wtext_length;

	if(!char_positions)
		char_positions = new title_char_position_t[text_len];

	text_rows = 0;
	text_w = 0;
	ascent = 0;

	for(int i = 0; i < glyphs.total; i++)
	{
		if(glyphs.values[i]->top > ascent)
			ascent = glyphs.values[i]->top;
	}

	// get the number of rows first
	for(int i = 0; i < text_len; i++)
	{
		if(config.wtext[i] == '\n' || i == text_len - 1)
			text_rows++;
	}

	if(!rows_bottom)
		rows_bottom = new int[text_rows + 1];

	text_rows = 0;
	rows_bottom[0] = 0;

	for(int i = 0; i < text_len; i++)
	{
		char_positions[i].x = current_w;
		char_positions[i].y = text_rows * get_char_height();
		char_positions[i].w = get_char_advance(config.wtext[i], config.wtext[i + 1]);
		TitleGlyph *current_glyph = 0;

		for(int j = 0; j < glyphs.total; j++)
		{
			if(glyphs.values[j]->char_code == config.wtext[i])
			{
				current_glyph = glyphs.values[j];
				break;
			}
		}

		int current_bottom = current_glyph->top - current_glyph->height;

		if (current_bottom < rows_bottom[text_rows])
			rows_bottom[text_rows] = current_bottom;

		current_w += char_positions[i].w;

		if(config.wtext[i] == '\n' || i == text_len - 1)
		{
			text_rows++;
			rows_bottom[text_rows] = 0;
			if(current_w > text_w)
				text_w = current_w;
			current_w = 0;
		}
	}
	text_w += config.dropshadow;
	text_h = text_rows * get_char_height();
	text_h += config.dropshadow;

// Now that text_w is known
// Justify rows based on configuration
	row_start = 0;
	for(int i = 0; i < text_len; i++)
	{
		if(config.wtext[i] == '\n' || i == text_len - 1)
		{
			for(int j = row_start; j <= i; j++)
			{
				switch(config.hjustification)
				{
				case JUSTIFY_LEFT:
					break;

				case JUSTIFY_MID:
					char_positions[j].x += (text_w -
							char_positions[i].x -
							char_positions[i].w) / 2;
					break;

				case JUSTIFY_RIGHT:
					char_positions[j].x += (text_w - 
						char_positions[i].x - 
						char_positions[i].w);
					break;
				}
			}
			row_start = i + 1;
		}
	}
}

int TitleMain::draw_mask()
{
	int old_visible_row1 = visible_row1;
	int old_visible_row2 = visible_row2;

// Determine y of visible text
	if(config.motion_strategy == BOTTOM_TO_TOP)
	{
		double magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_h + output->get_h();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y1 = config.y + output->get_h() - magnitude;
	}
	else
	if(config.motion_strategy == TOP_TO_BOTTOM)
	{
		double magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_h + output->get_h();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y1 = config.y + magnitude;
		text_y1 -= text_h;
	}
	else
	if(config.vjustification == JUSTIFY_TOP)
		text_y1 = config.y;
	else
	if(config.vjustification == JUSTIFY_MID)
		text_y1 = config.y + output->get_h() / 2 - text_h / 2;
	else
	if(config.vjustification == JUSTIFY_BOTTOM)
		text_y1 = config.y + output->get_h() - text_h;

	text_y2 = text_y1 + text_h + 0.5;

// Determine x of visible text
	if(config.motion_strategy == RIGHT_TO_LEFT)
	{
		double magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_w + output->get_w();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x1 = config.x + (double)output->get_w() - magnitude;
	}
	else
	if(config.motion_strategy == LEFT_TO_RIGHT)
	{
		double magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_w + output->get_w();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x1 = config.x + -(double)text_w + magnitude;
	}
	else
	if(config.hjustification == JUSTIFY_LEFT)
		text_x1 = config.x;
	else
	if(config.hjustification == JUSTIFY_MID)
		text_x1 = config.x + output->get_w() / 2 - text_w / 2;
	else
	if(config.hjustification == JUSTIFY_RIGHT)
		text_x1 = config.x + output->get_w() - text_w;

// Determine y extents just of visible text
	visible_row1 = (int)(-text_y1 / get_char_height());

	if(visible_row1 < 0)
		visible_row1 = 0;

	visible_row2 = (int)((double)text_rows - (text_y2 - output->get_h()) /
		get_char_height() + 1);

	if(visible_row2 > text_rows)
		visible_row2 = text_rows;

	if(visible_row2 <= visible_row1)
		return 1;

	mask_y1 = text_y1 + visible_row1 * get_char_height();
	mask_y2 = text_y1 + visible_row2 * get_char_height();
	text_x1 += config.x;

	visible_char1 = visible_char2 = 0;
	int got_char1 = 0;

	for(int i = 0; i < text_len; i++)
	{
		title_char_position_t *char_position = char_positions + i;
		int char_row = char_position->y / get_char_height();

		if(char_row >= visible_row1 &&
			char_row < visible_row2)
		{
			if(!got_char1)
			{
				visible_char1 = i;
				got_char1 = 1;
			}
			visible_char2 = i;
		}
	}
	visible_char2++;

	int visible_rows = visible_row2 - visible_row1;
	int need_redraw = 0;

	if(text_mask &&
		(text_mask->get_w() != text_w ||
		text_mask->get_h() != visible_rows * get_char_height() -
			rows_bottom[visible_row2 - 1]))
	{
		delete text_mask;
		delete text_mask_stroke;
		text_mask = 0;
		text_mask_stroke = 0;
	}

	if(!text_mask)
	{
		text_mask = new VFrame(0,
			text_w,
			visible_rows * get_char_height() - rows_bottom[visible_row2 - 1],
			BC_A8);
		text_mask_stroke = new VFrame(0,
			text_w,
			visible_rows * get_char_height() - rows_bottom[visible_row2 - 1],
			BC_A8);
		need_redraw = 1;
	}

// Draw on text mask if different
	if(old_visible_row1 != visible_row1 ||
		old_visible_row2 != visible_row2 ||
		need_redraw)
	{
		text_mask->clear_frame();
		text_mask_stroke->clear_frame();

		if(!title_engine)
			title_engine = new TitleEngine(this, get_project_smp());

		title_engine->set_package_count(visible_char2 - visible_char1);
		title_engine->process_packages();
	}
	return 0;
}

void TitleMain::overlay_mask()
{
	alpha = 0x10000;

	if(!EQUIV(config.fade_in, 0))
	{
		ptstime fade_pts = source_pts - config.prev_border_pts;

		if(fade_pts >= 0 && fade_pts < config.fade_in)
			alpha = round(0x10000 * fade_pts / config.fade_in);
	}

	if(!EQUIV(config.fade_out, 0))
	{
		ptstime fade_pts = config.next_border_pts - source_pts;

		if(fade_pts >= 0 && fade_pts < config.fade_out)
		{
			alpha = round(0x10000 * fade_pts / config.fade_out);
		}
	}

	if(config.dropshadow)
	{
		text_x1 += config.dropshadow;
		text_x2 += config.dropshadow;
		mask_y1 += config.dropshadow;
		mask_y2 += config.dropshadow;
		if(text_x1 < output->get_w() && text_x1 + text_w > 0 &&
			mask_y1 < output->get_h() && mask_y2 > 0)
		{
			if(!translate)
				translate = new TitleTranslate(this, get_project_smp());
// Do 2 passes if dropshadow.
			int temp_color_r = config.color_red;
			int temp_color_g = config.color_green;
			int temp_color_b = config.color_blue;
			config.color_red = 0;
			config.color_green = 0;
			config.color_blue = 0;
			translate->process_packages();
			config.color_red = temp_color_r;
			config.color_green = temp_color_g;
			config.color_blue = temp_color_b;
		}
		text_x1 -= config.dropshadow;
		text_x2 -= config.dropshadow;
		mask_y1 -= config.dropshadow;
		mask_y2 -= config.dropshadow;
	}

	if(text_x1 < output->get_w() && text_x1 + text_w > 0 &&
		mask_y1 < output->get_h() && mask_y2 > 0)
	{
		if(!translate)
			translate = new TitleTranslate(this, get_project_smp());
		translate->process_packages();

		if(config.stroke_width >= ZERO &&
			(config.style & FONT_OUTLINE))
		{
			int temp_color_r = config.color_red;
			int temp_color_g = config.color_green;
			int temp_color_b = config.color_blue;
			VFrame *tmp_text_mask = this->text_mask;

			config.color_red = config.color_stroke_red;
			config.color_green = config.color_stroke_green;
			config.color_blue = config.color_stroke_blue;
			this->text_mask = this->text_mask_stroke;

			translate->process_packages();
			config.color_red = temp_color_r;
			config.color_green = temp_color_g;
			config.color_blue = temp_color_b;
			this->text_mask = tmp_text_mask;
		}
	}
}

void TitleMain::clear_glyphs()
{
	glyphs.remove_all_objects();
}

const char *TitleMain::motion_to_text(int motion)
{
	switch(motion)
	{
	case NO_MOTION:
		return _("No motion");
	case BOTTOM_TO_TOP:
		return _("Bottom to top");
	case TOP_TO_BOTTOM:
		return _("Top to bottom");
	case RIGHT_TO_LEFT:
		return _("Right to left");
	case LEFT_TO_RIGHT:
		return _("Left to right");
	}
	return "Unknown";
}

int TitleMain::text_to_motion(const char *text)
{
	for(int i = 0; i < TOTAL_PATHS; i++)
	{
		if(!strcasecmp(motion_to_text(i), text))
			return i;
	}
	return 0;
}

VFrame *TitleMain::process_tmpframe(VFrame *input_ptr)
{
	int result = 0;
	int do_reconfigure = 0;
	output = input_ptr;
	int color_model = input_ptr->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input_ptr;
	}

	if(load_configuration())
	{
		update_gui();
		do_reconfigure = 1;
	}

// Always synthesize text and redraw it for timecode
	if(config.timecode)
	{
		int tcf = Units::timeformat_totype(config.timecodeformat);

		if(tcf < 0)
		{
			tcf = TIME_HMSF;
			strcpy(config.timecodeformat, DEFAULT_TIMECODEFORMAT);
		}
		Units::totext(config.text, 
				output->get_pts(),
				tcf, 
				0,
				get_project_framerate());
		config.text_to_ucs4(DEFAULT_ENCODING);
		do_reconfigure = 1;
	}

// Check boundaries
	if(config.size <= 0 || config.size >= 2048)
		config.size = 72;
	if(config.stroke_width < 0 || config.stroke_width >= 512)
		config.stroke_width = 0.0;
	if(!config.wtext_length)
		return output;
	if(!config.encoding)
		strcpy(config.encoding, DEFAULT_ENCODING);

// Handle reconfiguration
	if(do_reconfigure)
	{
		if(text_mask)
			delete text_mask;
		if(text_mask_stroke)
			delete text_mask_stroke;
		text_mask = 0;
		text_mask_stroke = 0;
		if(freetype_face)
			FT_Done_Face(freetype_face);
		freetype_face = 0;
		if(glyph_engine)
			delete glyph_engine;
		glyph_engine = 0;
		if(char_positions)
			delete [] char_positions;
		char_positions = 0;
		if(rows_bottom)
			delete [] rows_bottom;
		rows_bottom = 0;
		clear_glyphs();
		visible_row1 = 0;
		visible_row2 = 0;
		ascent = 0;

		if(!freetype_library)
			FT_Init_FreeType(&freetype_library);

		if(!freetype_face)
		{
			BC_FontEntry *font = get_font();
			if(load_freetype_face(freetype_library,
				freetype_face,
				font->path))
			{
				errorbox("TitleMain::process_realtime %s: FT_New_Face failed.",
					font->displayname);
				result = 1;
			}

			if(!result)
				FT_Set_Pixel_Sizes(freetype_face, config.size, 0);
		}

		if(!result)
		{
			draw_glyphs();
			get_total_extents();
		}
	}

	if(!result)
	{
// Determine region of text visible on the output and draw mask
		result = draw_mask();
	}

// Overlay mask on output
	if(!result)
	{
		overlay_mask();
	}
	output->set_transparent();
	return output;
}

void TitleMain::load_defaults()
{
	int color;
	char text_path[1024];

	defaults = load_defaults_file("title.rc");

	defaults->get("FONT", config.font);
	defaults->get("ENCODING", config.encoding);
	config.style = defaults->get("STYLE", config.style);
	config.size = defaults->get("SIZE", config.size);
	color = defaults->get("COLOR", 0);
	if(color)
	{
		config.color_red = (color >> 8) & 0xff00;
		config.color_green = (color & 0xff00);
		config.color_blue = (color << 8) & 0xff00;
	}
	config.color_red = defaults->get("COLOR_R", config.color_red);
	config.color_green = defaults->get("COLOR_G", config.color_green);
	config.color_blue = defaults->get("COLOR_B", config.color_blue);
	color = defaults->get("COLOR_STROKE", 0);
	if(color)
	{
		config.color_stroke_red = (color >> 8) & 0xff00;
		config.color_stroke_green = (color & 0xff00);
		config.color_stroke_blue = (color << 8) & 0xff00;
	}
	config.color_stroke_red = defaults->get("COLOR_STROKE_R", config.color_stroke_red);
	config.color_stroke_green = defaults->get("COLOR_STROKE_G", config.color_stroke_green);
	config.color_stroke_blue = defaults->get("COLOR_STROKE_B", config.color_stroke_blue);
	config.stroke_width = defaults->get("STROKE_WIDTH", config.stroke_width);
	config.motion_strategy = defaults->get("MOTION_STRATEGY", config.motion_strategy);
	config.loop = defaults->get("LOOP", config.loop);
	config.pixels_per_second = defaults->get("PIXELS_PER_SECOND", config.pixels_per_second);
	config.hjustification = defaults->get("HJUSTIFICATION", config.hjustification);
	config.vjustification = defaults->get("VJUSTIFICATION", config.vjustification);
	config.fade_in = defaults->get("FADE_IN", config.fade_in);
	config.fade_out = defaults->get("FADE_OUT", config.fade_out);
	config.x = defaults->get("TITLE_X", config.x);
	config.y = defaults->get("TITLE_Y", config.y);
	config.dropshadow = defaults->get("DROPSHADOW", config.dropshadow);
	config.timecode = defaults->get("TIMECODE", config.timecode);
	defaults->get("TIMECODEFORMAT", config.timecodeformat);

// Store text in separate path to isolate special characters
	FileSystem fs;
	sprintf(text_path, "%stitle_text.rc", plugin_conf_dir());
	fs.complete_path(text_path);
	FILE *fd = fopen(text_path, "rb");
	if(fd)
	{
		fseek(fd, 0, SEEK_END);
		long len = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		if(len && fread(config.text, len, 1, fd) < 1){
			errormsg("Title: failed to load default text");
			len = 0;
		}
		config.text[len] = 0;
		fclose(fd);
		config.text_to_ucs4(config.encoding);
		strcpy(config.encoding, DEFAULT_ENCODING);
	}
	else
	{
		config.text[0] = 0;
		config.wtext[0] = 0;
		config.wtext_length = 0;
	}
}

void TitleMain::save_defaults()
{
	char text_path[1024];

	defaults->update("FONT", config.font);
	defaults->update("ENCODING", config.encoding);
	defaults->update("STYLE", config.style);
	defaults->update("SIZE", config.size);
	defaults->delete_key("COLOR");
	defaults->update("COLOR_R", config.color_red);
	defaults->update("COLOR_G", config.color_green);
	defaults->update("COLOR_B", config.color_blue);
	defaults->delete_key("COLOR_STROKE");
	defaults->update("COLOR_STROKE_R", config.color_stroke_red);
	defaults->update("COLOR_STROKE_G", config.color_stroke_green);
	defaults->update("COLOR_STROKE_B", config.color_stroke_blue);
	defaults->update("STROKE_WIDTH", config.stroke_width);
	defaults->update("MOTION_STRATEGY", config.motion_strategy);
	defaults->update("LOOP", config.loop);
	defaults->update("PIXELS_PER_SECOND", config.pixels_per_second);
	defaults->update("HJUSTIFICATION", config.hjustification);
	defaults->update("VJUSTIFICATION", config.vjustification);
	defaults->update("FADE_IN", config.fade_in);
	defaults->update("FADE_OUT", config.fade_out);
	defaults->update("TITLE_X", config.x);
	defaults->update("TITLE_Y", config.y);
	defaults->update("DROPSHADOW", config.dropshadow);
	defaults->update("TIMECODE", config.timecode);
	defaults->update("TIMECODEFORMAT", config.timecodeformat);
	defaults->save();

// Store text in separate path to isolate special characters
	FileSystem fs;
	int txlen = BC_Resources::encode(BC_Resources::wide_encoding, DEFAULT_ENCODING,
		(char*)config.wtext, config.text, BCTEXTLEN, 
		config.wtext_length * sizeof(wchar_t));
	sprintf(text_path, "%stitle_text.rc", plugin_conf_dir());
	fs.complete_path(text_path);
	FILE *fd = fopen(text_path, "wb");
	if(fd)
	{
		fwrite(config.text, txlen, 1, fd);
		fclose(fd);
	}
}

void TitleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TITLE");
	output.tag.set_property("FONT", config.font);
	output.tag.set_property("ENCODING", config.encoding);
	output.tag.set_property("STYLE", (int64_t)config.style);
	output.tag.set_property("SIZE", config.size);
	output.tag.set_property("COLOR_R", config.color_red);
	output.tag.set_property("COLOR_G", config.color_green);
	output.tag.set_property("COLOR_B", config.color_blue);
	output.tag.set_property("COLOR_STROKE_R", config.color_stroke_red);
	output.tag.set_property("COLOR_STROKE_G", config.color_stroke_green);
	output.tag.set_property("COLOR_STROKE_B", config.color_stroke_blue);
	output.tag.set_property("STROKE_WIDTH", config.stroke_width);
	output.tag.set_property("MOTION_STRATEGY", config.motion_strategy);
	output.tag.set_property("LOOP", config.loop);
	output.tag.set_property("PIXELS_PER_SECOND", config.pixels_per_second);
	output.tag.set_property("HJUSTIFICATION", config.hjustification);
	output.tag.set_property("VJUSTIFICATION", config.vjustification);
	output.tag.set_property("FADE_IN", config.fade_in);
	output.tag.set_property("FADE_OUT", config.fade_out);
	output.tag.set_property("TITLE_X", config.x);
	output.tag.set_property("TITLE_Y", config.y);
	output.tag.set_property("DROPSHADOW", config.dropshadow);
	output.tag.set_property("TIMECODE", config.timecode);
	output.tag.set_property("TIMECODEFORMAT", config.timecodeformat);
	output.append_tag();
	output.append_newline();
	BC_Resources::encode(BC_Resources::wide_encoding, DEFAULT_ENCODING, (char*)config.wtext,
		config.text, BCTEXTLEN, config.wtext_length * sizeof(wchar_t));
	output.encode_text(config.text);
	output.tag.set_title("/TITLE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void TitleMain::read_data(KeyFrame *keyframe)
{
	int color;
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("TITLE"))
		{
			input.tag.get_property("FONT", config.font);
			input.tag.get_property("ENCODING", config.encoding);
			config.style = input.tag.get_property("STYLE", (int64_t)config.style);
			config.size = input.tag.get_property("SIZE", config.size);
			color = input.tag.get_property("COLOR", 0);
			if(color)
			{
				config.color_red = (color >> 8) & 0xff00;
				config.color_green = (color & 0xff00);
				config.color_blue = (color << 8) & 0xff00;
			}
			config.color_red = input.tag.get_property("COLOR_R", config.color_red);
			config.color_green = input.tag.get_property("COLOR_G", config.color_green);
			config.color_blue = input.tag.get_property("COLOR_B", config.color_blue);
			color = input.tag.get_property("COLOR_STROKE", 0);
			if(color)
			{
				config.color_stroke_red = (color >> 8) & 0xff00;
				config.color_stroke_green = (color & 0xff00);
				config.color_stroke_blue = (color << 8) & 0xff00;
			}
			config.color_stroke_red = input.tag.get_property("COLOR_STROKE_R",
				config.color_stroke_red);
			config.color_stroke_green = input.tag.get_property("COLOR_STROKE_G",
				config.color_stroke_green);
			config.color_stroke_blue = input.tag.get_property("COLOR_STROKE_B",
				config.color_stroke_blue);
			config.stroke_width = input.tag.get_property("STROKE_WIDTH", config.stroke_width);
			config.motion_strategy = input.tag.get_property("MOTION_STRATEGY", config.motion_strategy);
			config.loop = input.tag.get_property("LOOP", config.loop);
			config.pixels_per_second = input.tag.get_property("PIXELS_PER_SECOND", config.pixels_per_second);
			config.hjustification = input.tag.get_property("HJUSTIFICATION", config.hjustification);
			config.vjustification = input.tag.get_property("VJUSTIFICATION", config.vjustification);
			config.fade_in = input.tag.get_property("FADE_IN", config.fade_in);
			config.fade_out = input.tag.get_property("FADE_OUT", config.fade_out);
			config.x = input.tag.get_property("TITLE_X", config.x);
			config.y = input.tag.get_property("TITLE_Y", config.y);
			config.dropshadow = input.tag.get_property("DROPSHADOW", config.dropshadow);
			config.timecode = input.tag.get_property("TIMECODE", config.timecode);
			input.tag.get_property("TIMECODEFORMAT", config.timecodeformat);
			strcpy(config.text, input.read_text());
			config.text_to_ucs4(config.encoding);
		}
		else
		if(input.tag.title_is("/TITLE"))
		{
			break;
		}
	}
}
