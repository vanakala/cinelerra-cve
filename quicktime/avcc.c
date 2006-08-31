#include "funcprotos.h"
#include "quicktime.h"
#include <string.h>


void quicktime_delete_avcc(quicktime_avcc_t *avcc)
{
	if(avcc->data) free(avcc->data);
}

// Set esds header to a copy of the argument
void quicktime_set_avcc_header(quicktime_avcc_t *avcc,
	unsigned char *data, 
	int size)
{
	if(avcc->data)
	{
		free(avcc->data);
	}

	avcc->data = calloc(1, size);
	memcpy(avcc->data, data, size);
	avcc->data_size = size;
}

void quicktime_write_avcc(quicktime_t *file, 
	quicktime_avcc_t *avcc)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "avcC");
	quicktime_write_data(file, avcc->data, avcc->data_size);
	quicktime_atom_write_footer(file, &atom);
}



int quicktime_read_avcc(quicktime_t *file, 
	quicktime_atom_t *parent_atom,
	quicktime_avcc_t *avcc)
{
	avcc->data_size = parent_atom->size - 8;
	avcc->data = calloc(1, avcc->data_size + 1024);
	quicktime_read_data(file, 
		avcc->data, 
		avcc->data_size);
	quicktime_atom_skip(file, parent_atom);
	return 0;
}

void quicktime_avcc_dump(quicktime_avcc_t *avcc)
{
	int i;
	printf("       h264 description\n");
	printf("        data_size=0x%x\n", avcc->data_size);
	printf("        data=");
	for(i = 0; i < avcc->data_size; i++)
	{
		printf("0x%02x ", (unsigned char)avcc->data[i]);
	}
	printf("\n");
}
