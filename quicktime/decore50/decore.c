/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani
 * Adam Li
 * Jonathan White
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// decore.c //

#include <stdio.h>
#include <stdlib.h>

#if ( (defined (WIN32)) && (! defined (_DECORE)) )
#include <string.h>
#include <io.h>
#include <fcntl.h>
#endif

#include "gen_usetime.h"
#include "debug.h"
#include "mp4_vars.h"
#include "getbits.h"
#include "yuv2rgb.h"
#include "decore.h"

/**
 *
**/

/***/

static int flag_firstpicture = 1;

/***/

// Set global variables for a decoding session
void decore_set_global(DEC_PARAM *param)
{
	DEC_BUFFERS *buffers;
	int cc;

	if(param)
	{
		buffers = &param->buffers;
		mp4_state = (MP4_STATE*)buffers->mp4_state;
		mp4_tables = (MP4_TABLES*)buffers->mp4_tables;
		ld = (MP4_STREAM *)buffers->mp4_stream;
		for(cc = 0; cc < 3; cc++)
		{
			edged_ref[cc] = buffers->edged_ref[cc];
			edged_for[cc] = buffers->edged_for[cc];
			frame_ref[cc] = buffers->frame_ref[cc];
			frame_for[cc] = buffers->frame_for[cc];
			display_frame[cc] = buffers->display_frame[cc];
		}
	}
}

void decore_save_global(DEC_PARAM *param)
{
	DEC_BUFFERS *buffers;
	int cc;

	if(param)
	{
		buffers = &param->buffers;
		buffers->mp4_state = mp4_state;
		buffers->mp4_tables = mp4_tables;
		buffers->mp4_stream = ld;
		for(cc = 0; cc < 3; cc++)
		{
			buffers->edged_ref[cc] = edged_ref[cc];
			buffers->edged_for[cc] = edged_for[cc];
			buffers->frame_ref[cc] = frame_ref[cc];
			buffers->frame_for[cc] = frame_for[cc];
			buffers->display_frame[cc] = display_frame[cc];
		}
	}
}

static int mmx_test()
{
	int result = 0;
	FILE *proc;
	char string[1024];


#ifdef ARCH_X86
	if(!(proc = fopen("/proc/cpuinfo", "r")))
	{
		fprintf(stderr, "mmx_test: failed to open /proc/cpuinfo\n");
		return 0;
	}
	
	while(!feof(proc))
	{
		fgets(string, 1024, proc);
/* Got the flags line */
		if(!strncasecmp(string, "flags", 5))
		{
			char *needle;
			needle = strstr(string, "mmx");
			if(!needle)
            {
            	fclose(proc);
            	return 0;
            }
			if(!strncasecmp(needle, "mmx", 3))
            {
            	fclose(proc);
            	return 1;
            }
		}
	}
   	fclose(proc);
#endif

	return 0;
}


