#include "funcprotos.h"
#include "quicktime.h"

int usage(void)
{
	printf("usage: rechunk [-f framerate] [-w width] [-h height] [-c fourcc] <input frames> <output movie>\n");
	printf("	Concatenate input frames into a Quicktime movie.\n");
	exit(1);
	return 0;
}

int main(int argc, char *argv[])
{
	quicktime_t *file;
	FILE *input;
	int result = 0;
	int i, j;
	int64_t length;
	char string[1024], *output = 0;
	char *data = 0;
	int bytes = 0, old_bytes = 0;
	float output_rate = 0;
	float input_rate;
	int64_t input_frame;
	int64_t new_length;
	int rgb_to_ppm = 0;
	char **input_frames = 0;
	int total_input_frames = 0;
	int width = 720, height = 480;
	char compressor[5] = "yv12";

	if(argc < 3)
	{
		usage();
	}

	for(i = 1, j = 0; i < argc; i++)
	{
		if(!strcmp(argv[i], "-f"))
		{
			if(i + 1 < argc)
			{
				output_rate = atof(argv[++i]);
			}
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-w"))
		{
			if(i + 1 < argc)
				width = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-h"))
		{
			if(i + 1 < argc)
				height = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-c"))
		{
			if(i + 1 < argc)
			{
				strncpy(compressor, argv[++i], 4);
			}
			else
				usage();
		}
		if(i == argc - 1)
		{
			output = argv[i];
		}
		else
		{
			total_input_frames++;
			input_frames = realloc(input_frames, sizeof(char*) * total_input_frames);
			input_frames[total_input_frames - 1] = argv[i];
		}
	}

	if(!input) usage();

	if(input = fopen(output, "rb"))
	{
		printf("Output file already exists.\n");
		exit(1);
	}

	if(!(file = quicktime_open(output, 0, 1)))
	{
		printf("Open failed\n");
		exit(1);
	}
	
	quicktime_set_video(file, 1, width, height, output_rate, compressor);
	
	for(i = 0; i < total_input_frames; i++)
	{
/* Get output file */
		if(!(input = fopen(input_frames[i], "rb")))
		{
			perror("Open failed");
			continue;
		}

/* Get input frame */
		fseek(input, 0, SEEK_END);
		bytes = ftell(input);
		fseek(input, 0, SEEK_SET);
		data = realloc(data, bytes);

		fread(data, bytes, 1, input);
		quicktime_write_frame(file, data, bytes, 0);
		fclose(input);
	}

	quicktime_close(file);
}
