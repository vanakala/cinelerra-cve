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



#include "colormodels.h"
#include <stdlib.h>
#include <string.h>


cmodel_yuv_t *yuv_table = 0;

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






void cmodel_init_yuv(cmodel_yuv_t *yuv_table)
{
	int i;

/* compression */
	for(i = 0; i < 0x100; i++)
	{
		yuv_table->rtoy_tab[i] = (int)(R_TO_Y * 0x10000 * i);
		yuv_table->rtou_tab[i] = (int)(R_TO_U * 0x10000 * i);
		yuv_table->rtov_tab[i] = (int)(R_TO_V * 0x10000 * i);

		yuv_table->gtoy_tab[i] = (int)(G_TO_Y * 0x10000 * i);
		yuv_table->gtou_tab[i] = (int)(G_TO_U * 0x10000 * i);
		yuv_table->gtov_tab[i] = (int)(G_TO_V * 0x10000 * i);

		yuv_table->btoy_tab[i] = (int)(B_TO_Y * 0x10000 * i);
		yuv_table->btou_tab[i] = (int)(B_TO_U * 0x10000 * i) + 0x800000;
		yuv_table->btov_tab[i] = (int)(B_TO_V * 0x10000 * i) + 0x800000;
	}

/* compression */
	for(i = 0; i < 0x10000; i++)
	{
		yuv_table->rtoy_tab16[i] = (int)(R_TO_Y * 0x100 * i);
		yuv_table->rtou_tab16[i] = (int)(R_TO_U * 0x100 * i);
		yuv_table->rtov_tab16[i] = (int)(R_TO_V * 0x100 * i);

		yuv_table->gtoy_tab16[i] = (int)(G_TO_Y * 0x100 * i);
		yuv_table->gtou_tab16[i] = (int)(G_TO_U * 0x100 * i);
		yuv_table->gtov_tab16[i] = (int)(G_TO_V * 0x100 * i);

		yuv_table->btoy_tab16[i] = (int)(B_TO_Y * 0x100 * i);
		yuv_table->btou_tab16[i] = (int)(B_TO_U * 0x100 * i) + 0x800000;
		yuv_table->btov_tab16[i] = (int)(B_TO_V * 0x100 * i) + 0x800000;
	}




/* decompression */
	yuv_table->vtor = &(yuv_table->vtor_tab[0x80]);
	yuv_table->vtog = &(yuv_table->vtog_tab[0x80]);
	yuv_table->utog = &(yuv_table->utog_tab[0x80]);
	yuv_table->utob = &(yuv_table->utob_tab[0x80]);
	yuv_table->vtor8 = &(yuv_table->vtor_tab8[0x80]);
	yuv_table->vtog8 = &(yuv_table->vtog_tab8[0x80]);
	yuv_table->utog8 = &(yuv_table->utog_tab8[0x80]);
	yuv_table->utob8 = &(yuv_table->utob_tab8[0x80]);
	for(i = -0x80; i < 0x80; i++)
	{
		yuv_table->vtor[i] = (int)(V_TO_R * 0x10000 * i);
		yuv_table->vtog[i] = (int)(V_TO_G * 0x10000 * i);

		yuv_table->utog[i] = (int)(U_TO_G * 0x10000 * i);
		yuv_table->utob[i] = (int)(U_TO_B * 0x10000 * i);

		yuv_table->vtor8[i] = (int)(V_TO_R * i);
		yuv_table->vtog8[i] = (int)(V_TO_G * i);

		yuv_table->utog8[i] = (int)(U_TO_G * i);
		yuv_table->utob8[i] = (int)(U_TO_B * i);
	}


/* decompression */
	yuv_table->vtor_float = &(yuv_table->vtor_float_tab[0x80]);
	yuv_table->vtog_float = &(yuv_table->vtog_float_tab[0x80]);
	yuv_table->utog_float = &(yuv_table->utog_float_tab[0x80]);
	yuv_table->utob_float = &(yuv_table->utob_float_tab[0x80]);
	for(i = -0x80; i < 0x80; i++)
	{
		yuv_table->vtor_float[i] = V_TO_R * i / 0xff;
		yuv_table->vtog_float[i] = V_TO_G * i / 0xff;

		yuv_table->utog_float[i] = U_TO_G * i / 0xff;
		yuv_table->utob_float[i] = U_TO_B * i / 0xff;
	}


/* decompression */
	yuv_table->vtor16 = &(yuv_table->vtor_tab16[0x8000]);
	yuv_table->vtog16 = &(yuv_table->vtog_tab16[0x8000]);
	yuv_table->utog16 = &(yuv_table->utog_tab16[0x8000]);
	yuv_table->utob16 = &(yuv_table->utob_tab16[0x8000]);
	for(i = -0x8000; i < 0x8000; i++)
	{
		yuv_table->vtor16[i] = (int)(V_TO_R * 0x100 * i);
		yuv_table->vtog16[i] = (int)(V_TO_G * 0x100 * i);

		yuv_table->utog16[i] = (int)(U_TO_G * 0x100 * i);
		yuv_table->utob16[i] = (int)(U_TO_B * 0x100 * i);
	}


/* decompression */
	yuv_table->v16tor_float = &(yuv_table->v16tor_float_tab[0x8000]);
	yuv_table->v16tog_float = &(yuv_table->v16tog_float_tab[0x8000]);
	yuv_table->u16tog_float = &(yuv_table->u16tog_float_tab[0x8000]);
	yuv_table->u16tob_float = &(yuv_table->u16tob_float_tab[0x8000]);
	for(i = -0x8000; i < 0x8000; i++)
	{
		yuv_table->v16tor_float[i] = V_TO_R * i / 0xffff;
		yuv_table->v16tog_float[i] = V_TO_G * i / 0xffff;

		yuv_table->u16tog_float[i] = U_TO_G * i / 0xffff;
		yuv_table->u16tob_float[i] = U_TO_B * i / 0xffff;
	}
}


