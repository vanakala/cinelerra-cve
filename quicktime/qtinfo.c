/* 	Qtinfo by Elliot Lee <sopwith@redhat.com> */

#include "quicktime.h"

static void file_info(char *filename);

int main(int argc, char *argv[])
{
	int i;

	if(argc < 2) {
		printf("Usage: %s filename...\n", argv[0]);
		return 1;
	}

	for(i = 1; i < argc; i++) {
		file_info(argv[i]);
	}

	return 0;
}

static void
file_info(char *filename)
{
	quicktime_t* qtfile;
	int i, n;

	qtfile = quicktime_open(filename, 1, 0);

	if(!qtfile) {
		printf("Couldn't open %s as a QuickTime file.\n", filename);
		return;
	}

	printf("\nFile %s:\n", filename);
	n = quicktime_audio_tracks(qtfile);
	printf("  %d audio tracks.\n", n);
	for(i = 0; i < n; i++) {

	  printf("    %d channels. %d bits. sample rate %ld. length %ld. compressor %s.\n",
		 quicktime_track_channels(qtfile, i),
		 quicktime_audio_bits(qtfile, i),
		 quicktime_sample_rate(qtfile, i),
		 quicktime_audio_length(qtfile, i),
		 quicktime_audio_compressor(qtfile, i));
	  printf("    %ssupported.\n",
		 quicktime_supported_audio(qtfile, i)?"":"NOT ");
	}

	n = quicktime_video_tracks(qtfile);
	printf("  %d video tracks.\n", n);
	for(i = 0; i < n; i++) {
	  printf("    %dx%d rate %f length %ld compressor %s.\n",
		 quicktime_video_width(qtfile, i),
		 quicktime_video_height(qtfile, i),
		 quicktime_frame_rate(qtfile, i),
		 quicktime_video_length(qtfile, i),
		 quicktime_video_compressor(qtfile, i));
	  printf("    %ssupported.\n",
		 quicktime_supported_video(qtfile, i)?"":"NOT ");
	}
}

