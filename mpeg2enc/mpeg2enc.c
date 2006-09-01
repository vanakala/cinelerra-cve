/* mpeg2enc.c, main() and parameter file reading                            */

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

#define MAX(a,b) ( (a)>(b) ? (a) : (b) )
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOBAL_ /* used by global.h */
#include "config.h"
#include "global.h"

/* private prototypes */
static void init _ANSI_ARGS_((void));
static void readcmdline _ANSI_ARGS_((int argc, char *argv[]));
static void readquantmat _ANSI_ARGS_((void));


// Hack for libdv to remove glib dependancy

void
g_log (const char    *log_domain,
       int  log_level,
       const char    *format,
       ...)
{
}

void
g_logv (const char    *log_domain,
       int  log_level,
       const char    *format,
       ...)
{
}


void mpeg2enc_set_w(int width)
{
	horizontal_size = width;
}

void mpeg2enc_set_h(int height)
{
	vertical_size = height;
}

void mpeg2enc_set_rate(double rate)
{
	input_frame_rate = rate;
}

void mpeg2enc_set_input_buffers(int eof, char *y, char *u, char *v)
{
	pthread_mutex_lock(&output_lock);
	input_buffer_end = eof;
	input_buffer_y = y;
	input_buffer_u = u;
	input_buffer_v = v;
	pthread_mutex_unlock(&input_lock);
// Wait for buffers to get copied before returning.
	pthread_mutex_lock(&copy_lock);
}

void mpeg2enc_init_buffers()
{
	pthread_mutex_init(&input_lock, 0);
	pthread_mutex_init(&output_lock, 0);
	pthread_mutex_init(&copy_lock, 0);
	pthread_mutex_lock(&input_lock);
	pthread_mutex_lock(&copy_lock);
	input_buffer_end = 0;
}

int mpeg2enc(int argc, char *argv[])
{
	stdin_fd = stdin;
	
	verbose = 1;


/* Read command line */
	readcmdline(argc, argv);

/* read quantization matrices */
	readquantmat();

	if(!strlen(out_path))
	{
		fprintf(stderr, "No output file given.\n");
	}

/* open output file */
	if(!(outfile = fopen(out_path, "wb")))
	{
      sprintf(errortext,"Couldn't create output file %s", out_path);
      error(errortext);
	}

	init();

	if(nframes < 0x7fffffff)
		printf("Frame    Completion    Current bitrate     Predicted file size\n");
	putseq();

	stop_slice_engines();
	stop_motion_engines();
  	stop_transform_engines();
  	stop_itransform_engines();

	fclose(outfile);
	fclose(statfile);

	if(qt_file) quicktime_close(qt_file);
	if(qt_output) quicktime_close(qt_output);
	if(mpeg_file) mpeg3_close(mpeg_file);

	if(do_stdin)
	{
		fclose(stdin_fd);
	}
	pthread_mutex_destroy(&input_lock);
	pthread_mutex_destroy(&output_lock);
	return 0;
}

int HorzMotionCode(int i)
{
	if (i < 8)
      return 1;
	if (i < 16)
      return 2;
	if (i < 32)
      return 3;
	if ((i < 64) || (constrparms))
      return 4;
	if (i < 128)
      return 5;
	if (i < 256)
      return 6;
	if ((i < 512) || (level == 10))
      return 7;
	if ((i < 1024) || (level == 8))
      return 8;
	if (i < 2048)
      return 9;
	return 1;
}

int VertMotionCode(int i)
{
	if (i < 8)
      return 1;
	if (i < 16)
      return 2;
	if (i < 32)
      return 3;
	if ((i < 64) || (level == 10) || (constrparms))
      return 4;
	return 5;
}

/*
	Wrapper for malloc that allocates pbuffers aligned to the 
	specified byte boundary and checks for failure.
	N.b.  don't try to free the resulting pointers, eh...
	BUG: 	Of course this won't work if a char * won't fit in an int....
*/
static uint8_t *bufalloc( size_t size )
{
	char *buf = malloc( size + BUFFER_ALIGN );
	int adjust;

	if( buf == NULL )
	{
		error("malloc failed\n");
	}
	adjust = BUFFER_ALIGN-((int)buf)%BUFFER_ALIGN;
	if( adjust == BUFFER_ALIGN )
		adjust = 0;
	return (uint8_t*)(buf+adjust);
}