int STDCALL decore(unsigned long handle, unsigned long dec_opt,
	void *param1, void *param2)
{
	int cc;



/*
 * printf("decore 1 %p %p %p %p %p\n", 
 * edged_ref[0], edged_for[0], frame_ref[0], frame_for[0], display_frame[0]);
 */

	if (handle) 
	{
		switch (dec_opt)
		{
			case DEC_OPT_MEMORY_REQS:
			{
				DEC_PARAM *dec_param = (DEC_PARAM *)param1;
				DEC_MEM_REQS *dec_mem_reqs = (DEC_MEM_REQS *)param2;

				int coded_y_size = ((dec_param->x_dim + 64) * (dec_param->y_dim + 64));
				int coded_c_size = (((dec_param->x_dim>>1) + 64) * ((dec_param->y_dim>>1) + 64));
				int display_y_size = (dec_param->x_dim * dec_param->y_dim);
				int display_c_size = ((dec_param->x_dim * dec_param->y_dim) >> 2);
				int edged_size = coded_y_size + (2 * coded_c_size);
				int display_size = display_y_size + (2 * display_c_size);

				dec_mem_reqs->mp4_edged_ref_buffers_size = edged_size;
				dec_mem_reqs->mp4_edged_for_buffers_size = edged_size;
				dec_mem_reqs->mp4_display_buffers_size = display_size;
				dec_mem_reqs->mp4_state_size = sizeof(MP4_STATE);
				dec_mem_reqs->mp4_tables_size = sizeof(MP4_TABLES);
				dec_mem_reqs->mp4_stream_size = sizeof(MP4_STREAM);

				return DEC_OK;
			}
			case DEC_OPT_INIT:
			{
				DEC_PARAM *dec_param = (DEC_PARAM *) param1;

//				have_mmx = mmx_test();
				decore_init(dec_param->x_dim, 
					dec_param->y_dim, 
					dec_param->output_format,
					dec_param->time_incr, 
					dec_param->buffers); // init decoder resources

				return DEC_OK;
			}
			break; 
			case DEC_OPT_RELEASE:
			{
				decore_release();

				return DEC_OK;
			}
			break;
			case DEC_OPT_SETPP:
			{
				DEC_SET *dec_set = (DEC_SET *) param1;
				int postproc_level = dec_set->postproc_level;

				if ((postproc_level < 0) | (postproc_level > 100))
					return DEC_BAD_FORMAT;

				if (postproc_level < 1) 
				{
					mp4_state->post_flag = 0;

					return DEC_OK;
				}
				else 
				{
					mp4_state->post_flag = 1;

					if (postproc_level < 10) {
						mp4_state->pp_options = PP_DEBLOCK_Y_H;
					}
					else if (postproc_level < 20) {
						mp4_state->pp_options = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V;
					}
					else if (postproc_level < 30) {
						mp4_state->pp_options = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DERING_Y;
					}
					else if (postproc_level < 40) {
						mp4_state->pp_options = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DERING_Y | PP_DEBLOCK_C_H;
					}
					else if (postproc_level < 50) {
						mp4_state->pp_options = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DERING_Y | 
							PP_DEBLOCK_C_H | PP_DEBLOCK_C_V;
					}
					else {
						mp4_state->pp_options = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DERING_Y | 
							PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_C;
					}
				}

				return DEC_OK;
			}
			break;
			case DEC_OPT_SETOUT:
			{
				DEC_PARAM *dec_param = (DEC_PARAM *) param1;

				decore_setoutput(dec_param->output_format);

				return DEC_OK;
			}
			break;
			default:
			{
				DEC_FRAME *dec_frame = (DEC_FRAME *) param1;


				if (decore_frame(dec_frame->bitstream, dec_frame->length, 
					dec_frame->bmp, dec_frame->stride, dec_frame->render_flag))
				{
/*
 * printf("decore 2 %p %p %p %p %p\n", 
 * edged_ref[0], edged_for[0], frame_ref[0], frame_for[0], display_frame[0]);
 */
					return DEC_OK;
				}
				else 
				{
					return DEC_EXIT;
				}
			}
			break;
		}
	}
	return DEC_BAD_FORMAT;
}

/***/

int decore_alloc(DEC_BUFFERS buffers);

static int decore_init(int hor_size, 
	int ver_size, 
	int output_format,
	int time_inc, 
	DEC_BUFFERS buffers)
{
	mp4_state = (MP4_STATE *) buffers.mp4_state;
	mp4_tables = (MP4_TABLES *) buffers.mp4_tables;
	ld = (MP4_STREAM *) buffers.mp4_stream;

#ifndef _DECORE
	// open input file
	if ((ld->infile = open (mp4_state->infilename, O_RDONLY | O_BINARY)) < 0) {
		_Print ("Input file %s not found\n", mp4_state->infilename);
		exit(91);
	}
	initbits (NULL, 0);
	mp4_state->juice_flag = 0;
#else 
	mp4_state->juice_flag = 1;
#endif // _DECORE

	mp4_state->post_flag = 0;

	// read first vol and vop
	mp4_state->hdr.width = hor_size;
	mp4_state->hdr.height = ver_size;
	
	mp4_state->hdr.quant_precision = 5;
	mp4_state->hdr.bits_per_pixel = 8;
	
	mp4_state->hdr.quant_type = 0;

	if (flag_firstpicture == 1) {
		mp4_state->hdr.time_increment_resolution = 15;
		flag_firstpicture = 0;
	}
	mp4_state->hdr.complexity_estimation_disable = 1;

	decore_alloc (buffers);
	decore_setoutput (output_format);

	return 1;
}

/***/

