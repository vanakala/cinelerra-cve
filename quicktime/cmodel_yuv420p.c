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



#include "cmodel_permutation.h"




static inline void transfer_YUV_PLANAR_to_RGB8(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v, r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_YUV_PLANAR_to_BGR565(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV_PLANAR_to_RGB565(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV_PLANAR_to_BGR888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void transfer_YUV_PLANAR_to_BGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}

static inline void transfer_YUV_PLANAR_to_RGB888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v, r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV_PLANAR_to_ARGB8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v, r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = 0xff;
	(*output)[1] = r;
	(*output)[2] = g;
	(*output)[3] = b;
	(*output) += 4;
}

static inline void transfer_YUV_PLANAR_to_ABGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v, r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = 0xff;
	(*output)[3] = r;
	(*output)[2] = g;
	(*output)[1] = b;
	(*output) += 4;
}

static inline void transfer_YUV_PLANAR_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV_PLANAR_to_RGB161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = (*input_u << 8) | *input_u;
	v = (*input_v << 8) | *input_v;
	YUV_TO_RGB16(y, u, v, r, g, b)
	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;

	(*output) += 3;
}


static inline void transfer_YUV_PLANAR_to_RGBA16161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = (*input_u << 8) | *input_u;
	v = (*input_v << 8) | *input_v;
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xffff;

	(*output) += 4;
}


static inline void transfer_YUV_PLANAR_to_RGB_FLOAT(float* *output, 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	float y = (float)*input_y / 0xff;
	int u, v;
	float r, g, b;
	u = *input_u;
	v = *input_v;
	YUV_TO_FLOAT(y, u, v, r, g, b)

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}


static inline void transfer_YUV_PLANAR_to_RGBA_FLOAT(float* *output, 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	float y = (float)*input_y / 0xff;
	int u, v;
	float r, g, b;
	u = *input_u;
	v = *input_v;
	YUV_TO_FLOAT(y, u, v, r, g, b)

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 1.0;
}



static inline void transfer_YUV_PLANAR_to_YUV888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = *input_y;
	(*output)[1] = *input_u;
	(*output)[2] = *input_v;
	(*output) += 3;
}

static inline void transfer_YUV_PLANAR_to_YUV161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = (*input_y << 8) | *input_y;
	(*output)[1] = (*input_u << 8) | *input_u;
	(*output)[2] = (*input_v << 8) | *input_v;
	(*output) += 3;
}

static inline void transfer_YUV_PLANAR_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = *input_y;
	(*output)[1] = *input_u;
	(*output)[2] = *input_v;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV_PLANAR_to_YUVA16161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = (((uint16_t)*input_y) << 8) | *input_y;
	(*output)[1] = (((uint16_t)*input_u) << 8) | *input_u;
	(*output)[2] = (((uint16_t)*input_v) << 8) | *input_v;

	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_YUV_PLANAR_to_YUV420P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j / 2] = *input_u;
	output_v[j / 2] = *input_v;
}

static inline void transfer_YUV_PLANAR_to_YUV444P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j] = *input_u;
	output_v[j] = *input_v;
}

static inline void transfer_YUV_PLANAR_to_YUV422(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[1] = *input_u;
		(*output)[3] = *input_v;
		(*output)[0] = *input_y;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = *input_y;
		(*output) += 4;
	}
}















// ******************************** YUV444P -> ********************************

static inline void transfer_YUV444P_to_YUV444P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j] = *input_u;
	output_v[j] = *input_v;
}




#define TRANSFER_FRAME_DEFAULT(output, \
	input, \
	y_in_offset, \
	u_in_offset, \
	v_in_offset, \
	input_column) \
{ \
	register int i, j; \
 \
	switch(in_colormodel) \
	{ \
		case BC_YUV420P: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB8((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_out_w; \
						unsigned char *output_u = out_u_plane + i / 2 * total_out_w / 2; \
						unsigned char *output_v = out_v_plane + i / 2 * total_out_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] / 2 * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] / 2 * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_out_w; \
						unsigned char *output_u = out_u_plane + i * total_out_w / 2; \
						unsigned char *output_v = out_v_plane + i * total_out_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] / 2 * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] / 2 * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_out_w; \
						unsigned char *output_u = out_u_plane + i * total_out_w; \
						unsigned char *output_v = out_v_plane + i * total_out_w; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] / 2 * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] / 2 * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_ARGB8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ABGR8888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_ABGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUV9P: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB8((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_out_w; \
						unsigned char *output_u = out_u_plane + i / 2 * total_out_w / 2; \
						unsigned char *output_v = out_v_plane + i / 2 * total_out_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] / 2 * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] / 2 * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_out_w; \
						unsigned char *output_u = out_u_plane + i * total_out_w / 2; \
						unsigned char *output_v = out_v_plane + i * total_out_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] / 2 * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] / 2 * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_out_w; \
						unsigned char *output_u = out_u_plane + i * total_out_w; \
						unsigned char *output_v = out_v_plane + i * total_out_w; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] / 2 * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] / 2 * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_ARGB8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ABGR8888:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_ABGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT:      \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV9P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUV422P: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB8((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_ARGB8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ABGR8888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_ABGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_in_w; \
						unsigned char *output_u = out_u_plane + i / 2 * total_in_w / 2; \
						unsigned char *output_v = out_v_plane + i / 2 * total_in_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV422_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_in_w; \
						unsigned char *output_u = out_u_plane + i * total_in_w / 2; \
						unsigned char *output_v = out_v_plane + i * total_in_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_in_w; \
						unsigned char *output_u = out_u_plane + i * total_in_w; \
						unsigned char *output_v = out_v_plane + i * total_in_w; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] * total_in_w / 2; \
						unsigned char *input_v = in_v_plane + row_table[i] * total_in_w / 2; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
			} \
			break; \
 \
 \
		case BC_YUV444P: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB8((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_ARGB8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ABGR8888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_ABGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGB_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_RGBA_FLOAT((float**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUVA16161616((uint16_t**)(output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_in_w; \
						unsigned char *output_u = out_u_plane + i / 2 * total_in_w / 2; \
						unsigned char *output_v = out_v_plane + i / 2 * total_in_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] * total_in_w; \
						unsigned char *input_v = in_v_plane + row_table[i] * total_in_w; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV_PLANAR_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_in_w; \
						unsigned char *output_u = out_u_plane + i * total_in_w / 2; \
						unsigned char *output_v = out_v_plane + i * total_in_w / 2; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] * total_in_w; \
						unsigned char *input_v = in_v_plane + row_table[i] * total_in_w; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV_PLANAR_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = out_y_plane + i * total_in_w; \
						unsigned char *output_u = out_u_plane + i * total_in_w; \
						unsigned char *output_v = out_v_plane + i * total_in_w; \
						unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
						unsigned char *input_u = in_u_plane + row_table[i] * total_in_w; \
						unsigned char *input_v = in_v_plane + row_table[i] * total_in_w; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV444P_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
			} \
			break; \
	} \
}

void cmodel_yuv420p(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			column_table[j],
			column_table[j] / 2,
			column_table[j] / 2,
			0);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			j,
			j / 2,
			j / 2,
			0);
	}
}

void cmodel_yuv9p(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			column_table[j],
			column_table[j] / 4,
			column_table[j] / 4,
			0);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			j,
			j / 4,
			j / 4,
			0);
	}
}

void cmodel_yuv444p(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			column_table[j],
			column_table[j],
			column_table[j],
			0);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			j,
			j,
			j,
			0);
	}
}