static void init()
{
	int i, n, size;
	static int block_count_tab[3] = {6,8,12};
	int lum_buffer_size, chrom_buffer_size;
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutex_init(&test_lock, &mutex_attr);

	bzero(&cur_picture, sizeof(pict_data_s));
	mpeg2_initbits();
	init_fdct();
	init_idct();
	init_motion();
	init_predict_hv();
	init_quantizer_hv();
	init_transform_hv();

/* round picture dimensions to nZearest multiple of 16 or 32 */
	mb_width = (horizontal_size+15)/16;
	mb_height = prog_seq ? 
		(vertical_size + 15) / 16 : 
		2 * ((vertical_size + 31) / 32);
	mb_height2 = fieldpic ? 
		mb_height >> 1 : 
		mb_height; /* for field pictures */
	width = 16 * mb_width;
	height = 16 * mb_height;

	chrom_width = (chroma_format==CHROMA444) ? width : width>>1;
	chrom_height = (chroma_format!=CHROMA420) ? height : height>>1;

	height2 = fieldpic ? height>>1 : height;
	width2 = fieldpic ? width<<1 : width;
	chrom_width2 = fieldpic ? chrom_width<<1 : chrom_width;

	block_count = block_count_tab[chroma_format-1];
	lum_buffer_size = (width*height) + 
					 sizeof(uint8_t) *(width/2)*(height/2) +
					 sizeof(uint8_t) *(width/4)*(height/4+1);
	chrom_buffer_size = chrom_width*chrom_height;

	fsubsample_offset = (width)*(height) * sizeof(uint8_t);
	qsubsample_offset =  fsubsample_offset + (width/2)*(height/2)*sizeof(uint8_t);

	rowsums_offset = 0;
	colsums_offset = 0;

	mb_per_pict = mb_width*mb_height2;

/* clip table */
	if (!(clp = (unsigned char *)malloc(1024)))
      error("malloc failed\n");
	clp+= 384;
	for (i=-384; i<640; i++)
      clp[i] = (i<0) ? 0 : ((i>255) ? 255 : i);


	
	/* Allocate the frame buffer */


	frame_buffers = (uint8_t ***) 
		bufalloc(2*READ_LOOK_AHEAD*sizeof(uint8_t**));
	
	for(n=0;n<2*READ_LOOK_AHEAD;n++)
	{
         frame_buffers[n] = (uint8_t **) bufalloc(3*sizeof(uint8_t*));
		 for (i=0; i<3; i++)
		 {
			 frame_buffers[n][i] = 
				 bufalloc( (i==0) ? lum_buffer_size : chrom_buffer_size );
		 }
	}



	/* TODO: The ref and aux frame buffers are no redundant! */
	for( i = 0 ; i<3; i++)
	{
		int size =  (i==0) ? lum_buffer_size : chrom_buffer_size;
		newrefframe[i] = bufalloc(size);
		oldrefframe[i] = bufalloc(size);
		auxframe[i]    = bufalloc(size);
		predframe[i]   = bufalloc(size);
	}

	cur_picture.qblocks =
		(int16_t (*)[64])bufalloc(mb_per_pict*block_count*sizeof(int16_t [64]));

	/* Initialise current transformed picture data tables
	   These will soon become a buffer for transformed picture data to allow
	   look-ahead for bit allocation etc.
	 */
	cur_picture.mbinfo = (
		struct mbinfo *)bufalloc(mb_per_pict*sizeof(struct mbinfo));

	cur_picture.blocks =
		(int16_t (*)[64])bufalloc(mb_per_pict * block_count * sizeof(int16_t [64]));
  
  
/* open statistics output file */
	if(statname[0]=='-') statfile = stdout;
	else 
	if(!(statfile = fopen(statname,"w")))
	{
      sprintf(errortext,"Couldn't create statistics output file %s",statname);
      error(errortext);
	}

	ratectl = malloc(processors * sizeof(ratectl_t*));
	for(i = 0; i < processors; i++)
		ratectl[i] = calloc(1, sizeof(ratectl_t));
	


/* Start parallel threads */

//printf("init 1\n");
	start_motion_engines();
//printf("init 2\n");
	start_transform_engines();
//printf("init 3\n");
	start_itransform_engines();
//printf("init 4\n");
	start_slice_engines();
//printf("init 5\n");
}

void error(text)
char *text;
{
  fprintf(stderr,text);
  putc('\n',stderr);
  exit(1);
}

#define STRINGLEN 254

