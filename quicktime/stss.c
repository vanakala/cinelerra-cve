#include "funcprotos.h"
#include "quicktime.h"



void quicktime_stss_init(quicktime_stss_t *stss)
{
	stss->version = 0;
	stss->flags = 0;
	stss->total_entries = 0;
	stss->entries_allocated = 2;
	stss->table = (quicktime_stss_table_t*)calloc(1, sizeof(quicktime_stss_table_t) * stss->entries_allocated);
}

void quicktime_stss_delete(quicktime_stss_t *stss)
{
	if(stss->entries_allocated) free(stss->table);
	stss->total_entries = 0;
	stss->entries_allocated = 0;
	stss->table = 0;
}

void quicktime_stss_dump(quicktime_stss_t *stss)
{
	int i;
	printf("     sync sample\n");
	printf("      version %d\n", stss->version);
	printf("      flags %d\n", stss->flags);
	printf("      total_entries %d\n", stss->total_entries);
	for(i = 0; i < stss->total_entries; i++)
	{
		printf("       sample %x\n", stss->table[i].sample);
	}
}

void quicktime_read_stss(quicktime_t *file, quicktime_stss_t *stss)
{
	int i;
	stss->version = quicktime_read_char(file);
	stss->flags = quicktime_read_int24(file);
	stss->total_entries = quicktime_read_int32(file);

	if(stss->entries_allocated < stss->total_entries)
	{
		stss->entries_allocated = stss->total_entries;
		stss->table = (quicktime_stss_table_t*)realloc(stss->table, sizeof(quicktime_stss_table_t) * stss->entries_allocated);
	}

	for(i = 0; i < stss->total_entries; i++)
	{
		stss->table[i].sample = quicktime_read_int32(file);
	}
}


void quicktime_write_stss(quicktime_t *file, quicktime_stss_t *stss)
{
	int i;
	quicktime_atom_t atom;

	if(stss->total_entries)
	{
		quicktime_atom_write_header(file, &atom, "stss");

		quicktime_write_char(file, stss->version);
		quicktime_write_int24(file, stss->flags);
		quicktime_write_int32(file, stss->total_entries);
		for(i = 0; i < stss->total_entries; i++)
		{
			quicktime_write_int32(file, stss->table[i].sample);
		}

		quicktime_atom_write_footer(file, &atom);
	}
}
