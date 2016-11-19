/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */



#include "bcsignals.h"
#include "colormodels.h"
#include <stdlib.h>
#include <string.h>

// Compression coefficients straight out of jpeglib
#define R_TO_Y    0.29900
#define G_TO_Y    0.58700
#define B_TO_Y    0.11400

#define R_TO_U    -0.16874
#define G_TO_U    -0.33126
#define B_TO_U    0.50000

#define R_TO_V    0.50000
#define G_TO_V    -0.41869
#define B_TO_V    -0.08131

// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200

struct yuvtables ColorModels::yuv_table = {0};


ColorModels::ColorModels()
{
	int i;
	int *vtor, *vtog, *utog, *utob;
	short int *vtor8, *vtog8, *utog8, *utob8;
	float *vtor_float, *vtog_float, *utog_float, *utob_float;
	int *vtor16, *vtog16, *utog16, *utob16;
	float *v16tor_float, *v16tog_float, *u16tog_float, *u16tob_float;
// compression
	for(i = 0; i < 0x100; i++)
	{
		yuv_table.rtoy_tab[i] = (int)(R_TO_Y * 0x10000 * i);
		yuv_table.rtou_tab[i] = (int)(R_TO_U * 0x10000 * i);
		yuv_table.rtov_tab[i] = (int)(R_TO_V * 0x10000 * i);

		yuv_table.gtoy_tab[i] = (int)(G_TO_Y * 0x10000 * i);
		yuv_table.gtou_tab[i] = (int)(G_TO_U * 0x10000 * i);
		yuv_table.gtov_tab[i] = (int)(G_TO_V * 0x10000 * i);

		yuv_table.btoy_tab[i] = (int)(B_TO_Y * 0x10000 * i);
		yuv_table.btou_tab[i] = (int)(B_TO_U * 0x10000 * i) + 0x800000;
		yuv_table.btov_tab[i] = (int)(B_TO_V * 0x10000 * i) + 0x800000;
	}

// compression
	for(i = 0; i < 0x10000; i++)
	{
		yuv_table.rtoy_tab16[i] = (int)(R_TO_Y * 0x100 * i);
		yuv_table.rtou_tab16[i] = (int)(R_TO_U * 0x100 * i);
		yuv_table.rtov_tab16[i] = (int)(R_TO_V * 0x100 * i);

		yuv_table.gtoy_tab16[i] = (int)(G_TO_Y * 0x100 * i);
		yuv_table.gtou_tab16[i] = (int)(G_TO_U * 0x100 * i);
		yuv_table.gtov_tab16[i] = (int)(G_TO_V * 0x100 * i);

		yuv_table.btoy_tab16[i] = (int)(B_TO_Y * 0x100 * i);
		yuv_table.btou_tab16[i] = (int)(B_TO_U * 0x100 * i) + 0x800000;
		yuv_table.btov_tab16[i] = (int)(B_TO_V * 0x100 * i) + 0x800000;
	}

// decompression
	vtor = &(yuv_table.vtor_tab[0x80]);
	vtog = &(yuv_table.vtog_tab[0x80]);
	utog = &(yuv_table.utog_tab[0x80]);
	utob = &(yuv_table.utob_tab[0x80]);
	vtor8 = &(yuv_table.vtor_tab8[0x80]);
	vtog8 = &(yuv_table.vtog_tab8[0x80]);
	utog8 = &(yuv_table.utog_tab8[0x80]);
	utob8 = &(yuv_table.utob_tab8[0x80]);

	for(i = -0x80; i < 0x80; i++)
	{
		vtor[i] = (int)(V_TO_R * 0x10000 * i);
		vtog[i] = (int)(V_TO_G * 0x10000 * i);

		utog[i] = (int)(U_TO_G * 0x10000 * i);
		utob[i] = (int)(U_TO_B * 0x10000 * i);

		vtor8[i] = (int)(V_TO_R * i);
		vtog8[i] = (int)(V_TO_G * i);

		utog8[i] = (int)(U_TO_G * i);
		utob8[i] = (int)(U_TO_B * i);
	}

// decompression
	vtor_float = &(yuv_table.vtor_float_tab[0x80]);
	vtog_float = &(yuv_table.vtog_float_tab[0x80]);
	utog_float = &(yuv_table.utog_float_tab[0x80]);
	utob_float = &(yuv_table.utob_float_tab[0x80]);

	for(i = -0x80; i < 0x80; i++)
	{
		vtor_float[i] = V_TO_R * i / 0xff;
		vtog_float[i] = V_TO_G * i / 0xff;

		utog_float[i] = U_TO_G * i / 0xff;
		utob_float[i] = U_TO_B * i / 0xff;
	}

// decompression
	vtor16 = &(yuv_table.vtor_tab16[0x8000]);
	vtog16 = &(yuv_table.vtog_tab16[0x8000]);
	utog16 = &(yuv_table.utog_tab16[0x8000]);
	utob16 = &(yuv_table.utob_tab16[0x8000]);

	for(i = -0x8000; i < 0x8000; i++)
	{
		vtor16[i] = (int)(V_TO_R * 0x100 * i);
		vtog16[i] = (int)(V_TO_G * 0x100 * i);

		utog16[i] = (int)(U_TO_G * 0x100 * i);
		utob16[i] = (int)(U_TO_B * 0x100 * i);
	}

// decompression
	v16tor_float = &(yuv_table.v16tor_float_tab[0x8000]);
	v16tog_float = &(yuv_table.v16tog_float_tab[0x8000]);
	u16tog_float = &(yuv_table.u16tog_float_tab[0x8000]);
	u16tob_float = &(yuv_table.u16tob_float_tab[0x8000]);

	for(i = -0x8000; i < 0x8000; i++)
	{
		v16tor_float[i] = V_TO_R * i / 0xffff;
		v16tog_float[i] = V_TO_G * i / 0xffff;

		u16tog_float[i] = U_TO_G * i / 0xffff;
		u16tob_float[i] = U_TO_B * i / 0xffff;
	}
}


