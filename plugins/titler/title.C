
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

// Originally developed by Heroine Virtual Ltd.
// Support for multiple encodings, outline (stroke) by 
// Andraz Tori <Andraz.tori1@guest.arnes.si>


#include "clip.h"
#include "colormodels.h"
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
#include "plugincolors.h"
#include "title.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <iconv.h>
#include <sys/stat.h>
#include <fontconfig/fontconfig.h>

#define ZERO (1.0 / 64.0)

#define FONT_SEARCHPATH "fonts"
//#define FONT_SEARCHPATH "/usr/X11R6/lib/X11/fonts"
#ifdef X_HAVE_UTF8_STRING
#define DEFAULT_ENCODING "UTF-8"
#else
#define DEFAULT_ENCODING "ISO8859-1"
#endif
#define DEFAULT_TIMECODEFORMAT "h:mm:ss:ff"


REGISTER_PLUGIN


TitleConfig::TitleConfig()
{
	style = 0;
	color = BLACK;
	color_stroke = 0xff0000;
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
	ucs4text = 0;
}

TitleConfig::~TitleConfig()
{
	delete [] ucs4text;
}

// Does not test equivalency but determines if redrawing text is necessary.
int TitleConfig::equivalent(TitleConfig &that)
{
	return dropshadow == that.dropshadow &&
		style == that.style &&
		size == that.size &&
		color == that.color &&
		color_stroke == that.color_stroke &&
		stroke_width == that.stroke_width &&
		timecode == that.timecode && 
		!strcasecmp(timecodeformat, that.timecodeformat) &&
		hjustification == that.hjustification &&
		vjustification == that.vjustification &&
		EQUIV(pixels_per_second, that.pixels_per_second) &&
		!strcasecmp(font, that.font) &&
		!strcasecmp(encoding, that.encoding) &&
		!strcmp(text, that.text);
}

void TitleConfig::copy_from(TitleConfig &that)
{
	strcpy(font, that.font);
	style = that.style;
	size = that.size;
	color = that.color;
	color_stroke = that.color_stroke;
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
	strcpy(text, that.text);
	strcpy(encoding, that.encoding);
	ucs4text = 0;
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
	color = prev.color;
	color_stroke = prev.color_stroke;
	stroke_width = prev.stroke_width;
	motion_strategy = prev.motion_strategy;
	loop = prev.loop;
	hjustification = prev.hjustification;
	vjustification = prev.vjustification;
	fade_in = prev.fade_in;
	fade_out = prev.fade_out;
	pixels_per_second = prev.pixels_per_second;
	strcpy(text, prev.text);

	this->x = prev.x;
	this->y = prev.y;
	timecode = prev.timecode;
	strcpy(timecodeformat, prev.timecodeformat);
	this->dropshadow = prev.dropshadow;
}

// this is a little routine that converts 8 bit string to FT_ULong array // akirad
void TitleConfig::convert_text()
{
	int text_len;
	int total_packages = 0;

	tlen = 0;

	text_len = strlen(text);

	for(int i = 0; i < text_len; i++)
	{
		int x = 0;
		int z;

		tlen++;

		z = text[i];

		if(!(z & 0x80))
			x = 0;
		else if(!(z & 0x20))
			x = 1;
		else if(!(z & 0x10))
			x = 2;
		else if(!(z & 0x08))
			x = 3;
		else if(!(z & 0x04))
			x = 4;
		else if(!(z & 0x02))
			x = 5;
		i += x;
	}
	if(!ucs4text)
		ucs4text = new FT_ULong[BCTEXTLEN];

	FcChar32 return_ucs4;
	int count = 0;

	for(int i = 0; i < text_len; i++)
	{
		int x = 0;
		int z;
		FcChar8 loadutf8[8];

		z = text[i];

		if(!(z & 0x80))
			x = 0;
		else if(!(z & 0x20))
			x = 2;
		else if(!(z & 0x10))
			x = 3;
		else if(!(z & 0x08))
			x = 4;
		else if(!(z & 0x04))
			x = 5;
		else if(!(z & 0x02))
			x = 6;
		if(x > 0)
		{
			memset(loadutf8, 0, sizeof(loadutf8));
			loadutf8[0] = text[i];

			for(int p = 0; p < x; p++)
				loadutf8[p] = text[i + p];
			i += (x - 1);
		}
		else
		{
			loadutf8[0] = z;
			loadutf8[1] = 0;
		}
		FcUtf8ToUcs4(loadutf8, &return_ucs4, 6);
		ucs4text[count] = (FT_ULong)return_ucs4;
		count++;
	}
}


FontEntry::FontEntry()
{
	path = 0;
	foundary = 0;
	family = 0;
	weight = 0;
	slant = 0;
	swidth = 0;
	adstyle = 0;
	spacing = 0;
	registry = 0;
	encoding = 0;
	fixed_title = 0;
	fixed_style = 0;
}

FontEntry::~FontEntry()
{
	if(path) delete [] path;
	if(foundary) delete [] foundary;
	if(family) delete [] family;
	if(weight) delete [] weight;
	if(slant) delete [] slant;
	if(swidth) delete [] swidth;
	if(adstyle) delete [] adstyle;
	if(spacing) delete [] spacing;
	if(registry) delete [] registry;
	if(encoding) delete [] encoding;
	if(fixed_title) delete [] fixed_title;
}

void FontEntry::dump()
{
	printf("%s: %s %s %s %s %s %s %d %d %d %d %s %d %s %s\n",
		path,
		foundary,
		family,
		weight,
		slant,
		swidth,
		adstyle,
		pixelsize,
		pointsize,
		xres,
		yres,
		spacing,
		avg_width,
		registry,
		encoding);
}


TitleGlyph::TitleGlyph()
{
	char_code = 0;
	c=0;
	data = 0;
	data_stroke = 0;
}

TitleGlyph::~TitleGlyph()
{
	if(data) delete data;
	if(data_stroke) delete data_stroke;
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
	if(freetype_library) FT_Done_FreeType(freetype_library);
}

