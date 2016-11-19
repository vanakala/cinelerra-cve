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

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "colormodels.h"
#include "colormodels_permutation.h"

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
	case BC_YUV888: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_RGB8((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_BGR565((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_RGB565((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_BGR888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_BGR8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_RGB888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_RGBA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ARGB8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_ARGB8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_RGB_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_RGBA_FLOAT((float**)(output), (input));   \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_YUV888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_YUV888_to_YUV420P_YUV422P(output_y, \
			output_u, output_v, \
			(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_YUV888_to_YUV444P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_YUV422((output), \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_YUV888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV888_to_YUVA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_YUVA8888: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_RGB8((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_BGR565((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_RGB565((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_BGR888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_BGR8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_RGB888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_RGBA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ARGB8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_ARGB8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_RGB_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_RGBA_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_YUVA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_YUVA8888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_YUVA8888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_YUVA8888_to_YUV444P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA8888_to_YUV422((output), \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_YUV161616: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_RGB8((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_BGR565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_RGB565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_BGR888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_BGR8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_RGB888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_RGBA8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ARGB8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_ARGB8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_RGB_FLOAT((float**)(output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_RGBA_FLOAT((float**)(output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_YUV161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_YUV161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_YUV161616_to_YUV444P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_YUV422((output), \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_YUVA8888((output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV161616: \
			TRANSFER_FRAME_HEAD \
			transfer_YUV161616_to_YUV161616((uint16_t**)(output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_YUVA16161616: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_RGB8((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_BGR565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_RGB565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_BGR888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_BGR8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_RGB888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_RGBA8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ARGB8888: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_ARGB8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_RGB_FLOAT((float**)(output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_RGBA_FLOAT((float**)(output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_YUVA16161616((uint16_t**)(output), \
				(uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_YUVA16161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_YUVA16161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_YUVA16161616_to_YUV444P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_YUVA16161616_to_YUV422((output), \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_ARGB8888: \
	case BC_ABGR8888: \
		switch(out_colormodel) \
		{ \
		case BC_ARGB8888: \
		case BC_ABGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_ARGB8888_to_ARGB8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_ARGB8888_to_RGBA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_ARGB8888_to_RGB888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_ARGB8888_to_BGR8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_RGB888: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGB8((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_BGR565((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGB565((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_BGR888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGB888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGBA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ARGB8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_ARGB8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGB161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGBA16161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGB_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_RGBA_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ABGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_ABGR8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_BGR8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_YUV888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_YUVA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_YUV161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_YUVA16161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_RGB888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB888_to_YUV422((output), (input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_RGB888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_RGB888_to_YUV444P(output_y, \
				output_u, output_v, \
				(input), j); \
				TRANSFER_FRAME_TAIL \
				break; \
		} \
		break; \
 \
	case BC_RGBA8888: \
		switch(out_colormodel) \
		{ \
		case BC_TRANSPARENCY: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_TRANSPARENCY((output), (input), &bit_counter); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB8: \
			if(bg_color > 0) \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_RGB8bg((output), (input), bg_r, bg_g, bg_b); \
				TRANSFER_FRAME_TAIL \
			else \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_RGB8((output), (input)); \
				TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			if(bg_color > 0) \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_BGR565bg((output), (input), bg_r, bg_g, bg_b); \
				TRANSFER_FRAME_TAIL \
			else \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_BGR565((output), (input)); \
				TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			if(bg_color > 0) \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_RGB565bg((output), (input), bg_r, bg_g, bg_b); \
				TRANSFER_FRAME_TAIL \
			else \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_RGB565((output), (input)); \
				TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			if(bg_color > 0) \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_BGR888bg((output), (input), bg_r, bg_g, bg_b); \
				TRANSFER_FRAME_TAIL \
			else \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_BGR888((output), (input)); \
				TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			if(bg_color > 0) \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_RGB888bg((output), (input), bg_r, bg_g, bg_b); \
				TRANSFER_FRAME_TAIL \
			else \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_RGB888((output), (input)); \
				TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_RGBA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_ARGB8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_ARGB8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_RGB161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_RGBA16161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_RGB_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_RGBA_FLOAT((float**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			if(bg_color > 0) \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_BGR8888bg((output), (input), bg_r, bg_g, bg_b); \
				TRANSFER_FRAME_TAIL \
			else \
				TRANSFER_FRAME_HEAD \
				transfer_RGBA8888_to_BGR8888((output), (input)); \
				TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_YUV888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_YUVA8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_YUV161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA8888_to_YUVA16161616((uint16_t**)(output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_RGBA888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA888_to_YUV422((output), (input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_RGBA888_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_RGBA888_to_YUV444P(output_y, \
				output_u, output_v, \
				(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_RGB161616: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_RGB8((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_BGR565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_RGB565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_BGR888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_BGR8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_RGB888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_RGBA8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_RGB_FLOAT((float**)(output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_RGBA_FLOAT((float**)(output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_YUV888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_YUVA8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_YUV161616((uint16_t**)(output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGB161616_to_YUVA16161616((uint16_t**)(output), (uint16_t*)(input));   \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_RGB161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_RGB161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_RGB161616_to_YUV444P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_RGBA16161616: \
		switch(out_colormodel) \
		{ \
		case BC_RGB8: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_RGB8((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR565: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_BGR565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB565: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_RGB565((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR888:      \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_BGR888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_BGR8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_RGB888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_RGBA8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGB_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_RGB_FLOAT((float**)(output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_RGBA_FLOAT: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_RGBA_FLOAT((float**)(output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_YUV888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA8888: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_YUVA8888((output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_YUV161616(((uint16_t**)output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUVA16161616: \
			TRANSFER_FRAME_HEAD \
			transfer_RGBA16161616_to_YUVA16161616((uint16_t**)(output), (uint16_t*)(input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV420P: \
			TRANSFER_YUV420P_OUT_HEAD \
			transfer_RGBA16161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV422P: \
			TRANSFER_YUV422P_OUT_HEAD \
			transfer_RGBA16161616_to_YUV420P_YUV422P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_YUV444P: \
			TRANSFER_YUV444P_OUT_HEAD \
			transfer_RGBA16161616_to_YUV444P(output_y, \
				output_u, output_v, \
				(uint16_t*)(input), j); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_BGR8888: \
		switch(out_colormodel) \
		{ \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_BGR8888_to_RGB888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		case BC_BGR8888: \
			TRANSFER_FRAME_HEAD \
			transfer_BGR8888_to_BGR8888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
 \
	case BC_BGR888: \
		switch(out_colormodel) \
		{ \
		case BC_RGB888: \
			TRANSFER_FRAME_HEAD \
			transfer_BGR888_to_RGB888((output), (input)); \
			TRANSFER_FRAME_TAIL \
			break; \
		} \
		break; \
	} \
}


void ColorModels::default_cmodel(PERMUTATION_ARGS)
{
	if(scale)
	{
		int done = 0;
		switch(in_colormodel)
		{
		case BC_YUVA8888:
			switch(out_colormodel)
			{
			case BC_BGR8888:
				short int *utog_tab = yuv_table.utog_tab8;
				short int *vtog_tab = yuv_table.vtog_tab8;
				short int *vtor_tab = yuv_table.vtor_tab8;
				short int *utob_tab = yuv_table.utob_tab8;

				short int i;
				for(i = 0; i < out_h; i++)
				{
					unsigned char *input_row = input_rows[row_table[i]];
					unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize;
					unsigned char *input_data = input_row + (column_table[0] << 2);
					__builtin_prefetch (input_data, 0, 0);

					short int j;

					for(j = 0; j < out_w; j++)
					{
						unsigned char y = input_data[0];
						unsigned char u = input_data[1];
						unsigned char v = input_data[2];
						unsigned char a = input_data[3];

						// the secret of the speedup ... delayed index calculation
						// we do not overflow at the end since column table holds + 1 element
						input_data = input_row + (column_table[j + 1] << 2);

						short int r = y + vtor_tab[v];
						short int g = y + utog_tab[u] + vtog_tab[v];
						short int b = y + utob_tab[u];

						r = (r < 0 ? 0 : (r > 0xff ? 0xff : r));
						g = (g < 0 ? 0 : (g > 0xff ? 0xff : g));
						b = (b < 0 ? 0 : (b > 0xff ? 0xff : b));

						r *= a;
						g *= a;
						b *= a;

						output_row[0] = ((b) >> 8);
						output_row[1] = ((g) >> 8);
						output_row[2] = ((r) >> 8);

						output_row += 4;
					}
				}

				done = 1; 
				break;
			}
			break;
		}

		if(!done)
		{
			TRANSFER_FRAME_DEFAULT(&output_row,
				input_row + column_table[j] * in_pixelsize,
				0,
				0,
				0,
				0);
		}
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row,
			input_row + j * in_pixelsize,
			0,
			0,
			0,
			0);
	}
}
