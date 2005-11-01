#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "encoder.h"
#include "options.h"
#include "portableio.h"
#include "audio_read.h"


/************************************************************************
*
* read_samples()
*
* PURPOSE:  reads the PCM samples from a file to the buffer
*
*  SEMANTICS:
* Reads #samples_read# number of shorts from #musicin# filepointer
* into #sample_buffer[]#.  Returns the number of samples read.
*
************************************************************************/

unsigned long
read_samples (FILE * musicin, short sample_buffer[2304],
	      unsigned long num_samples, unsigned long frame_size)
{
  unsigned long samples_read;
  static unsigned long samples_to_read;
  static char init = TRUE;

  if (init) {
    samples_to_read = num_samples;
//    init = FALSE;
  }
  if (samples_to_read >= frame_size)
    samples_read = frame_size;
  else
    samples_read = samples_to_read;
/*
 *   if ((samples_read =
 *        fread (sample_buffer, sizeof (short), (int) samples_read,
 * 	      musicin)) == 0)
 */
 	if((samples_read = toolame_buffer_read((char*)sample_buffer, 
		sizeof(short), 
		samples_read)) == 0)
    fprintf (stderr, "Hit end of audio data\n");
  /*
     Samples are big-endian. If this is a little-endian machine
     we must swap
   */
  if (NativeByteOrder == order_unknown) {
    NativeByteOrder = DetermineByteOrder ();
    if (NativeByteOrder == order_unknown) {
      fprintf (stderr, "byte order not determined\n");
      exit (1);
    }
  }
  if (NativeByteOrder != order_littleEndian || (glopts.byteswap == TRUE))
    SwapBytesInWords (sample_buffer, samples_read);

  if (num_samples != MAX_U_32_NUM)
    samples_to_read -= samples_read;

  if (samples_read < frame_size && samples_read > 0) {
    /* fill out frame with zeros */
    for (; samples_read < frame_size; sample_buffer[samples_read++] = 0);
    samples_to_read = 0;
    samples_read = frame_size;
  }
  return (samples_read);
}

/************************************************************************
*
* get_audio()
*
* PURPOSE:  reads a frame of audio data from a file to the buffer,
*   aligns the data for future processing, and separates the
*   left and right channels
*
*
************************************************************************/
unsigned long
get_audio (FILE * musicin, short buffer[2][1152], unsigned long num_samples,
	   int nch, frame_header *header)
{
  int j;
  short insamp[2304];
  unsigned long samples_read;

  if (nch == 2) {		/* stereo */
    samples_read =
      read_samples (musicin, insamp, num_samples, (unsigned long) 2304);
    if (glopts.channelswap == TRUE) {
      for (j = 0; j < 1152; j++) {
	buffer[1][j] = insamp[2 * j];
	buffer[0][j] = insamp[2 * j + 1];
      }
    } else {
      for (j = 0; j < 1152; j++) {
	buffer[0][j] = insamp[2 * j];
	buffer[1][j] = insamp[2 * j + 1];
      }
    }
  } else if (glopts.downmix == TRUE) {
    samples_read =
      read_samples (musicin, insamp, num_samples, (unsigned long) 2304);
    for (j = 0; j < 1152; j++) {
      buffer[0][j] = 0.5 * (insamp[2 * j] + insamp[2 * j + 1]);
    }
  } else {			/* mono */
    samples_read =
      read_samples (musicin, insamp, num_samples, (unsigned long) 1152);
    for (j = 0; j < 1152; j++) {
      buffer[0][j] = insamp[j];
      /* buffer[1][j] = 0;  don't bother zeroing this buffer. MFC Nov 99 */
    }
  }
  return (samples_read);
}


/*****************************************************************************
*
*  Routines to determine byte order and swap bytes
*
*****************************************************************************/

enum byte_order DetermineByteOrder (void)
{
  char s[sizeof (long) + 1];
  union {
    long longval;
    char charval[sizeof (long)];
  } probe;
  probe.longval = 0x41424344L;	/* ABCD in ASCII */
  strncpy (s, probe.charval, sizeof (long));
  s[sizeof (long)] = '\0';
  /* fprintf( stderr, "byte order is %s\n", s ); */
  if (strcmp (s, "ABCD") == 0)
    return order_bigEndian;
  else if (strcmp (s, "DCBA") == 0)
    return order_littleEndian;
  else
    return order_unknown;
}

