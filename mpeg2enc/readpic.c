/* readpic.c, read source pictures                                          */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "colormodels.h"
#include "config.h"
#include "global.h"


static void read_quicktime(frame, number)
unsigned char *frame[];
long number;
{
	int i, j;
	int r, g, b;
	int y, u, v;
	double cr, cg, cb, cu, cv;
	char name[128];
	unsigned char *yp, *up, *vp;
	static unsigned char *u444, *v444, *u422, *v422;
	static double coef[7][3] = {
		{0.2125,0.7154,0.0721}, /* ITU-R Rec. 709 (1990) */
		{0.299, 0.587, 0.114},  /* unspecified */
		{0.299, 0.587, 0.114},  /* reserved */
		{0.30,  0.59,  0.11},   /* FCC */
		{0.299, 0.587, 0.114},  /* ITU-R Rec. 624-4 System B, G */
		{0.299, 0.587, 0.114},  /* SMPTE 170M */
		{0.212, 0.701, 0.087}}; /* SMPTE 240M (1987) */
	static long rtoy_tab[256], gtoy_tab[256], btoy_tab[256];
	static long rtou_tab[256], gtou_tab[256], btou_tab[256];
	static long rtov_tab[256], gtov_tab[256], btov_tab[256];
	static int need_tables = 1;   // Initialize tables on first read
	int colormodel;
	long real_number;

	i = matrix_coefficients;
	if(i > 8) i = 3;

	cr = coef[i - 1][0];
	cg = coef[i - 1][1];
	cb = coef[i - 1][2];
	cu = 0.5 / (1.0 - cb);
	cv = 0.5 / (1.0 - cr);

// Allocate output buffers
	if(chroma_format == CHROMA444)
	{
// Not supported by libMPEG3
    	u444 = frame[1];
    	v444 = frame[2];
	}
	else
	{
    	if (!u444)
    	{
    		if (!(u444 = (unsigned char *)malloc(width*height)))
        		error("malloc failed");
    		if (!(v444 = (unsigned char *)malloc(width*height)))
        		error("malloc failed");
    		if (chroma_format==CHROMA420)
    		{
        		if (!(u422 = (unsigned char *)malloc((width>>1)*height)))
        			error("malloc failed");
        		if (!(v422 = (unsigned char *)malloc((width>>1)*height)))
        			error("malloc failed");
    		}
    	}
	}

// Initialize YUV tables
	if(need_tables)
	{
		for(i = 0; i < 256; i++)
		{
			rtoy_tab[i] = (long)( 0.2990 * 65536 * i);
			rtou_tab[i] = (long)(-0.1687 * 65536 * i);
			rtov_tab[i] = (long)( 0.5000 * 65536 * i);

			gtoy_tab[i] = (long)( 0.5870 * 65536 * i);
			gtou_tab[i] = (long)(-0.3320 * 65536 * i);
			gtov_tab[i] = (long)(-0.4187 * 65536 * i);

			btoy_tab[i] = (long)( 0.1140 * 65536 * i);
			btou_tab[i] = (long)( 0.5000 * 65536 * i);
			btov_tab[i] = (long)(-0.0813 * 65536 * i);
		}
		need_tables = 0;
	}

	real_number = (long)((double)quicktime_frame_rate(qt_file, 0) / 
			frame_rate * 
			number + 
			0.5);
	quicktime_set_video_position(qt_file, 
		real_number, 
		0);

//printf("readframe 1 %d %d\n", width, height);
	quicktime_set_row_span(qt_file, width);
	quicktime_set_window(qt_file,
		0, 
		0,
		horizontal_size,
		vertical_size,
		horizontal_size,
		vertical_size);
	quicktime_set_cmodel(qt_file, (chroma_format == 1) ? BC_YUV420P : BC_YUV422P);
	
	quicktime_decode_video(qt_file, 
		frame, 
		0);
//printf("readframe 2\n");
}

static void read_mpeg(long number, unsigned char *frame[])
{
	int i;
	char name[128];
	long real_number;

// Normalize frame_rate
	real_number = (long)((double)mpeg3_frame_rate(mpeg_file, 0) / 
		frame_rate * 
		number + 
		0.5);

	while(mpeg3_get_frame(mpeg_file, 0) <= real_number)
		mpeg3_read_yuvframe(mpeg_file,
			frame[0],
			frame[1],
			frame[2],
			0,
			0,
			horizontal_size,
			vertical_size,
			0);
/* Terminate encoding after processing this frame */
	if(mpeg3_end_of_video(mpeg_file, 0)) frames_scaled = 1; 
}

static void read_stdin(long number, unsigned char *frame[])
{
	int chroma_denominator = 1;
	unsigned char data[5];


	if(chroma_format == 1) chroma_denominator = 2;

	fread(data, 4, 1, stdin_fd);

// Terminate encoding before processing this frame
	if(data[0] == 0xff && data[1] == 0xff && data[2] == 0xff && data[3] == 0xff)
	{
		frames_scaled = 0;
		return;
	}

	fread(frame[0], width * height, 1, stdin_fd);
	fread(frame[1], width / 2 * height / chroma_denominator, 1, stdin_fd);
	fread(frame[2], width / 2 * height / chroma_denominator, 1, stdin_fd);
}

static void read_buffers(long number, unsigned char *frame[])
{
	int chroma_denominator = 1;
	unsigned char data[5];

	if(chroma_format == 1) chroma_denominator = 2;

	pthread_mutex_lock(&input_lock);

	if(input_buffer_end) 
	{
		frames_scaled = 0;
		pthread_mutex_unlock(&copy_lock);
		pthread_mutex_unlock(&output_lock);
		return;
	}

	memcpy(frame[0], input_buffer_y, width * height);
	memcpy(frame[1], input_buffer_u, width / 2 * height / chroma_denominator);
	memcpy(frame[2], input_buffer_v, width / 2 * height / chroma_denominator);
	pthread_mutex_unlock(&copy_lock);
	pthread_mutex_unlock(&output_lock);
}

void readframe(int frame_num, uint8_t *frame[])
{
	int n;
	n = frame_num % (2 * READ_LOOK_AHEAD);

	frame[0] = frame_buffers[n][0];
	frame[1] = frame_buffers[n][1];
	frame[2] = frame_buffers[n][2];
	

	switch (inputtype)
	{
		case T_QUICKTIME:
			read_quicktime(frame, frame_num);
			break;
		case T_MPEG:
			read_mpeg(frame_num, frame);
			break;
		case T_STDIN:
			read_stdin(frame_num, frame);
			break;
		case T_BUFFERS:
			read_buffers(frame_num, frame);
			break;
		default:
			break;
  }
}
