#include "quicktime.h"

int usage(void)
{
	printf("usage: yuv4toyuv <input movie> <output.yuv>\n");
	printf("	Write a YUV4 encoded movie as a planar YUV 4:2:0 file.\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	quicktime_t *file;
	FILE *output;
	int64_t length, width, height, bytes;
	char *buffer_in, *y_out, *u_out, *v_out;
	char *y_out1, *y_out2, *u_out1, *v_out1;
	char *input_row;
	int i, j, k, l, m;

	if(argc < 3)
	{
		usage();
	}

	if(!(file = quicktime_open(argv[1], 1, 0)))
	{
		printf("Open input failed\n");
		exit(1);
	}

	if(!(output = fopen(argv[2], "wb")))
	{
		perror("Open output failed");
		exit(1);
	}

	if(!quicktime_video_tracks(file))
	{
		printf("No video tracks.\n");
		exit(1);
	}

	length = quicktime_video_length(file, 0);
	width = quicktime_video_width(file, 0);
	height = quicktime_video_height(file, 0);
	bytes = width * height + width * height / 2;
	buffer_in = calloc(1, bytes);
	y_out = calloc(1, width * height);
	u_out = calloc(1, width * height / 4);
	v_out = calloc(1, width * height / 4);

	for(i = 0; i < length; i++)
	{
		quicktime_set_video_position(file, i, 0);
		quicktime_read_data(file, buffer_in, bytes);

		u_out1 = u_out;
		v_out1 = v_out;
		for(j = 0; j < height; j += 2)
		{
// Get 2 rows
			input_row = &buffer_in[j * width  + j * width / 2];
			y_out1 = y_out + j * width;
			y_out2 = y_out1 + width;

			for(k = 0; k < width / 2; k++)
			{
				*u_out1++ = (int)*input_row++ + 0x80;
				*v_out1++ = (int)*input_row++ + 0x80;
				*y_out1++ = *input_row++;
				*y_out1++ = *input_row++;
				*y_out2++ = *input_row++;
				*y_out2++ = *input_row++;
			}
		}

		if(!fwrite(y_out, width * height, 1, output))
		{
			perror("write failed");
			fclose(output);
			exit(1);
		}
		if(!fwrite(u_out, width * height / 4, 1, output))
		{
			perror("write failed");
			fclose(output);
			exit(1);
		}
		if(!fwrite(v_out, width * height / 4, 1, output))
		{
			perror("write failed");
			fclose(output);
			exit(1);
		}
	}

	quicktime_close(file);
}
