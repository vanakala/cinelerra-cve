#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <stdlib.h>
#include <string.h>


mpeg3_strack_t* mpeg3_new_strack(int id)
{
	mpeg3_strack_t *result = calloc(sizeof(mpeg3_strack_t), 1);
	result->id = id;
	return result;
}

void mpeg3_delete_strack(mpeg3_strack_t *ptr)
{
	int i;
	for(i = 0; i < ptr->total_subtitles; i++)
	{
		mpeg3_delete_subtitle(ptr->subtitles[i]);
	}
	if(ptr->subtitles) free(ptr->subtitles);
	if(ptr->offsets) free(ptr->offsets);
	free(ptr);
}

void mpeg3_delete_subtitle(mpeg3_subtitle_t *subtitle)
{
	if(subtitle->data) free(subtitle->data);
	if(subtitle->image_y) free(subtitle->image_y);
	if(subtitle->image_u) free(subtitle->image_u);
	if(subtitle->image_v) free(subtitle->image_v);
	if(subtitle->image_a) free(subtitle->image_a);
	free(subtitle);
}


void mpeg3_copy_strack(mpeg3_strack_t *dst, mpeg3_strack_t *src)
{
	dst->id = src->id;
	dst->offsets = calloc(sizeof(int64_t) * src->allocated_offsets, 1);
	memcpy(dst->offsets, src->offsets, sizeof(int64_t) * src->total_offsets);
	dst->total_offsets = src->total_offsets;
	dst->allocated_offsets = src->allocated_offsets;
}

mpeg3_strack_t* mpeg3_get_strack_id(mpeg3_t *file, int id)
{
	int i;
	for(i = 0; i < file->total_sstreams; i++)
	{
		if(file->strack[i]->id == id) return file->strack[i];
	}
	return 0;
}

mpeg3_strack_t* mpeg3_get_strack(mpeg3_t *file, int number)
{
	int i;
	if(number >= file->total_sstreams || number < 0) return 0;
	return file->strack[number];
}

mpeg3_strack_t* mpeg3_create_strack(mpeg3_t *file, int id)
{
	int i;
	int j;
	mpeg3_strack_t *result = 0;

	if(!(result = mpeg3_get_strack_id(file, id)))
	{
		result = mpeg3_new_strack(id);
		for(i = 0; i < file->total_sstreams; i++)
		{
/* Search for first ID > current id */
			if(file->strack[i]->id > id)
			{
/* Shift back 1 */
				for(j = file->total_sstreams; j >= i; j--)
				{
					file->strack[j] = file->strack[j - 1];
				}
				break;
			}
		}

/* Store in table */
		file->strack[i] = result;
		file->total_sstreams++;
	}

	return result;
}

void mpeg3_append_subtitle_offset(mpeg3_strack_t *dst, int64_t program_offset)
{
	int new_total = dst->total_offsets + 1;
	if(new_total >= dst->allocated_offsets)
	{
		int new_allocated = MAX(new_total, dst->allocated_offsets * 2);
		int64_t *new_offsets = malloc(sizeof(int64_t) * new_allocated);

		if(dst->offsets)
		{
			memcpy(new_offsets, dst->offsets, dst->total_offsets * sizeof(int64_t));
			free(dst->offsets);
		}

		dst->offsets = new_offsets;
		dst->allocated_offsets = new_allocated;
	}

	dst->offsets[dst->total_offsets++] = program_offset;
}

void mpeg3_append_subtitle(mpeg3_strack_t *strack, mpeg3_subtitle_t *subtitle)
{
	int new_total = strack->total_subtitles + 1;
	if(new_total >= strack->allocated_subtitles)
	{
		int new_allocated = MAX(new_total, strack->allocated_subtitles * 2);
		mpeg3_subtitle_t **new_subtitles = malloc(sizeof(mpeg3_subtitle_t*) * new_allocated);


		if(strack->subtitles)
		{
			memcpy(new_subtitles, 
				strack->subtitles, 
				strack->total_subtitles * sizeof(mpeg3_subtitle_t*));
			free(strack->subtitles);
		}
		
		strack->subtitles = new_subtitles;
		strack->allocated_subtitles = new_allocated;
	}

	strack->subtitles[strack->total_subtitles++] = subtitle;


	while(strack->total_subtitles > MPEG3_MAX_SUBTITLES)
		mpeg3_pop_subtitle(strack, 0, 1);
}

void mpeg3_pop_subtitle(mpeg3_strack_t *strack, int number, int delete_it)
{
	int i;
	if(strack->total_subtitles)
	{
		if(delete_it) mpeg3_delete_subtitle(strack->subtitles[number]);
		for(i = number; i < strack->total_subtitles - 1; i++)
			strack->subtitles[i] = strack->subtitles[i + 1];
		strack->total_subtitles--;
	}
}


void mpeg3_pop_all_subtitles(mpeg3_strack_t *strack)
{
	int i;
	for(i = 0; i < strack->total_subtitles; i++)
	{
		mpeg3_delete_subtitle(strack->subtitles[i]);
	}
	strack->total_subtitles = 0;
}


mpeg3_subtitle_t* mpeg3_get_subtitle(mpeg3_strack_t *strack)
{
	int i;
	for(i = 0; i < strack->total_subtitles; i++)
	{
		if(!strack->subtitles[i]->active)
			return strack->subtitles[i];
	}
	return 0;
}

int mpeg3_subtitle_tracks(mpeg3_t *file)
{
	return file->total_sstreams;
}

void mpeg3_show_subtitle(mpeg3_t *file, int track)
{
	file->subtitle_track = track;
}

void mpeg3_reset_subtitles(mpeg3_t *file)
{
	int i;
	for(i = 0; i < file->total_sstreams; i++)
	{
		mpeg3_pop_all_subtitles(file->strack[i]);
	}
}


