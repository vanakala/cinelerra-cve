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



static inline void transfer_YUV422_to_RGB8(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;

	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_YUV422_to_BGR565(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV422_to_RGB565(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV422_to_BGR888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void transfer_YUV422_to_RGB888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV422_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV422_to_RGB161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = (input[1] << 8) | input[1];
	v = (input[3] << 8) | input[3];
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV422_to_RGBA16161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = (input[1] << 8) | input[1];
	v = (input[3] << 8) | input[3];
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_YUV422_to_RGB_FLOAT(float* *output, 
	unsigned char *input, 
	int column)
{
	float y;
// Signedness is important
	int u, v;
	float r, g, b;

// Even pixel
	if(!(column & 1))
		y = (float)input[0] / 0xff;
	else
// Odd pixel
		y = (float)input[2] / 0xff;
	u = input[1];
	v = input[3];
	YUV_TO_FLOAT(y, u, v, r, g, b)

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_YUV422_to_RGBA_FLOAT(float* *output, 
	unsigned char *input, 
	int column)
{
	float y;
// Signedness is important
	int u, v;
	float r, g, b;

// Even pixel
	if(!(column & 1))
		y = (float)input[0] / 0xff;
	else
// Odd pixel
		y = (float)input[2] / 0xff;
	u = input[1];
	v = input[3];
	YUV_TO_FLOAT(y, u, v, r, g, b)

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 1.0;
}

static inline void transfer_YUV422_to_YUV888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = input[0];
	else
// Odd pixel
		(*output)[0] = input[2];

	(*output)[1] = input[1];
	(*output)[2] = input[3];
	(*output) += 3;
}

static inline void transfer_YUV422_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = input[0];
	else
// Odd pixel
		(*output)[0] = input[2];

	(*output)[1] = input[1];
	(*output)[2] = input[3];
	(*output)[3] = 255;
	(*output) += 4;
}

static inline void transfer_YUV422_to_YUV161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = (input[0] << 8) | input[0];
	else
// Odd pixel
		(*output)[0] = (input[2] << 8) | input[2];

	(*output)[1] = (input[1] << 8) | input[1];
	(*output)[2] = (input[3] << 8) | input[3];
	(*output) += 3;
}

static inline void transfer_YUV422_to_YUVA16161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = (input[0] << 8) | input[0];
	else
// Odd pixel
		(*output)[0] = (input[2] << 8) | input[2];

	(*output)[1] = (input[1] << 8) | input[1];
	(*output)[2] = (input[3] << 8) | input[3];
	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_YUV422_to_BGR8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];

	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}


static inline void transfer_YUV422_to_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
// Store U and V for even pixels only
	if(!(output_column & 1))
	{
		output_y[output_column] = input[0];
		output_u[output_column / 2] = input[1];
		output_v[output_column / 2] = input[3];
	}
	else
// Store Y and advance output for odd pixels only
	{
		output_y[output_column] = input[2];
	}
}

static inline void transfer_YUV422_to_YUV420P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column,
	int output_row)
{
// Even column
	if(!(output_column & 1))
	{
		output_y[output_column] = input[0];
// Store U and V for even columns and even rows only
		if(!(output_row & 1))
		{
			output_u[output_column / 2] = input[1];
			output_v[output_column / 2] = input[3];
		}
	}
	else
// Odd column
	{
		output_y[output_column] = input[2];
	}
}

static inline void transfer_YUV422_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[0] = input[0];
		(*output)[1] = input[1];
		(*output)[3] = input[3];
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = input[2];
		(*output) += 4;
	}
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
	switch(out_colormodel) \
	{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGB8((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGB565((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGB888((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGBA8888((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV888:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_YUV888((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_YUVA8888((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB161616:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGB161616((uint16_t**)(output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA16161616:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGBA16161616((uint16_t**)(output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGB_FLOAT((float**)(output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_RGBA_FLOAT((float**)(output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV161616:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_YUV161616((uint16_t**)(output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA16161616:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_YUVA16161616((uint16_t**)(output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888:      \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_BGR888((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_BGR8888((output), (input), (input_column)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_YUV422_to_YUV422P(output_y, \
				output_u, \
				output_v, \
				(input), \
				j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV422_to_YUV422((output), \
				(input), \
				j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_YUV422_to_YUV420P(output_y, \
				output_u, \
				output_v, \
				(input), \
				j, \
				i); \
			TRANSFER_FRAME_TAIL \
			break; \
	} \
}

void cmodel_yuv422(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + ((column_table[j] * in_pixelsize) & 0xfffffffc),
			0,
			0,
			0,
			column_table[j]);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + ((j * in_pixelsize) & 0xfffffffc),
			0,
			0,
			0,
			j);
	}
}