int ColorModels::is_planar(int colormodel)
{
	switch(colormodel)
	{
	case BC_YUV420P:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
		return 1;
	}
	return 0;
}

int ColorModels::components(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_A16:
	case BC_A_FLOAT:
		return 1;

	case BC_RGB888:
	case BC_YUV888:
	case BC_RGB161616:
	case BC_YUV161616:
	case BC_RGB_FLOAT:
		return 3;

	case BC_RGBA8888:
	case BC_YUVA8888:
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_RGBA_FLOAT:
		return 4;
	}
	return 0;
}

int ColorModels::calculate_pixelsize(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_RGB8:
	case BC_TRANSPARENCY:
	case BC_COMPRESSED:
	case BC_YUV420P:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
		return 1;

	case BC_A16:
	case BC_RGB565:
	case BC_BGR565:
	case BC_YUV422:
		return 2;

	case BC_BGR888:
	case BC_RGB888:
	case BC_YUV888:
		return 3;

	case BC_A_FLOAT:
	case BC_BGR8888:
	case BC_ARGB8888:
	case BC_ABGR8888:
	case BC_RGBA8888:
	case BC_YUVA8888:
		return 4;

	case BC_RGB161616:
	case BC_YUV161616:
		return 6;

	case BC_RGBA16161616:
	case BC_YUVA16161616:
		return 8;

	case BC_RGB_FLOAT:
		return 12;

	case BC_RGBA_FLOAT:
		return 16;
	}
	return 0;
}

int ColorModels::calculate_max(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_RGB888:
	case BC_RGBA8888:
	case BC_YUV888:
	case BC_YUVA8888:
		return 0xff;

	case BC_A16:
	case BC_RGB161616:
	case BC_RGBA16161616:
	case BC_YUV161616:
	case BC_YUVA16161616:
		return 0xffff;

	case BC_A_FLOAT:
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		return 1;
	}
	return 0;
}

int ColorModels::calculate_datasize(int w, int h, int bytes_per_line, int color_model)
{
	if(bytes_per_line < 0)
		bytes_per_line = w * calculate_pixelsize(color_model);

	switch(color_model)
	{
	case BC_YUV420P:
	case BC_YUV411P:
		return w * h + w * h / 2 + 4;

	case BC_YUV422P:
		return w * h * 2 + 4;

	case BC_YUV444P:
		return w * h * 3 + 4;

	default:
		return h * bytes_per_line + 4;
	}
	return 0;
}

static void get_scale_tables(int **column_table,
	int **row_table,
	int in_x1,
	int in_y1,
	int in_x2,
	int in_y2,
	int out_x1,
	int out_y1,
	int out_x2,
	int out_y2)
{
	int y_out, i;
	float w_in = in_x2 - in_x1;
	float h_in = in_y2 - in_y1;
	int w_out = out_x2 - out_x1;
	int h_out = out_y2 - out_y1;

	float hscale = w_in / w_out;
	float vscale = h_in / h_out;

/* + 1 so we don't overflow when calculating in advance */
	*column_table = (int*)malloc(sizeof(int) * (w_out + 1));
	*row_table = (int*)malloc(sizeof(int) * h_out);

	for(i = 0; i < w_out; i++)
		(*column_table)[i] = (int)(hscale * i) + in_x1;

	for(i = 0; i < h_out; i++)
		(*row_table)[i] = (int)(vscale * i) + in_y1;
}

