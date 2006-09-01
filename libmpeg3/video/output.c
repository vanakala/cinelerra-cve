#include "../libmpeg3.h"
#include "mpeg3video.h"
#include <string.h>

#define CLIP(x)  ((x) >= 0 ? ((x) < 255 ? (x) : 255) : 0)


static unsigned char mpeg3_601_to_rgb[256];

/* Algorithm */
/* 			r = (int)(*y + 1.371 * (*cr - 128)); */
/* 			g = (int)(*y - 0.698 * (*cr - 128) - 0.336 * (*cb - 128)); */
/* 			b = (int)(*y + 1.732 * (*cb - 128)); */


#define DITHER_ROW_HEAD \
	for(h = 0; h < video->out_h; h++) \
	{ \
		y_in = &src[0][(video->y_table[h] + video->in_y) * \
			video->coded_picture_width] + \
			video->in_x; \
		if(video->chroma_format == CHROMA420) \
		{ \
			cb_in = &src[1][((video->y_table[h] + video->in_y) >> 1) * \
				video->chrom_width] + \
				(video->in_x >> 1); \
			cr_in = &src[2][((video->y_table[h] + video->in_y) >> 1) * \
				video->chrom_width] + \
				(video->in_x >> 1); \
		} \
		else \
		{ \
			cb_in = &src[1][(video->y_table[h] + video->in_y) * \
				video->chrom_width] + \
				(video->in_x >> 1); \
			cr_in = &src[2][(video->y_table[h] + video->in_y) * \
				video->chrom_width] + \
				(video->in_x >> 1); \
		} \
		data = output_rows[h];

#define DITHER_ROW_TAIL \
	}

#define DITHER_SCALE_HEAD \
	for(w = 0; w < video->out_w; w++) \
	{ \
		uv_subscript = video->x_table[w] / 2; \
		y_l = y_in[video->x_table[w]]; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[cr_in[uv_subscript]]) >> 16; \
		g_l = (y_l + video->cr_to_g[cr_in[uv_subscript]] + video->cb_to_g[cb_in[uv_subscript]]) >> 16; \
		b_l = (y_l + video->cb_to_b[cb_in[uv_subscript]]) >> 16;

#define DITHER_SCALE_601_HEAD \
	for(w = 0; w < video->out_w; w++) \
	{ \
		uv_subscript = video->x_table[w] / 2; \
		y_l = mpeg3_601_to_rgb[y_in[video->x_table[w]]]; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[cr_in[uv_subscript]]) >> 16; \
		g_l = (y_l + video->cr_to_g[cr_in[uv_subscript]] + video->cb_to_g[cb_in[uv_subscript]]) >> 16; \
		b_l = (y_l + video->cb_to_b[cb_in[uv_subscript]]) >> 16;

#define DITHER_SCALE_TAIL \
	}

#define DITHER_HEAD \
    for(w = 0; w < video->horizontal_size; w++) \
	{ \
		y_l = *y_in++; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[*cr_in]) >> 16; \
		g_l = (y_l + video->cr_to_g[*cr_in] + video->cb_to_g[*cb_in]) >> 16; \
		b_l = (y_l + video->cb_to_b[*cb_in]) >> 16;

#define DITHER_601_HEAD \
    for(w = 0; w < video->horizontal_size; w++) \
	{ \
		y_l = mpeg3_601_to_rgb[*y_in++]; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[*cr_in]) >> 16; \
		g_l = (y_l + video->cr_to_g[*cr_in] + video->cb_to_g[*cb_in]) >> 16; \
		b_l = (y_l + video->cb_to_b[*cb_in]) >> 16;

#define DITHER_TAIL \
    	if(w & 1) \
		{ \
        	cr_in++; \
        	cb_in++; \
    	} \
    }


