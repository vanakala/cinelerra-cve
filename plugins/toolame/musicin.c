#include <stdlib.h>
#include "common.h"
#include "encoder.h"
#include <assert.h>

/* Global variable definitions for "musicin.c" */

FILE *musicin;
Bit_stream_struc bs;
char *programName;

/************************************************************************
*
* main
*
* PURPOSE:  MPEG I Encoder supporting layer 2 only
* psychoacoustic models 1 (MUSICAM) and 2 (AT&T)
*
* SEMANTICS:  One overlapping frame of audio of up to 2 channels are
* processed at a time in the following order:
* (associated routines are in parentheses)
*
* 1.  Filter sliding window of data to get 32 subband
* samples per channel.
* (window_subband,filter_subband)
*
* 2.  If joint stereo mode, combine left and right channels
* for subbands above #jsbound#.
* (combine_LR)
*
* 3.  Calculate scalefactors for the frame, and if layer 2,
* also calculate scalefactor select information.
* (scale_factor_calc)
*
* 4.  Calculate psychoacoustic masking levels using selected
* psychoacoustic model.
* (tonal.c/psycho_one, psy.c/psycho_two)
*
* 5.  Perform iterative bit allocation for subbands with low
* mask_to_noise ratios using masking levels from step 4.
* (main_bit_allocation)
*
* 6.  If error protection flag is active, add redundancy for
* error protection.
* (CRC_calc)
*
* 7.  Pack bit allocation, scalefactors, and scalefactor select
* information (layer 2) onto bitstream.
* (encode_bit_alloc,encode_scale,transmission_pattern)
*
* 8.  Quantize subbands and pack them into bitstream
* (subband_quantization, sample_encoding)
*
************************************************************************/