void SwapBytesInWords (short *loc, int words)
{
  int i;
  short thisval;
  char *dst, *src;
  src = (char *) &thisval;
  for (i = 0; i < words; i++) {
    thisval = *loc;
    dst = (char *) loc++;
    dst[0] = src[1];
    dst[1] = src[0];
  }
}

/*****************************************************************************
 *
 *  Read Audio Interchange File Format (AIFF) headers.
 *
 *****************************************************************************/

int aiff_read_headers (FILE * file_ptr, IFF_AIFF * aiff_ptr)
{
  int chunkSize, subSize, sound_position;

  if (fseek (file_ptr, 0, SEEK_SET) != 0)
    return -1;

  if (Read32BitsHighLow (file_ptr) != IFF_ID_FORM)
    return -1;

  chunkSize = Read32BitsHighLow (file_ptr);

  if (Read32BitsHighLow (file_ptr) != IFF_ID_AIFF)
    return -1;

  sound_position = 0;
  while (chunkSize > 0) {
    chunkSize -= 4;
    switch (Read32BitsHighLow (file_ptr)) {

    case IFF_ID_COMM:
      chunkSize -= subSize = Read32BitsHighLow (file_ptr);
      aiff_ptr->numChannels = Read16BitsHighLow (file_ptr);
      subSize -= 2;
      aiff_ptr->numSampleFrames = Read32BitsHighLow (file_ptr);
      subSize -= 4;
      aiff_ptr->sampleSize = Read16BitsHighLow (file_ptr);
      subSize -= 2;
      aiff_ptr->sampleRate = ReadIeeeExtendedHighLow (file_ptr);
      subSize -= 10;
      while (subSize > 0) {
	getc (file_ptr);
	subSize -= 1;
      }
      break;

    case IFF_ID_SSND:
      chunkSize -= subSize = Read32BitsHighLow (file_ptr);
      aiff_ptr->blkAlgn.offset = Read32BitsHighLow (file_ptr);
      subSize -= 4;
      aiff_ptr->blkAlgn.blockSize = Read32BitsHighLow (file_ptr);
      subSize -= 4;
      sound_position = ftell (file_ptr) + aiff_ptr->blkAlgn.offset;
      if (fseek (file_ptr, (long) subSize, SEEK_CUR) != 0)
	return -1;
      aiff_ptr->sampleType = IFF_ID_SSND;
      break;

    default:
      chunkSize -= subSize = Read32BitsHighLow (file_ptr);
      while (subSize > 0) {
	getc (file_ptr);
	subSize -= 1;
      }
      break;
    }
  }
  return sound_position;
}

/*****************************************************************************
 *
 *  Seek past some Audio Interchange File Format (AIFF) headers to sound data.
 *
 *****************************************************************************/

int aiff_seek_to_sound_data (FILE * file_ptr)
{
  if (fseek
      (file_ptr, AIFF_FORM_HEADER_SIZE + AIFF_SSND_HEADER_SIZE,
       SEEK_SET) != 0)
    return (-1);
  return (0);
}