int decore_alloc(DEC_BUFFERS buffers)
{
	mp4_state->hdr.picnum = 0;
	mp4_state->hdr.mb_xsize = mp4_state->hdr.width / 16;
	mp4_state->hdr.mb_ysize = mp4_state->hdr.height / 16;
	mp4_state->hdr.mba_size = mp4_state->hdr.mb_xsize * mp4_state->hdr.mb_ysize;

	// set picture dimension global vars
	{
		mp4_state->horizontal_size = mp4_state->hdr.width;
		mp4_state->vertical_size = mp4_state->hdr.height;

		mp4_state->mb_width = mp4_state->horizontal_size / 16;
		mp4_state->mb_height = mp4_state->vertical_size / 16;

		mp4_state->coded_picture_width = mp4_state->horizontal_size + 64;
		mp4_state->coded_picture_height = mp4_state->vertical_size + 64;
		mp4_state->chrom_width = mp4_state->coded_picture_width >> 1;
		mp4_state->chrom_height = mp4_state->coded_picture_height >> 1;
	}

	// init decoder
	initdecoder (buffers);

	return 1;
}

/***/

int decore_frame(unsigned char *stream, int length, unsigned char *bmp[], 
	unsigned int stride, int render_flag)
{
#ifndef _DECORE
	mp4_state->juice_flag = 0;
	_SetPrintCond(0, 1000, 0, 1000);
	_Print("- Picture %d\r", mp4_state->hdr.picnum);
#else
	initbits (stream, length);
#endif // _DECORE

	// mp4_state->hdr.time_increment_resolution = 15; // [Ag][Review] This must be passed by the app!
//printf("decore_frame 1\n");

	getvolhdr();
	getgophdr();	

//printf("decore_frame 1\n");
	if (! getvophdr()) // read vop header
		return 0;
//printf("decore_frame 1\n");

	get_mp4picture(bmp, stride, render_flag); // decode vop
	mp4_state->hdr.picnum++;
//printf("decore_frame 2\n");

	return 1;
}

/***/

int decore_release()
{
	closedecoder();

	/*
			I have to check and close the decoder only when it is really been opened: 
			for some reason VirtualDub first of all wants to close the decoder and this 
			cause a free(nothing) and a crash.
	*/
#ifndef _DECORE
	close (ld->infile);
#endif // _DECORE

	return 1;
}

/***/

int decore_setoutput(int output_format)
{
	mp4_state->flag_invert = +1;

	switch (output_format)
	{
	case DEC_RGB32:
		mp4_state->convert_yuv = yuv2rgb_32;
		mp4_state->flag_invert = -1;
		break;
	case DEC_RGB32_INV:
		mp4_state->convert_yuv = yuv2rgb_32;
		mp4_state->flag_invert = +1;
		break;
	case DEC_RGB24:
		mp4_state->convert_yuv = yuv2rgb_24;
		mp4_state->flag_invert = -1;
		break;
	case DEC_RGB24_INV:
		mp4_state->convert_yuv = yuv2rgb_24;
		mp4_state->flag_invert = +1;
		break;
	case DEC_RGB555:
		mp4_state->convert_yuv = yuv2rgb_555;
		mp4_state->flag_invert = -1;
		break;
	case DEC_RGB555_INV:
		mp4_state->convert_yuv = yuv2rgb_555;
		mp4_state->flag_invert = +1;
		break;
	case DEC_RGB565:
		mp4_state->convert_yuv = yuv2rgb_565;
		mp4_state->flag_invert = -1;
		break;
	case DEC_RGB565_INV:
		mp4_state->convert_yuv = yuv2rgb_565;
		mp4_state->flag_invert = +1;
		break;
	case DEC_420:
		mp4_state->convert_yuv = yuv12_out;
		break;
	case DEC_YUV2:
		mp4_state->convert_yuv = yuy2_out;
		break;
	case DEC_UYVY:
		mp4_state->convert_yuv = uyvy_out;
		break;
	}

	return 1;
}

/**
 *	for a console application
**/

#ifndef _DECORE

/***/

static int dec_reinit();

static void options (int *argcp, char **argvp[]);
static void optionhelp (int *argcp);

/***

int main (int argc, char *argv[])
{
	char * infilename = argv[1];
	mp4_state->output_flag = mp4_state->juice_flag = mp4_state->post_flag = 0;

	// decode args from input line
	optionhelp (&argc);
  options (&argc, &argv);

//	startTimer();

	dec_init(infilename, mp4_state->juice_hor, mp4_state->juice_ver); // init decoder resources

	_SetPrintCond(0, 1000, 0, 1000);

  while (dec_frame()) // cycle on decoding engine
		;

//	stopTimer();
//	displayTimer(mp4_hdr.picnum);

  dec_release(); // release decoder resources

  return 0;
}

***/