int
main (int argc, char **argv)
{
  typedef double SBS[2][3][SCALE_BLOCK][SBLIMIT];
  SBS *sb_sample;
  typedef double JSBS[3][SCALE_BLOCK][SBLIMIT];
  JSBS *j_sample;
  typedef double IN[2][HAN_SIZE];
  IN *win_que;
  typedef unsigned int SUB[2][3][SCALE_BLOCK][SBLIMIT];
  SUB *subband;
  FILE *stdin_fd = stdin;
  
	int use_stdin = 0;

  int frameNum = 0;

  frame_params fr_ps;
  layer info;
  char original_file_name[MAX_NAME_SIZE];
  char encoded_file_name[MAX_NAME_SIZE];
  short **win_buf;
  static short buffer[2][1152];
  static unsigned int bit_alloc[2][SBLIMIT], scfsi[2][SBLIMIT];
  static unsigned int scalar[2][3][SBLIMIT], j_scale[3][SBLIMIT];
  static double ltmin[2][SBLIMIT], lgmin[2][SBLIMIT], max_sc[2][SBLIMIT];
  FLOAT snr32[32];
  short sam[2][1344];		/* was [1056]; */
  int whole_SpF, extra_slot = 0;
  double avg_slots_per_frame, frac_SpF, slot_lag;
  int model, stereo, error_protection;
  static unsigned int crc;
  int i, j, k, adb;
  unsigned long bitsPerSlot, samplesPerFrame;
  unsigned long frameBits, sentBits = 0;
  unsigned long num_samples;


  /* Most large variables are declared dynamically to ensure
     compatibility with smaller machines */

  sb_sample = (SBS *) mem_alloc (sizeof (SBS), "sb_sample");
  j_sample = (JSBS *) mem_alloc (sizeof (JSBS), "j_sample");
  win_que = (IN *) mem_alloc (sizeof (IN), "Win_que");
  subband = (SUB *) mem_alloc (sizeof (SUB), "subband");
  win_buf = (short **) mem_alloc (sizeof (short *) * 2, "win_buf");

  /* clear buffers */
  memset ((char *) buffer, 0, sizeof (buffer));
  memset ((char *) bit_alloc, 0, sizeof (bit_alloc));
  memset ((char *) scalar, 0, sizeof (scalar));
  memset ((char *) j_scale, 0, sizeof (j_scale));
  memset ((char *) scfsi, 0, sizeof (scfsi));
  memset ((char *) ltmin, 0, sizeof (ltmin));
  memset ((char *) lgmin, 0, sizeof (lgmin));
  memset ((char *) max_sc, 0, sizeof (max_sc));
  memset ((char *) snr32, 0, sizeof (snr32));
  memset ((char *) sam, 0, sizeof (sam));

  fr_ps.header = &info;
  fr_ps.tab_num = -1;		/* no table loaded */
  fr_ps.alloc = NULL;
  info.version = MPEG_AUDIO_ID;	/* Default: MPEG-1 */

  programName = argv[0];
  if (argc == 1)		/* no command-line args */
    usage ();
  else
    parse_args (argc, argv, &fr_ps, &model, &num_samples, &use_stdin, 
		original_file_name, encoded_file_name);



  print_config (&fr_ps, &model, original_file_name, encoded_file_name);

  hdr_to_frps (&fr_ps);
  stereo = fr_ps.stereo;
  error_protection = info.error_protection;

  bitsPerSlot = 8;
  samplesPerFrame = 1152;

  /* Figure average number of 'slots' per frame. */
  /* Bitrate means TOTAL for both channels, not per side. */
  avg_slots_per_frame = ((double) samplesPerFrame /
			 s_freq[info.version][info.sampling_frequency]) *
    ((double) bitrate[info.version][info.bitrate_index] /
     (double) bitsPerSlot);
  whole_SpF = (int) avg_slots_per_frame;
  fprintf (stderr, "slots/frame = %d\n", whole_SpF);
  frac_SpF = avg_slots_per_frame - (double) whole_SpF;
  slot_lag = -frac_SpF;
  fprintf (stderr, "frac SpF=%.3f, tot bitrate=%d kbps, s freq=%.1f kHz\n",
	   frac_SpF, bitrate[info.version][info.bitrate_index],
	   s_freq[info.version][info.sampling_frequency]);

  if (frac_SpF != 0)
    fprintf (stderr, "Fractional number of slots, padding required\n");
  else
    info.padding = 0;

  if (model == 2)
    init_psycho_two((FLOAT) s_freq[info.version][info.sampling_frequency] * 1000);

  while (get_audio (stdin_fd, musicin, buffer, num_samples, stereo, &info) > 0)
    {
      if (++frameNum % 10 == 0)
	{
	  fprintf (stderr, "{%4u}\r", frameNum);
	  fflush (stderr);
	}
      win_buf[0] = &buffer[0][0];
      win_buf[1] = &buffer[1][0];
      if (frac_SpF != 0)
	{
	  if (slot_lag > (frac_SpF - 1.0))
	    {
	      slot_lag -= frac_SpF;
	      extra_slot = 0;
	      info.padding = 0;
	      /*  fprintf(stderr,"No padding for this frame\n"); */
	    }
	  else
	    {
	      extra_slot = 1;
	      info.padding = 1;
	      slot_lag += (1 - frac_SpF);
	      /*  fprintf(stderr,"Padding for this frame\n");    */
	    }
	}
      adb = (whole_SpF + extra_slot) * bitsPerSlot;

      {
	for (i = 0; i < 3; i++)
	  for (j = 0; j < SCALE_BLOCK; j++)
	    for (k = 0; k < stereo; k++)
	      {
		window_subband (&win_buf[k], &(*win_que)[k][0], k);
		filter_subband (&(*win_que)[k][0], &(*sb_sample)[k][i][j][0]);
	      }

	scale_factor_calc (*sb_sample, scalar, stereo, fr_ps.sblimit);
	pick_scale (scalar, &fr_ps, max_sc);
	if (fr_ps.actual_mode == MPG_MD_JOINT_STEREO)
	  {
	    combine_LR (*sb_sample, *j_sample, fr_ps.sblimit);
	    scale_factor_calc (j_sample, &j_scale, 1, fr_ps.sblimit);
	  }			/* this way we calculate more mono than we need */
	/* but it is cheap */

	if (model == 1)
	  psycho_one (buffer, max_sc, ltmin, &fr_ps);
	else
	  {
	    for (k = 0; k < stereo; k++)
	      {
		psycho_two (&buffer[k][0], &sam[k][0], k, snr32,
			     (FLOAT) s_freq[info.version][info.
							  sampling_frequency]
			     * 1000);
		for (i = 0; i < SBLIMIT; i++)
		  ltmin[k][i] = (double) snr32[i];
	      }
	  }

	transmission_pattern (scalar, scfsi, &fr_ps);
	main_bit_allocation (ltmin, scfsi, bit_alloc, &adb, &fr_ps);

	if (error_protection)
	  CRC_calc (&fr_ps, bit_alloc, scfsi, &crc);

	encode_info (&fr_ps, &bs);

	if (error_protection)
	  encode_CRC (crc, &bs);

	encode_bit_alloc (bit_alloc, &fr_ps, &bs);
	encode_scale (bit_alloc, scfsi, scalar, &fr_ps, &bs);
	subband_quantization (scalar, *sb_sample, j_scale,
				 *j_sample, bit_alloc, *subband, &fr_ps);
	sample_encoding (*subband, bit_alloc, &fr_ps, &bs);
	for (i = 0; i < adb; i++)
	  put1bit (&bs, 0);

      }

      frameBits = sstell (&bs) - sentBits;
      if (frameBits % bitsPerSlot)	/* a program failure */
	fprintf (stderr, "Sent %ld bits = %ld slots plus %ld\n",
		 frameBits, frameBits / bitsPerSlot, frameBits % bitsPerSlot);
      sentBits += frameBits;

    }

  close_bit_stream_w (&bs);

  fprintf (stderr, "Avg slots/frame = %.3f; b/smp = %.2f; br = %.3f kbps\n",
	   (FLOAT) sentBits / (frameNum * bitsPerSlot),
	   (FLOAT) sentBits / (frameNum * samplesPerFrame),
	   (FLOAT) sentBits / (frameNum * samplesPerFrame) *
	   s_freq[info.version][info.sampling_frequency]);

  if (use_stdin && fclose (musicin) != 0)
    {
      fprintf (stderr, "Could not close \"%s\".\n", original_file_name);
      exit (2);
    }

  fprintf (stderr,
	   "Encoding of \"%s\" with psychoacoustic model %d is finished\n",
	   original_file_name, model);
  fprintf (stderr, "The MPEG encoded output file name is \"%s\"\n",
	   encoded_file_name);

  /* Cleanup */
  if (model == 2)
    cleanup_psycho_two();

  free (sb_sample);
  free (j_sample);
  free (win_que);
  free (subband);
  free (win_buf);
  free (fr_ps.alloc);

  exit (0);
}