/************************************************************
*   parse_input_file()
*   Determine the type of sound file. (stdin, wav, aiff, raw pcm)
*   Determine Sampling Frequency
*             number of samples
*	      whether the new sample is stereo or mono. 
*
*   If file is coming from /dev/stdin assume it is raw PCM. (it's what I use. YMMV)
*
*  This is just a hacked together function. The aiff parsing comes from the ISO code.
*  The WAV code comes from Nick Burch
*  The ugly /dev/stdin hack comes from me.
*                                                  MFC Dec 99
**************************************************************/
void
parse_input_file (FILE * musicin, char inPath[MAX_NAME_SIZE], frame_header *header,
		  unsigned long *num_samples)
{

  IFF_AIFF pcm_aiff_data;
  long soundPosition;

  unsigned char wave_header_buffer[40];	//HH fixed
  int wave_header_read = 0;
  int wave_header_stereo = -1;
  int wave_header_16bit = -1;
  unsigned long samplerate;

  /*************************** STDIN ********************************/
  /* check if we're reading from stdin. Assume it's a raw PCM file. */
  /* Of course, you could be piping a WAV file into stdin. Not done in this code */
  /* this code is probably very dodgy and was written to suit my needs. MFC Dec 99 */
/*
 *   if ((strcmp (inPath, "/dev/stdin") == 0)) {
 *     fprintf (stderr, "Reading from stdin\n");
 *     fprintf (stderr, "Remember to set samplerate with '-s'.\n");
 *     *num_samples = MAX_U_32_NUM;	
 *     return;
 *   }
 */
*num_samples = MAX_U_32_NUM;
return;

  /****************************  AIFF ********************************/
  if ((soundPosition = aiff_read_headers (musicin, &pcm_aiff_data)) != -1) {
    fprintf (stderr, ">>> Using Audio IFF sound file headers\n");
    aiff_check (inPath, &pcm_aiff_data, &header->version);
    if (fseek (musicin, soundPosition, SEEK_SET) != 0) {
      fprintf (stderr, "Could not seek to PCM sound data in \"%s\".\n",
	       inPath);
      exit (1);
    }
    fprintf (stderr, "Parsing AIFF audio file \n");
    header->sampling_frequency =
      toolame_SmpFrqIndex ((long) pcm_aiff_data.sampleRate, &header->version);
    fprintf (stderr, ">>> %f Hz sampling frequency selected\n",
	     pcm_aiff_data.sampleRate);

    /* Determine number of samples in sound file */
    *num_samples = pcm_aiff_data.numChannels * pcm_aiff_data.numSampleFrames;

    if (pcm_aiff_data.numChannels == 1) {
      header->mode = MPG_MD_MONO;
      header->mode_ext = 0;
    }
    return;
  }

  /**************************** WAVE *********************************/
  /*   Nick Burch <The_Leveller@newmail.net> */
  /*********************************/
  /* Wave File Headers:   (Dec)    */
  /* 8-11 = "WAVE"                 */
  /* 22 = Stereo / Mono            */
  /*       01 = mono, 02 = stereo  */
  /* 24 = Sampling Frequency       */
  /* 32 = Data Rate                */
  /*       01 = x1 (8bit Mono)     */
  /*       02 = x2 (8bit Stereo or */
  /*                16bit Mono)    */
  /*       04 = x4 (16bit Stereo)  */
  /*********************************/

  fseek (musicin, 0, SEEK_SET);
  fread (wave_header_buffer, 1, 40, musicin);

  if (wave_header_buffer[8] == 'W' && wave_header_buffer[9] == 'A'
      && wave_header_buffer[10] == 'V' && wave_header_buffer[11] == 'E') {
    fprintf (stderr, "Parsing Wave File Header\n");
    if (NativeByteOrder == order_unknown) {
      NativeByteOrder = DetermineByteOrder ();
      if (NativeByteOrder == order_unknown) {
	fprintf (stderr, "byte order not determined\n");
	exit (1);
      }
    }
    if (NativeByteOrder == order_littleEndian) {
      samplerate = *(unsigned long *) (&wave_header_buffer[24]);
    } else {
      samplerate = wave_header_buffer[27] +
	(wave_header_buffer[26] << 8) +
	(wave_header_buffer[25] << 16) + (wave_header_buffer[24] << 24);
    }
    /* Wave File */
    wave_header_read = 1;
    switch (samplerate) {
    case 44100:
    case 48000:
    case 32000:
    case 24000:
    case 22050:
    case 16000:
      fprintf (stderr, ">>> %ld Hz sampling freq selected\n", samplerate);
      break;
    default:
      /* Unknown Unsupported Frequency */
      fprintf (stderr, ">>> Unknown samp freq %ld Hz in Wave Header\n",
	       samplerate);
      fprintf (stderr, ">>> Default 44.1 kHz samp freq selected\n");
      samplerate = 44100;
    }

    if ((header->sampling_frequency =
	 toolame_SmpFrqIndex ((long) samplerate, &header->version)) < 0) {
      fprintf (stderr, "invalid sample rate\n");
      exit (0);
    }

    if ((long) wave_header_buffer[22] == 1) {
      fprintf (stderr, ">>> Input Wave File is Mono\n");
      wave_header_stereo = 0;
      header->mode = MPG_MD_MONO;
      header->mode_ext = 0;
    }
    if ((long) wave_header_buffer[22] == 2) {
      fprintf (stderr, ">>> Input Wave File is Stereo\n");
      wave_header_stereo = 1;
    }
    if ((long) wave_header_buffer[32] == 1) {
      fprintf (stderr, ">>> Input Wave File is 8 Bit\n");
      wave_header_16bit = 0;
      fprintf (stderr, "Input File must be 16 Bit! Please Re-sample");
      exit (1);
    }
    if ((long) wave_header_buffer[32] == 2) {
      if (wave_header_stereo == 1) {
	fprintf (stderr, ">>> Input Wave File is 8 Bit\n");
	wave_header_16bit = 0;
	fprintf (stderr, "Input File must be 16 Bit! Please Re-sample");
	exit (1);
      } else {
	/* fprintf(stderr,  ">>> Input Wave File is 16 Bit\n" ); */
	wave_header_16bit = 1;
      }
    }
    if ((long) wave_header_buffer[32] == 4) {
      /* fprintf(stderr,  ">>> Input Wave File is 16 Bit\n" ); */
      wave_header_16bit = 1;
    }
    /* should probably use the wave header to determine size here FIXME MFC Feb 2003 */
    *num_samples = MAX_U_32_NUM;
    if (fseek (musicin, 44, SEEK_SET) != 0) {	/* there's a way of calculating the size of the
						   wave header. i'll just jump 44 to start with */
      fprintf (stderr, "Could not seek to PCM sound data in \"%s\".\n",
	       inPath);
      exit (1);
    }
    return;
  }

  /*************************** PCM **************************/
  fprintf (stderr, "No header found. Assuming Raw PCM sound file\n");
  /* Raw PCM. No header. Reset the input file to read from the start */
  fseek (musicin, 0, SEEK_SET);
  /* Assume it is a huge sound file since there's no real info available */
  /* FIXME: Could always fstat the file? Probably not worth it. MFC Feb 2003 */
  *num_samples = MAX_U_32_NUM;
}



