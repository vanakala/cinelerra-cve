#include <fcntl.h>
#include <linux/cdrom.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"

/* Disk I/O */

longest quicktime_get_file_length(quicktime_t *file)
{
	struct stat status;
	if(fstat(fileno(file->stream), &status))
		perror("quicktime_get_file_length fstat:");
	return status.st_size;
}

int quicktime_file_open(quicktime_t *file, char *path, int rd, int wr)
{
	int exists = 0;
	char flags[10];
	if(rd && (file->stream = fopen(path, "rb")))
	{
		exists = 1; 
		fclose(file->stream); 
	}

	if(rd && !wr) sprintf(flags, "rb");
	else
	if(!rd && wr) sprintf(flags, "wb");
	else
	if(rd && wr)
	{
		if(exists) 
			sprintf(flags, "rb+");
		else
			sprintf(flags, "wb+");
	}

	if(!(file->stream = fopen(path, flags)))
	{
		perror(__FUNCTION__);
		return 1;
	}

	if(rd && exists)
	{
		file->total_length = quicktime_get_file_length(file);		
	}
//printf("quicktime_file_open 1\n");
	
	return 0;
}

int quicktime_file_close(quicktime_t *file)
{
//printf("quicktime_file_close 1\n");
	if(file->stream)
	{
//printf("quicktime_file_close 2\n");
		fclose(file->stream);
	}
	file->stream = 0;
	return 0;
}



longest quicktime_ftell(quicktime_t *file)
{
	return file->ftell_position;
}

int quicktime_fseek(quicktime_t *file, longest offset)
{
	file->ftell_position = offset;
	if(offset > file->total_length || offset < 0) return 1;
//printf("quicktime_fseek %llx\n", offset);
	if(FSEEK(file->stream, file->ftell_position, SEEK_SET))
	{
//		perror("quicktime_fseek FSEEK");
		return 1;
	}
	return 0;
}

/* Read entire buffer from the preload buffer */
static int read_preload(quicktime_t *file, char *data, longest size)
{
	longest selection_start = 0;
	longest selection_end = 0;
	longest fragment_start = 0;
	longest fragment_len = 0;

	selection_start = file->file_position;
	selection_end = quicktime_add(file->file_position, size);
//printf("read_preload 1 %lld + %lld = %lld\n", file->file_position, size, selection_end);

	fragment_start = file->preload_ptr + (selection_start - file->preload_start);
	while(fragment_start < 0) fragment_start += file->preload_size;
	while(fragment_start >= file->preload_size) fragment_start -= file->preload_size;

	while(selection_start < selection_end)
	{
		fragment_len = selection_end - selection_start;
//printf("read_preload 1 %lld %lld %lld %lld\n", file->preload_size, fragment_start, fragment_len, size);
		if(fragment_start + fragment_len > file->preload_size)
			fragment_len = file->preload_size - fragment_start;

		memcpy(data, file->preload_buffer + fragment_start, fragment_len);
//printf("read_preload 2 %lld %lld %lld\n", file->preload_size, fragment_start, fragment_start + fragment_len);
		fragment_start += fragment_len;
		data += fragment_len;

		if(fragment_start >= file->preload_size) fragment_start = (longest)0;
		selection_start += fragment_len;
	}
//printf("read_preload 2\n");
	return 0;
}