/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by #stderr#
*
************************************************************************/

void
usage ()			/* print syntax & exit */
{
  fprintf (stdout,
	   "usage: %s\n",
	   programName);
  fprintf (stdout,
	   "       %s [-m mode][-p psy][-s sfrq][-b br][-d emp]\n",
	   programName);
  fprintf (stdout, "          [-c][-o][-e] inputPCM [outBS]\n");
  fprintf (stdout, "where\n");
  fprintf (stdout, " -m mode  channel mode : s/d/j/m   (dflt %4c)\n",
	   DFLT_MOD);
  fprintf (stdout, " -p psy   psychoacoustic model 1/2 (dflt %4u)\n",
	   DFLT_PSY);
  fprintf (stdout, " -s sfrq  input smpl rate in kHz   (dflt %4.1f)\n",
	   DFLT_SFQ);
  fprintf (stdout, " -b br    total bitrate in kbps    (dflt highest)\n");
  fprintf (stdout, " -d emp   de-emphasis n/5/c        (dflt %4c)\n",
	   DFLT_EMP);
  fprintf (stdout, " -c       mark as copyright\n");
  fprintf (stdout, " -o       mark as original\n");
  fprintf (stdout, " -e       add error protection\n");
  fprintf (stdout, " inputPCM input PCM sound file (standard or AIFF)\n");
  fprintf (stdout,
	   " outBS    output bit stream of encoded audio (dflt inName+%s)\n",
	   DFLT_EXT);
  exit (1);
}

