#include <ctype.h>
#include <stdio.h>
#include "funcprotos.h"
#include "quicktime.h"




static int read_type(char *data, char *type)
{
	type[0] = data[4];
	type[1] = data[5];
	type[2] = data[6];
	type[3] = data[7];

/*printf("%c%c%c%c ", type[0], type[1], type[2], type[3]); */
/* need this for quicktime_check_sig */
	if(isalpha(type[0]) && isalpha(type[1]) && isalpha(type[2]) && isalpha(type[3]))
	return 0;
	else
	return 1;
}


static unsigned long read_size(char *data)
{
	unsigned long result;
	unsigned long a, b, c, d;
	
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (a << 24) | (b << 16) | (c << 8) | d;

// extended header is size 1
//	if(result < HEADER_LENGTH) result = HEADER_LENGTH;
	return result;
}

static longest read_size64(char *data)
{
	ulongest result, a, b, c, d, e, f, g, h;

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

	if(result < HEADER_LENGTH) result = HEADER_LENGTH;
	return (longest)result;
}

static int reset(quicktime_atom_t *atom)
{
	atom->end = 0;
	atom->type[0] = atom->type[1] = atom->type[2] = atom->type[3] = 0;
	return 0;
}

int quicktime_atom_read_header(quicktime_t *file, quicktime_atom_t *atom)
{
	int result = 0;
	char header[10];

	if(file->use_avi)
	{
		reset(atom);
		atom->start = quicktime_position(file);
		if(!quicktime_read_data(file, header, HEADER_LENGTH)) return 1;
		atom->type[0] = header[0];
		atom->type[1] = header[1];
		atom->type[2] = header[2];
		atom->type[3] = header[3];
		atom->size = 
			(((unsigned char)header[4])      ) |
			(((unsigned char)header[5]) << 8 ) |
			(((unsigned char)header[6]) << 16) |
			(((unsigned char)header[7]) << 24);
		atom->end = quicktime_add3(atom->start, atom->size, 8);
	}
	else
	{
		longest size2;

		reset(atom);

		atom->start = quicktime_position(file);

		if(!quicktime_read_data(file, header, HEADER_LENGTH)) return 1;
		result = read_type(header, atom->type);
		atom->size = read_size(header);
		atom->end = atom->start + atom->size;
/*
 * printf("quicktime_atom_read_header 1 %c%c%c%c start %llx size %llx end %llx ftell %llx %llx\n", 
 * 	atom->type[0], atom->type[1], atom->type[2], atom->type[3],
 * 	atom->start, atom->size, atom->end,
 * 	file->file_position,
 * 	(longest)FTELL(file->stream));
 */

/* Skip placeholder atom */
		if(quicktime_match_32(atom->type, "wide"))
		{
			atom->start = quicktime_position(file);
			reset(atom);
			if(!quicktime_read_data(file, header, HEADER_LENGTH)) return 1;
			result = read_type(header, atom->type);
			atom->size -= 8;
			if(atom->size <= 0)
			{
/* Wrapper ended.  Get new atom size */
				atom->size = read_size(header);
			}
			atom->end = atom->start + atom->size;
		}
		else
/* Get extended size */
		if(atom->size == 1)
		{
			if(!quicktime_read_data(file, header, HEADER_LENGTH)) return 1;
			atom->size = read_size64(header);
			atom->end = atom->start + atom->size;
/*
 * printf("quicktime_atom_read_header 2 %c%c%c%c start %llx size %llx end %llx ftell %llx\n", 
 * 	atom->type[0], atom->type[1], atom->type[2], atom->type[3],
 * 	atom->start, atom->size, atom->end,
 * 	file->file_position);
 */
		}
	}


	return result;
}

int quicktime_atom_write_header64(quicktime_t *file, quicktime_atom_t *atom, char *text)
{
	int result = 0;
	atom->start = quicktime_position(file);

	result = !quicktime_write_int32(file, 1);
	if(!result) result = !quicktime_write_char32(file, text);
	if(!result) result = !quicktime_write_int64(file, 0);

	atom->use_64 = 1;
	return result;
}

int quicktime_atom_write_header(quicktime_t *file, 
	quicktime_atom_t *atom, 
	char *text)
{
	int result = 0;
	if(file->use_avi)
	{
		reset(atom);
		atom->start = quicktime_position(file) + 8;
		result = !quicktime_write_char32(file, text);
		if(!result) result = !quicktime_write_int32_le(file, 0);
		atom->use_64 = 0;
	}
	else
	{
		atom->start = quicktime_position(file);
		result = !quicktime_write_int32(file, 0);
		if(!result) result = !quicktime_write_char32(file, text);
		atom->use_64 = 0;
	}
	return result;
}

void quicktime_atom_write_footer(quicktime_t *file, quicktime_atom_t *atom)
{
	atom->end = quicktime_position(file);
	if(file->use_avi)
	{
		quicktime_set_position(file, atom->start - 4);
		quicktime_write_int32_le(file, atom->end - atom->start);
	}
	else
	{
		if(atom->use_64)
		{
			quicktime_set_position(file, atom->start + 8);
//printf("quicktime_atom_write_footer %llx %llx %llx %llx\n", file->total_length, file->file_position, atom->start, atom->end);
			quicktime_write_int64(file, atom->end - atom->start);
		}
		else
		{
			quicktime_set_position(file, atom->start);
			quicktime_write_int32(file, atom->end - atom->start);
		}
	}

	quicktime_set_position(file, atom->end);
}

int quicktime_atom_is(quicktime_atom_t *atom, char *type)
{
	if(atom->type[0] == type[0] &&
		atom->type[1] == type[1] &&
		atom->type[2] == type[2] &&
		atom->type[3] == type[3])
	return 1;
	else
	return 0;
}

int quicktime_atom_skip(quicktime_t *file, quicktime_atom_t *atom)
{
	if(atom->start == atom->end) atom->end++;
	return quicktime_set_position(file, atom->end);
}