int quicktime_read_data(quicktime_t *file, char *data, longest size)
{
	int result = 1;
//printf("quicktime_read_data %d\n", file->preload_size);
	if(!file->preload_size)
	{
//printf("quicktime_read_data %llx %llx\n", file->file_position, size);
		quicktime_fseek(file, file->file_position);
		result = fread(data, size, 1, file->stream);
		file->ftell_position += size;
	}
	else
	{
/* Region requested for loading */
		longest selection_start = file->file_position;
		longest selection_end = file->file_position + size;
		longest fragment_start, fragment_len;

		if(selection_end - selection_start > file->preload_size)
		{
/* Size is larger than preload size.  Should never happen. */
printf("read data Size is larger than preload size. size=%llx preload_size=%llx\n",
	selection_end - selection_start, file->preload_size);
			quicktime_fseek(file, file->file_position);
			result = fread(data, size, 1, file->stream);
			file->ftell_position += size;
		}
		else
		if(selection_start >= file->preload_start && 
			selection_start < file->preload_end &&
			selection_end <= file->preload_end &&
			selection_end > file->preload_start)
		{
/* Entire range is in buffer */
//printf("read data 2\n");
			read_preload(file, data, size);
		}
		else
		if(selection_end > file->preload_end && 
			selection_end - file->preload_size < file->preload_end)
		{
/* Range is after buffer */
/* Move the preload start to within one preload length of the selection_end */
//printf("read data 3 %lld\n", file->file_position);
			while(selection_end - file->preload_start > file->preload_size)
			{
				fragment_len = selection_end - file->preload_start - file->preload_size;
				if(file->preload_ptr + fragment_len > file->preload_size) 
					fragment_len = file->preload_size - file->preload_ptr;
				file->preload_start += fragment_len;
				file->preload_ptr += fragment_len;
				if(file->preload_ptr >= file->preload_size) file->preload_ptr = 0;
			}

/* Append sequential data after the preload end to the new end */
			fragment_start = file->preload_ptr + file->preload_end - file->preload_start;
			while(fragment_start >= file->preload_size) 
				fragment_start -= file->preload_size;

			while(file->preload_end < selection_end)
			{
				fragment_len = selection_end - file->preload_end;
				if(fragment_start + fragment_len > file->preload_size) fragment_len = file->preload_size - fragment_start;
				quicktime_fseek(file, file->preload_end);
				result = fread(&(file->preload_buffer[fragment_start]), fragment_len, 1, file->stream);
				file->ftell_position += fragment_len;
				file->preload_end += fragment_len;
				fragment_start += fragment_len;
				if(fragment_start >= file->preload_size) fragment_start = 0;
			}

			read_preload(file, data, size);
		}
		else
		{
//printf("quicktime_read_data 4 selection_start %lld selection_end %lld preload_start %lld\n", selection_start, selection_end, file->preload_start);
/* Range is before buffer or over a preload_size away from the end of the buffer. */
/* Replace entire preload buffer with range. */
			quicktime_fseek(file, file->file_position);
			result = fread(file->preload_buffer, size, 1, file->stream);
			file->ftell_position += size;
			file->preload_start = file->file_position;
			file->preload_end = file->file_position + size;
			file->preload_ptr = 0;
//printf("quicktime_read_data 5\n");
			read_preload(file, data, size);
//printf("quicktime_read_data 6 %lld %lld %lld\n", file->file_position, file->preload_start, file->preload_end);
		}
	}

//printf("quicktime_read_data 1 %lld %lld\n", file->file_position, size);
	file->file_position += size;
//printf("quicktime_read_data 2 %lld %lld\n", file->file_position, size);
	return result;
}

int quicktime_write_data(quicktime_t *file, char *data, int size)
{
	int result;
	quicktime_fseek(file, file->file_position);
	result = fwrite(data, size, 1, file->stream);
// Defeat 0 length blocks
	if(size == 0) result = 1;

	if(result)
	{
		file->ftell_position += size;
		file->file_position += size;
		if(file->total_length < file->ftell_position) file->total_length = file->ftell_position;
//printf(__FUNCTION__ " %lld\n", file->total_length);
	}
	return result;
}

longest quicktime_byte_position(quicktime_t *file)
{
	return quicktime_position(file);
}


void quicktime_read_pascal(quicktime_t *file, char *data)
{
	char len = quicktime_read_char(file);
	quicktime_read_data(file, data, len);
	data[len] = 0;
}

void quicktime_write_pascal(quicktime_t *file, char *data)
{
	char len = strlen(data);
	quicktime_write_data(file, &len, 1);
	quicktime_write_data(file, data, len);
}

float quicktime_read_fixed32(quicktime_t *file)
{
	unsigned long a, b, c, d;
	unsigned char data[4];

	quicktime_read_data(file, data, 4);
	a = data[0];
	b = data[1];
	c = data[2];
	d = data[3];
	
	a = (a << 8) + b;
	b = (c << 8) + d;

	if(b)
		return (float)a + (float)b / 65536;
	else
		return a;
}

int quicktime_write_fixed32(quicktime_t *file, float number)
{
	unsigned char data[4];
	int a, b;

	a = number;
	b = (number - a) * 65536;
	data[0] = a >> 8;
	data[1] = a & 0xff;
	data[2] = b >> 8;
	data[3] = b & 0xff;

	return quicktime_write_data(file, data, 4);
}

int quicktime_write_int64(quicktime_t *file, longest value)
{
	unsigned char data[8];

	data[0] = (((ulongest)value) & 0xff00000000000000LL) >> 56;
	data[1] = (((ulongest)value) & 0xff000000000000LL) >> 48;
	data[2] = (((ulongest)value) & 0xff0000000000LL) >> 40;
	data[3] = (((ulongest)value) & 0xff00000000LL) >> 32;
	data[4] = (((ulongest)value) & 0xff000000LL) >> 24;
	data[5] = (((ulongest)value) & 0xff0000LL) >> 16;
	data[6] = (((ulongest)value) & 0xff00LL) >> 8;
	data[7] =  ((ulongest)value) & 0xff;

	return quicktime_write_data(file, data, 8);
}

int quicktime_write_int32(quicktime_t *file, long value)
{
	unsigned char data[4];

	data[0] = (value & 0xff000000) >> 24;
	data[1] = (value & 0xff0000) >> 16;
	data[2] = (value & 0xff00) >> 8;
	data[3] = value & 0xff;

	return quicktime_write_data(file, data, 4);
}

