#include "funcprotos.h"
#include "quicktime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdint.h>

#define FSEEK fseeko64


#define WIDTH 720
#define HEIGHT 480
#define FRAMERATE (double)30000/1001
#define CHANNELS 2
#define SAMPLERATE 48000
#define BITS 16
#define TEMP_FILE "/tmp/temp.mov"
#define VCODEC QUICKTIME_MJPA





#define SEARCH_FRAGMENT (longest)0x100000
#define SEARCH_PAD 8

int main(int argc, char *argv[])
{
	FILE *in;
	FILE *temp;
	quicktime_t *out;
	longest current_byte, ftell_byte;
	longest jpeg_end;
	longest audio_start = 0, audio_end = 0;
	unsigned char *search_buffer = calloc(1, SEARCH_FRAGMENT);
	unsigned char *copy_buffer = 0;
	int i;
	longest file_size;
	struct stat status;
	unsigned char data[8];
	struct stat ostat;
	int fields = 1;
	time_t current_time = time(0);
	time_t prev_time = 0;
	int jpeg_header_offset;
	int64_t field1_offset = 0;
	int field_count = 0;
	int eoi_count = 0;
	int update_time = 0;

	printf("Recover JPEG and PCM audio in a corrupted movie.\n"
		"Usage: recover <input>\n"
		"Compiled settings:\n"
		"   WIDTH %d\n"
		"   HEIGHT %d\n"
		"   FRAMERATE %.2f\n"
		"   CHANNELS %d\n"
		"   SAMPLERATE %d\n"
		"   BITS %d\n"
		"   VCODEC %s\n",
		WIDTH,
		HEIGHT,
		FRAMERATE,
		CHANNELS,
		SAMPLERATE,
		BITS,
		VCODEC);
	if(argc < 2)	   
	{				   
		exit(1);
	}

	in = fopen(argv[1], "rb+");
	out = quicktime_open(TEMP_FILE, 0, 1);

	if(!in)
	{
		perror("open input");
		exit(1);
	}
	if(!out)
	{
		perror("open temp");
		exit(1);
	}

	quicktime_set_audio(out, 
		CHANNELS, 
		SAMPLERATE, 
		BITS, 
		QUICKTIME_TWOS);
	quicktime_set_video(out, 
		1, 
		WIDTH, 
		HEIGHT, 
		FRAMERATE, 
		VCODEC);
	audio_start = (longest)0x10;
	field_count = 0;
	eoi_count = 0;
	ftell_byte = 0;

	if(!memcmp(VCODEC, QUICKTIME_MJPA, 4))
	{
		fields = 2;
		jpeg_header_offset = 46;
	}
	else
	{
		jpeg_header_offset = 2;
	}
//printf("fields =%d\n", fields);

	if(fstat(fileno(in), &status))
		perror("get_file_length fstat:");
	file_size = status.st_size;
	

//printf("recover %lld\n", file_size);
	while(ftell_byte < file_size)
	{
// Search forward for JFIF
		current_byte = ftell_byte;
		fread(search_buffer, SEARCH_FRAGMENT, 1, in);
		ftell_byte = current_byte + SEARCH_FRAGMENT - SEARCH_PAD;
		FSEEK(in, ftell_byte, SEEK_SET);

		for(i = 0; i < SEARCH_FRAGMENT - SEARCH_PAD; i++)
		{
			update_time = 0;
			if(field_count < fields)
			{
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xe0 &&
					search_buffer[i + 4] == 'J' &&
					search_buffer[i + 5] == 'F' &&
					search_buffer[i + 6] == 'I' &&
					search_buffer[i + 7] == 'F')
				{
					if(field_count == 0)
					{
						field1_offset = current_byte + i - jpeg_header_offset;
						audio_end = field1_offset;
//printf("main 1 audio_end=%lld audio_start=%lld\n", audio_end, audio_start);
					}
					field_count++;
				}
			}
			else
			if(!eoi_count)
			{
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd9)
				{
					audio_start = jpeg_end = current_byte + i + 2;
//printf("main 2 audio_end=%lld audio_start=%lld\n", audio_end, audio_start);
					eoi_count = 1;
				}
			}

// Got an audio chunk
			if(audio_end - audio_start > 0)
			{
				long samples = (audio_end - audio_start) / (CHANNELS * BITS / 8);
				quicktime_update_tables(out, 
							out->atracks[0].track, 
							audio_start, 
							out->atracks[0].current_chunk, 
							out->atracks[0].current_position, 
							samples, 
							0);
				out->atracks[0].current_position += samples;
				out->atracks[0].current_chunk++;
				audio_start = audio_end;
				update_time = 1;
//printf("audio chunk %d\n", out->atracks[0].current_position);
			}

// Got video frame
			if(field_count == fields && eoi_count)
			{
				quicktime_update_tables(out,
							out->vtracks[0].track,
							field1_offset,
							out->vtracks[0].current_chunk,
							out->vtracks[0].current_position,
							1,
							jpeg_end - field1_offset);
				out->vtracks[0].current_position++;
				out->vtracks[0].current_chunk++;
//printf("video chunk %d %d\n", found_jfif, out->vtracks[0].current_position);
				field_count = 0;
				eoi_count = 0;
				update_time = 1;
			}

			if(update_time)
			{
				current_time = time(0);
				if((int64_t)current_time - (int64_t)prev_time >= 1)
				{
printf("samples %d frames %d\r", out->atracks[0].current_position, out->vtracks[0].current_position);
fflush(stdout);
					prev_time = current_time;
				}
			}



		}

	}
printf("\n\n");
// Force header out
	quicktime_close(out);

// Transfer header
	FSEEK(in, 0x8, SEEK_SET);

	data[0] = (ftell_byte & 0xff00000000000000LL) >> 56;
	data[1] = (ftell_byte & 0xff000000000000LL) >> 48;
	data[2] = (ftell_byte & 0xff0000000000LL) >> 40;
	data[3] = (ftell_byte & 0xff00000000LL) >> 32;
	data[4] = (ftell_byte & 0xff000000LL) >> 24;
	data[5] = (ftell_byte & 0xff0000LL) >> 16;
	data[6] = (ftell_byte & 0xff00LL) >> 8;
	data[7] = ftell_byte & 0xff;
	fwrite(data, 8, 1, in);

	FSEEK(in, ftell_byte, SEEK_SET);
	stat(TEMP_FILE, &ostat);
	temp = fopen(TEMP_FILE, "rb");
	FSEEK(temp, 0x10, SEEK_SET);
	copy_buffer = calloc(1, ostat.st_size);
	fread(copy_buffer, ostat.st_size, 1, temp);
	fclose(temp);
	fwrite(copy_buffer, ostat.st_size, 1, in);

	fclose(in);
}




