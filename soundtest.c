/*
  Broadcast 2.0 multitrack audio editing
  (c) 1997 Heroine Virtual

  This program is distributed with the intent that it will be useful, without
  any warranty.  The code is being updated and has many bugs which are
  constantly being elucidated so changes should not be made without notifying
  the author.
*/

/* Run at startup to permanently allocate your DMA buffers */

#include <sys/soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>

int main(int argc, char *argv[]){
	int dsp;
	int duplexenable = 1;
	audio_buf_info playinfo, recinfo;
	int bufsize = 0x7FFF0000;
	int fragsize;

	int format = AFMT_S16_LE;
	int channels = 2;
	int samplerate = 44100;

	if(argc < 2)
	{
		bufsize += 15;
	}
	else
	{
		fragsize = atol(argv[1]);
		if(fragsize <= 0) fragsize = 32768;
		bufsize += (long)(log(fragsize) / log(2));
	}

#ifdef USE_FULLDUPLEX

	printf("*** Full duplex\n");

	if((dsp = open("/dev/dsp", O_RDWR)) < 0){
    	printf("Can't open audio device.\n");
    	close(dsp);
    	return 1;
	}

	if (ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &bufsize))
  		printf("Couldn't set buffer parameters.\n");

	if(duplexenable && ioctl(dsp, SNDCTL_DSP_SETDUPLEX, 0) == -1){
		printf("Couldn't enable full duplex audio.\n");
		duplexenable = 0;
	}

	if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) < 0)
       printf("playback file format failed\n");
	if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) < 0)
       printf("playback channel allocation failed\n");
	if(ioctl(dsp, SNDCTL_DSP_SPEED, &samplerate) < 0)
       printf("playback sample rate set failed\n");

	ioctl(dsp, SNDCTL_DSP_GETOSPACE, &playinfo);

	printf("          fragments fragstotal fragsize   bytes  TOTAL BYTES AVAILABLE\n");
	printf("Playback: %9d %10d %8d %7d %12d\n", playinfo.fragments, 
  		playinfo.fragstotal, playinfo.fragsize, playinfo.bytes, playinfo.bytes);

	if(duplexenable)
	{
  		if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) < 0)
    		 printf("record file format failed\n");
  		if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) < 0)
    		 printf("record channel allocation failed\n");
  		if(ioctl(dsp, SNDCTL_DSP_SPEED, &samplerate) < 0)
    		 printf("record sample rate set failed\n");

  		ioctl(dsp, SNDCTL_DSP_GETISPACE, &recinfo);

  		printf("Record:   %9d %10d %8d %7d %12d\n", recinfo.fragments, 
  			recinfo.fragstotal, recinfo.fragsize, recinfo.bytes, recinfo.fragstotal * recinfo.fragsize);
	}
	close(dsp);


#endif


	printf("\n*** Half duplex\n");

  if((dsp = open("/dev/dsp", O_WRONLY)) < 0){
    printf("Can't open audio device.\n");
    close(dsp);
    return 1;
  }

  if (ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &bufsize))
  	printf("Couldn't set buffer parameters.\n");
  
	if(duplexenable && ioctl(dsp, SNDCTL_DSP_SETDUPLEX, 0) == -1){
		printf("Couldn't enable full duplex audio.\n");
		duplexenable = 0;
	}

  if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) < 0)
     printf("playback file format failed\n");
  if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) < 0)
     printf("playback channel allocation failed\n");
  if(ioctl(dsp, SNDCTL_DSP_SPEED, &samplerate) < 0)
     printf("playback sample rate set failed\n");

  ioctl(dsp, SNDCTL_DSP_GETOSPACE, &playinfo);

	printf("          fragments fragstotal fragsize   bytes  TOTAL BYTES AVAILABLE\n");
	printf("Playback: %9d %10d %8d %7d %12d\n", playinfo.fragments, 
  		playinfo.fragstotal, playinfo.fragsize, playinfo.bytes, playinfo.bytes);
	close(dsp);



  if((dsp = open("/dev/dsp", O_RDONLY)) < 0){
    printf("Can't open audio device.\n");
    close(dsp);
    return 1;
  }

  if (ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &bufsize))
  	printf("Couldn't set buffer parameters.\n");
  
	if(duplexenable && ioctl(dsp, SNDCTL_DSP_SETDUPLEX, 0) == -1){
		printf("Couldn't enable full duplex audio.\n");
		duplexenable = 0;
	}

  if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) < 0)
     printf("playback file format failed\n");
  if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) < 0)
     printf("playback channel allocation failed\n");
  if(ioctl(dsp, SNDCTL_DSP_SPEED, &samplerate) < 0)
     printf("playback sample rate set failed\n");

  	if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) < 0)
    	 printf("record file format failed\n");
  	if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) < 0)
    	 printf("record channel allocation failed\n");
  	if(ioctl(dsp, SNDCTL_DSP_SPEED, &samplerate) < 0)
    	 printf("record sample rate set failed\n");




  	ioctl(dsp, SNDCTL_DSP_GETISPACE, &recinfo);

  	printf("Record:   %9d %10d %8d %7d %12d\n", recinfo.fragments, 
  		recinfo.fragstotal, recinfo.fragsize, recinfo.bytes, recinfo.fragstotal * recinfo.fragsize);
	
  ioctl(dsp, SNDCTL_DSP_RESET);
  close(dsp);
}