void cmodel_delete_yuv(cmodel_yuv_t *yuv_table)
{
}

int cmodel_is_planar(int colormodel)
{
	switch(colormodel)
	{
		case BC_YUV420P:      return 1; break;
		case BC_YUV422P:      return 1; break;
		case BC_YUV444P:      return 1; break;
		case BC_YUV411P:      return 1; break;
	}
	return 0;
}

int cmodel_components(int colormodel)
{
	switch(colormodel)
	{
		case BC_A8:           return 1; break;
		case BC_A16:          return 1; break;
		case BC_A_FLOAT:      return 1; break;
		case BC_RGB888:       return 3; break;
		case BC_RGBA8888:     return 4; break;
		case BC_RGB161616:    return 3; break;
		case BC_RGBA16161616: return 4; break;
		case BC_YUV888:       return 3; break;
		case BC_YUVA8888:     return 4; break;
		case BC_YUV161616:    return 3; break;
		case BC_YUVA16161616: return 4; break;
		case BC_YUV101010:    return 3; break;
		case BC_RGB_FLOAT:    return 3; break;
		case BC_RGBA_FLOAT:   return 4; break;
	}
}

int cmodel_calculate_pixelsize(int colormodel)
{
	switch(colormodel)
	{
		case BC_A8:           return 1; break;
		case BC_A16:          return 2; break;
		case BC_A_FLOAT:      return 4; break;
		case BC_TRANSPARENCY: return 1; break;
		case BC_COMPRESSED:   return 1; break;
		case BC_RGB8:         return 1; break;
		case BC_RGB565:       return 2; break;
		case BC_BGR565:       return 2; break;
		case BC_BGR888:       return 3; break;
		case BC_BGR8888:      return 4; break;
// Working bitmaps are packed to simplify processing
		case BC_RGB888:       return 3; break;
		case BC_ARGB8888:     return 4; break;
		case BC_ABGR8888:     return 4; break;
		case BC_RGBA8888:     return 4; break;
		case BC_RGB161616:    return 6; break;
		case BC_RGBA16161616: return 8; break;
		case BC_YUV888:       return 3; break;
		case BC_YUVA8888:     return 4; break;
		case BC_YUV161616:    return 6; break;
		case BC_YUVA16161616: return 8; break;
		case BC_YUV101010:    return 4; break;
		case BC_VYU888:       return 3; break;
		case BC_UYVA8888:     return 4; break;
		case BC_RGB_FLOAT:    return 12; break;
		case BC_RGBA_FLOAT:   return 16; break;
// Planar
		case BC_YUV420P:      return 1; break;
		case BC_YUV422P:      return 1; break;
		case BC_YUV444P:      return 1; break;
		case BC_YUV422:       return 2; break;
		case BC_YUV411P:      return 1; break;
		case BC_YUV9P:        return 1; break;
	}
	return 0;
}

int cmodel_calculate_max(int colormodel)
{
	switch(colormodel)
	{
// Working bitmaps are packed to simplify processing
		case BC_A8:           return 0xff; break;
		case BC_A16:          return 0xffff; break;
		case BC_A_FLOAT:      return 1; break;
		case BC_RGB888:       return 0xff; break;
		case BC_RGBA8888:     return 0xff; break;
		case BC_RGB161616:    return 0xffff; break;
		case BC_RGBA16161616: return 0xffff; break;
		case BC_YUV888:       return 0xff; break;
		case BC_YUVA8888:     return 0xff; break;
		case BC_YUV161616:    return 0xffff; break;
		case BC_YUVA16161616: return 0xffff; break;
		case BC_RGB_FLOAT:    return 1; break;
		case BC_RGBA_FLOAT:   return 1; break;
	}
	return 0;
}

