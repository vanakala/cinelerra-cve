/* putseq.c, sequence level routines                                        */

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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "global.h"




void putseq()
{
	/* this routine assumes (N % M) == 0 */
	int i, j, k, f, f0, n, np, nb;
	int ipflag;
	int sxf = 0, sxb = 0, syf = 0, syb = 0;
	motion_comp_s mc_data;

	for(k = 0; k < processors; k++)
		ratectl_init_seq(ratectl[k]); /* initialize rate control */


	if(end_frame == 0x7fffffff)
		frames_scaled = end_frame;
	else
		frames_scaled = (end_frame - start_frame) * 
			frame_rate / 
			input_frame_rate;
//frames_scaled = 100;

	/* If we're not doing sequence header, sequence extension and
	   sequence display extension every GOP at least has to be one at the
	   start of the sequence.
	*/
	
	if(!seq_header_every_gop) putseqhdr();


	/* optionally output some text data (description, copyright or whatever) */
//	if (strlen(id_string) > 1)
//		putuserdata(id_string);


	/* loop through all frames in encoding/decoding order */
	for(i = 0; 
		i < frames_scaled; 
		i++)
	{
//printf("putseq 1\n");
		if(i != 0 && verbose)
		{
			if(end_frame == 0x7fffffff)
				fprintf(stderr,"Encoding frame %d.  bitrate achieved: %d         \r", 
					frame0 + i + 1, 
					(int)((float)ftell(outfile)  / ((float)i / frame_rate)) * 8);
			else
				fprintf(stderr,"%5d %13d%% %17d %23d        \r", 
					frame0 + i + 1, 
					(int)((float)i / frames_scaled * 100),
					(int)((float)ftell(outfile)  / ((float)i / frame_rate)) * 8,
					(int)((float)ftell(outfile)  / ((float)i / frames_scaled)));
		}
		fflush(stderr);

		/* f0: lowest frame number in current GOP
		 *
		 * first GOP contains N-(M-1) frames,
		 * all other GOPs contain N frames
		 */
		f0 = N*((i+(M-1))/N) - (M-1);

		if (f0<0)
			f0=0;

		if(i == 0 || (i - 1) % M == 0)
		{

			/* I or P frame: Somewhat complicated buffer handling.
			   The original reference frame data is actually held in
			   the frame input buffers.  In input read-ahead buffer
			   management code worries about rotating them for use.
			   So to make the new old one the current new one we
			   simply move the pointers.  However for the
			   reconstructed "ref" data we are managing our a seperate
			   pair of buffers. We need to swap these to avoid losing
			   one!  */

			for (j=0; j<3; j++)
			{
				unsigned char *tmp;
				oldorgframe[j] = neworgframe[j];
				tmp = oldrefframe[j];
				oldrefframe[j] = newrefframe[j];
				newrefframe[j] = tmp;
			}

			/* For an I or P frame the "current frame" is simply an alias
			   for the new new reference frame. Saves the need to copy
			   stuff around once the frame has been processed.
			*/

			cur_picture.curorg = neworgframe;
			cur_picture.curref = newrefframe;
//printf("putseq 1 %p %p %p\n", curorg[0], curorg[1], curorg[2]);


			/* f: frame number in display order */
			f = (i==0) ? 0 : i+M-1;
			if (f>=end_frame)
				f = end_frame - 1;

			if (i==f0) /* first displayed frame in GOP is I */
			{
				/* I frame */
				cur_picture.pict_type = I_TYPE;

				cur_picture.forw_hor_f_code = 
					cur_picture.forw_vert_f_code = 15;
				cur_picture.back_hor_f_code = 
					cur_picture.back_vert_f_code = 15;

				/* n: number of frames in current GOP
				 *
				 * first GOP contains (M-1) less (B) frames
				 */
				n = (i==0) ? N-(M-1) : N;

				/* last GOP may contain less frames */
				if (n > end_frame-f0)
					n = end_frame-f0;

				/* number of P frames */
				if (i==0)
					np = (n + 2*(M-1))/M - 1; /* first GOP */
				else
					np = (n + (M-1))/M - 1;

				/* number of B frames */
				nb = n - np - 1;

				for(k = 0; k < processors; k++)
					ratectl_init_GOP(ratectl[k], np, nb);
				
				/* set closed_GOP in first GOP only 
				   No need for per-GOP seqhdr in first GOP as one
				   has already been created.
				 */
//				putgophdr(f0,i==0, i!=0 && seq_header_every_gop);
				if(seq_header_every_gop) putseqhdr();
				putgophdr(f0, i == 0);
			}
			else
			{
				/* P frame */
				cur_picture.pict_type = P_TYPE;
				cur_picture.forw_hor_f_code = motion_data[0].forw_hor_f_code;
				cur_picture.forw_vert_f_code = motion_data[0].forw_vert_f_code;
				cur_picture.back_hor_f_code = 
					cur_picture.back_vert_f_code = 15;
				sxf = motion_data[0].sxf;
				syf = motion_data[0].syf;
			}
		}
		else
		{
			/* B frame: no need to change the reference frames.
			   The current frame data pointers are a 3rd set
			   seperate from the reference data pointers.
			*/
			cur_picture.curorg = auxorgframe;
			cur_picture.curref = auxframe;

			/* f: frame number in display order */
			f = i - 1;
			cur_picture.pict_type = B_TYPE;
			n = (i-2)%M + 1; /* first B: n=1, second B: n=2, ... */
			cur_picture.forw_hor_f_code = motion_data[n].forw_hor_f_code;
			cur_picture.forw_vert_f_code = motion_data[n].forw_vert_f_code;
			cur_picture.back_hor_f_code = motion_data[n].back_hor_f_code;
			cur_picture.back_vert_f_code = motion_data[n].back_vert_f_code;
			sxf = motion_data[n].sxf;
			syf = motion_data[n].syf;
			sxb = motion_data[n].sxb;
			syb = motion_data[n].syb;
		}

		cur_picture.temp_ref = f - f0;
		cur_picture.frame_pred_dct = frame_pred_dct_tab[cur_picture.pict_type - 1];
		cur_picture.q_scale_type = qscale_tab[cur_picture.pict_type - 1];
		cur_picture.intravlc = intravlc_tab[cur_picture.pict_type - 1];
		cur_picture.altscan = altscan_tab[cur_picture.pict_type - 1];

//printf("putseq 2 %d\n", cur_picture.frame_pred_dct);
		readframe(f + frame0, cur_picture.curorg);
		if(!frames_scaled) break;
//printf("putseq 3 %p %p %p\n", curorg[0], curorg[1], curorg[2]);

		mc_data.oldorg = oldorgframe;
		mc_data.neworg = neworgframe;
		mc_data.oldref = oldrefframe;
		mc_data.newref = newrefframe;
		mc_data.cur    = cur_picture.curorg;
		mc_data.curref = cur_picture.curref;
		mc_data.sxf = sxf;
		mc_data.syf = syf;
		mc_data.sxb = sxb;
		mc_data.syb = syb;

        if (fieldpic)
		{
//printf("putseq 4\n");
			cur_picture.topfirst = opt_topfirst;
			if (!quiet)
			{
				fprintf(stderr,"\nfirst field  (%s) ",
						cur_picture.topfirst ? "top" : "bot");
				fflush(stderr);
			}

			cur_picture.pict_struct = cur_picture.topfirst ? TOP_FIELD : BOTTOM_FIELD;
/* A.Stevens 2000: Append fast motion compensation data for new frame */
			fast_motion_data(cur_picture.curorg[0], cur_picture.pict_struct);
			motion_estimation(&cur_picture, &mc_data,0,0);

			predict(&cur_picture,oldrefframe,newrefframe,predframe,0);
			dct_type_estimation(&cur_picture,predframe[0],cur_picture.curorg[0]);
			transform(&cur_picture,predframe,cur_picture.curorg);

			putpict(&cur_picture);		/* Quantisation: blocks -> qblocks */
#ifndef OUTPUT_STAT
			if( cur_picture.pict_type!=B_TYPE)
			{
#endif
				iquantize( &cur_picture );
				itransform(&cur_picture,predframe,cur_picture.curref);
/*
 * 				calcSNR(cur_picture.curorg,cur_picture.curref);
 * 				stats();
 */
#ifndef OUTPUT_STAT
			}
#endif
			if (!quiet)
			{
				fprintf(stderr,"second field (%s) ",cur_picture.topfirst ? "bot" : "top");
				fflush(stderr);
			}

			cur_picture.pict_struct = cur_picture.topfirst ? BOTTOM_FIELD : TOP_FIELD;

			ipflag = (cur_picture.pict_type==I_TYPE);
			if (ipflag)
			{
				/* first field = I, second field = P */
				cur_picture.pict_type = P_TYPE;
				cur_picture.forw_hor_f_code = motion_data[0].forw_hor_f_code;
				cur_picture.forw_vert_f_code = motion_data[0].forw_vert_f_code;
				cur_picture.back_hor_f_code = 
					cur_picture.back_vert_f_code = 15;
				sxf = motion_data[0].sxf;
				syf = motion_data[0].syf;
			}

			motion_estimation(&cur_picture, &mc_data ,1,ipflag);

			predict(&cur_picture,oldrefframe,newrefframe,predframe,1);
			dct_type_estimation(&cur_picture,predframe[0],cur_picture.curorg[0]);
			transform(&cur_picture,predframe,cur_picture.curorg);

			putpict(&cur_picture);	/* Quantisation: blocks -> qblocks */

#ifndef OUTPUT_STAT
			if( cur_picture.pict_type!=B_TYPE)
			{
#endif
				iquantize( &cur_picture );
				itransform(&cur_picture,predframe,cur_picture.curref);
/*
 * 				calcSNR(cur_picture.curorg,cur_picture.curref);
 * 				stats();
 */
#ifndef OUTPUT_STAT
			}
#endif
				
		}
		else
		{
//printf("putseq 5\n");
			cur_picture.pict_struct = FRAME_PICTURE;
			fast_motion_data(cur_picture.curorg[0], cur_picture.pict_struct);
//printf("putseq 5\n");

/* do motion_estimation
 *
 * uses source frames (...orgframe) for full pel search
 * and reconstructed frames (...refframe) for half pel search
 */

			motion_estimation(&cur_picture,&mc_data,0,0);

//printf("putseq 5\n");
			predict(&cur_picture, oldrefframe,newrefframe,predframe,0);
//printf("putseq 5\n");
			dct_type_estimation(&cur_picture,predframe[0],cur_picture.curorg[0]);
//printf("putseq 5\n");

			transform(&cur_picture,predframe,cur_picture.curorg);
//printf("putseq 5\n");

/* Side-effect: quantisation blocks -> qblocks */
			putpict(&cur_picture);	
//printf("putseq 6\n");

#ifndef OUTPUT_STAT
			if( cur_picture.pict_type!=B_TYPE)
			{
#endif
				iquantize( &cur_picture );
//printf("putseq 5\n");
				itransform(&cur_picture,predframe,cur_picture.curref);
//printf("putseq 6\n");
/*
 * 				calcSNR(cur_picture.curorg,cur_picture.curref);
 * 				stats();
 */
#ifndef OUTPUT_STAT
			}
#endif
		}
		writeframe(f + frame0, cur_picture.curref);
			
//printf("putseq 7\n");
	}
	putseqend();
  	if(verbose) fprintf(stderr, "\nDone.  Be sure to visit heroinewarrior.com for updates.\n");
}
