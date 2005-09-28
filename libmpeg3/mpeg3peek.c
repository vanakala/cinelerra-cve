#include "libmpeg3.h"
#include <stdlib.h>




int main(int argc, char *argv[])
{
	mpeg3_t *file;
	if(argc < 3)
	{
		printf("Usage: mpeg3peek <table of contents> <frame number>\n");
		printf("Print the byte offset of a given frame.\n");
		printf("Only works for video.  Requires table of contents.\n");
		printf("Example: mpeg3peek heroine.toc 123\n");
		exit(1);
	}


	file = mpeg3_open(argv[1]);
	if(file)
	{
		if(!mpeg3_total_vstreams(file))
		{
			printf("Need a video stream.\n");
			exit(1);
		}

		if(!file->vtrack[0]->total_frame_offsets)
		{
			printf("Zero length track.  Did you load a table of contents?\n");
			exit(1);
		}

		int frame_number = atoi(argv[2]);
		if(frame_number < 0) frame_number = 0;
		if(frame_number > file->vtrack[0]->total_frame_offsets)
			frame_number = file->vtrack[0]->total_frame_offsets - 1;
		printf("frame=%d offset=0x%llx\n", 
			frame_number,
			file->vtrack[0]->frame_offsets[frame_number]);
	}
}
