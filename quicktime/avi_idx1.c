#include "funcprotos.h"
#include "quicktime.h"
#include <string.h>



typedef struct
{
	char tag[4];
	int32_t flags;
	int32_t offset;
	int32_t size;
} avi_tag_t;



static int is_keyframe(quicktime_trak_t *trak, int frame)
{
	int i;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	frame++;
	for(i = 0; i < stss->total_entries; i++)
	{
		if(stss->table[i].sample == frame) return 1;
	}
	return 0;
}


void quicktime_delete_idx1(quicktime_idx1_t *idx1)
{
	if(idx1->table) free(idx1->table);
}

void quicktime_read_idx1(quicktime_t *file, 
	quicktime_riff_t *riff,
	quicktime_atom_t *parent_atom)
{
	int i;
	quicktime_riff_t *first_riff = file->riff[0];
	quicktime_hdrl_t *hdrl = &first_riff->hdrl;
	quicktime_idx1_t *idx1 = &riff->idx1;

//printf("quicktime_read_idx1 1 %llx\n", quicktime_position(file));

// Allocate table.
	idx1->table_size = (parent_atom->end - quicktime_position(file)) / 16;
	idx1->table_allocation = idx1->table_size;
	idx1->table = calloc(sizeof(quicktime_idx1table_t), idx1->table_size);
//printf("quicktime_read_idx1 10\n");

// Store it in idx1 table now.
// Wait for full ix table discovery before converting to stco.
	for(i = 0; i < idx1->table_size; i++)
	{
		quicktime_idx1table_t *idx1table = idx1->table + i;

		quicktime_read_data(file, idx1table->tag, 4);
		idx1table->flags = quicktime_read_int32_le(file);
		idx1table->offset = quicktime_read_int32_le(file);
		idx1table->size = quicktime_read_int32_le(file);
	}

//printf("quicktime_read_idx1 100\n");







}

void quicktime_write_idx1(quicktime_t *file, 
	quicktime_idx1_t *idx1)
{
	int i;
	quicktime_idx1table_t *table = idx1->table;
	int table_size = idx1->table_size;



// Write table
	quicktime_atom_write_header(file, &idx1->atom, "idx1");

	for(i = 0; i < table_size; i++)
	{
		quicktime_idx1table_t *entry = &table[i];
		quicktime_write_char32(file, entry->tag);
		quicktime_write_int32_le(file, entry->flags);
		quicktime_write_int32_le(file, entry->offset);
		quicktime_write_int32_le(file, entry->size);
	}


	quicktime_atom_write_footer(file, &idx1->atom);
}

void quicktime_set_idx1_keyframe(quicktime_t *file, 
	quicktime_trak_t *trak,
	int new_keyframe)
{
	quicktime_riff_t *riff = file->riff[0];
	quicktime_hdrl_t *hdrl = &riff->hdrl;
	quicktime_strl_t *strl = hdrl->strl[trak->tkhd.track_id - 1];
	char *tag = strl->tag;
	quicktime_idx1_t *idx1 = &riff->idx1;
	int i;
	int counter = -1;

// Search through entire index for right numbered tag.
// Since all the tracks are combined in the same index, this is unavoidable.
	for(i = 0; i < idx1->table_size; i++)
	{
		quicktime_idx1table_t *idx1_table = &idx1->table[i];
		if(!memcmp(idx1_table->tag, tag, 4))
		{
			counter++;
			if(counter == new_keyframe)
			{
				idx1_table->flags |= AVI_KEYFRAME;
				break;
			}
		}
	}
}

void quicktime_update_idx1table(quicktime_t *file, 
	quicktime_trak_t *trak, 
	int offset,
	int size)
{
	quicktime_riff_t *riff = file->riff[0];
	quicktime_hdrl_t *hdrl = &riff->hdrl;
	quicktime_strl_t *strl = hdrl->strl[trak->tkhd.track_id - 1];
	char *tag = strl->tag;
	quicktime_idx1_t *idx1 = &riff->idx1;
	quicktime_movi_t *movi = &riff->movi;
	quicktime_idx1table_t *idx1_table;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	uint32_t flags = 0;
	int i;
	int keyframe_frame = idx1->table_size + 1;

// Set flag for keyframe
	for(i = stss->total_entries - 1; i >= 0; i--)
	{
		if(stss->table[i].sample == keyframe_frame)
		{
			flags |= AVI_KEYFRAME;
			break;
		}
		else
		if(stss->table[i].sample < keyframe_frame)
		{
			break;
		}
	}


// Allocation
	if(idx1->table_size >= idx1->table_allocation)
	{
		quicktime_idx1table_t *old_table = idx1->table;
		int new_allocation = idx1->table_allocation * 2;
		if(new_allocation < 1) new_allocation = 1;
		idx1->table = calloc(1, sizeof(quicktime_idx1table_t) * new_allocation);
		if(old_table)
		{
			memcpy(idx1->table, old_table, sizeof(quicktime_idx1table_t) * idx1->table_size);
			free(old_table);
		}
		idx1->table_allocation = new_allocation;
	}


// Appendage
	idx1_table = &idx1->table[idx1->table_size];
	memcpy(idx1_table->tag, tag, 4);
	idx1_table->flags = flags;
	idx1_table->offset = offset - 8 - movi->atom.start;
	idx1_table->size = size;
	idx1->table_size++;
}