/************************************************************************
*
* parse_args
*
* PURPOSE:  Sets encoding parameters to the specifications of the
* command line.  Default settings are used for parameters
* not specified in the command line.
*
* SEMANTICS:  The command line is parsed according to the following
* syntax:
*
* -m  is followed by the mode
* -p  is followed by the psychoacoustic model number
* -s  is followed by the sampling rate
* -b  is followed by the total bitrate, irrespective of the mode
* -d  is followed by the emphasis flag
* -c  is followed by the copyright/no_copyright flag
* -o  is followed by the original/not_original flag
* -e  is followed by the error_protection on/off flag
*
* If the input file is in AIFF format, the sampling frequency is read
* from the AIFF header.
*
* The input and output filenames are read into #inpath# and #outpath#.
*
************************************************************************/

void
parse_args (int argc, char **argv, frame_params * fr_ps, int *psy,
	    unsigned long *num_samples, int *use_stdin, char inPath[MAX_NAME_SIZE],
	    char outPath[MAX_NAME_SIZE])
{
  FLOAT srate;
  int brate;
  layer *info = fr_ps->header;
  int err = 0, i = 0;
  IFF_AIFF pcm_aiff_data;
  long samplerate;
  long soundPosition;

  /* preset defaults */
  inPath[0] = '\0';
  outPath[0] = '\0';
  info->lay = 2;
  switch (DFLT_MOD)
    {
    case 's':
      info->mode = MPG_MD_STEREO;
      info->mode_ext = 0;
      break;
    case 'd':
      info->mode = MPG_MD_DUAL_CHANNEL;
      info->mode_ext = 0;
      break;
    case 'j':
      info->mode = MPG_MD_JOINT_STEREO;
      break;
    case 'm':
      info->mode = MPG_MD_MONO;
      info->mode_ext = 0;
      break;
    default:
      fprintf (stderr, "%s: Bad mode dflt %c\n", programName, DFLT_MOD);
      abort ();
    }
  *psy = DFLT_PSY;
  if ((info->sampling_frequency =
       SmpFrqIndex ((long) (1000 * DFLT_SFQ), &info->version)) < 0)
    {
      fprintf (stderr, "%s: bad sfrq default %.2f\n", programName, DFLT_SFQ);
      abort ();
    }
  info->bitrate_index = 14;
  brate = 0;
  switch (DFLT_EMP)
    {
    case 'n':
      info->emphasis = 0;
      break;
    case '5':
      info->emphasis = 1;
      break;
    case 'c':
      info->emphasis = 3;
      break;
    default:
      fprintf (stderr, "%s: Bad emph dflt %c\n", programName, DFLT_EMP);
      abort ();
    }
  info->copyright = 0;
  info->original = 0;
  info->error_protection = FALSE;

  /* process args */
  while (++i < argc && err == 0)
    {
      char c, *token, *arg, *nextArg;
      int argUsed;

      token = argv[i];
      if (*token++ == '-')
	{
	  if (i + 1 < argc)
	    nextArg = argv[i + 1];
	  else
	    nextArg = "";
	  argUsed = 0;

// Plain '-'
		if(*token == 0)
		{
			*use_stdin = 1;
		}



	  while ((c = *token++))
	    {
	      if (*token /* NumericQ(token) */ )
		arg = token;
	      else
		arg = nextArg;







	      switch (c)
		{
		case 'm':
		  argUsed = 1;
		  if (*arg == 's')
		    {
		      info->mode = MPG_MD_STEREO;
		      info->mode_ext = 0;
		    }
		  else if (*arg == 'd')
		    {
		      info->mode = MPG_MD_DUAL_CHANNEL;
		      info->mode_ext = 0;
		    }
		  else if (*arg == 'j')
		    {
		      info->mode = MPG_MD_JOINT_STEREO;
		    }
		  else if (*arg == 'm')
		    {
		      info->mode = MPG_MD_MONO;
		      info->mode_ext = 0;
		    }
		  else
		    {
		      fprintf (stderr, "%s: -m mode must be s/d/j/m not %s\n",
			       programName, arg);
		      err = 1;
		    }
		  break;
		case 'p':
		  *psy = atoi (arg);
		  argUsed = 1;
		  if (*psy < 1 || *psy > 2)
		    {
		      fprintf (stderr,
			       "%s: -p model must be 1 or 2, not %s\n",
			       programName, arg);
		      err = 1;
		    }
		  break;

		case 's':
		  argUsed = 1;
		  srate = atof (arg);
		  /* samplerate = rint( 1000.0 * srate ); $A  */
		  samplerate = (long) ((1000.0 * srate) + 0.5);
		  if ((info->sampling_frequency =
		       SmpFrqIndex ((long) samplerate, &info->version)) < 0)
		    err = 1;
		  break;

		case 'b':
		  argUsed = 1;
		  brate = atoi (arg);
		  break;
		case 'd':
		  argUsed = 1;
		  if (*arg == 'n')
		    info->emphasis = 0;
		  else if (*arg == '5')
		    info->emphasis = 1;
		  else if (*arg == 'c')
		    info->emphasis = 3;
		  else
		    {
		      fprintf (stderr, "%s: -d emp must be n/5/c not %s\n",
			       programName, arg);
		      err = 1;
		    }
		  break;
		case 'c':
		  info->copyright = 1;
		  break;
		case 'o':
		  info->original = 1;
		  break;
		case 'e':
		  info->error_protection = TRUE;
		  break;
		default:
		  fprintf (stderr, "%s: unrec option %c\n", programName, c);
		  err = 1;
		  break;
		}
	      if (argUsed)
		{
		  if (arg == token)
		    token = "";	/* no more from token */
		  else
		    ++i;	/* skip arg we used */
		  arg = "";
		  argUsed = 0;
		}
	    }
	}
      else
	{
	  if (inPath[0] == '\0' && !*use_stdin)
	    strcpy (inPath, argv[i]);
	  else if (outPath[0] == '\0')
	    strcpy (outPath, argv[i]);
	  else
	    {
	      fprintf (stderr, "%s: excess arg %s\n", programName, argv[i]);
	      err = 1;
	    }
	}
    }

  if (err || (inPath[0] == '\0' && !*use_stdin))
    usage ();			/* never returns */

  if (outPath[0] == '\0')
    {
      /* replace old extension with new one, 1992-08-19, 1995-06-12 shn */
      new_ext (inPath, DFLT_EXT, outPath);
    }

  if (!*use_stdin && (musicin = fopen (inPath, "rb")) == NULL)
    {
      fprintf (stderr, "Could not find \"%s\".\n", inPath);
      exit (1);
    }

  open_bit_stream_w (&bs, outPath, BUFFER_SIZE);

  if (!*use_stdin && (soundPosition = aiff_read_headers (musicin, &pcm_aiff_data)) != -1)
    {

      fprintf (stderr, ">>> Using Audio IFF sound file headers\n");

      aiff_check (inPath, &pcm_aiff_data, &info->version);

      if (fseek (musicin, soundPosition, SEEK_SET) != 0)
	{
	  fprintf (stderr, "Could not seek to PCM sound data in \"%s\".\n",
		   inPath);
	  exit (1);
	}

      info->sampling_frequency =
	SmpFrqIndex ((long) pcm_aiff_data.sampleRate, &info->version);
      fprintf (stderr, ">>> %f Hz sampling frequency selected\n",
	       pcm_aiff_data.sampleRate);

      /* Determine number of samples in sound file */
      *num_samples = pcm_aiff_data.numChannels *
	pcm_aiff_data.numSampleFrames;
      if (pcm_aiff_data.numChannels == 1)
	{
	  info->mode = MPG_MD_MONO;
	  info->mode_ext = 0;
	}
    }
  else
    {				/* Not using Audio IFF sound file headers. */

      if (!*use_stdin && fseek (musicin, 0, SEEK_SET) != 0)
	{
	  fprintf (stderr, "Could not seek to PCM sound data in \"%s\".\n",
		   inPath);
	  exit (1);
	}

      /* Declare sound file to have "infinite" number of samples. */
      *num_samples = MAX_U_32_NUM;

    }
  if (brate == 0)
    brate = bitrate[info->version][14];
  if ((info->bitrate_index = BitrateIndex (brate, info->version)) < 0)
    err = 1;
  if (err || (!*use_stdin && inPath[0] == '\0'))
    usage ();			/* never returns */

}