void ColorModels::transfer(unsigned char **output_rows,
	unsigned char **input_rows,
	unsigned char *out_y_plane,
	unsigned char *out_u_plane,
	unsigned char *out_v_plane,
	unsigned char *in_y_plane,
	unsigned char *in_u_plane,
	unsigned char *in_v_plane,
	int in_x,
	int in_y,
	int in_w,
	int in_h,
	int out_x,
	int out_y,
	int out_w,
	int out_h,
	int in_colormodel,
	int out_colormodel,
	int bg_color,
	int in_rowspan,
	int out_rowspan)
{
	int *column_table;
	int *row_table;
	int scale;
	int bg_r, bg_g, bg_b;
	int in_pixelsize = calculate_pixelsize(in_colormodel);
	int out_pixelsize = calculate_pixelsize(out_colormodel);

	bg_r = (bg_color & 0xff0000) >> 16;
	bg_g = (bg_color & 0xff00) >> 8;
	bg_b = (bg_color & 0xff);

// Get scaling
	scale = (out_w != in_w) || (in_x != 0);
	get_scale_tables(&column_table, &row_table,
		in_x, in_y, in_x + in_w, in_y + in_h,
		out_x, out_y, out_x + out_w, out_y + out_h);

#define PERMUTATION_VALUES \
	output_rows,  \
	input_rows, \
	out_y_plane, \
	out_u_plane, \
	out_v_plane, \
	in_y_plane, \
	in_u_plane, \
	in_v_plane, \
	in_x,  \
	in_y,  \
	in_w,  \
	in_h, \
	out_x,  \
	out_y,  \
	out_w,  \
	out_h, \
	in_colormodel,  \
	out_colormodel, \
	bg_color, \
	in_rowspan, \
	out_rowspan, \
	scale, \
	out_pixelsize, \
	in_pixelsize, \
	row_table, \
	column_table, \
	bg_r, \
	bg_g, \
	bg_b

// Handle planar cmodels separately
	switch(in_colormodel)
	{
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		float_cmodel(PERMUTATION_VALUES);
		break;

	case BC_YUV420P:
	case BC_YUV422P:
		yuv420p_cmodel(PERMUTATION_VALUES);
		break;

	case BC_YUV444P:
		yuv444p_cmodel(PERMUTATION_VALUES);
		break;

	case BC_YUV422:
		yuv422_cmodel(PERMUTATION_VALUES);
		break;

	default:
		default_cmodel(PERMUTATION_VALUES);
		break;
	}

	free(column_table);
	free(row_table);
}

void ColorModels::to_text(char *string, int cmodel)
{
	strcpy(string, name(cmodel));
}

const char *ColorModels::name(int cmodel)
{
	switch(cmodel)
	{
	case BC_RGB888:
		return "RGB-8 Bit";
	case BC_RGBA8888:
		return "RGBA-8 Bit";
	case BC_RGB161616:
		return "RGB-16 Bit";
	case BC_RGBA16161616:
		return "RGBA-16 Bit";
	case BC_BGR8888:
		return "BGRA-8 Bit";
	case BC_YUV888:
		return "YUV-8 Bit";
	case BC_YUVA8888:
		return "YUVA-8 Bit";
	case BC_YUV161616:
		return "YUV-16 Bit";
	case BC_YUVA16161616:
		return "YUVA-16 Bit";
	case BC_RGB_FLOAT:
		return "RGB-FLOAT";
	case BC_RGBA_FLOAT:
		return "RGBA-FLOAT";
	case BC_YUV420P:
		return "YUV420P";
	case BC_YUV422P:
		return "YUV422P";
	case BC_YUV444P:
		return "YUV444P";
	case BC_YUV422:
		return "YUV422";
	}
	return "Unknown";
}

int ColorModels::from_text(char *text)
{
	if(!strcasecmp(text, "RGB-8 Bit"))
		return BC_RGB888;
	if(!strcasecmp(text, "RGBA-8 Bit"))
		return BC_RGBA8888;
	if(!strcasecmp(text, "RGB-16 Bit"))
		return BC_RGB161616;
	if(!strcasecmp(text, "RGBA-16 Bit"))
		return BC_RGBA16161616;
	if(!strcasecmp(text, "RGB-FLOAT"))
		return BC_RGB_FLOAT;
	if(!strcasecmp(text, "RGBA-FLOAT"))
		return BC_RGBA_FLOAT;
	if(!strcasecmp(text, "YUV-8 Bit"))
		return BC_YUV888;
	if(!strcasecmp(text, "YUVA-8 Bit"))
		return BC_YUVA8888;
	if(!strcasecmp(text, "YUV-16 Bit"))
		return BC_YUV161616;
	if(!strcasecmp(text, "YUVA-16 Bit"))
		return BC_YUVA16161616;
	return BC_RGB888;
}

int ColorModels::is_yuv(int colormodel)
{
	switch(colormodel)
	{
	case BC_YUV888:
	case BC_YUVA8888:
	case BC_YUV161616:
	case BC_YUVA16161616:
	case BC_YUV422:
	case BC_YUV420P:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
		return 1;
	}
	return 0;
}

int ColorModels::has_alpha(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_A16:
	case BC_A_FLOAT:
	case BC_RGBA8888:
	case BC_RGBA16161616:
	case BC_YUVA8888:
	case BC_YUVA16161616:
	case BC_RGBA_FLOAT:
		return 1;
	}
	return 0;
}
