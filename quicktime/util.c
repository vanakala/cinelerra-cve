#include <fcntl.h>
#include <linux/cdrom.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"

/* Disk I/O */

int64_t quicktime_get_file_length(char *path)
{
	struct stat64 status;
	if(stat64(path, &status))
		perror("quicktime_get_file_length stat64:");
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
		file->total_length = quicktime_get_file_length(path);		
	}

	file->presave_buffer = calloc(1, QUICKTIME_PRESAVE);
	return 0;
}

int quicktime_file_close(quicktime_t *file)
{
/* Flush presave buffer */
	if(file->presave_size)
	{
		quicktime_fseek(file, file->presave_position - file->presave_size);
		fwrite(file->presave_buffer, 1, file->presave_size, file->stream);
		file->presave_size = 0;
	}

	if(file->stream)
	{
		fclose(file->stream);
	}
	file->stream = 0;
	return 0;
}



int64_t quicktime_ftell(quicktime_t *file)
{
	return file->ftell_position;
}

int quicktime_fseek(quicktime_t *file, int64_t offset)
{
	file->ftell_position = offset;
	if(offset > file->total_length || offset < 0) return 1;
	if(FSEEK(file->stream, file->ftell_position, SEEK_SET))
	{
//		perror("quicktime_fseek FSEEK");
		return 1;
	}
	return 0;
}

/* Read entire buffer from the preload buffer */
static int read_preload(quicktime_t *file, char *data, int64_t size)
{
	int64_t selection_start = 0;
	int64_t selection_end = 0;
	int64_t fragment_start = 0;
	int64_t fragment_len = 0;

	selection_start = file->file_position;
	selection_end = quicktime_add(file->file_position, size);

	fragment_start = file->preload_ptr + (selection_start - file->preload_start);
	while(fragment_start < 0) fragment_start += file->preload_size;
	while(fragment_start >= file->preload_size) fragment_start -= file->preload_size;

	while(selection_start < selection_end)
	{
		fragment_len = selection_end - selection_start;
		if(fragment_start + fragment_len > file->preload_size)
			fragment_len = file->preload_size - fragment_start;

		memcpy(data, file->preload_buffer + fragment_start, fragment_len);
		fragment_start += fragment_len;
		data += fragment_len;

		if(fragment_start >= file->preload_size) fragment_start = (int64_t)0;
		selection_start += fragment_len;
	}
	return 0;
}

int quicktime_read_data(quicktime_t *file, char *data, int64_t size)
{
	int result = 1;

	if(!file->preload_size)
	{
		quicktime_fseek(file, file->file_position);
		result = fread(data, size, 1, file->stream);
		file->ftell_position += size;
	}
	else
	{
/* Region requested for loading */
		int64_t selection_start = file->file_position;
		int64_t selection_end = file->file_position + size;
		int64_t fragment_start, fragment_len;

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
			read_preload(file, data, size);
		}
		else
		if(selection_end > file->preload_end && 
			selection_end - file->preload_size < file->preload_end)
		{
/* Range is after buffer */
/* Move the preload start to within one preload length of the selection_end */
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
/* Range is before buffer or over a preload_size away from the end of the buffer. */
/* Replace entire preload buffer with range. */
			quicktime_fseek(file, file->file_position);
			result = fread(file->preload_buffer, size, 1, file->stream);
			file->ftell_position += size;
			file->preload_start = file->file_position;
			file->preload_end = file->file_position + size;
			file->preload_ptr = 0;
			read_preload(file, data, size);
		}
	}

	file->file_position += size;
	return result;
}

