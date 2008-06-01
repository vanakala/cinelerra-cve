#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <stdlib.h>
#include <string.h>

static unsigned char get_nibble(unsigned char **ptr, int *nibble)
{
	if(*nibble)
	{
		*nibble = !*nibble;
		return (*(*ptr)++) & 0xf;
	}
	else
	{
		*nibble = !*nibble;
		return (*(*ptr)) >> 4;
	}
}

/* Returns 1 if failure */
/* This is largely from spudec, ffmpeg */
int decompress_subtitle(mpeg3_t *file, mpeg3_subtitle_t *subtitle)
{
	int i;
	unsigned char *ptr = subtitle->data;
	unsigned char *end = subtitle->data + subtitle->size;
	int even_offset = 0;
	int odd_offset = 0;

/* packet size */
	ptr += 2;

/* data packet size */
	if(ptr + 2 > end) return 1;

	int data_size = (*ptr++) << 8;
	data_size |= *ptr++;

	unsigned char *data_start = ptr;
	if(ptr + data_size > end) return 1;

/* Advance to control sequences */
	ptr += data_size - 2;

	subtitle->palette[0] = 0x00;
	subtitle->palette[1] = 0x01;
	subtitle->palette[2] = 0x02;
	subtitle->palette[3] = 0x03;

	subtitle->alpha[0] = 0xff;
	subtitle->alpha[1] = 0x00;
	subtitle->alpha[2] = 0x40;
	subtitle->alpha[3] = 0xc0;

/* Control sequence */
	unsigned char *control_start = 0;
	unsigned char *next_control_start = ptr;
	int got_alpha = 0;
	while(ptr < end && control_start != next_control_start)
	{
		control_start = next_control_start;

/* Date */
		if(ptr + 2 > end) break;
		int date = (*ptr++) << 8;
		date |= *ptr++;

/* Offset of next control sequence */
		if(ptr + 2 > end) break;
		int next = (*ptr++) << 8;
		next |= *ptr++;

		next_control_start = subtitle->data + next;

		int done = 0;
		while(ptr < end && !done)
		{
			int type = *ptr++;

			switch(type)
			{
				case 0x00:
					subtitle->force = 1;
					break;

				case 0x01:
					subtitle->start_time = date;
//printf("decompress_subtitle %d\n", subtitle->start_time);
					break;

				case 0x02:
					subtitle->stop_time = date;
					break;

				case 0x03:
/* Entry in palette of each color */
					if(ptr + 4 > end) return 1;
					subtitle->palette[0] = (*ptr) >> 4;
					subtitle->palette[1] = (*ptr++) & 0xf;
					subtitle->palette[2] = (*ptr) >> 4;
					subtitle->palette[3] = (*ptr++) & 0xf;
//printf("subtitle palette %d %d %d %d\n", subtitle->palette[0], subtitle->palette[1], subtitle->palette[2], subtitle->palette[3]);
					break;

				case 0x04:
/* Alpha corresponding to each color */
					if(ptr + 4 > end) return 1;
					subtitle->alpha[0] = ((*ptr) >> 4) * 255 / 15;
					subtitle->alpha[1] = ((*ptr) & 0xf) * 255 / 15;
					subtitle->alpha[2] = ((*ptr++) >> 4) * 255 / 15;
					subtitle->alpha[3] = ((*ptr++) & 0xf) * 255 / 15;
					got_alpha = 1;
//printf("subtitle alpha %d %d %d %d\n", subtitle->alpha[0], subtitle->alpha[1], subtitle->alpha[2], subtitle->alpha[3]);
					break;

				case 0x05:
/* Extent of image on screen */
					if(ptr + 6 > end) return 1;
/*
 * printf("decompress_subtitle 10 %02x %02x %02x %02x %02x %02x\n",
 * ptr[0],
 * ptr[1],
 * ptr[2],
 * ptr[3],
 * ptr[4],
 * ptr[5]);
 */
					subtitle->x1 = (*ptr++) << 4;
					subtitle->x1 |= (*ptr) >> 4;
					subtitle->x2 = ((*ptr++) & 0xf) << 8;
					subtitle->x2 |= *ptr++;
					subtitle->y1 = (*ptr++) << 4;
					subtitle->y1 |= (*ptr) >> 4;
					subtitle->y2 = ((*ptr++) & 0xf) << 8;
					subtitle->y2 |= *ptr++;
					subtitle->x2++;
					subtitle->y2++;
					subtitle->w = subtitle->x2 - subtitle->x1;
					subtitle->h = subtitle->y2 - subtitle->y1;
/*
 * printf("decompress_subtitle 20 x1=%d x2=%d y1=%d y2=%d\n", 
 * subtitle->x1, 
 * subtitle->x2, 
 * subtitle->y1, 
 * subtitle->y2);
 */
					CLAMP(subtitle->w, 1, 2048);
					CLAMP(subtitle->h, 1, 2048);
					CLAMP(subtitle->x1, 0, 2048);
					CLAMP(subtitle->x2, 0, 2048);
					CLAMP(subtitle->y1, 0, 2048);
					CLAMP(subtitle->y2, 0, 2048);
					break;

				case 0x06:
/* offsets of even and odd field in compressed data */
					if(ptr + 4 > end) return 1;
					even_offset = (ptr[0] << 8) | (ptr[1]);
					odd_offset = (ptr[2] << 8) | (ptr[3]);
//printf("decompress_subtitle 30 even=0x%x odd=0x%x\n", even_offset, odd_offset);
					ptr += 4;
					break;

				case 0xff:
					done = 1;
					break;

				default:
//					printf("unknown type %02x\n", type);
					break;
			}
		}

	}



/* Allocate image buffer */
	subtitle->image_y = (unsigned char*)calloc(1, subtitle->w * subtitle->h + subtitle->w);
	subtitle->image_u = (unsigned char*)calloc(1, subtitle->w * subtitle->h / 4 + subtitle->w);
	subtitle->image_v = (unsigned char*)calloc(1, subtitle->w * subtitle->h / 4 + subtitle->w);
	subtitle->image_a = (unsigned char*)calloc(1, subtitle->w * subtitle->h + subtitle->w);

/* Decode image */
	int current_nibble = 0;
	int x = 0, y = 0, field = 0;
	ptr = data_start;
	int first_pixel = 1;

	while(ptr < end && y < subtitle->h + 1 && x < subtitle->w)
	{
// Start new field based on offset, not total lines
		if(ptr - data_start >= odd_offset - 4 && 
			field == 0)
		{
			field = 1;
			y = 1;
			x = 0;

			if(current_nibble)
			{
				ptr++;
				current_nibble = 0;
			}
		}


		unsigned int code = get_nibble(&ptr, &current_nibble);
		if(code < 0x4 && ptr < end)
		{
			code = (code << 4) | get_nibble(&ptr, &current_nibble);
			if(code < 0x10 && ptr < end)
			{
				code = (code << 4) | get_nibble(&ptr, &current_nibble);
				if(code < 0x40 && ptr < end)
				{
					code = (code << 4) | get_nibble(&ptr, &current_nibble);
/* carriage return */
					if(code < 0x4 && ptr < end)
						code |= (subtitle->w - x) << 2;
				}
			}
		}

		int color = (code & 0x3);
		int len = code >> 2;
		if(len > subtitle->w - x)
			len = subtitle->w - x;

		int y_color = file->palette[subtitle->palette[color] * 4];
		int u_color = file->palette[subtitle->palette[color] * 4 + 1];
		int v_color = file->palette[subtitle->palette[color] * 4 + 2];
		int a_color = subtitle->alpha[color];

// The alpha seems to be arbitrary.  Assume the top left pixel is always 
// transparent.
		if(first_pixel)
		{
			subtitle->alpha[color] = 0x0;
			a_color = 0x0;
			first_pixel = 0;
		}

/*
 * printf("0x%02x 0x%02x 0x%02x\n", 
 * y_color,
 * u_color,
 * v_color);
 */
		if(y < subtitle->h)
		{
			for(i = 0; i < len; i++)
			{
				subtitle->image_y[y * subtitle->w + x] = y_color;
				if(!(x % 2) && !(y % 2))
				{
					subtitle->image_u[y / 2 * subtitle->w / 2 + x / 2] = u_color;
					subtitle->image_v[y / 2 * subtitle->w / 2 + x / 2] = v_color;
				}
				subtitle->image_a[y * subtitle->w + x] = a_color;
				x++;
			}
		}

		if(x >= subtitle->w)
		{
			x = 0;
			y += 2;

/* Byte alignment */
			if(current_nibble)
			{
				ptr++;
				current_nibble = 0;
			}

// Start new field based on total lines, not offset
			if(y >= subtitle->h)
			{
				y = subtitle->h - 1;
/*
 * 				if(!field)
 * 				{
 * 					y = 1;
 * 					field = 1;
 * 				}
 */
			}
		}
	}

/*
 * printf("decompress_subtitle coords: %d,%d - %d,%d size: %d,%d start_time=%d end_time=%d\n", 
 * subtitle->x1, 
 * subtitle->y1, 
 * subtitle->x2, 
 * subtitle->y2,
 * subtitle->w,
 * subtitle->h,
 * subtitle->start_time,
 * subtitle->stop_time);
 */
	return 0;
}




