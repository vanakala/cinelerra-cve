/* puthdr.c, generation of headers                                          */

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
#include <math.h>
#include "config.h"
#include "global.h"

/* private prototypes */
static int frametotc _ANSI_ARGS_((int frame));

/* generate sequence header (6.2.2.1, 6.3.3)
 *
 * matrix download not implemented
 */
void putseqhdr()
{
  int i;

  alignbits();
  mpeg2enc_putbits(SEQ_START_CODE,32); /* sequence_header_code */
  mpeg2enc_putbits(horizontal_size,12); /* horizontal_size_value */
  mpeg2enc_putbits(vertical_size,12); /* vertical_size_value */
  mpeg2enc_putbits(aspectratio,4); /* aspect_ratio_information */
  mpeg2enc_putbits(frame_rate_code,4); /* frame_rate_code */
  mpeg2enc_putbits((int)ceil(bit_rate/400.0),18); /* bit_rate_value */
  mpeg2enc_putbits(1,1); /* marker_bit */
  mpeg2enc_putbits(vbv_buffer_size,10); /* vbv_buffer_size_value */
  mpeg2enc_putbits(constrparms,1); /* constrained_parameters_flag */

  mpeg2enc_putbits(load_iquant,1); /* load_intra_quantizer_matrix */
  if (load_iquant)
    for (i=0; i<64; i++)  /* matrices are always downloaded in zig-zag order */
      mpeg2enc_putbits(intra_q[mpeg2_zig_zag_scan[i]],8); /* intra_quantizer_matrix */

  mpeg2enc_putbits(load_niquant,1); /* load_non_intra_quantizer_matrix */
  if (load_niquant)
    for (i=0; i<64; i++)
      mpeg2enc_putbits(inter_q[mpeg2_zig_zag_scan[i]],8); /* non_intra_quantizer_matrix */
	if (!mpeg1)
	{
		putseqext();
		putseqdispext();
	}
}

/* generate sequence extension (6.2.2.3, 6.3.5) header (MPEG-2 only) */
void putseqext()
{
  alignbits();
  mpeg2enc_putbits(EXT_START_CODE,32); /* extension_start_code */
  mpeg2enc_putbits(SEQ_ID,4); /* extension_start_code_identifier */
  mpeg2enc_putbits((profile<<4)|level,8); /* profile_and_level_indication */
  mpeg2enc_putbits(prog_seq,1); /* progressive sequence */
  mpeg2enc_putbits(chroma_format,2); /* chroma_format */
  mpeg2enc_putbits(horizontal_size>>12,2); /* horizontal_size_extension */
  mpeg2enc_putbits(vertical_size>>12,2); /* vertical_size_extension */
  mpeg2enc_putbits(((int)ceil(bit_rate/400.0))>>18,12); /* bit_rate_extension */
  mpeg2enc_putbits(1,1); /* marker_bit */
  mpeg2enc_putbits(vbv_buffer_size>>10,8); /* vbv_buffer_size_extension */
  mpeg2enc_putbits(0,1); /* low_delay  -- currently not implemented */
  mpeg2enc_putbits(0,2); /* frame_rate_extension_n */
  mpeg2enc_putbits(0,5); /* frame_rate_extension_d */
}

/* generate sequence display extension (6.2.2.4, 6.3.6)
 *
 * content not yet user setable
 */
void putseqdispext()
{
  alignbits();
  mpeg2enc_putbits(EXT_START_CODE,32); /* extension_start_code */
  mpeg2enc_putbits(DISP_ID,4); /* extension_start_code_identifier */
  mpeg2enc_putbits(video_format,3); /* video_format */
  mpeg2enc_putbits(1,1); /* colour_description */
  mpeg2enc_putbits(color_primaries,8); /* colour_primaries */
  mpeg2enc_putbits(transfer_characteristics,8); /* transfer_characteristics */
  mpeg2enc_putbits(matrix_coefficients,8); /* matrix_coefficients */
  mpeg2enc_putbits(display_horizontal_size,14); /* display_horizontal_size */
  mpeg2enc_putbits(1,1); /* marker_bit */
  mpeg2enc_putbits(display_vertical_size,14); /* display_vertical_size */
}

/* output a zero terminated string as user data (6.2.2.2.2, 6.3.4.1)
 *
 * string must not emulate start codes
 */
void putuserdata(userdata)
char *userdata;
{
  alignbits();
  mpeg2enc_putbits(USER_START_CODE,32); /* user_data_start_code */
  while (*userdata)
    mpeg2enc_putbits(*userdata++,8);
}

