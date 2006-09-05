#include "funcprotos.h"
#include "quicktime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdint.h>




/*

Face it kids, Linux crashes.  Regardless of how good the free software
model is, you still need to run on unreliable hardware and to get any
hardware support at all you need to compete with McRosoft on features. 
Ever since RedHat started trying to copy McRosoft's every move,
reliability has been fickle at best.

The X server crashes, the filesystem crashes, the network crashes, the
sound driver crashes, the video driver crashes.  Signal handlers only
handle application failures but most of the crashes are complete system
lockups.   

This utility should read through a truncated movie file and put a
header on it, no matter what immitated McRosoft feature caused the
crash.  It only handles JPEG encoded with jpeg-6b and MJPEG encoded
with the BUZ driver with PCM audio.

*/







#define FSEEK fseeko64


#define WIDTH 720
#define HEIGHT 480
#define FRAMERATE (double)30000/1001
#define CHANNELS 2
#define SAMPLERATE 48000
#define BITS 24
#define TEMP_FILE "/tmp/temp.mov"
#define VCODEC QUICKTIME_MJPA
//#define VCODEC QUICKTIME_JPEG





#define SEARCH_FRAGMENT (int64_t)0x100000
//#define SEARCH_PAD 8
#define SEARCH_PAD 16


#define GOT_NOTHING 0
#define IN_FIELD1   1
#define GOT_FIELD1  2
#define IN_FIELD2   3
#define GOT_FIELD2  4
#define GOT_AUDIO   5
#define GOT_IMAGE_START 6
#define GOT_IMAGE_END   7

// Table utilities
#define NEW_TABLE(ptr, size, allocation) \
{ \
	(ptr) = 0; \
	(size) = 0; \
	(allocation) = 0; \
}

#define APPEND_TABLE(ptr, size, allocation, value) \
{ \
	if((allocation) <= (size)) \
	{ \
		if(!(allocation)) \
			(allocation) = 1024; \
		else \
			(allocation) *= 2; \
		int64_t *new_table = calloc(1, sizeof(int64_t) * (allocation)); \
		memcpy(new_table, (ptr), sizeof(int64_t) * (size)); \
		free((ptr)); \
		(ptr) = new_table; \
	} \
	(ptr)[(size)] = (value); \
	(size)++; \
}



int main(int argc, char *argv[])
{
	FILE *in;
	FILE *temp;
	quicktime_t *out;
	int64_t current_byte, ftell_byte;
	int64_t jpeg_end;
	int64_t audio_start = 0, audio_end = 0;
	unsigned char *search_buffer = calloc(1, SEARCH_FRAGMENT);
	unsigned char *copy_buffer = 0;
	int i;
	int64_t file_size;
	struct stat status;
	unsigned char data[8];
	struct stat ostat;
	int fields = 1;
	time_t current_time = time(0);
	time_t prev_time = 0;
	int jpeg_header_offset;
	int64_t field1_offset = 0;
	int64_t field2_offset = 0;
	int64_t image_start = 0;
	int64_t image_end = 0;
	int update_time = 0;
	int state = GOT_NOTHING;
	char *in_path;
	int audio_frame;
	int total_samples;
	int field;

// Value taken from Cinelerra preferences
	int audio_chunk = 131072;


	int64_t *start_table;
	int start_size;
	int start_allocation;
	int64_t *end_table;
	int end_size;
	int end_allocation;
	int64_t *field_table;
	int64_t field_size;
	int64_t field_allocation;

// Dump codec settings
	printf("Codec settings:\n"
		"   WIDTH=%d HEIGHT=%d\n"
		"   FRAMERATE=%.2f\n"
		"   CHANNELS=%d\n"
		"   SAMPLERATE=%d\n"
		"   BITS=%d\n"
		"   audio chunk=%d\n"
		"   VCODEC=\"%s\"\n",
		WIDTH,
		HEIGHT,
		FRAMERATE,
		CHANNELS,
		SAMPLERATE,
		BITS,
		audio_chunk,
		VCODEC);

	if(argc < 2)
	{
		printf("Recover JPEG and PCM audio in a corrupted movie.\n"
			"Usage: recover [options] <input>\n"
			"Options:\n"
			" -b samples     number of samples in an audio chunk (%d)\n"
			"\n",
			audio_chunk);
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-b"))
		{
			if(i + 1 < argc)
			{
				audio_chunk = atol(argv[i + 1]);
				i++;
				if(audio_chunk <= 0)
				{
					printf("Sample count for -b is out of range.\n");
					exit(1);
				}
			}
			else
			{
				printf("-b needs a sample count.\n");
				exit(1);
			}
		}
		else
		{
			in_path = argv[i];
		}
	}




	in = fopen(in_path, "rb+");
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
	audio_start = (int64_t)0x10;
	ftell_byte = 0;

	if(fstat(fileno(in), &status))
		perror("get_file_length fstat:");
	file_size = status.st_size;


	NEW_TABLE(start_table, start_size, start_allocation)
	NEW_TABLE(end_table, end_size, end_allocation)
	NEW_TABLE(field_table, field_size, field_allocation)


// Get the field count
	if(!memcmp(VCODEC, QUICKTIME_MJPA, 4))
	{
		fields = 2;
	}
	else
	{
		fields = 1;
	}

	audio_frame = BITS * CHANNELS / 8;

// Tabulate the start and end of all the JPEG images.
// This search is intended to be as simple as possible, reserving more
// complicated operations for a table pass.
printf("Pass 1 video only.\n");
	while(ftell_byte < file_size)
	{
		current_byte = ftell_byte;
		fread(search_buffer, SEARCH_FRAGMENT, 1, in);
		ftell_byte = current_byte + SEARCH_FRAGMENT - SEARCH_PAD;
		FSEEK(in, ftell_byte, SEEK_SET);

		for(i = 0; i < SEARCH_FRAGMENT - SEARCH_PAD; i++)
		{
// Search for image start
			if(state == GOT_NOTHING)
			{
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd8 &&
					search_buffer[i + 2] == 0xff &&
					search_buffer[i + 3] == 0xe1 &&
					search_buffer[i + 10] == 'm' &&
					search_buffer[i + 11] == 'j' &&
					search_buffer[i + 12] == 'p' &&
					search_buffer[i + 13] == 'g')
				{
					state = GOT_IMAGE_START;
					image_start = current_byte + i;

// Determine the field
					if(fields == 2)
					{
// Next field offset is nonzero in first field
						if(search_buffer[i + 22] != 0 ||
							search_buffer[i + 23] != 0 ||
							search_buffer[i + 24] != 0 ||
							search_buffer[i + 25] != 0)
						{
							field = 0;
						}
						else
						{
							field = 1;
						}
						APPEND_TABLE(field_table, field_size, field_allocation, field)
					}
				}
				else
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd8 &&
					search_buffer[i + 2] == 0xff &&
					search_buffer[i + 3] == 0xe0 &&
					search_buffer[i + 6] == 'J' &&
					search_buffer[i + 7] == 'F' &&
					search_buffer[i + 8] == 'I' &&
					search_buffer[i + 9] == 'F')
				{
					state = GOT_IMAGE_START;
					image_start = current_byte + i;
				}
			}
			else
// Search for image end
			if(state == GOT_IMAGE_START)
			{
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd9)
				{
// ffd9 sometimes occurs inside the mjpg tag
					if(current_byte + i - image_start > 0x2a)
					{
						state = GOT_NOTHING;
// Put it in the table
						image_end = current_byte + i + 2;

// An image may have been lost due to encoding errors but we can't do anything
// because the audio may by misaligned.  Use the extract utility to get the audio.
						if(image_end - image_start > audio_chunk * audio_frame)
						{
							printf("Possibly lost image between %llx and %llx\n", 
								image_start,
								image_end);
// Put in fake image
/*
 * 							APPEND_TABLE(start_table, start_size, start_allocation, image_start)
 * 							APPEND_TABLE(end_table, end_size, end_allocation, image_start + 1024)
 * 							APPEND_TABLE(start_table, start_size, start_allocation, image_end - 1024)
 * 							APPEND_TABLE(end_table, end_size, end_allocation, image_end)
 */
						}

						APPEND_TABLE(start_table, start_size, start_allocation, image_start)
						APPEND_TABLE(end_table, end_size, end_allocation, image_end)

//printf("%d %llx - %llx\n", start_size, image_start, image_end - image_start);

if(!(start_size % 100))
{
printf("Got %d frames. %d%%\r", 
start_size, 
current_byte * (int64_t)100 / file_size);
fflush(stdout);
}
					}
				}
			}
		}
	}




