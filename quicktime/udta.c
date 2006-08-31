#include "funcprotos.h"
#include "quicktime.h"
#include <string.h>

#define DEFAULT_INFO "Made with Quicktime for Linux"
static unsigned char cpy_tag[] = {0xa9, 'c', 'p', 'y'};
static unsigned char nam_tag[] = {0xa9, 'n', 'a', 'm'};
static unsigned char inf_tag[] = {0xa9, 'i', 'n', 'f'};
static unsigned char req_tag[] = {0xa9, 'r', 'e', 'q'};
static unsigned char enc_tag[] = {0xa9, 'e', 'n', 'c'};

int quicktime_udta_init(quicktime_udta_t *udta)
{
	udta->copyright = 0;
	udta->copyright_len = 0;
	udta->name = 0;
	udta->name_len = 0;
	udta->require = 0;
	udta->require_len = 0;
	udta->encoder = 0;
	udta->encoder_len = 0;

	udta->info = malloc(strlen(DEFAULT_INFO) + 1);
	udta->info_len = strlen(DEFAULT_INFO);
	sprintf(udta->info, DEFAULT_INFO);
	return 0;
}

int quicktime_udta_delete(quicktime_udta_t *udta)
{
	if(udta->copyright_len)
	{
		free(udta->copyright);
	}
	if(udta->name_len)
	{
		free(udta->name);
	}
	if(udta->info_len)
	{
		free(udta->info);
	}
	if(udta->require_len)
	{
		free(udta->require);
	}
	if(udta->encoder_len)
	{
		free(udta->encoder);
	}
//	quicktime_udta_init(udta);
	return 0;
}

void quicktime_udta_dump(quicktime_udta_t *udta)
{
	printf(" user data (udta)\n");
	if(udta->copyright_len) printf("  copyright -> %s\n", udta->copyright);
	if(udta->name_len) printf("  name -> %s\n", udta->name);
	if(udta->info_len) printf("  info -> %s\n", udta->info);
	if(udta->require_len) printf("  require -> %s\n", udta->require);
	if(udta->encoder_len) printf("  encoder -> %s\n", udta->encoder);
}

int quicktime_read_udta(quicktime_t *file, quicktime_udta_t *udta, quicktime_atom_t *udta_atom)
{
	quicktime_atom_t leaf_atom;
	int result = 0;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);


		if(quicktime_atom_is(&leaf_atom, cpy_tag))
		{
			result += quicktime_read_udta_string(file, &(udta->copyright), &(udta->copyright_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, nam_tag))
		{
			result += quicktime_read_udta_string(file, &(udta->name), &(udta->name_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, inf_tag))
		{
			result += quicktime_read_udta_string(file, &(udta->info), &(udta->info_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, req_tag))
		{
			result += quicktime_read_udta_string(file, &(udta->require), &(udta->require_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, enc_tag))
		{
			result += quicktime_read_udta_string(file, &(udta->encoder), &(udta->encoder_len));
		}
		else
		{
			quicktime_atom_skip(file, &leaf_atom);
		}
	}while(quicktime_position(file) < udta_atom->end);


	return result;
}

void quicktime_write_udta(quicktime_t *file, quicktime_udta_t *udta)
{
	quicktime_atom_t atom, subatom;
	quicktime_atom_write_header(file, &atom, "udta");

	if(udta->copyright_len)
	{
		quicktime_atom_write_header(file, &subatom, cpy_tag);
		quicktime_write_udta_string(file, udta->copyright, udta->copyright_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->name_len)
	{
		quicktime_atom_write_header(file, &subatom, nam_tag);
		quicktime_write_udta_string(file, udta->name, udta->name_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->info_len)
	{
		quicktime_atom_write_header(file, &subatom, inf_tag);
		quicktime_write_udta_string(file, udta->info, udta->info_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->require_len)
	{
		quicktime_atom_write_header(file, &subatom, req_tag);
		quicktime_write_udta_string(file, udta->require, udta->require_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->encoder_len)
	{
		quicktime_atom_write_header(file, &subatom, enc_tag);
		quicktime_write_udta_string(file, udta->encoder, udta->encoder_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_read_udta_string(quicktime_t *file, char **string, int *size)
{
	int result;

	if(*size) free(*string);
	*size = quicktime_read_int16(file);  /* Size of string */
	quicktime_read_int16(file);  /* Discard language code */
	*string = malloc(*size + 1);
	result = quicktime_read_data(file, *string, *size);
	(*string)[*size] = 0;
	return !result;
}

int quicktime_write_udta_string(quicktime_t *file, char *string, int size)
{
	int new_size = strlen(string);
	int result;

	quicktime_write_int16(file, new_size);    /* String size */
	quicktime_write_int16(file, 0);    /* Language code */
	result = quicktime_write_data(file, string, new_size);
	return !result;
}

int quicktime_set_udta_string(char **string, int *size, char *new_string)
{
	if(*size) free(*string);
	*size = strlen(new_string) + 1;
	*string = malloc(*size);
	strcpy(*string, new_string);
	return 0;
}
