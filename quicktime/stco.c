#include "funcprotos.h"
#include "quicktime.h"



void quicktime_stco_init(quicktime_stco_t *stco)
{
	stco->version = 0;
	stco->flags = 0;
	stco->total_entries = 0;
	stco->entries_allocated = 0;
}

void quicktime_stco_delete(quicktime_stco_t *stco)
{
	if(stco->total_entries) free(stco->table);
	stco->total_entries = 0;
	stco->entries_allocated = 0;
}

void quicktime_stco_init_common(quicktime_t *file, quicktime_stco_t *stco)
{
	if(!stco->entries_allocated)
	{
		stco->entries_allocated = 2048;
		stco->total_entries = 0;
		stco->table = (quicktime_stco_table_t*)malloc(sizeof(quicktime_stco_table_t) * stco->entries_allocated);
/*printf("quicktime_stco_init_common %x\n", stco->table); */
	}
}

void quicktime_stco_dump(quicktime_stco_t *stco)
{
	int i;
	printf("     chunk offset\n");
	printf("      version %d\n", stco->version);
	printf("      flags %d\n", stco->flags);
	printf("      total_entries %d\n", stco->total_entries);
	for(i = 0; i < stco->total_entries; i++)
	{
		printf("       offset %d %llx\n", i, stco->table[i].offset);
	}
}

void quicktime_read_stco(quicktime_t *file, quicktime_stco_t *stco)
{
	int i;
	stco->version = quicktime_read_char(file);
	stco->flags = quicktime_read_int24(file);
	stco->total_entries = quicktime_read_int32(file);
	stco->entries_allocated = stco->total_entries;
	stco->table = (quicktime_stco_table_t*)calloc(1, sizeof(quicktime_stco_table_t) * stco->entries_allocated);
	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset = quicktime_read_uint32(file);
	}
}

void quicktime_read_stco64(quicktime_t *file, quicktime_stco_t *stco)
{
	int i;
	stco->version = quicktime_read_char(file);
	stco->flags = quicktime_read_int24(file);
	stco->total_entries = quicktime_read_int32(file);
	stco->entries_allocated = stco->total_entries;
	stco->table = (quicktime_stco_table_t*)calloc(1, sizeof(quicktime_stco_table_t) * stco->entries_allocated);
	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset = quicktime_read_int64(file);
	}
}

void quicktime_write_stco(quicktime_t *file, quicktime_stco_t *stco)
{
	int i;
	quicktime_atom_t atom;
//	quicktime_atom_write_header(file, &atom, "stco");
	quicktime_atom_write_header(file, &atom, "co64");

	quicktime_write_char(file, stco->version);
	quicktime_write_int24(file, stco->flags);
	quicktime_write_int32(file, stco->total_entries);
	for(i = 0; i < stco->total_entries; i++)
	{
		quicktime_write_int64(file, stco->table[i].offset);
	}

	quicktime_atom_write_footer(file, &atom);
}

// Chunk starts at 1
void quicktime_update_stco(quicktime_stco_t *stco, long chunk, int64_t offset)
{
	long i;
	if(chunk <= 0)
		printf("quicktime_update_stco chunk must start at 1. chunk=%d\n",
			chunk);

	if(chunk > stco->entries_allocated)
	{
		stco->entries_allocated = chunk * 2;
//printf("quicktime_update_stco 1\n");
		stco->table = (quicktime_stco_table_t*)realloc(stco->table, 
			sizeof(quicktime_stco_table_t) * stco->entries_allocated);
//printf("quicktime_update_stco 2\n");
	}
	
	stco->table[chunk - 1].offset = offset;
	if(chunk > stco->total_entries) stco->total_entries = chunk;
}

