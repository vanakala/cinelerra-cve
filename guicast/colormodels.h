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
 
 
#ifndef COLORMODELS_H
#define COLORMODELS_H

#include "colormodels.inc"


#define PERMUTATION_ARGS \
	unsigned char **output_rows,  \
	unsigned char **input_rows, \
	unsigned char *out_y_plane, \
	unsigned char *out_u_plane, \
	unsigned char *out_v_plane, \
	unsigned char *in_y_plane, \
	unsigned char *in_u_plane, \
	unsigned char *in_v_plane, \
	int in_x,  \
	int in_y,  \
	int in_w,  \
	int in_h, \
	int out_x,  \
	int out_y,  \
	int out_w,  \
	int out_h, \
	int in_colormodel,  \
	int out_colormodel, \
	int bg_color, \
	int total_in_w, \
	int total_out_w, \
	int scale, \
	int out_pixelsize, \
	int in_pixelsize, \
	int *row_table, \
	int *column_table, \
	int bg_r, \
	int bg_g, \
	int bg_b

	struct yuvtables
	{
		int rtoy_tab[0x100], gtoy_tab[0x100], btoy_tab[0x100];
		int rtou_tab[0x100], gtou_tab[0x100], btou_tab[0x100];
		int rtov_tab[0x100], gtov_tab[0x100], btov_tab[0x100];

		int vtor_tab[0x100], vtog_tab[0x100];
		int utog_tab[0x100], utob_tab[0x100];

		short int vtor_tab8[0x100], vtog_tab8[0x100];
		short int utog_tab8[0x100], utob_tab8[0x100];

		float vtor_float_tab[0x100], vtog_float_tab[0x100];
		float utog_float_tab[0x100], utob_float_tab[0x100];

		int rtoy_tab16[0x10000], gtoy_tab16[0x10000], btoy_tab16[0x10000];
		int rtou_tab16[0x10000], gtou_tab16[0x10000], btou_tab16[0x10000];
		int rtov_tab16[0x10000], gtov_tab16[0x10000], btov_tab16[0x10000];

		int vtor_tab16[0x10000], vtog_tab16[0x10000];
		int utog_tab16[0x10000], utob_tab16[0x10000];

		float v16tor_float_tab[0x10000], v16tog_float_tab[0x10000];
		float u16tog_float_tab[0x10000], u16tob_float_tab[0x10000];
	};

class ColorModels
{
public:
	ColorModels();

	static int calculate_pixelsize(int colormodel);
	static int calculate_datasize(int w, int h,
		int bytes_per_line, int color_model);
	static int calculate_max(int colormodel);
	static int components(int colormodel);
	static int is_yuv(int colormodel);
	static int has_alpha(int colormodel);

	// Tell when to use plane arguments or row pointer arguments to functions
	static int is_planar(int color_model);
	static void to_text(char *string, int cmodel);
	static const char *name(int cmodel);
	static int from_text(char *text);

	static void transfer(unsigned char **output_rows, // Leave NULL if non existent
		unsigned char **input_rows,
		unsigned char *out_y_plane, // Leave NULL if non existent
		unsigned char *out_u_plane,
		unsigned char *out_v_plane,
		unsigned char *in_y_plane, // Leave NULL if non existent
		unsigned char *in_u_plane,
		unsigned char *in_v_plane,
		int in_x,        // Dimensions to capture from input frame
		int in_y,
		int in_w,
		int in_h,
		int out_x,       // Dimensions to project on output frame
		int out_y,
		int out_w, 
		int out_h,
		int in_colormodel,
		int out_colormodel,
		int bg_color,         // When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex
		int in_rowspan,       // For planar use the luma rowspan
		int out_rowspan);     // For planar use the luma rowspan

	static struct yuvtables yuv_table;

private:
	static void default_cmodel(PERMUTATION_ARGS);
	static void float_cmodel(PERMUTATION_ARGS);
	static void yuv420p_cmodel(PERMUTATION_ARGS);
	static void yuv444p_cmodel(PERMUTATION_ARGS);
	static void yuv422_cmodel(PERMUTATION_ARGS);
};

#endif
