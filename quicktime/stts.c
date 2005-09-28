#include "funcprotos.h"
#include "quicktime.h"



void quicktime_stts_init(quicktime_stts_t *stts)
{
	stts->version = 0;
	stts->flags = 0;
	stts->total_entries = 0;
}

void quicktime_stts_init_table(quicktime_stts_t *stts)
{
	if(!stts->total_entries)
	{
		stts->total_entries = 1;
		stts->table = (quicktime_stts_table_t*)malloc(sizeof(quicktime_stts_table_t) * stts->total_entries);
	}
}

void quicktime_stts_init_video(quicktime_t *file, quicktime_stts_t *stts, int time_scale, float frame_rate)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;      /* need to set this when closing */
	table->sample_duration = time_scale / frame_rate;
//printf("quicktime_stts_init_video %d %f\n", time_scale, frame_rate);
}

void quicktime_stts_init_audio(quicktime_t *file, quicktime_stts_t *stts, int sample_rate)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;     /* need to set this when closing */
	table->sample_duration = 1;
}

void quicktime_stts_append_audio(quicktime_t *file, 
	quicktime_stts_t *stts, 
	int sample_duration)
{
	quicktime_stts_table_t *table;
	if(stts->total_entries)
		table = &(stts->table[stts->total_entries - 1]);
	else
		table = 0;

	stts->is_vbr = 1;

// Expand existing entry
	if(table && table->sample_count)
	{
		if(table->sample_duration == sample_duration)
		{
			table->sample_count++;
			return;
		}
	}
	else
// Override existing entry
	if(table && table->sample_count == 0)
	{
		table->sample_duration = sample_duration;
		table->sample_count = 1;
		return;
	}

// Append new entry
	stts->total_entries++;
	stts->table = realloc(stts->table, 
		sizeof(quicktime_stts_table_t) * stts->total_entries);
	table = &(stts->table[stts->total_entries - 1]);
	table->sample_duration = sample_duration;
	table->sample_count++;
}


int64_t quicktime_stts_total_samples(quicktime_t *file, 
	quicktime_stts_t *stts)
{
	int i;
	int64_t result = 0;
	for(i = 0; i < stts->total_entries; i++)
	{
		result += stts->table[i].sample_count;
	}
	return result;
}


void quicktime_stts_delete(quicktime_stts_t *stts)
{
	if(stts->total_entries) free(stts->table);
	stts->total_entries = 0;
}

void quicktime_stts_dump(quicktime_stts_t *stts)
{
	int i;
	printf("     time to sample\n");
	printf("      version %d\n", stts->version);
	printf("      flags %d\n", stts->flags);
	printf("      total_entries %d\n", stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		printf("       count %ld duration %ld\n", stts->table[i].sample_count, stts->table[i].sample_duration);
	}
}

void quicktime_read_stts(quicktime_t *file, quicktime_stts_t *stts)
{
	int i;
	stts->version = quicktime_read_char(file);
	stts->flags = quicktime_read_int24(file);
	stts->total_entries = quicktime_read_int32(file);

	stts->table = (quicktime_stts_table_t*)malloc(sizeof(quicktime_stts_table_t) * stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		stts->table[i].sample_count = quicktime_read_int32(file);
		stts->table[i].sample_duration = quicktime_read_int32(file);
	}
}

void quicktime_write_stts(quicktime_t *file, quicktime_stts_t *stts)
{
	int i;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "stts");

	quicktime_write_char(file, stts->version);
	quicktime_write_int24(file, stts->flags);
	quicktime_write_int32(file, stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		quicktime_write_int32(file, stts->table[i].sample_count);
		quicktime_write_int32(file, stts->table[i].sample_duration);
	}

	quicktime_atom_write_footer(file, &atom);
}

// Return the sample which contains the start_time argument.
// Stores the actual starting time of the sample in the start_time argument.
int quicktime_time_to_sample(quicktime_stts_t *stts,
	int64_t *start_time)
{
	int result = 0;
	int current_entry = 0;
	int64_t current_start_time = 0;
	while(current_entry < stts->total_entries)
	{
		quicktime_stts_table_t *stts_table = &stts->table[current_entry];
		int sample_count = stts_table->sample_count;
		while(sample_count > 0)
		{
			current_start_time += stts_table->sample_duration;
// Current sample contains start time
			if(current_start_time > *start_time)
			{
				current_start_time -= stts_table->sample_duration;
				*start_time = current_start_time;
				return result;
			}

// Next sample
			sample_count--;
			result++;
		}
		current_entry++;
	}

	return result > 0 ? result - 1 : result;
}

