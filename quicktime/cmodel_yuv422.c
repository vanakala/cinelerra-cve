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