// With the image table complete, 
// write chunk table from the gaps in the image table
printf("Pass 2 audio table.\n");
	total_samples = 0;
	for(i = 1; i < start_size; i++)
	{
		int64_t next_image_start = start_table[i];
		int64_t prev_image_end = end_table[i - 1];

// Got a chunk
		if(next_image_start - prev_image_end >= audio_chunk * audio_frame)
		{
			long samples = (next_image_start - prev_image_end) / audio_frame;
			quicktime_atom_t chunk_atom;

			quicktime_set_position(out, prev_image_end);
			quicktime_write_chunk_header(out, 
				out->atracks[0].track, 
				&chunk_atom);
			quicktime_set_position(out, next_image_start);
			quicktime_write_chunk_footer(out,
				out->atracks[0].track, 
				out->atracks[0].current_chunk, 
				&chunk_atom,
				samples);
/*
 * 			quicktime_update_tables(out, 
 * 						out->atracks[0].track, 
 * 						prev_image_end, 
 * 						out->atracks[0].current_chunk, 
 * 						out->atracks[0].current_position, 
 * 						samples, 
 * 						0);
 */
			out->atracks[0].current_position += samples;
			out->atracks[0].current_chunk++;
			total_samples += samples;
		}
	}





// Put image table in movie
printf("Got %d frames %d samples total.\n", start_size, total_samples);
	for(i = 0; i < start_size - fields; i += fields)
	{
// Got a field out of order.  Skip just 1 image instead of 2.
		if(fields == 2 && field_table[i] != 0)
		{
			printf("Got field out of order at 0x%llx\n", start_table[i]);
			i--;
		}
		else
		{
			quicktime_atom_t chunk_atom;
			quicktime_set_position(out, start_table[i]);
			quicktime_write_chunk_header(out, 
				out->vtracks[0].track,
				&chunk_atom);
			quicktime_set_position(out, end_table[i + fields - 1]);
			quicktime_write_chunk_footer(out,
				out->vtracks[0].track, 
				out->vtracks[0].current_chunk, 
				&chunk_atom,
				1);
/*
 * 			quicktime_update_tables(out,
 * 						out->vtracks[0].track,
 * 						start_table[i],
 * 						out->vtracks[0].current_chunk,
 * 						out->vtracks[0].current_position,
 * 						1,
 * 						end_table[i + fields - 1] - start_table[i]);
 */
			out->vtracks[0].current_position++;
			out->vtracks[0].current_chunk++;
		}
	}







// Force header out at beginning of temp file
	quicktime_set_position(out, 0x10);
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