/************************************************************************
*
* print_config
*
* PURPOSE:  Prints the encoding parameters used
*
************************************************************************/

void
print_config (frame_params * fr_ps, int *psy, char *inPath, char *outPath)
{
  layer *info = fr_ps->header;

  fprintf (stderr, "Encoding configuration:\n");
  fprintf (stderr, "Algorithm=%s\n", version_names[info->version]);
  if (info->mode != MPG_MD_JOINT_STEREO)
    fprintf (stderr, "Layer=II   mode=%s   extn=%d   psy model=%d\n",
	     mode_names[info->mode], info->mode_ext, *psy);
  else
    fprintf (stderr,
	     "Layer=II   mode=%s   extn=data dependant   psy model=%d\n",
	     mode_names[info->mode], *psy);
  fprintf (stderr, "samp frq=%.1f kHz   total bitrate=%d kbps\n",
	   s_freq[info->version][info->sampling_frequency],
	   bitrate[info->version][info->bitrate_index]);
  fprintf (stderr, "de-emph=%d   c/right=%d   orig=%d   errprot=%s\n",
	   info->emphasis, info->copyright, info->original,
	   ((info->error_protection) ? "on" : "off"));
  fprintf (stderr, "input file: '%s'\noutput file: '%s'\n", inPath, outPath);
}