void GlyphUnit::process_package(LoadPackage *package)
{
	GlyphPackage *pkg = (GlyphPackage*)package;
	TitleGlyph *glyph = pkg->glyph;
	int result = 0;
	int altfont = 0;
	char new_path[200];

	current_font = plugin->get_font();
	plugin->check_char_code_path(current_font->path,
		glyph->char_code, new_path);

	if(plugin->load_freetype_face(freetype_library,
		freetype_face, new_path))
	{
		errormsg(_("GlyphUnit::process_package FT_New_Face failed"));
		result = 1;
	}
	else
		FT_Set_Pixel_Sizes(freetype_face, plugin->config.size, 0);

	if(!result)
	{
		int gindex = FT_Get_Char_Index(freetype_face, glyph->char_code);

// Char not found
		if (gindex == 0) 
		{
// carrige return
			if (glyph->char_code != 10)  
				errormsg(_("GlyphUnit::process_package FT_Load_Char failed - char: %li.\n"),
					glyph->char_code);
// Prevent a crash here
			glyph->width = 8;
			glyph->height = 8;
			glyph->pitch = 8;
			glyph->left = 9;
			glyph->top = 9;
			glyph->freetype_index = 0;
			glyph->advance_w = 8;
			glyph->data = new VFrame(0,
				8,
				8,
				BC_A8,
				8);
			glyph->data->clear_frame();
			glyph->data_stroke = 0;

// create outline glyph
			if (plugin->config.stroke_width >= ZERO && 
				(plugin->config.style & FONT_OUTLINE))
			{
				glyph->data_stroke = new VFrame(0,
					8,
					8,
					BC_A8,
					8);
				glyph->data_stroke->clear_frame();
			}
		}
		else
// char found and no outline desired
		if (plugin->config.stroke_width < ZERO ||
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
			if (glyph->left < 0) glyph->left = 0;
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
			FT_Outline_Get_Bitmap( freetype_library,
				&((FT_OutlineGlyph) glyph_image)->outline,
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

			typedef struct  FT_LibraryRec_ 
			{
				FT_Memory memory;
			} FT_LibraryRec;

			FT_Load_Glyph(freetype_face, gindex, FT_LOAD_DEFAULT);
			FT_Get_Glyph(freetype_face->glyph, &glyph_image);

// check if the outline is ok (non-empty);
			FT_Outline_Get_BBox(&((FT_OutlineGlyph) glyph_image)->outline, &bbox);
			if (bbox.xMin == 0 && bbox.xMax == 0 && bbox.yMin ==0 && bbox.yMax == 0)
			{
				FT_Done_Glyph(glyph_image);
				glyph->data =  new VFrame(0, 0, BC_A8,0);
				glyph->data_stroke =  new VFrame(0, 0, BC_A8,0);;
				glyph->width=0;
				glyph->height=0;
				glyph->top=0;
				glyph->left=0;
				glyph->advance_w =((int)(freetype_face->glyph->advance.x + 
					plugin->config.stroke_width * 64)) >> 6;
				return;
			}
#if FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 2)
			FT_Stroker_New(freetype_library, &stroker);
#else
			FT_Stroker_New(((FT_LibraryRec *)freetype_library)->memory, &stroker);
#endif
			FT_Stroker_Set(stroker, (int)(plugin->config.stroke_width * 64), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
			FT_Stroker_ParseOutline(stroker, &((FT_OutlineGlyph) glyph_image)->outline,1);
			FT_Stroker_GetCounts(stroker,&npoints, &ncontours);
			if (npoints ==0 && ncontours == 0) 
			{
// this never happens, but FreeType has a bug regarding Linotype's Palatino font
				FT_Stroker_Done(stroker);
				FT_Done_Glyph(glyph_image);
				glyph->data =  new VFrame(0, 0, BC_A8,0);
				glyph->data_stroke =  new VFrame(0, 0, BC_A8,0);;
				glyph->width=0;
				glyph->height=0;
				glyph->top=0;
				glyph->left=0;
				glyph->advance_w =((int)(freetype_face->glyph->advance.x + 
					plugin->config.stroke_width * 64)) >> 6;
				return;
			};

			FT_Outline_New(freetype_library, npoints, ncontours, &outline);
			outline.n_points=0;
			outline.n_contours=0;
			FT_Stroker_Export (stroker, &outline);
			FT_Outline_Get_BBox(&outline, &bbox);
			FT_Outline_Translate(&outline,
					- bbox.xMin,
					- bbox.yMin);
			FT_Outline_Translate(&((FT_OutlineGlyph) glyph_image)->outline,
					- bbox.xMin,
					- bbox.yMin + (int)(plugin->config.stroke_width*32));
			glyph->width = bm.width = ((bbox.xMax - bbox.xMin) >> 6)+1;
			glyph->height = bm.rows = ((bbox.yMax - bbox.yMin) >> 6) +1;
			glyph->pitch = bm.pitch = bm.width;
			bm.pixel_mode = FT_PIXEL_MODE_GRAY;
			bm.num_grays = 256;
			glyph->left = (bbox.xMin + 31) >> 6;
			if (glyph->left < 0) glyph->left = 0;
			glyph->top = (bbox.yMax + 31) >> 6;
			glyph->freetype_index = gindex;
			int real_advance = ((int)ceil((float)freetype_face->glyph->advance.x + 
				plugin->config.stroke_width * 64) >> 6);
			glyph->advance_w = glyph->width + glyph->left;
			if (real_advance > glyph->advance_w) 
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
			bm.buffer=glyph->data->get_data();
			FT_Outline_Get_Bitmap( freetype_library,
				&((FT_OutlineGlyph) glyph_image)->outline,
				&bm);	
			bm.buffer=glyph->data_stroke->get_data();
			FT_Outline_Get_Bitmap( freetype_library,
				&outline,
				&bm);
			FT_Outline_Done(freetype_library,&outline);
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
	unsigned char **in_rows = glyph->data->get_rows();
	unsigned char **out_rows = output->get_rows();

	for(int in_y = 0; in_y < glyph_h; in_y++)
	{
		int y_out = y + plugin->get_char_height() + in_y - glyph->top;

		if(y_out >= 0 && y_out < output_h)
		{
			unsigned char *out_row = out_rows[y_out];
			unsigned char *in_row = in_rows[in_y];
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

	if(pkg->c != 0xa)
	{
		for(int i = 0; i < plugin->glyphs.total; i++)
		{
			TitleGlyph *glyph = plugin->glyphs.values[i];
			if(glyph->c == pkg->c)
			{
				draw_glyph(plugin->text_mask, glyph, pkg->x, pkg->y);
				if(plugin->config.stroke_width >= ZERO &&
					(plugin->config.style & FONT_OUTLINE)) 
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
		pkg->c = plugin->config.ucs4text[i];
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
	float out_x1,
	float out_x2,
	float in_x1,
	float in_x2,
	int in_total,
	int out_total,
	int &out_x1_int,
	int &out_x2_int)
{
	int out_w_int;
	float offset = out_x1 - in_x1;

	out_x1_int = (int)out_x1;
	out_x2_int = MIN((int)ceil(out_x2), out_total);
	out_w_int = out_x2_int - out_x1_int;

	table = new transfer_table_f[out_w_int];
	memset(table, 0, sizeof(transfer_table_f) * out_w_int);

	float in_x = in_x1;

	for(int out_x = out_x1_int; out_x < out_x2_int; out_x++)
	{
		transfer_table_f *entry = &table[out_x - out_x1_int];

		entry->in_x1 = (int)in_x;
		entry->in_x2 = (int)in_x + 1;

// Get fraction of output pixel to fill
		entry->output_fraction = 1;

		if(out_x1 > out_x)
		{
			entry->output_fraction -= out_x1 - out_x;
		}

		if(out_x2 < out_x + 1)
		{
			entry->output_fraction = (out_x2 - out_x);
		}

// Advance in_x until out_x_fraction is filled
		float out_x_fraction = entry->output_fraction;
		float in_x_fraction = floor(in_x + 1) - in_x;

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


#define TRANSLATE(type, max, components, r, g, b) \
{ \
	unsigned char **in_rows = plugin->text_mask->get_rows(); \
	type **out_rows = (type**)plugin->output->get_rows(); \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		if(i + server->out_y1_int >= 0 && \
			i + server->out_y1_int < server->output_h) \
		{ \
			int in_y1, in_y2; \
			float y_fraction1, y_fraction2; \
			float y_output_fraction; \
			in_y1 = server->y_table[i].in_x1; \
			in_y2 = server->y_table[i].in_x2; \
			y_fraction1 = server->y_table[i].in_fraction1; \
			y_fraction2 = server->y_table[i].in_fraction2; \
			y_output_fraction = server->y_table[i].output_fraction; \
			unsigned char *in_row1 = in_rows[in_y1]; \
			unsigned char *in_row2 = in_rows[in_y2]; \
			type *out_row = out_rows[i + server->out_y1_int]; \
 \
			for(int j = server->out_x1_int; j < server->out_x2_int; j++) \
			{ \
				if(j >= 0 && j < server->output_w) \
				{ \
					int in_x1; \
					int in_x2; \
					float x_fraction1; \
					float x_fraction2; \
					float x_output_fraction; \
					in_x1 =  \
						server->x_table[j - server->out_x1_int].in_x1; \
					in_x2 =  \
						server->x_table[j - server->out_x1_int].in_x2; \
					x_fraction1 =  \
						server->x_table[j - server->out_x1_int].in_fraction1; \
					x_fraction2 =  \
						server->x_table[j - server->out_x1_int].in_fraction2; \
					x_output_fraction =  \
						server->x_table[j - server->out_x1_int].output_fraction; \
 \
					float fraction1 = x_fraction1 * y_fraction1; \
					float fraction2 = x_fraction2 * y_fraction1; \
					float fraction3 = x_fraction1 * y_fraction2; \
					float fraction4 = x_fraction2 * y_fraction2; \
					int input = (int)(in_row1[in_x1] * fraction1 +  \
								in_row1[in_x2] * fraction2 +  \
								in_row2[in_x1] * fraction3 +  \
								in_row2[in_x2] * fraction4 + 0.5); \
					input *= plugin->alpha; \
/* Alpha is 0 - 256 */ \
					input >>= 8; \
 \
					int anti_input = 0xff - input; \
					if(components == 4) \
					{ \
						out_row[j * components + 0] =  \
							(r * input + out_row[j * components + 0] * anti_input) / 0xff; \
						out_row[j * components + 1] =  \
							(g * input + out_row[j * components + 1] * anti_input) / 0xff; \
						out_row[j * components + 2] =  \
							(b * input + out_row[j * components + 2] * anti_input) / 0xff; \
						if(max == 0xffff) \
							out_row[j * components + 3] =  \
								MAX((input << 8) | input, out_row[j * components + 3]); \
						else \
							out_row[j * components + 3] =  \
								MAX(input, out_row[j * components + 3]); \
					} \
					else \
					{ \
						out_row[j * components + 0] =  \
							(r * input + out_row[j * components + 0] * anti_input) / 0xff; \
						out_row[j * components + 1] =  \
							(g * input + out_row[j * components + 1] * anti_input) / 0xff; \
						out_row[j * components + 2] =  \
							(b * input + out_row[j * components + 2] * anti_input) / 0xff; \
					} \
				} \
			} \
		} \
	} \
}

#define TRANSLATEA(type, max, components, r, g, b) \
{ \
	unsigned char **in_rows = plugin->text_mask->get_rows(); \
	type **out_rows = (type**)plugin->output->get_rows(); \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		if(i + server->out_y1_int >= 0 && \
			i + server->out_y1_int < server->output_h) \
		{ \
			unsigned char *in_row = in_rows[i]; \
			type *out_row = out_rows[i + server->out_y1_int]; \
 \
			for(int j = server->out_x1; j < server->out_x2_int; j++) \
			{ \
				if(j  >= 0 && \
					j < server->output_w) \
				{ \
					int input = (int)(in_row[j - server->out_x1]);  \
 \
					input *= plugin->alpha; \
/* Alpha is 0 - 256 */ \
					input >>= 8; \
 \
					int anti_input = 0xff - input; \
					if(components == 4) \
					{ \
						out_row[j * components + 0] =  \
							(r * input + out_row[j * components + 0] * anti_input) / 0xff; \
						out_row[j * components + 1] =  \
							(g * input + out_row[j * components + 1] * anti_input) / 0xff; \
						out_row[j * components + 2] =  \
							(b * input + out_row[j * components + 2] * anti_input) / 0xff; \
						if(max == 0xffff) \
							out_row[j * components + 3] =  \
								MAX((input << 8) | input, out_row[j * components + 3]); \
						else \
							out_row[j * components + 3] =  \
								MAX(input, out_row[j * components + 3]); \
					} \
					else \
					{ \
						out_row[j * components + 0] =  \
							(r * input + out_row[j * components + 0] * anti_input) / 0xff; \
						out_row[j * components + 1] =  \
							(g * input + out_row[j * components + 1] * anti_input) / 0xff; \
						out_row[j * components + 2] =  \
							(b * input + out_row[j * components + 2] * anti_input) / 0xff; \
					} \
				} \
			} \
		} \
	} \
}

static YUV yuv;

void TitleTranslateUnit::process_package(LoadPackage *package)
{
	TitleTranslatePackage *pkg = (TitleTranslatePackage*)package;
	TitleTranslate *server = (TitleTranslate*)this->server;
	int r_in, g_in, b_in;

	r_in = (plugin->config.color & 0xff0000) >> 16;
	g_in = (plugin->config.color & 0xff00) >> 8;
	b_in = plugin->config.color & 0xff;
	switch(plugin->output->get_color_model())
	{
	case BC_RGB888:
		TRANSLATE(unsigned char, 0xff, 3, r_in, g_in, b_in);
		break;

	case BC_RGB_FLOAT:
	{
		float r, g, b;
		r = (float)r_in / 0xff;
		g = (float)g_in / 0xff;
		b = (float)b_in / 0xff;
		TRANSLATE(float, 0xff, 3, r, g, b);
		break;
	}
	case BC_YUV888:
	{
		unsigned char y, u, v;
		yuv.rgb_to_yuv_8(r_in, g_in, b_in, y, u, v);
		TRANSLATE(unsigned char, 0xff, 3, y, u, v);
		break;
	}
	case BC_RGB161616:
	{
		uint16_t r, g, b;
		r = (r_in << 8) | r_in;
		g = (g_in << 8) | g_in;
		b = (b_in << 8) | b_in;
		TRANSLATE(uint16_t, 0xffff, 3, r, g, b);
		break;
	}
	case BC_YUV161616:
	{
		uint16_t y, u, v;
		yuv.rgb_to_yuv_16(
			(r_in << 8) | r_in, 
			(g_in << 8) | g_in, 
			(b_in << 8) | b_in, 
			y, 
			u, 
			v);
		TRANSLATE(uint16_t, 0xffff, 3, y, u, v);
		break;
	}
	case BC_RGBA_FLOAT:
	{
		float r, g, b;
		r = (float)r_in / 0xff;
		g = (float)g_in / 0xff;
		b = (float)b_in / 0xff;
		TRANSLATE(float, 0xff, 4, r, g, b);
		break;
	}
	case BC_RGBA8888:
	{
		TRANSLATE(unsigned char, 0xff, 4, r_in, g_in, b_in);
		break;
	}
	case BC_YUVA8888:
	{
		unsigned char y, u, v;
		yuv.rgb_to_yuv_8(r_in, g_in, b_in, y, u, v);
		TRANSLATE(unsigned char, 0xff, 4, y, u, v);
		break;
	}
	case BC_RGBA16161616:
	{
		uint16_t r, g, b;
		r = (r_in << 8) | r_in;
		g = (g_in << 8) | g_in;
		b = (b_in << 8) | b_in;
		TRANSLATE(uint16_t, 0xffff, 4, r, g, b);
		break;
	}
	case BC_YUVA16161616:
	{
		uint16_t y, u, v;
		yuv.rgb_to_yuv_16(
			(r_in << 8) | r_in, 
			(g_in << 8) | g_in, 
			(b_in << 8) | b_in, 
			y, 
			u, 
			v);
		TRANSLATE(uint16_t, 0xffff, 4, y, u, v);
		break;
	}
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

ArrayList<FontEntry*>* TitleMain::fonts = 0;

TitleMain::TitleMain(PluginServer *server)
 : PluginVClient(server)
{
// Build font database
	build_fonts();
	text_mask = 0;
	text_mask_stroke = 0;
	glyph_engine = 0;
	title_engine = 0;
	freetype_library = 0;
	freetype_face = 0;
	char_positions = 0;
	rows_bottom = 0;
	translate = 0;
	need_reconfigure = 1;
	PLUGIN_CONSTRUCTOR_MACRO
}

TitleMain::~TitleMain()
{
	if(text_mask) delete text_mask;
	if(text_mask_stroke) delete text_mask_stroke;
	if(char_positions) delete [] char_positions;
	if(rows_bottom) delete [] rows_bottom;
	clear_glyphs();
	if(glyph_engine) delete glyph_engine;
	if(title_engine) delete title_engine;
	if(freetype_library) FT_Done_FreeType(freetype_library);
	if(translate) delete translate;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


void TitleMain::build_fonts()
{
	if(!fonts)
	{
		fonts = new ArrayList<FontEntry*>;
// Construct path from location of the plugin
		char search_path[BCTEXTLEN];
		strcpy(search_path, PluginClient::get_path());
		char *ptr = strrchr(search_path, '/');
		strcpy(ptr + 1, FONT_SEARCHPATH);

		FcPattern *pat;
		FcFontSet *fs;
		FcObjectSet *os;
		FcChar8 *family, *file, *foundry, *style, *format;
		int slant, spacing, width, weight;
		int force_style = 0;
		int limit_to_trutype = 0; // if you want limit search to TrueType put 1
		FcConfig *config;
		FcBool resultfc;
		int i;
		resultfc = FcInit();
		FcConfigAppFontAddDir(0, (const FcChar8*)search_path);
		config = FcConfigGetCurrent();
		FcConfigSetRescanInterval(config, 0);

		pat = FcPatternCreate();
		os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_FOUNDRY, FC_WEIGHT,
			FC_WIDTH, FC_SLANT, FC_FONTFORMAT, FC_SPACING, FC_STYLE,
			(char *)0);
		fs = FcFontList(config, pat, os);
		FcPattern *font;

		for(i = 0; fs && i < fs->nfont; i++)
		{
			font = fs->fonts[i];
			force_style = 0;
			FcPatternGetString(font, FC_FONTFORMAT, 0, &format);
			if((!strcmp((char *)format, "TrueType")) || limit_to_trutype) // on this point you can limit font search
			{
				FontEntry *entry = new FontEntry;

				if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
				{
					entry->path = new char[strlen((char*)file) + 1];
					strcpy(entry->path, (char*)file);
				}

				if(FcPatternGetString(font, FC_FOUNDRY, 0, &foundry) == FcResultMatch)
				{
					entry->foundary = new char[strlen((char*)foundry) + 2];
					strcpy(entry->foundary, (char *)foundry);
				}

				if(FcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch)
				{
					const char *s;

					switch(weight)
					{
					case FC_WEIGHT_THIN:
					case FC_WEIGHT_EXTRALIGHT:
					case FC_WEIGHT_LIGHT:
					case FC_WEIGHT_BOOK:
						force_style = 1;
						s = "medium";
						break;

					case FC_WEIGHT_NORMAL:
					case FC_WEIGHT_MEDIUM:
					default:
						s = "medium";
						break;

					case FC_WEIGHT_BLACK:
					case FC_WEIGHT_SEMIBOLD:
					case FC_WEIGHT_BOLD:
						s = "bold";
						entry->fixed_style |= FONT_BOLD;
						break;

					case FC_WEIGHT_EXTRABOLD:
					case FC_WEIGHT_EXTRABLACK:
						force_style = 1;
						s = "bold";
						entry->fixed_style |= FONT_BOLD;
						break;
					}
					entry->weight = new char[strlen(s) + 1];
					strcpy(entry->weight, s);
				}

				if(FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch)
				{
					entry->family = new char[strlen((char*)family) + 2];
					strcpy(entry->family, (char*)family);
				}

				if(FcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch)
				{
					int slant;

					switch(slant)
					{
					case FC_SLANT_ROMAN:
					default:
						slant = 'r';
						break;
					case FC_SLANT_ITALIC:
						slant =  'i';
						entry->fixed_style |= FONT_ITALIC;
						break;
					case FC_SLANT_OBLIQUE:
						slant = 'o';
						entry->fixed_style |= FONT_ITALIC;
						break;
					}
					entry->slant = new char[2];
					entry->slant[0] = slant;
					entry->slant[1] = 0;
				}

				if(FcPatternGetInteger(font, FC_WIDTH, 0, &width) == FcResultMatch)
				{
					const char *s;

					switch(width)
					{
					case FC_WIDTH_ULTRACONDENSED:
						s = "ultracondensed";
						break;

					case FC_WIDTH_EXTRACONDENSED:
						s = "extracondensed";
						break;

					case FC_WIDTH_CONDENSED:
						s = "condensed";
						break;

					case FC_WIDTH_SEMICONDENSED:
						s = "semicondensed";
						break;

					case FC_WIDTH_NORMAL:
					default:
						s = "normal";
						break;

					case FC_WIDTH_SEMIEXPANDED:
						s = "semiexpanded";
						break;

					case FC_WIDTH_EXPANDED:
						s = "expanded";
						break;

					case FC_WIDTH_EXTRAEXPANDED:
						s = "extraexpanded";
						break;

					case FC_WIDTH_ULTRAEXPANDED:
						s = "ultraexpanded";
						break;
					}
					entry->swidth = new char[strlen(s) + 1];
					strcpy(entry->swidth, s);
				}

				if(FcPatternGetInteger(font, FC_SPACING, 0, &spacing) == FcResultMatch)
				{
					int spacs;

					switch(spacing)
					{
						case 0:
						default:
							spacs = 'p';
							break;

						case 90:
							spacs = 'd';
							break;

						case 100:
							spacs = 'm';
							break;

						case 110:
							spacs = 'c';
							break;
					}
					entry->spacing = new char[2];
					entry->spacing[0] = spacs;
					entry->spacing[1] = 0;
				}

				// Add fake stuff for compatibility
				entry->adstyle = new char[2];
				entry->adstyle[0] = ' ';
				entry->adstyle[1] = 0;
				entry->pixelsize = 0;
				entry->pointsize = 0;
				entry->xres = 0;
				entry->yres = 0;
				entry->avg_width = 0;
				entry->registry = new char[strlen("utf") + 1];
				strcpy(entry->registry, "utf");
				entry->encoding = new char[2];
				entry->encoding[0] = '8';
				entry->encoding[1] = 0;

				if(!FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
					force_style = 0;

				// If font has a style unmanaged by titler plugin, force style to be displayed on name
				// in this way we can shown all available fonts styles.
				char tmpstring[BCTEXTLEN];
				int len;

				if(force_style)
				{
					len = sprintf(tmpstring, "%s (%s)", entry->family, style);
					entry->fixed_title = new char[len + 1];
					strcpy(entry->fixed_title, tmpstring);
				}
				else
				{
					if(strcmp(entry->foundary, "unknown"))
					{
						len = sprintf(tmpstring, "%s (%s)", entry->family, entry->foundary);
						entry->fixed_title = new char[len + 1];
						strcpy(entry->fixed_title, tmpstring);
					}
					else
					{
						len = sprintf(tmpstring, "%s", entry->family);
						entry->fixed_title = new char[len + 1];
						strcpy(entry->fixed_title, tmpstring);
					}
				}
				fonts->append(entry);
			}
		}
		FcFontSetDestroy(fs);
	}
}

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
	} else
	{
		return 0;
	}
}

FontEntry* TitleMain::get_font_entry(char *title,
	int style,
	int size)
{
	FontEntry *result = 0;
	int got_title = 0;

	for(int i = 0; i < fonts->total; i++)
	{
		FontEntry *entry = fonts->values[i];

		if(!result) result = entry;

		if(!strcmp(title, entry->fixed_title))
		{
			if(!got_title) result = entry;
			got_title = 1;

// Not every font has a size but every font has a style
			if(entry->fixed_style == style)
				result = entry;

			if(entry->fixed_style == style && entry->pointsize == size) 
				result = entry;
		}
	}
	return result;
}


FontEntry* TitleMain::get_font()
{
	return get_font_entry(config.font,
		config.style,
		config.size);
}


int TitleMain::get_char_height()
{
// this is height above the zero line, but does not include characters that go below
	int result = config.size;
	if((config.style & FONT_OUTLINE)) result += (int)ceil(config.stroke_width * 2);
	return result;
}

int TitleMain::get_char_advance(int current, int next)
{
	FT_Vector kerning;
	int result = 0;
	TitleGlyph *current_glyph = 0;
	TitleGlyph *next_glyph = 0;

	if(current == 0xa) return 0;

	for(int i = 0; i < glyphs.total; i++)
	{
		if(glyphs.values[i]->c == current)
		{
			current_glyph = glyphs.values[i];
			break;
		}
	}

	for(int i = 0; i < glyphs.total; i++)
	{
		if(glyphs.values[i]->c == next)
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

	// now convert text to FT_Ulong array
	config.convert_text();
#ifndef X_HAVE_UTF8_STRING
	// temp conversion to utf8
	convert_encoding();
#endif

	for(int i = 0; i < config.tlen; i++)
	{
		FT_ULong char_code;
		int c = config.ucs4text[i];
		int exists = 0;
		char_code = config.ucs4text[i];

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
			glyph->c = c;
			glyph->char_code = char_code;
		}
	}

	if(!glyph_engine)
		glyph_engine = new GlyphEngine(this, PluginClient::smp + 1);

	glyph_engine->set_package_count(total_packages);
	glyph_engine->process_packages();
}

void TitleMain::get_total_extents()
{
// Determine extents of total text
	int current_w = 0;
	int row_start = 0;
	text_len = config.tlen;

	if(!char_positions) char_positions = new title_char_position_t[text_len];
	text_rows = 0;
	text_w = 0;
	ascent = 0;

	for(int i = 0; i < glyphs.total; i++)
		if(glyphs.values[i]->top > ascent) ascent = glyphs.values[i]->top;

	// get the number of rows first
	for(int i = 0; i < text_len; i++)
	{
		if(config.ucs4text[i] == 0xa || i == text_len - 1)
		{
			text_rows++;
		}
	}
	if (!rows_bottom) rows_bottom = new int[text_rows+1];
	text_rows = 0;
	rows_bottom[0] = 0;

	for(int i = 0; i < text_len; i++)
	{
		char_positions[i].x = current_w;
		char_positions[i].y = text_rows * get_char_height();
		char_positions[i].w = get_char_advance(config.ucs4text[i], config.ucs4text[i + 1]);
		TitleGlyph *current_glyph = 0;
		for(int j = 0; j < glyphs.total; j++)
		{
			if(glyphs.values[j]->c == config.ucs4text[i])
			{
				current_glyph = glyphs.values[j];
				break;
			}
		}
		int current_bottom = current_glyph->top - current_glyph->height;
		if (current_bottom < rows_bottom[text_rows])
			rows_bottom[text_rows] = current_bottom ;

		current_w += char_positions[i].w;

		if(config.ucs4text[i] == 0xa || i == text_len - 1)
		{
			text_rows++;
			rows_bottom[text_rows] = 0;
			if(current_w > text_w) text_w = current_w;
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
		if(config.ucs4text[i] == 0xa || i == text_len - 1)
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
							char_positions[i].w) /
						2;
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
		float magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_h + input->get_h();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y1 = config.y + input->get_h() - magnitude;
	}
	else
	if(config.motion_strategy == TOP_TO_BOTTOM)
	{
		float magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_h + input->get_h();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y1 = config.y + magnitude;
		text_y1 -= text_h;
	}
	else
	if(config.vjustification == JUSTIFY_TOP)
	{
		text_y1 = config.y;
	}
	else
	if(config.vjustification == JUSTIFY_MID)
	{
		text_y1 = config.y + input->get_h() / 2 - text_h / 2;
	}
	else
	if(config.vjustification == JUSTIFY_BOTTOM)
	{
		text_y1 = config.y + input->get_h() - text_h;
	}

	text_y2 = text_y1 + text_h + 0.5;

// Determine x of visible text
	if(config.motion_strategy == RIGHT_TO_LEFT)
	{
		float magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_w + input->get_w();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x1 = config.x + (float)input->get_w() - magnitude;
	}
	else
	if(config.motion_strategy == LEFT_TO_RIGHT)
	{
		float magnitude = config.pixels_per_second *
			(source_pts - config.prev_border_pts);

		if(config.loop)
		{
			int loop_size = text_w + input->get_w();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x1 = config.x + -(float)text_w + magnitude;
	}
	else
	if(config.hjustification == JUSTIFY_LEFT)
	{
		text_x1 = config.x;
	}
	else
	if(config.hjustification == JUSTIFY_MID)
	{
		text_x1 = config.x + input->get_w() / 2 - text_w / 2;
	}
	else
	if(config.hjustification == JUSTIFY_RIGHT)
	{
		text_x1 = config.x + input->get_w() - text_w;
	}

// Determine y extents just of visible text
	visible_row1 = (int)(-text_y1 / get_char_height());
	if(visible_row1 < 0) visible_row1 = 0;

	visible_row2 = (int)((float)text_rows - (text_y2 - input->get_h()) / get_char_height() + 1);
	if(visible_row2 > text_rows) visible_row2 = text_rows;

	if(visible_row2 <= visible_row1) return 1;

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
		text_mask->get_h() != visible_rows * get_char_height() - rows_bottom[visible_row2 - 1]))
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
			visible_rows * get_char_height() - rows_bottom[visible_row2-1],
			BC_A8);
		text_mask_stroke = new VFrame(0,
			text_w,
			visible_rows * get_char_height() - rows_bottom[visible_row2-1],
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
			title_engine = new TitleEngine(this, PluginClient::smp + 1);

		title_engine->set_package_count(visible_char2 - visible_char1);
		title_engine->process_packages();
	}

	return 0;
}


void TitleMain::overlay_mask()
{
	alpha = 0x100;
	if(!EQUIV(config.fade_in, 0))
	{
		ptstime fade_pts = source_pts - config.prev_border_pts;

		if(fade_pts >= 0 && fade_pts < config.fade_in)
		{
			alpha = lroundf(256.0f * fade_pts / config.fade_in);
		}
	}

	if(!EQUIV(config.fade_out, 0))
	{
		ptstime fade_pts = config.next_border_pts - source_pts;

		if(fade_pts >= 0 && fade_pts < config.fade_out)
		{
			alpha = lroundf(256.0f * fade_pts / config.fade_out);
		}
	}

	if(config.dropshadow)
	{
		text_x1 += config.dropshadow;
		text_x2 += config.dropshadow;
		mask_y1 += config.dropshadow;
		mask_y2 += config.dropshadow;
		if(text_x1 < input->get_w() && text_x1 + text_w > 0 &&
			mask_y1 < input->get_h() && mask_y2 > 0)
		{
			if(!translate) translate = new TitleTranslate(this, PluginClient::smp + 1);
// Do 2 passes if dropshadow.
			int temp_color = config.color;
			config.color = 0x0;
			translate->process_packages();
			config.color = temp_color;
		}
		text_x1 -= config.dropshadow;
		text_x2 -= config.dropshadow;
		mask_y1 -= config.dropshadow;
		mask_y2 -= config.dropshadow;
	}

	if(text_x1 < input->get_w() && text_x1 + text_w > 0 &&
		mask_y1 < input->get_h() && mask_y2 > 0)
	{
		if(!translate) translate = new TitleTranslate(this, PluginClient::smp + 1);
		translate->process_packages();
		if (config.stroke_width >= ZERO &&
			(config.style & FONT_OUTLINE)) 
		{
			int temp_color = config.color;
			VFrame *tmp_text_mask = this->text_mask;
			config.color = config.color_stroke;
			this->text_mask = this->text_mask_stroke;

			translate->process_packages();
			config.color = temp_color;
			this->text_mask = tmp_text_mask;
		}
	}
}

void TitleMain::clear_glyphs()
{
	glyphs.remove_all_objects();
}

const char* TitleMain::motion_to_text(int motion)
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
}

int TitleMain::text_to_motion(const char *text)
{
	for(int i = 0; i < TOTAL_PATHS; i++)
	{
		if(!strcasecmp(motion_to_text(i), text)) return i;
	}
	return 0;
}


void TitleMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int result = 0;
	input = input_ptr;
	output = output_ptr;

	need_reconfigure |= load_configuration();

// Always synthesize text and redraw it for timecode
	if(config.timecode)
	{
		int tcf = Units::timeformat_totype(config.timecodeformat);
		if (tcf < 0) {
			tcf = TIME_HMSF;
			strcpy(config.timecodeformat, DEFAULT_TIMECODEFORMAT);
		}
		Units::totext(config.text, 
				input->get_pts(),
				tcf, 
				0,
				PluginVClient::project_frame_rate, 
				0);
		need_reconfigure = 1;
	}

// Check boundaries
	if(config.size <= 0 || config.size >= 2048) config.size = 72;
	if(config.stroke_width < 0 || 
		config.stroke_width >= 512) config.stroke_width = 0.0;
	if(!strlen(config.text)) return;
	if(!strlen(config.encoding)) strcpy(config.encoding, DEFAULT_ENCODING);

// Handle reconfiguration
	if(need_reconfigure)
	{
		if(text_mask) delete text_mask;
		if(text_mask_stroke) delete text_mask_stroke;
		text_mask = 0;
		text_mask_stroke = 0;
		if(freetype_face) FT_Done_Face(freetype_face);
		freetype_face = 0;
		if(glyph_engine) delete glyph_engine;
		glyph_engine = 0;
		if(char_positions) delete [] char_positions;
		char_positions = 0;
		if(rows_bottom) delete [] rows_bottom;
		rows_bottom = 0;
		clear_glyphs();
		visible_row1 = 0;
		visible_row2 = 0;
		ascent = 0;

		if(!freetype_library) 
			FT_Init_FreeType(&freetype_library);

		if(!freetype_face)
		{
			FontEntry *font = get_font();
			if(load_freetype_face(freetype_library,
				freetype_face,
				font->path))
			{
				errorbox("TitleMain::process_realtime %s: FT_New_Face failed.",
					font->fixed_title);
				result = 1;
			}

			if(!result) FT_Set_Pixel_Sizes(freetype_face, config.size, 0);
		}

		if(!result)
		{
			draw_glyphs();
			get_total_extents();
			need_reconfigure = 0;
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
}

void TitleMain::load_defaults()
{
	char text_path[1024];

	defaults = load_defaults_file("title.rc");

	defaults->get("FONT", config.font);
	defaults->get("ENCODING", config.encoding);
	config.style = defaults->get("STYLE", config.style);
	config.size = defaults->get("SIZE", config.size);
	config.color = defaults->get("COLOR", config.color);
	config.color_stroke = defaults->get("COLOR_STROKE", config.color_stroke);
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
	window_w = defaults->get("WINDOW_W", 660);
	window_h = defaults->get("WINDOW_H", 480);

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
	}
	else
		config.text[0] = 0;
}

void TitleMain::save_defaults()
{
	char text_path[1024];

	defaults->update("FONT", config.font);
	defaults->update("ENCODING", config.encoding);
	defaults->update("STYLE", config.style);
	defaults->update("SIZE", config.size);
	defaults->update("COLOR", config.color);
	defaults->update("COLOR_STROKE", config.color_stroke);
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
	defaults->update("WINDOW_W", window_w);
	defaults->update("WINDOW_H", window_h);
	defaults->save();

// Store text in separate path to isolate special characters
	FileSystem fs;
	sprintf(text_path, "%stitle_text.rc", plugin_conf_dir());
	fs.complete_path(text_path);
	FILE *fd = fopen(text_path, "wb");
	if(fd)
	{
		fwrite(config.text, strlen(config.text), 1, fd);
		fclose(fd);
	}
}

void TitleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
#ifdef X_HAVE_UTF8_STRING
	convert_encoding();
#endif
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("TITLE");
	output.tag.set_property("FONT", config.font);
	output.tag.set_property("ENCODING", config.encoding);
	output.tag.set_property("STYLE", (int64_t)config.style);
	output.tag.set_property("SIZE", config.size);
	output.tag.set_property("COLOR", config.color);
	output.tag.set_property("COLOR_STROKE", config.color_stroke);
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

	output.encode_text(config.text);

	output.tag.set_title("/TITLE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void TitleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int new_interlace = 0;
	int new_horizontal = 0;
	int new_luminance = 0;

	config.prev_border_pts = keyframe->pos_time;
	while(!input.read_tag())
	{
		if(input.tag.title_is("TITLE"))
		{
			input.tag.get_property("FONT", config.font);
			input.tag.get_property("ENCODING", config.encoding);
			config.style = input.tag.get_property("STYLE", (int64_t)config.style);
			config.size = input.tag.get_property("SIZE", config.size);
			config.color = input.tag.get_property("COLOR", config.color);
			config.color_stroke = input.tag.get_property("COLOR_STROKE", config.color_stroke);
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
		}
		else
		if(input.tag.title_is("/TITLE"))
		{
			break;
		}
	}
#ifdef X_HAVE_UTF8_STRING
	convert_encoding();
#endif
}

void TitleMain::convert_encoding()
{
	if(strcmp(config.encoding, "UTF-8"))
	{
		iconv_t cd;
		char *utf8text = new char[sizeof(config.text) * 6];

		cd = iconv_open("UTF-8",config.encoding);
		if(cd == (iconv_t)-1)
		{
			// Something went wrong.
			errormsg(_("Iconv conversion from %s to UTF-8 not available"), config.encoding);
		}
		else
		{
		// if iconv is working ok for current encoding
			char *inbuf = config.text;
			char *outbuf = utf8text;
			size_t inbytes = strlen(config.text);
			size_t outbytes = sizeof(config.text) * 6 - 1;
			int noconv = 0;
			do {
				if(iconv(cd, &inbuf, &inbytes, &outbuf, &outbytes) == (size_t) -1)
				{
					errormsg("TitleMain::convert_encoding: iconv failed!");
					noconv = 1;
					break;
				}
			} while(inbytes > 0 && outbytes > 0);

			*outbuf = 0;
			strcpy(config.text, utf8text);
			strcpy(config.encoding, "UTF-8");

			iconv_close(cd);
		}
		delete [] utf8text;
	}
}

// Checks if char_code is on the selected font and 
//    changes font with the first compatible //Akirad
int TitleMain::check_char_code_path(const char *path_old, FT_ULong &char_code,
	char *path_new)
{
	int result = 0;
	int match_charset = 0;
	int limit_to_truetype = 1; // if you want to limit search to truetype put 0

// Try to open char_set with ft_Library
	FT_Library temp_freetype_library;
	FT_Face temp_freetype_face = 0;

	// Do not search for non control codes
	if(char_code < ' ')
	{
		strcpy(path_new, path_old);
		return 0;
	}

	FT_Init_FreeType(&temp_freetype_library);

	if(!FT_New_Face(temp_freetype_library, path_old, 0, &temp_freetype_face))
	{
		if(FT_Get_Char_Index(temp_freetype_face, char_code))
			match_charset = 1;
	}
	if(temp_freetype_face) FT_Done_Face(temp_freetype_face);
	FT_Done_FreeType(temp_freetype_library);

	if(!match_charset)
	{
		FcPattern *pat;
		FcFontSet *fs;
		FcObjectSet *os;
		FcChar8 *file, *format;
		FcCharSet *fcs;
		FcConfig *config;
		FcBool resultfc;

		resultfc = FcInit();
		config = FcConfigGetCurrent();
		FcConfigSetRescanInterval(config, 0);

		pat = FcPatternCreate();
		os = FcObjectSetBuild(FC_FILE, FC_CHARSET, FC_FONTFORMAT, (char *)0);
		fs = FcFontList(config, pat, os);

		for(int i = 0; i < fs->nfont; i++)
		{
			FcPattern *font = fs->fonts[i];
			FcPatternGetString(font, FC_FONTFORMAT, 0, &format);
			if((!strcmp((char *)format, "TrueType")) || limit_to_truetype)
			{
				if(FcPatternGetCharSet(font, FC_CHARSET, 0, &fcs) == FcResultMatch)
				{
					if(FcCharSetHasChar(fcs, char_code))
					{
						if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
						{
							strcpy(path_new, (char*)file);
							result = 1;
							break;
						}
					}
				}
			}
		}
		FcFontSetDestroy(fs);
	}
	if(!result)
		strcpy(path_new, path_old);

	return result;
}