int main (int argc, char *argv[])
{
	char * infilename = argv[1];
	char outputfilename[256] = "Test.yuv";

	DEC_MEM_REQS decMemReqs;
	DEC_PARAM decParam;

	decParam.x_dim = 352;
	decParam.y_dim = 288;
	decParam.output_format = 0;
	decParam.time_incr = 0;
	
	decore(1, DEC_OPT_MEMORY_REQS, &decParam, &decMemReqs);

	decParam.buffers.mp4_edged_ref_buffers = malloc(decMemReqs.mp4_edged_ref_buffers_size);
	decParam.buffers.mp4_edged_for_buffers = malloc(decMemReqs.mp4_edged_for_buffers_size);
	decParam.buffers.mp4_display_buffers = malloc(decMemReqs.mp4_display_buffers_size);
	decParam.buffers.mp4_state = malloc(decMemReqs.mp4_state_size);
	decParam.buffers.mp4_tables = malloc(decMemReqs.mp4_tables_size);
	decParam.buffers.mp4_stream = malloc(decMemReqs.mp4_stream_size);

	memset(decParam.buffers.mp4_state, 0, decMemReqs.mp4_state_size);
	memset(decParam.buffers.mp4_tables, 0, decMemReqs.mp4_tables_size);
	memset(decParam.buffers.mp4_stream, 0, decMemReqs.mp4_stream_size);

	((MP4_STATE *) decParam.buffers.mp4_state)->infilename = infilename;
	((MP4_STATE *) decParam.buffers.mp4_state)->outputname = outputfilename;

	decore(1, DEC_OPT_INIT, &decParam, NULL);

	startTimer();

	// decode frames
	{
		DEC_FRAME decFrame;

		decFrame.bitstream = NULL;
		decFrame.bmp = NULL;
		decFrame.length = 0;
		decFrame.render_flag = 0;

		while ( decore(1, 0, &decFrame, NULL) == DEC_OK )
			;
	}

	stopTimer();
	displayTimer(mp4_state->hdr.picnum);

	return 1;
}

/***/

static void options (int *argcp, char **argvp[])
{
	(*argvp)++;
	(*argcp)--;

  while (*argcp > 1 && (*argvp)[1][0] == '-')
  {
    switch (toupper ((*argvp)[1][1]))
    {
      case 'O':
				mp4_state->output_flag = 1;
        mp4_state->outputname = (*argvp)[2];
				(*argvp) += 2;
				(*argcp) -= 2;
				break;
			case 'J':
				mp4_state->juice_flag = 1;
				mp4_state->juice_hor = atoi ((*argvp)[2]);
				mp4_state->juice_ver = atoi ((*argvp)[3]);
		    (*argvp) += 3;
				(*argcp) -= 3;
				break;
      default:
        printf ("Error: Undefined option -%c ignored\n", (*argvp)[1][1]);
    }
  }
}

/***/

static void optionhelp(int *argcp)
{
  if (*argcp < 2)
  {
    _Print ("Usage: opendivx_dec bitstream {options} \n");
		_Print ("Options: -o {outputfilename} YUV concatenated file output\n");
		_Print ("         -j {hor_size ver_size} juice stream and its picture format\n");
    exit (0);
  }
}

/***

int dec_init(char *infilename, int hor_size, int ver_size, DEC_BUFFERS buffers)
{
	mp4_state = (MP4_STATE *) buffers.mp4_state;
	mp4_tables = (MP4_TABLES *) buffers.mp4_tables;

	// open input file
	if ((mp4_state->ld.infile = open (infilename, O_RDONLY | O_BINARY)) < 0) {
		_Print ("Input file %s not found\n", infilename);
		exit(91);
	}

	initbits (NULL);

	// read header info (contains info to correctly initialize the decoder)
	getvolhdr();

	getgophdr();
	getvophdr(); // read vop header

	decore_alloc(buffers);

	return 1;
}



int dec_frame()
{
	// decoded vop
	get_mp4picture(NULL, 0, 0);
	mp4_state->hdr.picnum++;

	_Print("- Picture %d\r", mp4_state->hdr.picnum);

	// read next vop header
	getvolhdr();
	getgophdr();
	return getvophdr(); 
}



int dec_release()
{
  close (mp4_state->ld.infile);
	decore_release();

	return 1;
}

***/

int dec_reinit()
{
  if (ld->infile != 0)
    lseek (ld->infile, 0l, 0);
  initbits (NULL, 0);

	return 1;
}

/***/

#endif // !_DECORE
