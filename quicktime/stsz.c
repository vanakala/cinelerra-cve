#include "funcprotos.h"
#include "quicktime.h"



void quicktime_stsz_init(quicktime_stsz_t *stsz)
{
	stsz->version = 0;
	stsz->flags = 0;
	stsz->sample_size = 0;
	stsz->total_entries = 0;
	stsz->entries_allocated = 0;
	stsz->table = 0;
}

void quicktime_stsz_init_video(quicktime_t *file, quicktime_stsz_t *stsz)
{
	stsz->sample_size = 0;
	if(!stsz->entries_allocated)
	{
		stsz->entries_allocated = 2048;
		stsz->total_entries = 0;
		stsz->table = (quicktime_stsz_table_t*)calloc(sizeof(quicktime_stsz_table_t), 
			stsz->entries_allocated);
//printf("quicktime_stsz_init_video 1 %p\n", stsz->table);
	}
}

void quicktime_stsz_init_audio(quicktime_t *file, 
	quicktime_stsz_t *stsz, 
	int channels, 
	int bits,
	char *compressor)
{
	/*stsz->sample_size = channels * bits / 8; */

	stsz->table = 0;
	stsz->sample_size = 0;

//printf("quicktime_stsz_init_audio 1 %d\n", stsz->sample_size);
	stsz->total_entries = 0;   /* set this when closing */
	stsz->entries_allocated = 0;
}

void quicktime_stsz_delete(quicktime_stsz_t *stsz)
{
	if(!stsz->sample_size && stsz->total_entries) free(stsz->table);
	stsz->table = 0;
	stsz->total_entries = 0;
	stsz->entries_allocated = 0;
}

void quicktime_stsz_dump(quicktime_stsz_t *stsz)
{
	int i;
	printf("     sample size\n");
	printf("      version %d\n", stsz->version);
	printf("      flags %d\n", stsz->flags);
	printf("      sample_size %d\n", stsz->sample_size);
	printf("      total_entries %d\n", stsz->total_entries);
	
	if(!stsz->sample_size)
	{
		for(i = 0; i < stsz->total_entries; i++)
		{
			printf("       sample_size %x\n", stsz->table[i].size);
		}
	}
}

void quicktime_read_stsz(quicktime_t *file, quicktime_stsz_t *stsz)
{
	int i;
	stsz->version = quicktime_read_char(file);
	stsz->flags = quicktime_read_int24(file);
	stsz->sample_size = quicktime_read_int32(file);
	stsz->total_entries = quicktime_read_int32(file);
	stsz->entries_allocated = stsz->total_entries;
//printf("quicktime_read_stsz 1 %d\n", stsz->sample_size);
	if(!stsz->sample_size)
	{
		stsz->table = (quicktime_stsz_table_t*)malloc(sizeof(quicktime_stsz_table_t) * stsz->entries_allocated);
		for(i = 0; i < stsz->total_entries; i++)
		{
			stsz->table[i].size = quicktime_read_int32(file);
		}
	}
}

void quicktime_write_stsz(quicktime_t *file, quicktime_stsz_t *stsz)
{
	int i, result;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "stsz");

/* optimize if possible */
/* Xanim requires an unoptimized table for video. */
/* 	if(!stsz->sample_size) */
/* 	{ */
/* 		for(i = 0, result = 0; i < stsz->total_entries && !result; i++) */
/* 		{ */
/* 			if(stsz->table[i].size != stsz->table[0].size) result = 1; */
/* 		} */
/* 		 */
/* 		if(!result) */
/* 		{ */
/* 			stsz->sample_size = stsz->table[0].size; */
/* 			stsz->total_entries = 0; */
/* 			free(stsz->table); */
/* 		} */
/* 	} */

	quicktime_write_char(file, stsz->version);
	quicktime_write_int24(file, stsz->flags);

// Force idiosynchratic handling of fixed bitrate audio.
// Since audio has millions of samples it's not practical to declare a size
// of each sample.  Instead Quicktime stores a 1 for every sample's size and
// relies on the samples per chunk table to determine the chunk size.
	quicktime_write_int32(file, stsz->sample_size);
	quicktime_write_int32(file, stsz->total_entries);

	if(!stsz->sample_size)
	{
		for(i = 0; i < stsz->total_entries; i++)
		{
			quicktime_write_int32(file, stsz->table[i].size);
		}
	}

	quicktime_atom_write_footer(file, &atom);
}

// Sample starts on 0
void quicktime_update_stsz(quicktime_stsz_t *stsz, 
	long sample, 
	long sample_size)
{
	int i;

	if(!stsz->sample_size)
	{
		if(sample >= stsz->entries_allocated)
		{
			stsz->entries_allocated = sample * 2;
//printf("quicktime_update_stsz 1 %d %d\n", sample, sample_size);
			stsz->table = (quicktime_stsz_table_t *)realloc(stsz->table,
				sizeof(quicktime_stsz_table_t) * stsz->entries_allocated);
//printf("quicktime_update_stsz 2 %d %d\n", sample, sample_size);
		}

		stsz->table[sample].size = sample_size;
		if(sample >= stsz->total_entries) stsz->total_entries = sample + 1;
	}

//printf("quicktime_update_stsz 5 %d %d\n", sample, sample_size);
}


int quicktime_sample_size(quicktime_trak_t *trak, int sample)
{
	quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
	if(stsz->sample_size) return stsz->sample_size;
	if(sample < stsz->total_entries && sample >= 0)
		return stsz->table[sample].size;
	return 0;
	
}