int cmodel_calculate_datasize(int w, int h, int bytes_per_line, int color_model)
{
	if(bytes_per_line < 0) bytes_per_line = w * 
		cmodel_calculate_pixelsize(color_model);
	switch(color_model)
	{
		case BC_YUV420P:
		case BC_YUV411P:
			return w * h + w * h / 2 + 4;
			break;

		case BC_YUV422P:
			return w * h * 2 + 4;
			break;

		case BC_YUV444P:
			return w * h * 3 + 4;
			break;

		default:
			return h * bytes_per_line + 4;
			break;
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
	(*column_table) = malloc(sizeof(int) * (w_out + 1));
	(*row_table) = malloc(sizeof(int) * h_out);
	for(i = 0; i < w_out; i++)
	{
		(*column_table)[i] = (int)(hscale * i) + in_x1;
	}

	for(i = 0; i < h_out; i++)
	{
		(*row_table)[i] = (int)(vscale * i) + in_y1;
//printf("get_scale_tables %d %d\n", (*row_table)[i], i);
	}
}

void cmodel_transfer(unsigned char **output_rows, 
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
	int in_pixelsize = cmodel_calculate_pixelsize(in_colormodel);
	int out_pixelsize = cmodel_calculate_pixelsize(out_colormodel);

	bg_r = (bg_color & 0xff0000) >> 16;
	bg_g = (bg_color & 0xff00) >> 8;
	bg_b = (bg_color & 0xff);

// Initialize tables
	if(yuv_table == 0)
	{
		yuv_table = calloc(1, sizeof(cmodel_yuv_t));
		cmodel_init_yuv(yuv_table);
	}

// Get scaling
	scale = (out_w != in_w) || (in_x != 0);
	get_scale_tables(&column_table, &row_table, 
		in_x, in_y, in_x + in_w, in_y + in_h,
		out_x, out_y, out_x + out_w, out_y + out_h);

/*
 * printf("cmodel_transfer 1 %d %d %d,%d %d,%d %d,%d %d,%d\n", 
 * in_colormodel, 
 * out_colormodel, 
 * out_x, 
 * out_y, 
 * out_w, 
 * out_h, 
 * in_x, 
 * in_y, 
 * in_w, 
 * in_h);
 */


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
			cmodel_float(PERMUTATION_VALUES);
			break;

		case BC_YUV420P:
		case BC_YUV422P:
			cmodel_yuv420p(PERMUTATION_VALUES);
			break;

		case BC_YUV9P:
			cmodel_yuv9p(PERMUTATION_VALUES);
			break;

		case BC_YUV444P:
			cmodel_yuv444p(PERMUTATION_VALUES);
			break;

		case BC_YUV422:
			cmodel_yuv422(PERMUTATION_VALUES);
			break;

		default:
			cmodel_default(PERMUTATION_VALUES);
			break;
	}

/*
 * printf("cmodel_transfer 100 %d %d\n", 
 * in_colormodel, 
 * out_colormodel);
 */

	free(column_table);
	free(row_table);
}

int cmodel_bc_to_x(int color_model)
{
	switch(color_model)
	{
		case BC_YUV420P:
			return FOURCC_YV12;
			break;
		case BC_YUV422:
			return FOURCC_YUV2;
			break;
	}
	return -1;
}

void cmodel_to_text(char *string, int cmodel)
{
	switch(cmodel)
	{
		case BC_RGB888:       strcpy(string, "RGB-8 Bit");   break;
		case BC_RGBA8888:     strcpy(string, "RGBA-8 Bit");  break;
		case BC_RGB161616:    strcpy(string, "RGB-16 Bit");  break;
		case BC_RGBA16161616: strcpy(string, "RGBA-16 Bit"); break;
		case BC_YUV888:       strcpy(string, "YUV-8 Bit");   break;
		case BC_YUVA8888:     strcpy(string, "YUVA-8 Bit");  break;
		case BC_YUV161616:    strcpy(string, "YUV-16 Bit");  break;
		case BC_YUVA16161616: strcpy(string, "YUVA-16 Bit"); break;
		case BC_RGB_FLOAT:    strcpy(string, "RGB-FLOAT");   break;
		case BC_RGBA_FLOAT:   strcpy(string, "RGBA-FLOAT");  break;
		default: strcpy(string, "RGB-8 Bit"); break;
	}
}

int cmodel_from_text(char *text)
{
	if(!strcasecmp(text, "RGB-8 Bit"))   return BC_RGB888;
	if(!strcasecmp(text, "RGBA-8 Bit"))  return BC_RGBA8888;
	if(!strcasecmp(text, "RGB-16 Bit"))  return BC_RGB161616;
	if(!strcasecmp(text, "RGBA-16 Bit")) return BC_RGBA16161616;
	if(!strcasecmp(text, "RGB-FLOAT"))   return BC_RGB_FLOAT;
	if(!strcasecmp(text, "RGBA-FLOAT"))  return BC_RGBA_FLOAT;
	if(!strcasecmp(text, "YUV-8 Bit"))   return BC_YUV888;
	if(!strcasecmp(text, "YUVA-8 Bit"))  return BC_YUVA8888;
	if(!strcasecmp(text, "YUV-16 Bit"))  return BC_YUV161616;
	if(!strcasecmp(text, "YUVA-16 Bit")) return BC_YUVA16161616;
	return BC_RGB888;
}

int cmodel_is_yuv(int colormodel)
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
			break;
		
		default:
			return 0;
			break;
	}
}

int cmodel_has_alpha(int colormodel)
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
			break;

		default:
			return 0;
			break;
	}
}