int quicktime_write_int32_le(quicktime_t *file, long value)
{
	unsigned char data[4];

	data[3] = (value & 0xff000000) >> 24;
	data[2] = (value & 0xff0000) >> 16;
	data[1] = (value & 0xff00) >> 8;
	data[0] = value & 0xff;

	return quicktime_write_data(file, data, 4);
}

int quicktime_write_char32(quicktime_t *file, char *string)
{
	return quicktime_write_data(file, string, 4);
}


float quicktime_read_fixed16(quicktime_t *file)
{
	unsigned char data[2];
	
	quicktime_read_data(file, data, 2);
//printf("quicktime_read_fixed16 %02x%02x\n", data[0], data[1]);
	if(data[1])
		return (float)data[0] + (float)data[1] / 256;
	else
		return (float)data[0];
}

int quicktime_write_fixed16(quicktime_t *file, float number)
{
	unsigned char data[2];
	int a, b;

	a = number;
	b = (number - a) * 256;
	data[0] = a;
	data[1] = b;

	return quicktime_write_data(file, data, 2);
}

unsigned long quicktime_read_uint32(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b, c, d;
	char data[4];

	quicktime_read_data(file, data, 4);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (a << 24) | (b << 16) | (c << 8) | d;
	return result;
}

long quicktime_read_int32(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b, c, d;
	char data[4];

	quicktime_read_data(file, data, 4);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (a << 24) | (b << 16) | (c << 8) | d;
	return (long)result;
}

long quicktime_read_int32_le(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b, c, d;
	char data[4];

	quicktime_read_data(file, data, 4);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (d << 24) | (c << 16) | (b << 8) | a;
	return (long)result;
}

longest quicktime_read_int64(quicktime_t *file)
{
	ulongest result, a, b, c, d, e, f, g, h;
	char data[8];

	quicktime_read_data(file, data, 8);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];
	e = (unsigned char)data[4];
	f = (unsigned char)data[5];
	g = (unsigned char)data[6];
	h = (unsigned char)data[7];

	result = (a << 56) | 
		(b << 48) | 
		(c << 40) | 
		(d << 32) | 
		(e << 24) | 
		(f << 16) | 
		(g << 8) | 
		h;
	return (longest)result;
}


long quicktime_read_int24(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b, c;
	char data[4];
	
	quicktime_read_data(file, data, 3);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];

	result = (a << 16) | (b << 8) | c;
	return (long)result;
}

int quicktime_write_int24(quicktime_t *file, long number)
{
	unsigned char data[3];
	data[0] = (number & 0xff0000) >> 16;
	data[1] = (number & 0xff00) >> 8;
	data[2] = (number & 0xff);
	
	return quicktime_write_data(file, data, 3);
}

int quicktime_read_int16(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b;
	char data[2];
	
	quicktime_read_data(file, data, 2);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];

	result = (a << 8) | b;
	return (int)result;
}

int quicktime_read_int16_le(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b;
	char data[2];
	
	quicktime_read_data(file, data, 2);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];

	result = (b << 8) | a;
	return (int)result;
}

int quicktime_write_int16(quicktime_t *file, int number)
{
	unsigned char data[2];
	data[0] = (number & 0xff00) >> 8;
	data[1] = (number & 0xff);
	
	return quicktime_write_data(file, data, 2);
}

int quicktime_write_int16_le(quicktime_t *file, int number)
{
	unsigned char data[2];
	data[1] = (number & 0xff00) >> 8;
	data[0] = (number & 0xff);
	
	return quicktime_write_data(file, data, 2);
}

int quicktime_read_char(quicktime_t *file)
{
	char output;
	quicktime_read_data(file, &output, 1);
	return output;
}

int quicktime_write_char(quicktime_t *file, char x)
{
	return quicktime_write_data(file, &x, 1);
}

void quicktime_read_char32(quicktime_t *file, char *string)
{
	quicktime_read_data(file, string, 4);
}

longest quicktime_position(quicktime_t *file) 
{ 
	return file->file_position; 
}

int quicktime_set_position(quicktime_t *file, longest position) 
{
//printf("quicktime_set_position %lld\n", position);
	file->file_position = position;
	return 0;
}

void quicktime_copy_char32(char *output, char *input)
{
	*output++ = *input++;
	*output++ = *input++;
	*output++ = *input++;
	*output = *input;
}


void quicktime_print_chars(char *desc, char *input, int len)
{
	int i;
	printf("%s", desc);
	for(i = 0; i < len; i++) printf("%c", input[i]);
	printf("\n");
}

unsigned long quicktime_current_time(void)
{
	time_t t;
	time (&t);
	return (t+(66*31536000)+1468800);
}

int quicktime_match_32(char *input, char *output)
{
	if(input[0] == output[0] &&
		input[1] == output[1] &&
		input[2] == output[2] &&
		input[3] == output[3])
		return 1;
	else 
		return 0;
}

int quicktime_match_24(char *input, char *output)
{
	if(input[0] == output[0] &&
		input[1] == output[1] &&
		input[2] == output[2])
		return 1;
	else 
		return 0;
}