/************************************************************************
*
* aiff_check
*
* PURPOSE:  Checks AIFF header information to make sure it is valid.
*           Exits if not.
*
************************************************************************/

void
aiff_check (char *file_name, IFF_AIFF * pcm_aiff_data, int *version)
{
  if (pcm_aiff_data->sampleType != IFF_ID_SSND)
    {
      fprintf (stderr, "Sound data is not PCM in \"%s\".\n", file_name);
      exit (1);
    }

  if (SmpFrqIndex ((long) pcm_aiff_data->sampleRate, version) < 0)
    {
      fprintf (stderr, "in \"%s\".\n", file_name);
      exit (1);
    }

  if (pcm_aiff_data->sampleSize != sizeof (short) * BITS_IN_A_BYTE)
    {
      fprintf (stderr, "Sound data is not %d bits in \"%s\".\n",
	       sizeof (short) * BITS_IN_A_BYTE, file_name);
      exit (1);
    }

  if (pcm_aiff_data->numChannels != MONO &&
      pcm_aiff_data->numChannels != STEREO)
    {
      fprintf (stderr, "Sound data is not mono or stereo in \"%s\".\n",
	       file_name);
      exit (1);
    }

  if (pcm_aiff_data->blkAlgn.blockSize != 0)
    {
      fprintf (stderr, "Block size is not %d bytes in \"%s\".\n", 0,
	       file_name);
      exit (1);
    }

  if (pcm_aiff_data->blkAlgn.offset != 0)
    {
      fprintf (stderr, "Block offset is not %d bytes in \"%s\".\n", 0,
	       file_name);
      exit (1);
    }
}