#define STORE_PIXEL_BGR888 \
	*data++ = CLIP(b_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(r_l);

#define STORE_PIXEL_BGRA8888 \
	*data++ = CLIP(b_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(r_l); \
	*data++ = 0;

#define STORE_PIXEL_RGB565 \
	*(*(unsigned short**)(&data))++ = \
		((CLIP(r_l) & 0xf8) << 8) | \
		((CLIP(g_l) & 0xfc) << 3) | \
		((CLIP(b_l) & 0xf8) >> 3); \
	data += 2;

#define STORE_PIXEL_RGB888 \
	*data++ = CLIP(r_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(b_l);

#define STORE_PIXEL_RGBA8888 \
	*data++ = CLIP(r_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(b_l); \
	*data++ = 0;

#define STORE_PIXEL_RGBA16161616 \
	*data_s++ = CLIP(r_l); \
	*data_s++ = CLIP(g_l); \
	*data_s++ = CLIP(b_l); \
	*data_s++ = 0;



/* Only good for YUV 4:2:0 */
int mpeg3video_ditherframe(mpeg3video_t *video, 
	unsigned char **src, 
	unsigned char **output_rows)
{
	int h = 0;
	unsigned char *y_in, *cb_in, *cr_in;
	int y_l, r_l, b_l, g_l;
	unsigned char *data;
	int uv_subscript, step, w = -1;



	DITHER_ROW_HEAD
/* Transfer row with scaling */
		if(video->out_w != video->horizontal_size)
		{
			switch(video->color_model)
			{
				case MPEG3_BGR888:
					DITHER_SCALE_HEAD
					STORE_PIXEL_BGR888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_BGRA8888:
					DITHER_SCALE_HEAD
					STORE_PIXEL_BGRA8888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_RGB565:
					DITHER_SCALE_HEAD
					STORE_PIXEL_RGB565
					DITHER_SCALE_TAIL
					break;
				case MPEG3_RGB888:
					DITHER_SCALE_HEAD
					STORE_PIXEL_RGB888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_RGBA8888:
					DITHER_SCALE_HEAD
					STORE_PIXEL_RGBA8888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_601_BGR888:
					DITHER_SCALE_601_HEAD
					STORE_PIXEL_BGR888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_601_BGRA8888:
					DITHER_SCALE_601_HEAD
					STORE_PIXEL_BGRA8888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_601_RGB565:
					DITHER_SCALE_601_HEAD
					STORE_PIXEL_RGB565
					DITHER_SCALE_TAIL
					break;
				case MPEG3_601_RGB888:
					DITHER_SCALE_601_HEAD
					STORE_PIXEL_RGB888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_601_RGBA8888:
					DITHER_SCALE_601_HEAD
					STORE_PIXEL_RGBA8888
					DITHER_SCALE_TAIL
					break;
				case MPEG3_RGBA16161616:
				{
					register unsigned short *data_s = (unsigned short*)data;
					DITHER_SCALE_HEAD
					STORE_PIXEL_RGBA16161616
					DITHER_SCALE_TAIL
				}
					break;
			}
		}
		else
		{
/* Transfer row unscaled */
			switch(video->color_model)
			{
				case MPEG3_BGR888:
					DITHER_HEAD
					STORE_PIXEL_BGR888
					DITHER_TAIL
					break;
				case MPEG3_BGRA8888:
					DITHER_HEAD
					STORE_PIXEL_BGRA8888
					DITHER_TAIL
					break;
				case MPEG3_RGB565:
					DITHER_HEAD
					STORE_PIXEL_RGB565
					DITHER_TAIL
					break;
				case MPEG3_RGB888:
					DITHER_HEAD
					STORE_PIXEL_RGB888
					DITHER_TAIL
					break;
				case MPEG3_RGBA8888:
					DITHER_HEAD
					STORE_PIXEL_RGBA8888
					DITHER_TAIL
					break;
				case MPEG3_601_BGR888:
					DITHER_601_HEAD
					STORE_PIXEL_BGR888
					DITHER_TAIL
					break;
				case MPEG3_601_BGRA8888:
					DITHER_601_HEAD
					STORE_PIXEL_BGRA8888
					DITHER_TAIL
					break;
				case MPEG3_601_RGB565:
					DITHER_601_HEAD
					STORE_PIXEL_RGB565
					DITHER_TAIL
					break;
				case MPEG3_601_RGB888:
					DITHER_601_HEAD
					STORE_PIXEL_RGB888
					DITHER_TAIL
					break;
				case MPEG3_601_RGBA8888:
					DITHER_601_HEAD
					STORE_PIXEL_RGBA8888
					DITHER_TAIL
					break;
				case MPEG3_RGBA16161616:
				{
					register unsigned short *data_s = (unsigned short*)data;
					DITHER_HEAD
					STORE_PIXEL_RGBA16161616
					DITHER_TAIL
				}
					break;
			}
		}
	DITHER_ROW_TAIL

	return 0;
}

int mpeg3video_ditherframe444(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

int mpeg3video_dithertop(mpeg3video_t *video, unsigned char *src[])
{
	return mpeg3video_ditherframe(video, src, video->output_rows);
}

int mpeg3video_dithertop444(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

int mpeg3video_ditherbot(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

int mpeg3video_ditherbot444(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

void memcpy_fast(unsigned char *output, unsigned char *input, long len)
{
	int i, len2;
/* 8 byte alignment */
/*
 * 	if(!((long)input & 0x7))
 * 	{
 * 		len2 = len >> 4;
 * 		for(i = 0; i < len2; )
 * 		{
 * 			((int64_t*)output)[i] = ((int64_t*)input)[i];
 * 			i++;
 * 			((int64_t*)output)[i] = ((int64_t*)input)[i];
 * 			i++;
 * 		}
 * 
 * 		for(i *= 16; i < len; i++)
 * 		{
 * 			output[i] = input[i];
 * 		}
 * 	}
 * 	else
 */
		memcpy(output, input, len);
}

int mpeg3video_init_output()
{
	int i, value;
	for(i = 0; i < 256; i++)
	{
		value = (int)(1.1644 * i - 255 * 0.0627 + 0.5);
		if(value < 0) value = 0;
		else
		if(value > 255) value = 255;
		mpeg3_601_to_rgb[i] = value;
	}
	return 0;
}

int mpeg3video_present_frame(mpeg3video_t *video)
{
	int i, j, k, l;
	unsigned char *src[3];
	src[0] = video->output_src[0];
	src[1] = video->output_src[1];
	src[2] = video->output_src[2];

/* Copy YUV buffers */
	if(video->want_yvu)
	{
		long size0, size1;
		long offset0, offset1;
		int chroma_denominator;
		
		if(video->chroma_format == CHROMA420)
			chroma_denominator = 2;
		else
			chroma_denominator = 1;

/* Drop a frame */
		if(!video->y_output) return 0;

/* Copy a frame */
/* Three blocks */
		if(video->in_x == 0 && 
			video->in_w >= video->coded_picture_width &&
			video->row_span == video->coded_picture_width)
		{
			size0 = video->coded_picture_width * video->in_h;
			size1 = video->chrom_width * (int)((float)video->in_h / chroma_denominator + 0.5);
			offset0 = video->coded_picture_width * video->in_y;
			offset1 = video->chrom_width * (int)((float)video->in_y / chroma_denominator + 0.5);

printf("mpeg3video_present_frame 1\n");
/*
 * 			if(video->in_y > 0)
 * 			{
 * 				offset[1] += video->chrom_width / 2;
 * 				size[1] += video->chrom_width / 2;
 * 			}
 */

			memcpy(video->y_output, src[0] + offset0, size0);
			memcpy(video->u_output, src[1] + offset1, size1);
			memcpy(video->v_output, src[2] + offset1, size1);
		}
		else
/* One block per row */
		{
//printf("mpeg3video_present_frame 2 %d %d %d\n", video->in_w, video->coded_picture_width, video->chrom_width);
			int row_span = video->in_w;
			int row_span0;
			int row_span1;

			if(video->row_span)
				row_span = video->row_span;

			row_span0 = row_span;
			row_span1 = (row_span >> 1);
			size0 = video->in_w;
			size1 = (video->in_w >> 1);
			offset0 = video->coded_picture_width * video->in_y;
			offset1 = video->chrom_width * video->in_y / chroma_denominator;
	
			for(i = 0; i < video->in_h; i++)
			{
				memcpy(video->y_output + i * row_span0, 
					src[0] + offset0 + video->in_x, 
					size0);

				offset0 += video->coded_picture_width;

				if(chroma_denominator == 1 || !(i % 2))
				{
					memcpy(video->u_output + i / chroma_denominator * row_span1, 
						src[1] + offset1 + (video->in_x >> 1), 
						size1);
					memcpy(video->v_output + i / chroma_denominator * row_span1, 
						src[2] + offset1 + (video->in_x >> 1), 
						size1);
					if(video->horizontal_size < video->in_w)
					{
						memset(video->u_output + 
							i / chroma_denominator * row_span1 +
							(video->horizontal_size >> 1),
							0x80,
							(video->in_w >> 1) - 
							(video->horizontal_size >> 1));
						memset(video->v_output + 
							i / chroma_denominator * row_span1 +
							(video->horizontal_size >> 1),
							0x80,
							(video->in_w >> 1) - 
							(video->horizontal_size >> 1));
					}
				}
				

				if(chroma_denominator == 1 || (i % 2))
					offset1 += video->chrom_width;
			}
		}

		return 0;
	}

/* Want RGB buffer */
/* Copy the frame to the output with YUV to RGB conversion */
  	if(video->prog_seq)
	{
    	if(video->chroma_format != CHROMA444)
		{
    		mpeg3video_ditherframe(video, src, video->output_rows);
    	}
    	else
    	  	mpeg3video_ditherframe444(video, src);
  	}
	else
	{
   		if((video->pict_struct == FRAME_PICTURE && video->topfirst) || 
			video->pict_struct == BOTTOM_FIELD)
		{
/* top field first */
    		if(video->chroma_format != CHROMA444)
			{
        		mpeg3video_dithertop(video, src);
        		mpeg3video_ditherbot(video, src);
    		}
    		else 
			{
        		mpeg3video_dithertop444(video, src);
        		mpeg3video_ditherbot444(video, src);
    		}
    	}
    	else 
		{
/* bottom field first */
    		if(video->chroma_format != CHROMA444)
			{
        		mpeg3video_ditherbot(video, src);
        		mpeg3video_dithertop(video, src);
    		}
    		else 
			{
        		mpeg3video_ditherbot444(video, src);
        		mpeg3video_dithertop444(video, src);
    		}
    	}
  	}
	return 0;
}

int mpeg3video_display_second_field(mpeg3video_t *video)
{
/* Not used */
	return 0;
}