/* generate group of pictures header (6.2.2.6, 6.3.9)
 *
 * uses tc0 (timecode of first frame) and frame0 (number of first frame)
 */
void putgophdr(frame,closed_gop)
int frame,closed_gop;
{
  int tc;

  alignbits();
  mpeg2enc_putbits(GOP_START_CODE,32); /* group_start_code */
  tc = frametotc(tc0+frame);
  mpeg2enc_putbits(tc,25); /* time_code */
  mpeg2enc_putbits(closed_gop,1); /* closed_gop */
  mpeg2enc_putbits(0,1); /* broken_link */
}

/* convert frame number to time_code
 *
 * drop_frame not implemented
 */
static int frametotc(frame)
int frame;
{
  int fps, pict, sec, minute, hour, tc;

  fps = (int)(frame_rate+0.5);
  pict = frame%fps;
  frame = (frame-pict)/fps;
  sec = frame%60;
  frame = (frame-sec)/60;
  minute = frame%60;
  frame = (frame-minute)/60;
  hour = frame%24;
  tc = (hour<<19) | (minute<<13) | (1<<12) | (sec<<6) | pict;

  return tc;
}

/* generate picture header (6.2.3, 6.3.10) */
void putpicthdr(pict_data_s *picture)
{
  alignbits();
  mpeg2enc_putbits(PICTURE_START_CODE,32); /* picture_start_code */
  mpeg2enc_putbits(picture->temp_ref,10); /* temporal_reference */
  mpeg2enc_putbits(picture->pict_type,3); /* picture_coding_type */
  mpeg2enc_putbits(picture->vbv_delay,16); /* vbv_delay */
//printf("putpicthdr %d %d %d\n", picture->temp_ref, picture->pict_type, picture->vbv_delay);

  if (picture->pict_type==P_TYPE || picture->pict_type==B_TYPE)
  {
    mpeg2enc_putbits(0,1); /* full_pel_forward_vector */
    if (mpeg1)
      mpeg2enc_putbits(picture->forw_hor_f_code,3);
    else
      mpeg2enc_putbits(7,3); /* forward_f_code */
  }

  if (picture->pict_type==B_TYPE)
  {
    mpeg2enc_putbits(0,1); /* full_pel_backward_vector */
    if (mpeg1)
      mpeg2enc_putbits(picture->back_hor_f_code,3);
    else
      mpeg2enc_putbits(7,3); /* backward_f_code */
  }

  mpeg2enc_putbits(0,1); /* extra_bit_picture */
}

/* generate picture coding extension (6.2.3.1, 6.3.11)
 *
 * composite display information (v_axis etc.) not implemented
 */
void putpictcodext(pict_data_s *picture)
{
  alignbits();
  mpeg2enc_putbits(EXT_START_CODE,32); /* extension_start_code */
  mpeg2enc_putbits(CODING_ID,4); /* extension_start_code_identifier */
  mpeg2enc_putbits(picture->forw_hor_f_code,4); /* forward_horizontal_f_code */
  mpeg2enc_putbits(picture->forw_vert_f_code,4); /* forward_vertical_f_code */
  mpeg2enc_putbits(picture->back_hor_f_code,4); /* backward_horizontal_f_code */
  mpeg2enc_putbits(picture->back_vert_f_code,4); /* backward_vertical_f_code */
  mpeg2enc_putbits(picture->dc_prec,2); /* intra_dc_precision */
  mpeg2enc_putbits(picture->pict_struct,2); /* picture_structure */
  mpeg2enc_putbits((picture->pict_struct==FRAME_PICTURE)?picture->topfirst:0,1); /* top_field_first */
  mpeg2enc_putbits(picture->frame_pred_dct,1); /* frame_pred_frame_dct */
  mpeg2enc_putbits(0,1); /* concealment_motion_vectors  -- currently not implemented */
  mpeg2enc_putbits(picture->q_scale_type,1); /* q_scale_type */
  mpeg2enc_putbits(picture->intravlc,1); /* intra_vlc_format */
  mpeg2enc_putbits(picture->altscan,1); /* alternate_scan_hv */
  mpeg2enc_putbits(picture->repeatfirst,1); /* repeat_first_field */
  mpeg2enc_putbits(picture->prog_frame,1); /* chroma_420_type */
  mpeg2enc_putbits(picture->prog_frame,1); /* progressive_frame */
  mpeg2enc_putbits(0,1); /* composite_display_flag */
}

/* generate sequence_end_code (6.2.2) */
void putseqend()
{
  alignbits();
  mpeg2enc_putbits(SEQ_END_CODE,32);
}