void overlay_subtitle(mpeg3video_t *video, mpeg3_subtitle_t *subtitle)
{
	int x, y;
	for(y = subtitle->y1; 
		y < subtitle->y2 && y < video->coded_picture_height; 
		y++)
	{
		unsigned char *output_y = video->subtitle_frame[0] + 
			y * video->coded_picture_width +
			subtitle->x1;
		unsigned char *output_u = video->subtitle_frame[1] + 
			y / 2 * video->chrom_width +
			subtitle->x1 / 2;
		unsigned char *output_v = video->subtitle_frame[2] + 
			y / 2 * video->chrom_width +
			subtitle->x1 / 2;
		unsigned char *input_y = subtitle->image_y + (y - subtitle->y1) * subtitle->w;
		unsigned char *input_u = subtitle->image_u + (y - subtitle->y1) / 2 * subtitle->w / 2;
		unsigned char *input_v = subtitle->image_v + (y - subtitle->y1) / 2 * subtitle->w / 2;
		unsigned char *input_a = subtitle->image_a + (y - subtitle->y1) * subtitle->w;

		for(x = subtitle->x1; 
			x < subtitle->x2 && x < video->coded_picture_width; 
			x++)
		{
			int opacity = *input_a;
			int transparency = 0xff - opacity;
			*output_y = (*input_y * opacity + *output_y * transparency) / 0xff;

			if(!(y % 2) && !(x % 2))
			{
				*output_u = (*input_u * opacity + *output_u * transparency) / 0xff;
				*output_v = (*input_v * opacity + *output_v * transparency) / 0xff;
				output_u++;
				output_v++;
				input_u++;
				input_v++;
			}

			output_y++;
			input_y++;
			input_a++;
		}
	}
}



