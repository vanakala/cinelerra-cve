// New version.
// Very basic table of contents utility since most of the time it's going to be
// built inside a graphical program.

#include "libmpeg3.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>





int main(int argc, char *argv[])
{
	int i, j, l;
	char *src = 0, *dst = 0;
	int verbose = 0;

	if(argc < 3)
	{
		fprintf(stderr, "Table of contents generator version %d.%d.%d\n"
			"Create a table of contents for a DVD or mpeg stream.\n"
			"Usage: mpeg3toc <path> <output>\n"
			"\n"
			"-v Print tracking information\n"
			"\n"
			"The path should be absolute unless you plan\n"
			"to always run your movie editor from the same directory\n"
			"as the filename.  For renderfarms the filesystem prefix\n"
			"should be / and the movie directory mounted under the same\n"
			"directory on each node.\n\n"
			"Example: mpeg3toc -v /cdrom/video_ts/vts_01_0.ifo titanic.toc\n",
			mpeg3_major(),
			mpeg3_minor(),
			mpeg3_release());
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-v"))
		{
			verbose = 1;
		}
		else
		if(argv[i][0] == '-')
		{
			fprintf(stderr, "Unrecognized command %s\n", argv[i]);
			exit(1);
		}
		else
		if(!src)
		{
			src = argv[i];
		}
		else
		if(!dst)
		{
			dst = argv[i];
		}
		else
		{
			fprintf(stderr, "Ignoring argument \"%s\"\n", argv[i]);
		}
	}

	if(!src)
	{
		fprintf(stderr, "source path not supplied.\n");
		exit(1);
	}

	if(!dst)
	{
		fprintf(stderr, "destination path not supplied.\n");
		exit(1);
	}



	int64_t total_bytes;
	mpeg3_t *file = mpeg3_start_toc(src, dst, &total_bytes);
	if(!file) exit(1);
	struct timeval new_time;
	struct timeval prev_time;
	struct timeval start_time;
	struct timeval current_time;
	gettimeofday(&prev_time, 0);
	gettimeofday(&start_time, 0);
	
	
	while(1)
	{
		int64_t bytes_processed = 0;
		mpeg3_do_toc(file, &bytes_processed);

		gettimeofday(&new_time, 0);
		if(verbose && new_time.tv_sec - prev_time.tv_sec > 1)
		{
			gettimeofday(&current_time, 0);
			int64_t elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
			int64_t total_seconds = elapsed_seconds * total_bytes / bytes_processed;
			int64_t eta = total_seconds - elapsed_seconds;
			fprintf(stderr, "%lld%% ETA: %dm%ds        \r", 
				bytes_processed * 100 / total_bytes,
				eta / 60,
				eta % 60);
			fflush(stdout);
			prev_time = new_time;
		}

		if(bytes_processed >= total_bytes) break;
	}

	mpeg3_stop_toc(file);
	gettimeofday(&current_time, 0);
	int64_t elapsed = current_time.tv_sec - start_time.tv_sec;
	if(verbose)
	{
		fprintf(stderr, "%dm%ds elapsed           \n", 
			elapsed / 60,
			elapsed % 60);
	}

	return 0;
}