int quicktime_write_data(quicktime_t *file, char *data, int size)
{
	int data_offset = 0;
	int writes_attempted = 0;
	int writes_succeeded = 0;
	int iterations = 0;
//printf("quicktime_write_data 1 %d\n", size);

// Flush existing buffer and seek to new position
	if(file->file_position != file->presave_position)
	{
		if(file->presave_size)
		{
			quicktime_fseek(file, file->presave_position - file->presave_size);
			writes_succeeded += fwrite(file->presave_buffer, 1, file->presave_size, file->stream);
			writes_attempted += file->presave_size;
			file->presave_size = 0;
		}
		file->presave_position = file->file_position;
	}

// Write presave buffers until done
	while(size > 0)
	{
		int fragment_size = QUICKTIME_PRESAVE;
		if(fragment_size > size) fragment_size = size;
		if(fragment_size + file->presave_size > QUICKTIME_PRESAVE)
			fragment_size = QUICKTIME_PRESAVE- file->presave_size;

		memcpy(file->presave_buffer + file->presave_size, 
			data + data_offset,
			fragment_size);

		file->presave_position += fragment_size;
		file->presave_size += fragment_size;
		data_offset += fragment_size;
		size -= fragment_size;

		if(file->presave_size >= QUICKTIME_PRESAVE)
		{
//if(++iterations > 1) printf("quicktime_write_data 2 iterations=%d\n", iterations);
			quicktime_fseek(file, file->presave_position - file->presave_size);
			writes_succeeded += fwrite(file->presave_buffer, 1, file->presave_size, file->stream);
			writes_attempted += file->presave_size;
			file->presave_size = 0;
		}
	}

/* Adjust file position */
	file->file_position = file->presave_position;
/* Adjust ftell position */
	file->ftell_position = file->presave_position;
/* Adjust total length */
	if(file->total_length < file->ftell_position) file->total_length = file->ftell_position;

/* fwrite failed */
	if(!writes_succeeded && writes_attempted)
	{
		return 0;
	}
	else
	if(!size)
		return 1;
	else
		return size;
}

int64_t quicktime_byte_position(quicktime_t *file)
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

int quicktime_write_int64(quicktime_t *file, int64_t value)
{
	unsigned char data[8];

	data[0] = (((uint64_t)value) & 0xff00000000000000LL) >> 56;
	data[1] = (((uint64_t)value) & 0xff000000000000LL) >> 48;
	data[2] = (((uint64_t)value) & 0xff0000000000LL) >> 40;
	data[3] = (((uint64_t)value) & 0xff00000000LL) >> 32;
	data[4] = (((uint64_t)value) & 0xff000000LL) >> 24;
	data[5] = (((uint64_t)value) & 0xff0000LL) >> 16;
	data[6] = (((uint64_t)value) & 0xff00LL) >> 8;
	data[7] =  ((uint64_t)value) & 0xff;

	return quicktime_write_data(file, data, 8);
}

int quicktime_write_int64_le(quicktime_t *file, int64_t value)
{
	unsigned char data[8];

	data[7] = (((uint64_t)value) & 0xff00000000000000LL) >> 56;
	data[6] = (((uint64_t)value) & 0xff000000000000LL) >> 48;
	data[5] = (((uint64_t)value) & 0xff0000000000LL) >> 40;
	data[4] = (((uint64_t)value) & 0xff00000000LL) >> 32;
	data[3] = (((uint64_t)value) & 0xff000000LL) >> 24;
	data[2] = (((uint64_t)value) & 0xff0000LL) >> 16;
	data[1] = (((uint64_t)value) & 0xff00LL) >> 8;
	data[0] =  ((uint64_t)value) & 0xff;

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

int64_t quicktime_read_int64(quicktime_t *file)
{
	uint64_t result, a, b, c, d, e, f, g, h;
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
	return (int64_t)result;
}

int64_t quicktime_read_int64_le(quicktime_t *file)
{
	uint64_t result, a, b, c, d, e, f, g, h;
	char data[8];

	quicktime_read_data(file, data, 8);
	a = (unsigned char)data[7];
	b = (unsigned char)data[6];
	c = (unsigned char)data[5];
	d = (unsigned char)data[4];
	e = (unsigned char)data[3];
	f = (unsigned char)data[2];
	g = (unsigned char)data[1];
	h = (unsigned char)data[0];

	result = (a << 56) | 
		(b << 48) | 
		(c << 40) | 
		(d << 32) | 
		(e << 24) | 
		(f << 16) | 
		(g << 8) | 
		h;
	return (int64_t)result;
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

int64_t quicktime_position(quicktime_t *file) 
{ 
	return file->file_position; 
}

int quicktime_set_position(quicktime_t *file, int64_t position) 
{
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
