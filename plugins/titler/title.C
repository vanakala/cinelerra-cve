// Originally developed by Heroine Virtual Ltd.
// Support for multiple encodings, outline (stroke) by 
// Andraz Tori <Andraz.tori1@guest.arnes.si>


#include "clip.h"
#include "colormodels.h"
#include "filexml.h"
#include "filesystem.h"
#include "freetype/ftbbox.h"
#include "freetype/ftglyph.h"
#include "freetype/ftoutln.h"
#include "freetype/ftstroker.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "title.h"
#include "titlewindow.h"


#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <iconv.h>

#define ZERO (1.0 / 64.0)

#define FONT_SEARCHPATH "fonts"
//#define FONT_SEARCHPATH "/usr/X11R6/lib/X11/fonts"


REGISTER_PLUGIN(TitleMain)


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
	sprintf(font, "fixed");
	sprintf(text, "hello world");
#define DEFAULT_ENCODING "ISO8859-1"
	sprintf(encoding, DEFAULT_ENCODING);
	pixels_per_second = 1.0;
	timecode = 0;
	stroke_width = 1.0;
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
	strcpy(text, that.text);
	strcpy(encoding, that.encoding);
}

void TitleConfig::interpolate(TitleConfig &prev, 
	TitleConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
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

	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


//	this->x = prev.x * prev_scale + next.x * next_scale;
//	this->y = prev.y * prev_scale + next.y * next_scale;
	this->x = prev.x;
	this->y = prev.y;
	timecode = prev.timecode;
//	this->dropshadow = (int)(prev.dropshadow * prev_scale + next.dropshadow * next_scale);
	this->dropshadow = prev.dropshadow;
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
//printf("TitleGlyph::~TitleGlyph 1\n");
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

	if(!freetype_library)
	{
		current_font = plugin->get_font();

		if(plugin->load_freetype_face(freetype_library,
			freetype_face,
			current_font->path))
		{
			printf("GlyphUnit::process_package FT_New_Face failed.\n");
			result = 1;
		}
		else
		{
			FT_Set_Pixel_Sizes(freetype_face, plugin->config.size, 0);
		}
	}

	if(!result)
	{
		int gindex = FT_Get_Char_Index(freetype_face, glyph->char_code);

//printf("GlyphUnit::process_package 1 %c\n", glyph->char_code);
// Char not found
		if (gindex == 0) 
		{
// carrige return
			if (glyph->char_code != 10)  
				printf("GlyphUnit::process_package FT_Load_Char failed - char: %i.\n",
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
//			printf("Stroke: Xmin: %ld, Xmax: %ld, Ymin: %ld, yMax: %ld\n",
//					bbox.xMin,bbox.xMax, bbox.yMin, bbox.yMax);

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
//printf("GlyphUnit::process_package 1 width=%d height=%d pitch=%d left=%d top=%d advance_w=%d freetype_index=%d\n", 
//glyph->width, glyph->height, glyph->pitch, glyph->left, glyph->top, glyph->advance_w, glyph->freetype_index);
	
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
			FT_Stroker_New(((FT_LibraryRec *)freetype_library)->memory, &stroker);
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
//			printf("Stroke: Xmin: %ld, Xmax: %ld, Ymin: %ld, yMax: %ld\nFill	Xmin: %ld, Xmax: %ld, Ymin: %ld, yMax: %ld\n",
//					bbox.xMin,bbox.xMax, bbox.yMin, bbox.yMax,
//					bbox_fill.xMin,bbox_fill.xMax, bbox_fill.yMin, bbox_fill.yMax);
	
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
//printf("GlyphUnit::process_package 1 width=%d height=%d pitch=%d left=%d top=%d advance_w=%d freetype_index=%d\n", 
//glyph->width, glyph->height, glyph->pitch, glyph->left, glyph->top, glyph->advance_w, glyph->freetype_index);


//printf("GlyphUnit::process_package 1\n");
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
// for debugging	memset(	glyph->data_stroke->get_data(), 60, glyph->pitch * glyph->height);
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

//printf("GlyphUnit::process_package 2\n");
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
			GlyphPackage *pkg = (GlyphPackage*)packages[current_package++];
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

//printf("TitleUnit::draw_glyph 1 %c %d %d\n", glyph->c, x, y);
	for(int in_y = 0; in_y < glyph_h; in_y++)
	{
//		int y_out = y + plugin->ascent + in_y - glyph->top;
		int y_out = y + plugin->get_char_height() + in_y - glyph->top;

//printf("TitleUnit::draw_glyph 1 %d\n", y_out);
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
//out_row[x_out] = 0xff;
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
//printf("TitleEngine::init_packages 1\n");
	int visible_y1 = plugin->visible_row1 * plugin->get_char_height();
//printf("TitleEngine::init_packages 1\n");
	int current_package = 0;
	for(int i = plugin->visible_char1; i < plugin->visible_char2; i++)
	{
		title_char_position_t *char_position = plugin->char_positions + i;
		TitlePackage *pkg = (TitlePackage*)get_package(current_package);
//printf("TitleEngine::init_packages 1\n");
		pkg->x = char_position->x;
//printf("TitleEngine::init_packages 1\n");
		pkg->y = char_position->y - visible_y1;
//printf("TitleEngine::init_packages 1\n");
		pkg->c = plugin->config.text[i];
//printf("TitleEngine::init_packages 2\n");
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

//printf("TitleTranslateUnit::process_package 1 %d %d\n", pkg->y1, pkg->y2);

	r_in = (plugin->config.color & 0xff0000) >> 16;
	g_in = (plugin->config.color & 0xff00) >> 8;
	b_in = plugin->config.color & 0xff;

	switch(plugin->output->get_color_model())
	{
		case BC_RGB888:
		{
			TRANSLATE(unsigned char, 0xff, 3, r_in, g_in, b_in);
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

//printf("TitleTranslateUnit::process_package 5\n");
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
//printf("TitleTranslate::init_packages 1\n");
// Generate scaling tables
	if(x_table) delete [] x_table;
	if(y_table) delete [] y_table;
//printf("TitleTranslate::init_packages 1\n");

	output_w = plugin->output->get_w();
	output_h = plugin->output->get_h();
//printf("TitleTranslate::init_packages 1 %f %d\n", plugin->text_x1, plugin->text_w);


	TranslateUnit::translation_array_f(x_table, 
		plugin->text_x1, 
		plugin->text_x1 + plugin->text_w,
		0,
		plugin->text_w,
		plugin->text_w, 
		output_w, 
		out_x1_int,
		out_x2_int);
//printf("TitleTranslate::init_packages 1 %f %f\n", plugin->mask_y1, plugin->mask_y2);

	TranslateUnit::translation_array_f(y_table, 
		plugin->mask_y1, 
		plugin->mask_y1 + plugin->text_mask->get_h(),
		0,
		plugin->text_mask->get_h(),
		plugin->text_mask->get_h(), 
		output_h, 
		out_y1_int,
		out_y2_int);

//printf("TitleTranslate::init_packages 1\n");

	
	out_y1 = out_y1_int;
	out_y2 = out_y2_int;
	out_x1 = out_x1_int;
	out_x2 = out_x2_int;
	int increment = (out_y2 - out_y1) / get_total_packages() + 1;

//printf("TitleTranslate::init_packages 1 %d %d %d %d\n", 
//	out_y1, out_y2, out_y1_int, out_y2_int);
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
//printf("TitleTranslate::init_packages 2\n");
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
	PLUGIN_CONSTRUCTOR_MACRO

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
}

TitleMain::~TitleMain()
{
//printf("TitleMain::~TitleMain 1\n");
	PLUGIN_DESTRUCTOR_MACRO
	if(text_mask) delete text_mask;
	if(text_mask_stroke) delete text_mask_stroke;
	if(char_positions) delete [] char_positions;
	if(rows_bottom) delete [] rows_bottom;
	clear_glyphs();
	if(glyph_engine) delete glyph_engine;
	if(title_engine) delete title_engine;
	if(freetype_library) FT_Done_FreeType(freetype_library);
	if(translate) delete translate;
}

char* TitleMain::plugin_title() { return "Title"; }
int TitleMain::is_realtime() { return 1; }
int TitleMain::is_synthesis() { return 1; }

VFrame* TitleMain::new_picon()
{
	return new VFrame(picon_png);
}


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
		char command_line[BCTEXTLEN];

		sprintf(command_line, 
			"find %s -name 'fonts.dir' -print -exec cat {} \\;", 
			search_path);
//printf("TitleMain::build_fonts %s\n", command_line);

		FILE *in = popen(command_line, "r");
		char current_dir[BCTEXTLEN];
		FT_Library freetype_library = 0;      	// Freetype library
		FT_Face freetype_face = 0;

//		FT_Init_FreeType(&freetype_library);
		current_dir[0] = 0;

		while(!feof(in))
		{
			char string[BCTEXTLEN], string2[BCTEXTLEN];
			fgets(string, BCTEXTLEN, in);
			if(!strlen(string)) break;

			char *in_ptr = string;
			char *out_ptr;

// Get current directory
			
			if(string[0] == '/')
			{
				out_ptr = current_dir;
				while(*in_ptr != 0 && *in_ptr != 0xa)
					*out_ptr++ = *in_ptr++;
				out_ptr--;
				while(*out_ptr != '/')
					*out_ptr-- = 0;
			}
			else
			{


//printf("TitleMain::build_fonts %s\n", string);
				FontEntry *entry = new FontEntry;

// Path
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != ' ' && *in_ptr != 0xa)
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				if(string2[0] == '/')
				{
					entry->path = new char[strlen(string2) + 1];
					sprintf(entry->path, "%s", string2);
				}
				else
				{
					entry->path = new char[strlen(current_dir) + strlen(string2) + 1];
					sprintf(entry->path, "%s%s", current_dir, string2);
				}

// Foundary
				while(*in_ptr != 0 && *in_ptr != 0xa && (*in_ptr == ' ' || *in_ptr == '-'))
					in_ptr++;

				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != ' ' && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->foundary = new char[strlen(string2) + 1];
				strcpy(entry->foundary, string2);
				if(*in_ptr == '-') in_ptr++;


// Family
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->family = new char[strlen(string2) + 1];
				strcpy(entry->family, string2);
				if(*in_ptr == '-') in_ptr++;

// Weight
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->weight = new char[strlen(string2) + 1];
				strcpy(entry->weight, string2);
				if(*in_ptr == '-') in_ptr++;

// Slant
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->slant = new char[strlen(string2) + 1];
				strcpy(entry->slant, string2);
				if(*in_ptr == '-') in_ptr++;

// SWidth
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->swidth = new char[strlen(string2) + 1];
				strcpy(entry->swidth, string2);
				if(*in_ptr == '-') in_ptr++;

// Adstyle
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->adstyle = new char[strlen(string2) + 1];
				strcpy(entry->adstyle, string2);
				if(*in_ptr == '-') in_ptr++;

// pixelsize
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->pixelsize = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// pointsize
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->pointsize = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// xres
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->xres = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// yres
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->yres = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// spacing
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->spacing = new char[strlen(string2) + 1];
				strcpy(entry->spacing, string2);
				if(*in_ptr == '-') in_ptr++;

// avg_width
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->avg_width = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// registry
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->registry = new char[strlen(string2) + 1];
				strcpy(entry->registry, string2);
				if(*in_ptr == '-') in_ptr++;

// encoding
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa)
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->encoding = new char[strlen(string2) + 1];
				strcpy(entry->encoding, string2);



// Add to list
				if(strlen(entry->foundary))
				{
//printf("TitleMain::build_fonts 1 %s\n", entry->path);
// This takes a real long time to do.  Instead just take all fonts
// 					if(!load_freetype_face(freetype_library, 
// 						freetype_face,
// 						entry->path))
//					if(1)
					if(entry->family[0])
					{
// Fix parameters
						sprintf(string, "%s (%s)", entry->family, entry->foundary);
						entry->fixed_title = new char[strlen(string) + 1];
						strcpy(entry->fixed_title, string);

						if(!strcasecmp(entry->weight, "demibold") ||
							!strcasecmp(entry->weight, "bold")) 
							entry->fixed_style |= FONT_BOLD;
						if(!strcasecmp(entry->slant, "i") ||
							!strcasecmp(entry->slant, "o")) 
							entry->fixed_style |= FONT_ITALIC;
						fonts->append(entry);
//						printf("TitleMain::build_fonts %s: success\n",
//							entry->path);
//printf("TitleMain::build_fonts 2\n");
					}
					else
					{
//						printf("TitleMain::build_fonts %s: FT_New_Face failed\n",
//							entry->path);
//printf("TitleMain::build_fonts 3\n");
						delete entry;
					}
				}
				else
				{
					delete entry;
				}
			}
		}
		pclose(in);

		if(freetype_library) FT_Done_FreeType(freetype_library);
	}


// for(int i = 0; i < fonts->total; i++)
//	fonts->values[i]->dump();


}

int TitleMain::load_freetype_face(FT_Library &freetype_library,
	FT_Face &freetype_face,
	char *path)
{
//printf("TitleMain::load_freetype_face 1\n");
	if(!freetype_library) FT_Init_FreeType(&freetype_library);
	if(freetype_face) FT_Done_Face(freetype_face);
	freetype_face = 0;
//printf("TitleMain::load_freetype_face 2\n");

// Use freetype's internal function for loading font
	if(FT_New_Face(freetype_library, 
		path, 
		0,
		&freetype_face))
	{
		fprintf(stderr, "TitleMain::load_freetype_face %s failed.\n");
		FT_Done_FreeType(freetype_library);
		freetype_face = 0;
		freetype_library = 0;
		return 1;
	} else
	{
		return 0;
	}

//printf("TitleMain::load_freetype_face 4\n");
}

FontEntry* TitleMain::get_font_entry(char *title,
	int style,
	int size)
{
//printf("TitleMain::get_font_entry %s %d %d\n", title, style, size);
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

//printf("TitleMain::get_char_advance 1 %c %c %p %p\n", current, next, current_glyph, next_glyph);
	if(next_glyph)
		FT_Get_Kerning(freetype_face, 
				current_glyph->freetype_index,
				next_glyph->freetype_index,
				ft_kerning_default,
				&kerning);
	else
		kerning.x = 0;
//printf("TitleMain::get_char_advance 2 %d %d\n", result, kerning.x);
	
	return result + (kerning.x >> 6);
}


void TitleMain::draw_glyphs()
{
// Build table of all glyphs needed
	int text_len = strlen(config.text);
	int total_packages = 0;
	iconv_t cd;
	cd = iconv_open ("UCS-4", config.encoding);


	if (cd == (iconv_t) -1)
	{
/* Something went wrong.  */
		fprintf (stderr, "Iconv conversion from %s to Unicode UCS-4 not available\n",config.encoding);
	};

	for(int i = 0; i < text_len; i++)
	{
		FT_ULong char_code;	
		int c = config.text[i];
		int exists = 0;

/* if iconv is working ok for current encoding */
		if (cd != (iconv_t) -1)
		{

			size_t inbytes,outbytes;
			char inbuf;
			char *inp = (char*)&inbuf, *outp = (char *)&char_code;

			inbuf = (char)c;
			inbytes = 1;
			outbytes = 4;
	
			iconv (cd, &inp, &inbytes, &outp, &outbytes);
#if     __BYTE_ORDER == __LITTLE_ENDIAN
			char_code = bswap_32(char_code);
#endif                          /* Big endian.  */

		}
		else 
		{
			char_code = c;
		}

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
//printf("TitleMain::draw_glyphs 1\n");
			TitleGlyph *glyph = new TitleGlyph;
//printf("TitleMain::draw_glyphs 2\n");
			glyphs.append(glyph);
			glyph->c = c;
			glyph->char_code = char_code;
		}
	}
	iconv_close(cd);

	if(!glyph_engine)
		glyph_engine = new GlyphEngine(this, PluginClient::smp + 1);

	glyph_engine->set_package_count(total_packages);
//printf("TitleMain::draw_glyphs 3 %d\n", glyphs.total);
	glyph_engine->process_packages();
//printf("TitleMain::draw_glyphs 4\n");
}

void TitleMain::get_total_extents()
{
// Determine extents of total text
	int current_w = 0;
	int row_start = 0;
	text_len = strlen(config.text);
	if(!char_positions) char_positions = new title_char_position_t[text_len];
	
	text_rows = 0;
	text_w = 0;
	ascent = 0;

	for(int i = 0; i < glyphs.total; i++)
		if(glyphs.values[i]->top > ascent) ascent = glyphs.values[i]->top;
//printf("TitleMain::get_total_extents %d\n", ascent);

	// get the number of rows first
	for(int i = 0; i < text_len; i++)
	{
		if(config.text[i] == 0xa || i == text_len - 1)
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
		char_positions[i].w = get_char_advance(config.text[i], config.text[i + 1]);
		TitleGlyph *current_glyph = 0;
		for(int j = 0; j < glyphs.total; j++)
		{
			if(glyphs.values[j]->c == config.text[i])
			{
				current_glyph = glyphs.values[j];
				break;
			}
		}
		int current_bottom = current_glyph->top - current_glyph->height;
		if (current_bottom < rows_bottom[text_rows])
			rows_bottom[text_rows] = current_bottom ;

// printf("TitleMain::get_total_extents 1 %c %d %d %d\n", 
// 	config.text[i], 
// 	char_positions[i].x, 
// 	char_positions[i].y, 
// 	char_positions[i].w);
		current_w += char_positions[i].w;

		if(config.text[i] == 0xa || i == text_len - 1)
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
		if(config.text[i] == 0xa || i == text_len - 1)
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


//printf("TitleMain::get_total_extents 2 %d %d\n", text_w, text_h);
}

int TitleMain::draw_mask()
{
	int old_visible_row1 = visible_row1;
	int old_visible_row2 = visible_row2;


// Determine y of visible text
	if(config.motion_strategy == BOTTOM_TO_TOP)
	{
// printf("TitleMain::draw_mask 1 %d %d %d %d\n", 
// 	config.motion_strategy,
// 	get_source_position(), 
// 	get_source_start(),
// 	config.prev_keyframe_position);
		float magnitude = config.pixels_per_second * 
			((get_source_position() - get_source_start()) - 
				(config.prev_keyframe_position - get_source_start())) / 
			PluginVClient::project_frame_rate;
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
			(get_source_position() - 
				get_source_start() -
				config.prev_keyframe_position) / 
			PluginVClient::project_frame_rate;
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
			(get_source_position() - 
				get_source_start() - 
				config.prev_keyframe_position) / 
			PluginVClient::project_frame_rate;
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
			(get_source_position() - 
				get_source_start() - 
				config.prev_keyframe_position) / 
			PluginVClient::project_frame_rate;
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
		int fade_len = (int)(config.fade_in * PluginVClient::project_frame_rate);
		int fade_position = get_source_position() - 
/*			get_source_start() -   */
			config.prev_keyframe_position;


		if(fade_position >= 0 && fade_position < fade_len)
		{
			alpha = (int)((float)0x100 * 
				fade_position /
				fade_len + 0.5);
		}
	}

	if(!EQUIV(config.fade_out, 0))
	{
		int fade_len = (int)(config.fade_out * 
			PluginVClient::project_frame_rate);
		int fade_position = config.next_keyframe_position - 
			get_source_position();


		if(fade_position > 0 && fade_position < fade_len)
		{
			alpha = (int)((float)0x100 *
				fade_position /
				fade_len + 0.5);
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
//printf("TitleMain::clear_glyphs 1\n");
	glyphs.remove_all_objects();
}

char* TitleMain::motion_to_text(int motion)
{
	switch(motion)
	{
		case NO_MOTION: return "No motion"; break;
		case BOTTOM_TO_TOP: return "Bottom to top"; break;
		case TOP_TO_BOTTOM: return "Top to bottom"; break;
		case RIGHT_TO_LEFT: return "Right to left"; break;
		case LEFT_TO_RIGHT: return "Left to right"; break;
	}
}

int TitleMain::text_to_motion(char *text)
{
	for(int i = 0; i < TOTAL_PATHS; i++)
	{
		if(!strcasecmp(motion_to_text(i), text)) return i;
	}
	return 0;
}







int TitleMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int result = 0;
	input = input_ptr;
	output = output_ptr;

	need_reconfigure |= load_configuration();


// Always synthesize text and redraw it for timecode
	if(config.timecode)
	{
		Units::totext(config.text, 
				(double)get_source_position() / PluginVClient::project_frame_rate, 
				TIME_HMSF, 
				0,
				PluginVClient::project_frame_rate, 
				0);
		need_reconfigure = 1;
	}

// Check boundaries
	if(config.size <= 0 || config.size >= 2048) config.size = 72;
	if(config.stroke_width < 0 || 
		config.stroke_width >= 512) config.stroke_width = 0.0;
	if(!strlen(config.text)) return 0;
	if(!strlen(config.encoding)) strcpy(config.encoding, DEFAULT_ENCODING);

//printf("TitleMain::process_realtime 4\n");

// Handle reconfiguration
	if(need_reconfigure)
	{
//printf("TitleMain::process_realtime 2\n");
		if(text_mask) delete text_mask;
		if(text_mask_stroke) delete text_mask_stroke;
		text_mask = 0;
		text_mask_stroke = 0;
//printf("TitleMain::process_realtime 2\n");
		if(freetype_face) FT_Done_Face(freetype_face);
		freetype_face = 0;
//printf("TitleMain::process_realtime 2\n");
		if(glyph_engine) delete glyph_engine;
		glyph_engine = 0;
//printf("TitleMain::process_realtime 2\n");
		if(char_positions) delete [] char_positions;
		char_positions = 0;
		if(rows_bottom) delete [] rows_bottom;
		rows_bottom = 0;
//printf("TitleMain::process_realtime 2\n");
		clear_glyphs();
//printf("TitleMain::process_realtime 2\n");
		visible_row1 = 0;
		visible_row2 = 0;
		ascent = 0;

		if(!freetype_library) 
			FT_Init_FreeType(&freetype_library);

//printf("TitleMain::process_realtime 2\n");
		if(!freetype_face)
		{
			FontEntry *font = get_font();
//printf("TitleMain::process_realtime 2.1 %s\n", font->path);
			if(load_freetype_face(freetype_library,
				freetype_face,
				font->path))
			{
				printf("TitleMain::process_realtime %s: FT_New_Face failed.\n",
					font->fixed_title);
				result = 1;
			}
//printf("TitleMain::process_realtime 2.2\n");

			if(!result) FT_Set_Pixel_Sizes(freetype_face, config.size, 0);
//printf("TitleMain::process_realtime 2.3\n");
		}

//printf("TitleMain::process_realtime 3\n");

		if(!result)
		{
			draw_glyphs();
//printf("TitleMain::process_realtime 4\n");
			get_total_extents();
//printf("TitleMain::process_realtime 5\n");
			need_reconfigure = 0;
		}
	}

	if(!result)
	{
//printf("TitleMain::process_realtime 4\n");
// Determine region of text visible on the output and draw mask
		result = draw_mask();
	}
//printf("TitleMain::process_realtime 50\n");


// Overlay mask on output
	if(!result)
	{
		overlay_mask();
	}
//printf("TitleMain::process_realtime 60 %d\n", glyphs.total);

	return 0;
}

int TitleMain::show_gui()
{
	load_configuration();
	thread = new TitleThread(this);
	thread->start();
	return 0;
}

int TitleMain::set_string()
{
	if(thread) thread->window->set_title(gui_string);
	return 0;
}

void TitleMain::raise_window()
{
	if(thread)
	{
		thread->window->raise_window();
		thread->window->flush();
	}
}

void TitleMain::update_gui()
{
	if(thread)
	{
		int reconfigure = load_configuration();
		if(reconfigure)
		{
			thread->window->lock_window();
			thread->window->update();
			thread->window->unlock_window();
		}
	}
}


int TitleMain::load_defaults()
{
	char directory[1024], text_path[1024];
// set the default directory
	sprintf(directory, "%stitle.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	defaults->get("FONT", config.font);
	defaults->get("ENCODING", config.encoding);
	config.style = defaults->get("STYLE", (int64_t)config.style);
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
	window_w = defaults->get("WINDOW_W", 660);
	window_h = defaults->get("WINDOW_H", 480);

// Store text in separate path to isolate special characters
	FileSystem fs;
	sprintf(text_path, "%stitle_text.rc", BCASTDIR);
	fs.complete_path(text_path);
	FILE *fd = fopen(text_path, "rb");
	if(fd)
	{
		fseek(fd, 0, SEEK_END);
		int64_t len = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		fread(config.text, len, 1, fd);
		config.text[len] = 0;
//printf("TitleMain::load_defaults %s\n", config.text);
		fclose(fd);
	}
	else
		config.text[0] = 0;
	return 0;
}

int TitleMain::save_defaults()
{
	char text_path[1024];

	defaults->update("FONT", config.font);
	defaults->update("ENCODING", config.encoding);
	defaults->update("STYLE", (int64_t)config.style);
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
	defaults->update("WINDOW_W", window_w);
	defaults->update("WINDOW_H", window_h);
	defaults->save();

// Store text in separate path to isolate special characters
	FileSystem fs;
	sprintf(text_path, "%stitle_text.rc", BCASTDIR);
	fs.complete_path(text_path);
	FILE *fd = fopen(text_path, "wb");
	if(fd)
	{
		fwrite(config.text, strlen(config.text), 1, fd);
		fclose(fd);
	}
//	else
//		perror("TitleMain::save_defaults");
	return 0;
}




int TitleMain::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	next_keyframe = get_next_keyframe(get_source_position());

// printf("TitleMain::load_configuration 1 %d %d\n", 
// prev_keyframe->position,
// next_keyframe->position);

	TitleConfig old_config, prev_config, next_config;
	old_config.copy_from(config);
	read_data(prev_keyframe);
	prev_config.copy_from(config);
	read_data(next_keyframe);
	next_config.copy_from(config);

	config.prev_keyframe_position = prev_keyframe->position;
	config.next_keyframe_position = next_keyframe->position;
	if(config.next_keyframe_position == config.prev_keyframe_position)
		config.next_keyframe_position = get_source_start() + get_total_len();


// printf("TitleMain::load_configuration 10 %d %d\n", 
// config.prev_keyframe_position,
// config.next_keyframe_position);

	config.interpolate(prev_config, 
		next_config, 
		(next_keyframe->position == prev_keyframe->position) ?
			get_source_position() :
			prev_keyframe->position,
		(next_keyframe->position == prev_keyframe->position) ?
			get_source_position() + 1 :
			next_keyframe->position,
		get_source_position());

	if(!config.equivalent(old_config))
		return 1;
	else
		return 0;
}












void TitleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
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
	output.append_tag();
	output.append_newline();
	
	output.append_text(config.text);

	output.tag.set_title("/TITLE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
//printf("TitleMain::save_data 1\n%s\n", output.string);
//printf("TitleMain::save_data 2\n%s\n", config.text);
}

void TitleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	int new_interlace = 0;
	int new_horizontal = 0;
	int new_luminance = 0;

	config.prev_keyframe_position = keyframe->position;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
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
				strcpy(config.text, input.read_text());
//printf("TitleMain::read_data 1\n%s\n", input.string);
//printf("TitleMain::read_data 2\n%s\n", config.text);
			}
			else
			if(input.tag.title_is("/TITLE"))
			{
				result = 1;
			}
		}
	}
}