int calculate_smp()
{
/* Get processor count */
	int result = 1;
	FILE *proc;
	if(proc = fopen("/proc/cpuinfo", "r"))
	{
		char string[1024];
		while(!feof(proc))
		{
			fgets(string, 1024, proc);
			if(!strncasecmp(string, "processor", 9))
			{
				char *ptr = strchr(string, ':');
				if(ptr)
				{
					ptr++;
					result = atol(ptr) + 1;
				}
			}
			else
			if(!strncasecmp(string, "cpus detected", 13))
			{
				char *ptr = strchr(string, ':');
				if(ptr)
				{
					ptr++;
					result = atol(ptr);
				}
			}
		}
		fclose(proc);
	}
	return result;
}

static void readcmdline(int argc, char *argv[])
{
	int i, j;
	int h,m,s,f;
	FILE *fd;
	char line[256];
// Master frame rate table must match decoder
	static double ratetab[]=
    	{24000.0/1001.0,  // Official rates
		24.0,
		25.0,
		30000.0/1001.0,
		30.0,
		50.0,
		60000.0/1001.0,
		60.0,

		1,           // Unofficial economy rates
		5,
		10,
		12, 
		15,
		0,
		0};
// VBV buffer size limits
	int vbvlim[4] = { 597, 448, 112, 29 };
	long total_frame_rates = (sizeof(ratetab) / sizeof(double));
	FILE *proc;
// Default 16
	int param_searchrad = 16;
	int isnum = 1;

//printf("readcmdline 1\n");
	frame0 =                   0;  /* number of first frame */
	start_frame = end_frame = -1;
	use_hires_quant = 0;
	use_denoise_quant = 0;
	quiet = 1;
	bit_rate = 5000000;                       /* default bit_rate (bits/s) */
	prog_seq = 0;                        /* progressive_sequence is faster */
	mpeg1 = 0;                                   /* ISO/IEC 11172-2 stream */
    fixed_mquant = 0;                             /* vary the quantization */
	quant_floor = 0;
	act_boost = 3.0;
	N = 15;                                      /* N (# of frames in GOP) */
	M = 1;  /* M (I/P frame distance) */
	processors = calculate_smp();
	frame_rate = -1;
	chroma_format =            1;  /* chroma_format: 1=4:2:0, 2=4:2:2, 3=4:4:4   LibMPEG3 only does 1 */
	mpeg_file = 0;
	qt_file = 0;
	do_stdin = 0;
	do_buffers = 1;
	seq_header_every_gop = 0;
/* aspect_ratio_information 1=square pel, 2=4:3, 3=16:9, 4=2.11:1 */
	aspectratio = 1;  





//printf("readcmdline 2\n");


	sprintf(tplorg, "");
	sprintf(out_path, "");

#define INTTOYES(x) ((x) ? "Yes" : "No")
// This isn't used anymore as this is a library entry point.
  if(argc < 2)
  {
    printf("mpeg2encode V1.3, 2000/01/10\n"
"(C) 1996, MPEG Software Simulation Group\n"
"(C) 2001 Heroine Virtual\n"
"Usage: %s [options] <input file> <output file>\n\n"
" -1                 generate an MPEG-1 stream instead of MPEG-2 (%s)\n"
" -422                                 generate YUV 4:2:2 output\n"
" -b bitrate              fix the bitrate, vary the quantization (%d)\n"
" -d                                                     Denoise (%s)\n"
" -f rate                                      Convert framerate\n"
" -h                          High resolution quantization table (%s)\n"
" -m frames                set number of frames between P frames (%d)\n"
" -n frames                set number of frames between I frames (%d)\n"
" -p                                   encode progressive frames (%s)\n"
" -q quantization         fix the quantization, vary the bitrate\n"
" [number]               Start encoding from frame number to end\n"
" [number1] [number2]    Encode frame number 1 to frame number 2\n"
" -u                                        Use only 1 processor\n\n"
"Default settings:\n"
"   fixed 5000000 bits/sec\n"
"   interlaced\n"
"   MPEG-2\n"
"   15 frames between I frames   0 frames between P frames\n\n"
"For the recommended encoding parameters see docs/index.html.\n",
argv[0], 
mpeg1 ? "MPEG-1" : "MPEG-2",
(int)bit_rate,
INTTOYES(use_denoise_quant),
INTTOYES(use_hires_quant),
M - 1,
N,
INTTOYES(prog_seq));
    exit(1);
  }
//printf("readcmdline 3\n");

	for(i = 1; i < argc; i++)
	{
		isnum = 1;

		for(j = 0; j < strlen(argv[i]) && isnum; j++)
		{
			if(isalpha(argv[i][j])) isnum = 0;
		}


//printf("readcmdline %s\n", argv[i]);
		if(!strcmp(argv[i], "-1"))
		{
			mpeg1 = 1;
		}
		else
		if(!strcmp(argv[i], "-a"))
		{
			i++;
			if(i < argc)
			{
				aspectratio = atoi(argv[i]);
			}
			else
			{
				fprintf(stderr, "-i needs an aspect ratio enumeration.\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-b"))
		{
			i++;
			if(i < argc)
			{
				bit_rate = atol(argv[i]);
			}
			else
			{
				fprintf(stderr, "-b requires a bitrate\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-d"))
		{
			use_denoise_quant = 1;
		}
		else
		if(!strcmp(argv[i], "-f"))
		{
			i++;
			if(i < argc)
			{
				frame_rate = atof(argv[i]);
			}
			else
			{
				fprintf(stderr, "-f requires a frame rate\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-h"))
		{
			use_hires_quant = 1;
		}
		else
		if(!strcmp(argv[i], "-m"))
		{
			i++;
			if(i < argc)
			{
				M = atol(argv[i]) + 1;
			}
			else
			{
				fprintf(stderr, "-m requires a frame count\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-n"))
		{
			i++;
			if(i < argc)
			{
				N = atol(argv[i]);
			}
			else
			{
				fprintf(stderr, "-n requires a frame count\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-p"))
		{
			prog_seq = 1;
		}
		else
		if(!strcmp(argv[i], "-q"))
		{
			i++;
			if(i < argc)
			{
				fixed_mquant = atol(argv[i]);
			}
			else
			{
				fprintf(stderr, "-q requires a quantization value\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-u"))
		{
			processors = 1;
		}
		else
		if(!strcmp(argv[i], "-422"))
		{
			chroma_format = 2;
		}
		else
		if(!strcmp(argv[i], "-g"))
		{
			seq_header_every_gop = 1;
		}
		else
		if(!strcmp(argv[i], "-"))
		{
			do_stdin = 1;
		}
		else
/* Start or end frame if number */
		if(isnum)
		{
			if(start_frame < 0)
				start_frame = atol(argv[i]);
			else
			if(end_frame < 0)
				end_frame = atol(argv[i]);
		}
		else
		if(!strlen(tplorg) && !do_stdin && !do_buffers)
		{
/* Input path */
			strncpy(tplorg, argv[i], STRINGLEN);
		}
		else
		if(!strlen(out_path))
		{
/* Output path */
			strncpy(out_path, argv[i], STRINGLEN);
		}
	}
//printf("readcmdline 4\n");

	if(!strlen(out_path))
	{
// Default output path
		strncpy(out_path, tplorg, STRINGLEN);
		for(i = strlen(out_path) - 1; i >= 0 && out_path[i] != '.'; i--)
			;

		if(i < 0) i = strlen(out_path);
		
		if(mpeg1)
			sprintf(&out_path[i], ".m1v");
		else
			sprintf(&out_path[i], ".m2v");
	}
//printf("readcmdline 5\n");

/* Get info from input file */
	if(do_stdin)
	{
		inputtype = T_STDIN;
	}
	else
	if(do_buffers)
	{
		inputtype = T_BUFFERS;
	}
	else
	if(mpeg3_check_sig(tplorg))
	{
		int error_return;
		mpeg_file = mpeg3_open(tplorg, &error_return);
		inputtype = T_MPEG;
	}
	else
	if(quicktime_check_sig(tplorg))
	{
		qt_file = quicktime_open(tplorg, 1, 0);
		inputtype = T_QUICKTIME;
	}
//printf("readcmdline 6\n");

	if(!qt_file && !mpeg_file && !do_stdin && !do_buffers)
	{
		fprintf(stderr, "File format not recognized.\n");
		exit(1);
	}

//printf("readcmdline 7\n");
	if(qt_file)
	{
		if(!quicktime_video_tracks(qt_file))
		{
			fprintf(stderr, "No video tracks in file.\n");
			exit(1);
		}

		if(!quicktime_supported_video(qt_file, 0))
		{
			fprintf(stderr, "Unsupported video codec.\n");
			exit(1);
		}
	}
//printf("readcmdline 8\n");

/************************************************************************
 *                            BEGIN PARAMETER FILE
 ************************************************************************/

/* To eliminate the user hassle we replaced the parameter file with hard coded constants. */
	strcpy(tplref,             "-");  /* name of intra quant matrix file	 ("-": default matrix) */
	strcpy(iqname,             "-");  /* name of intra quant matrix file	 ("-": default matrix) */
	strcpy(niqname,            "-");  /* name of non intra quant matrix file ("-": default matrix) */
	strcpy(statname,           "/dev/null");  /* name of statistics file ("-": stdout ) */

	if(qt_file)
	{
		nframes =                  quicktime_video_length(qt_file, 0);  /* number of frames */
		horizontal_size =          quicktime_video_width(qt_file, 0);
		vertical_size =            quicktime_video_height(qt_file, 0);
	}
	else
	if(mpeg_file)
	{
		nframes =                  0x7fffffff;  /* Use percentage instead */
		horizontal_size =          mpeg3_video_width(mpeg_file, 0);
		vertical_size =            mpeg3_video_height(mpeg_file, 0);
	}
	else
	if(do_stdin)
	{
		unsigned char data[1024];
		nframes =                  0x7fffffff;
		
		fgets(data, 1024, stdin_fd);
		horizontal_size =          atol(data);
		fgets(data, 1024, stdin_fd);
		vertical_size =            atol(data);
	}
	else
	if(do_buffers)
	{
		nframes = 0x7fffffff;
	}
	
	
	h = m = s = f =            0;  /* timecode of first frame */
	fieldpic =                 0;  /* 0: progressive, 1: bottom first, 2: top first, 3 = progressive seq, field MC and DCT in picture */
	low_delay =                0;  /* low_delay  */
	constrparms =              0;  /* constrained_parameters_flag */
	profile =                  4;  /* Profile ID: Simple = 5, Main = 4, SNR = 3, Spatial = 2, High = 1 */
	level =                    4;  /* Level ID:   Low = 10, Main = 8, High 1440 = 6, High = 4		   */
	video_format =             2;  /* video_format: 0=comp., 1=PAL, 2=NTSC, 3=SECAM, 4=MAC, 5=unspec. */
	color_primaries =          5;  /* color_primaries */
	dctsatlim		= mpeg1 ? 255 : 2047;
	dctsatlim = 255;
	transfer_characteristics = 5;  /* transfer_characteristics */
	matrix_coefficients =      4;  /* matrix_coefficients (not used) */
	display_horizontal_size =  horizontal_size;
	display_vertical_size =    vertical_size;
	cur_picture.dc_prec =      0;  /* intra_dc_precision (0: 8 bit, 1: 9 bit, 2: 10 bit, 3: 11 bit */
	cur_picture.topfirst =     1;  /* top_field_first */

	frame_pred_dct_tab[0] =    mpeg1 ? 1 : 0;  /* frame_pred_frame_dct (I P B) */
	frame_pred_dct_tab[1] =    mpeg1 ? 1 : 0;  /* frame_pred_frame_dct (I P B) */
	frame_pred_dct_tab[2] =    mpeg1 ? 1 : 0;  /* frame_pred_frame_dct (I P B) */

	conceal_tab[0]  		 = 0;  /* concealment_motion_vectors (I P B) */
	conceal_tab[1]  		 = 0;  /* concealment_motion_vectors (I P B) */
	conceal_tab[2]  		 = 0;  /* concealment_motion_vectors (I P B) */
	qscale_tab[0]   		 = mpeg1 ? 0 : 1;  /* q_scale_type  (I P B) */
	qscale_tab[1]   		 = mpeg1 ? 0 : 1;  /* q_scale_type  (I P B) */
	qscale_tab[2]   		 = mpeg1 ? 0 : 1;  /* q_scale_type  (I P B) */

	intravlc_tab[0] 		 = 0;  /* intra_vlc_format (I P B)*/
	intravlc_tab[1] 		 = 0;  /* intra_vlc_format (I P B)*/
	intravlc_tab[2] 		 = 0;  /* intra_vlc_format (I P B)*/
	altscan_tab[0]  		 = 0;  /* alternate_scan_hv (I P B) */
	altscan_tab[1]  		 = 0;  /* alternate_scan_hv (I P B) */
	altscan_tab[2]  		 = 0;  /* alternate_scan_hv (I P B) */
	opt_dc_prec         = 0;  /* 8 bits */
	opt_topfirst        = (fieldpic == 2);
	opt_repeatfirst     = 0;
	opt_prog_frame      = prog_seq;
	cur_picture.repeatfirst =              0;  /* repeat_first_field */
	cur_picture.prog_frame =               prog_seq;  /* progressive_frame */
/* P:  forw_hor_f_code forw_vert_f_code search_width/height */
    motion_data = (struct motion_data *)malloc(3 * sizeof(struct motion_data));
	video_buffer_size =        46 * 1024 * 8;

/************************************************************************
 *                                END PARAMETER FILE
 ************************************************************************/
//printf("readcmdline 10\n");

	if(mpeg1)
	{
		opt_prog_frame = 1;
		cur_picture.prog_frame = 1;
		prog_seq = 1;
	}

	if(qt_file)
	{
		input_frame_rate = quicktime_frame_rate(qt_file, 0);
	}
	else
	if(mpeg_file)
	{
		input_frame_rate = mpeg3_frame_rate(mpeg_file, 0);
	}
	else
	if(do_stdin)
	{
		char data[1024];
		
		fgets(data, 1024, stdin_fd);
		
		input_frame_rate = atof(data);
	}
	
	
	if(frame_rate < 0)
	{
		frame_rate = input_frame_rate;
	}
//printf("readcmdline 11\n");

//processors = 1;
//nframes = 16;
	if(start_frame >= 0 && end_frame >= 0)
	{
		nframes = end_frame - start_frame;
		frame0 = start_frame;
	}
	else
	if(start_frame >= 0)
	{
		end_frame = nframes;
		nframes -= start_frame;
		frame0 = start_frame;
	}
	else
	{
		start_frame = 0;
		end_frame = nframes;
	}
//printf("readcmdline 12\n");

// Show status
	if(verbose)
	{
		printf("Encoding: %s frames %ld\n", out_path, nframes);

    	if(fixed_mquant == 0) 
    		printf("   bitrate %.0f\n", bit_rate);
    	else
    		printf("   quantization %d\n", fixed_mquant);
    	printf("   %d frames between I frames   %d frames between P frames\n", N, M - 1);
    	printf("   %s\n", (prog_seq ? "progressive" : "interlaced"));
		printf("   %s\n", (mpeg1 ? "MPEG-1" : "MPEG-2"));
		printf("   %s\n", (chroma_format == 1) ? "YUV-420" : "YUV-422");
		printf("   %d processors\n", processors);
		printf("   %.02f frames per second\n", frame_rate);
		printf("   Denoise %s\n", INTTOYES(use_denoise_quant));
		printf("   Aspect ratio index %d\n", aspectratio);
		printf("   Hires quantization %s\n", INTTOYES(use_hires_quant));


		if(mpeg_file)
		{
			fprintf(stderr, "(MPEG to MPEG transcoding for official use only.)\n");
		}
	}



	{ 
		int radius_x = ((param_searchrad + 4) / 8) * 8;
		int radius_y = ((param_searchrad * vertical_size / horizontal_size + 4) / 8) * 8;
		int c;
	
		/* TODO: These f-codes should really be adjusted for each
		   picture type... */
		c=5;
		if( radius_x*M < 64) c = 4;
		if( radius_x*M < 32) c = 3;
		if( radius_x*M < 16) c = 2;
		if( radius_x*M < 8) c = 1;

		if (!motion_data)
			error("malloc failed\n");

		for (i=0; i<M; i++)
		{
			if(i==0)
			{
				motion_data[i].forw_hor_f_code  = c;
				motion_data[i].forw_vert_f_code = c;
				motion_data[i].sxf = MAX(1,radius_x*M);
				motion_data[i].syf = MAX(1,radius_y*M);
			}
			else
			{
				motion_data[i].forw_hor_f_code  = c;
				motion_data[i].forw_vert_f_code = c;
				motion_data[i].sxf = MAX(1,radius_x*i);
				motion_data[i].syf = MAX(1,radius_y*i);
				motion_data[i].back_hor_f_code  = c;
				motion_data[i].back_vert_f_code = c;
				motion_data[i].sxb = MAX(1,radius_x*(M-i));
				motion_data[i].syb = MAX(1,radius_y*(M-i));
			}
		}
		
	}

//    vbv_buffer_size = floor(((double)bit_rate * 0.20343) / 16384.0);
    if(mpeg1)
		vbv_buffer_size = 20 * 16384;
	else
		vbv_buffer_size = 112 * 16384;

	fast_mc_frac    = 10;
	mc_44_red		= 2;
	mc_22_red		= 3;

    if(vbv_buffer_size > vbvlim[(level - 4) >> 1])
        vbv_buffer_size = vbvlim[(level - 4) >> 1];

/* Set up frame buffers */
	frame_buffer = malloc(horizontal_size * vertical_size * 3 + 4);
	row_pointers = malloc(sizeof(unsigned char*) * vertical_size);
	for(i = 0; i < vertical_size; i++) row_pointers[i] = &frame_buffer[horizontal_size * 3 * i];

// Get frame rate code from input frame rate
	for(i = 0; i < total_frame_rates; i++)
	{
		if(fabs(frame_rate - ratetab[i]) < 0.001) frame_rate_code = i + 1;
	}

/* make flags boolean (x!=0 -> x=1) */
  mpeg1 = !!mpeg1;
  fieldpic = !!fieldpic;
  low_delay = !!low_delay;
  constrparms = !!constrparms;
  prog_seq = !!prog_seq;
  cur_picture.topfirst = !!cur_picture.topfirst;

  for (i = 0; i < 3; i++)
  {
    frame_pred_dct_tab[i] = !!frame_pred_dct_tab[i];
    conceal_tab[i] = !!conceal_tab[i];
    qscale_tab[i] = !!qscale_tab[i];
    intravlc_tab[i] = !!intravlc_tab[i];
    altscan_tab[i] = !!altscan_tab[i];
  }
  cur_picture.repeatfirst = !!cur_picture.repeatfirst;
  cur_picture.prog_frame = !!cur_picture.prog_frame;

  /* make sure MPEG specific parameters are valid */
  range_checks();

  /* timecode -> frame number */
  tc0 = h;
  tc0 = 60*tc0 + m;
  tc0 = 60*tc0 + s;
  tc0 = (int)(frame_rate+0.5)*tc0 + f;

  qt_output = 0;










  if (!mpeg1)
  {
    profile_and_level_checks();
  }
  else
  {
    /* MPEG-1 */
    if (constrparms)
    {
      if (horizontal_size>768
          || vertical_size>576
          || ((horizontal_size+15)/16)*((vertical_size+15) / 16) > 396
          || ((horizontal_size+15)/16)*((vertical_size+15) / 16)*frame_rate>396*25.0
          || frame_rate>30.0)
      {
        if (!quiet)
          fprintf(stderr,"*** Warning: setting constrained_parameters_flag = 0\n");
        constrparms = 0;
      }
    }
  }

  /* relational checks */

  if (mpeg1)
  {
    if (!prog_seq)
    {
      prog_seq = 1;
    }

    if (chroma_format!=CHROMA420)
    {
      chroma_format = CHROMA420;
    }

    if (cur_picture.dc_prec!=0)
    {
      cur_picture.dc_prec = 0;
    }

    for (i=0; i<3; i++)
      if (qscale_tab[i])
      {
        qscale_tab[i] = 0;
      }

    for (i=0; i<3; i++)
      if (intravlc_tab[i])
      {
        intravlc_tab[i] = 0;
      }

    for (i=0; i<3; i++)
      if (altscan_tab[i])
      {
        altscan_tab[i] = 0;
      }
  }

  if (!mpeg1 && constrparms)
  {
    constrparms = 0;
  }

  if (prog_seq && !cur_picture.prog_frame)
  {
    cur_picture.prog_frame = 1;
  }

  if (cur_picture.prog_frame && fieldpic)
  {
    fieldpic = 0;
  }

  if (!cur_picture.prog_frame && cur_picture.repeatfirst)
  {
    cur_picture.repeatfirst = 0;
  }

  if (cur_picture.prog_frame)
  {
    for (i=0; i<3; i++)
      if (!frame_pred_dct_tab[i])
      {
        frame_pred_dct_tab[i] = 1;
      }
  }

  if (prog_seq && !cur_picture.repeatfirst && cur_picture.topfirst)
  {
     if (!quiet)
       fprintf(stderr,"Warning: setting top_field_first = 0\n");
    cur_picture.topfirst = 0;
  }

  /* search windows */
  for (i=0; i<M; i++)
  {
    if (motion_data[i].sxf > (4<<motion_data[i].forw_hor_f_code)-1)
    {
      if (!quiet)
        fprintf(stderr,
          "Warning: reducing forward horizontal search width to %d\n",
          (4<<motion_data[i].forw_hor_f_code)-1);
      motion_data[i].sxf = (4<<motion_data[i].forw_hor_f_code)-1;
    }

    if (motion_data[i].syf > (4<<motion_data[i].forw_vert_f_code)-1)
    {
      if (!quiet)
        fprintf(stderr,
          "Warning: reducing forward vertical search width to %d\n",
          (4<<motion_data[i].forw_vert_f_code)-1);
      motion_data[i].syf = (4<<motion_data[i].forw_vert_f_code)-1;
    }

    if (i!=0)
    {
      if (motion_data[i].sxb > (4<<motion_data[i].back_hor_f_code)-1)
      {
        if (!quiet)
          fprintf(stderr,
            "Warning: reducing backward horizontal search width to %d\n",
            (4<<motion_data[i].back_hor_f_code)-1);
        motion_data[i].sxb = (4<<motion_data[i].back_hor_f_code)-1;
      }

      if (motion_data[i].syb > (4<<motion_data[i].back_vert_f_code)-1)
      {
        if (!quiet)
          fprintf(stderr,
            "Warning: reducing backward vertical search width to %d\n",
            (4<<motion_data[i].back_vert_f_code)-1);
        motion_data[i].syb = (4<<motion_data[i].back_vert_f_code)-1;
      }
    }
  }

}

/*
  If the user has selected suppression of hf noise via
  quantisation then we boost quantisation of hf components
  EXPERIMENTAL: currently a linear ramp from 0 at 4pel to 
  50% increased quantisation...
*/

static int quant_hfnoise_filt(int orgquant, int qmat_pos )
{
	int x = qmat_pos % 8;
	int y = qmat_pos / 8;
	int qboost = 1024;

	if(!use_denoise_quant)
	{
		return orgquant;
	}

	/* Maximum 50% quantisation boost for HF components... */
	if( x > 4 )
		qboost += (256*(x-4)/3);
	if( y > 4 )
		qboost += (256*(y-4)/3);

	return (orgquant * qboost + 512)/ 1024;
}

static void readquantmat()
{
  int i,v,q;
	load_iquant = 0;
	load_niquant = 0;

  if (iqname[0]=='-')
  {
  	if(use_hires_quant)
	{
		load_iquant |= 1;
		for (i=0; i<64; i++)
		{
			intra_q[i] = hires_intra_quantizer_matrix_hv[i];
		}	
	}
	else
	{
		load_iquant = use_denoise_quant;
		for (i=0; i<64; i++)
		{
			v = quant_hfnoise_filt(default_intra_quantizer_matrix_hv[i], i);
			if (v<1 || v>255)
				error("value in intra quant matrix invalid (after noise filt adjust)");
			intra_q[i] = v;
		} 
	}
  }

/* TODO: Inv Quant matrix initialisation should check if the fraction fits in 16 bits! */
  if (niqname[0]=='-')
  {
		if(use_hires_quant)
		{
			for (i=0; i<64; i++)
			{
				inter_q[i] = hires_nonintra_quantizer_matrix_hv[i];
			}	
		}
		else
		{
/* default non-intra matrix is all 16's. For *our* default we use something
   more suitable for domestic analog sources... which is non-standard...*/
			load_niquant |= 1;
			for (i=0; i<64; i++)
			{
				v = quant_hfnoise_filt(default_nonintra_quantizer_matrix_hv[i],i);
				if (v<1 || v>255)
					error("value in non-intra quant matrix invalid (after noise filt adjust)");
				inter_q[i] = v;
			}
		}
  }

	for (i=0; i<64; i++)
	{
		i_intra_q[i] = (int)(((double)IQUANT_SCALE) / ((double)intra_q[i]));
		i_inter_q[i] = (int)(((double)IQUANT_SCALE) / ((double)inter_q[i]));
	}
	
	for( q = 1; q <= 112; ++q )
	{
		for (i=0; i<64; i++)
		{
			intra_q_tbl[q][i] = intra_q[i] * q;
			inter_q_tbl[q][i] = inter_q[i] * q;
			intra_q_tblf[q][i] = (float)intra_q_tbl[q][i];
			inter_q_tblf[q][i] = (float)inter_q_tbl[q][i];
			i_intra_q_tblf[q][i] = 1.0f/ ( intra_q_tblf[q][i] * 0.98);
			i_intra_q_tbl[q][i] = (IQUANT_SCALE/intra_q_tbl[q][i]);
			i_inter_q_tblf[q][i] =  1.0f/ (inter_q_tblf[q][i] * 0.98);
			i_inter_q_tbl[q][i] = (IQUANT_SCALE/inter_q_tbl[q][i] );
		}
	}
}