/************************************************************************
*
* aiff_check
*
* PURPOSE:  Checks AIFF header information to make sure it is valid.
*           Exits if not.
*
************************************************************************/

void aiff_check (char *file_name, IFF_AIFF * pcm_aiff_data, int *version)
{
  if (pcm_aiff_data->sampleType != IFF_ID_SSND) {
    fprintf (stderr, "Sound data is not PCM in \"%s\".\n", file_name);
    exit (1);
  }

  if (toolame_SmpFrqIndex ((long) pcm_aiff_data->sampleRate, version) < 0) {
    fprintf (stderr, "in \"%s\".\n", file_name);
    exit (1);
  }

  if (pcm_aiff_data->sampleSize != sizeof (short) * BITS_IN_A_BYTE) {
    fprintf (stderr, "Sound data is not %d bits in \"%s\".\n",
	     sizeof (short) * BITS_IN_A_BYTE, file_name);
    exit (1);
  }

  if (pcm_aiff_data->numChannels != MONO
      && pcm_aiff_data->numChannels != STEREO) {
    fprintf (stderr, "Sound data is not mono or stereo in \"%s\".\n",
	     file_name);
    exit (1);
  }

  if (pcm_aiff_data->blkAlgn.blockSize != 0) {
    fprintf (stderr, "Block size is not %d bytes in \"%s\".\n", 0, file_name);
    exit (1);
  }

  if (pcm_aiff_data->blkAlgn.offset != 0) {
    fprintf (stderr, "Block offset is not %d bytes in \"%s\".\n", 0,
	     file_name);
    exit (1);
  }
}