void mpeg3_decode_subtitle(mpeg3video_t *video)
{
/* Test demuxer for subtitle */
	mpeg3_vtrack_t *vtrack  = (mpeg3_vtrack_t*)video->track;
	mpeg3_t *file = (mpeg3_t*)video->file;

/* Clear subtitles from inactive subtitle tracks */
	int i;
	for(i = 0; i < mpeg3_subtitle_tracks(file); i++)
	{
		if(i != file->subtitle_track)
			mpeg3_pop_all_subtitles(mpeg3_get_strack(file, i));
	}

	if(file->subtitle_track >= 0 &&
		file->subtitle_track < mpeg3_subtitle_tracks(file))
	{
		mpeg3_strack_t *strack = mpeg3_get_strack(file, file->subtitle_track);
		int total = 0;
		if(strack)
		{
			for(i = 0; i < strack->total_subtitles; i++)
			{
				mpeg3_subtitle_t *subtitle = strack->subtitles[i];
				if(!subtitle->active)
				{
/* Exclude object from future activation */
					subtitle->active = 1;

/* Decompress subtitle */
					if(decompress_subtitle(file, subtitle))
					{
/* Remove subtitle if failed */
						mpeg3_pop_subtitle(strack, i, 1);
						i--;
						continue;
					}
				}



/* Test start and end time of subtitle */
				if(subtitle->stop_time > 0)
				{
/* Copy video to temporary */
					if(!total)
					{
						if(!video->subtitle_frame[0])
						{
							video->subtitle_frame[0] = malloc(
								video->coded_picture_width * 
								video->coded_picture_height + 8);
							video->subtitle_frame[1] = malloc(
								video->chrom_width * 
								video->chrom_height + 8);
							video->subtitle_frame[2] = malloc(
								video->chrom_width * 
								video->chrom_height + 8);
						}

						memcpy(video->subtitle_frame[0],
							video->output_src[0],
							video->coded_picture_width * video->coded_picture_height);
						memcpy(video->subtitle_frame[1],
							video->output_src[1],
							video->chrom_width * video->chrom_height);
						memcpy(video->subtitle_frame[2],
							video->output_src[2],
							video->chrom_width * video->chrom_height);

						video->output_src[0] = video->subtitle_frame[0];
						video->output_src[1] = video->subtitle_frame[1];
						video->output_src[2] = video->subtitle_frame[2];
					}
					total++;


/* Overlay subtitle on video */
					overlay_subtitle(video, subtitle);
					subtitle->stop_time -= (int)(100.0 / video->frame_rate);
				}

				if(subtitle->stop_time <= 0)
				{
					mpeg3_pop_subtitle(strack, i, 1);
					i--;
				}
			}
		}
	}




}





