#include "funcprotos.h"
#include "quicktime.h"


void quicktime_delete_indx(quicktime_indx_t *indx)
{
	int i;
	if(indx->table)
	{
		for(i = 0; i < indx->table_size; i++)
		{
			quicktime_indxtable_t *indx_table = &indx->table[i];
			if(indx_table->ix) quicktime_delete_ix(indx_table->ix);
		}
		free(indx->table);
	}
}

void quicktime_init_indx(quicktime_t *file, 
	quicktime_indx_t *indx, 
	quicktime_strl_t *strl)
{
	indx->longs_per_entry = 4;
	indx->index_subtype = 0;
	indx->index_type = AVI_INDEX_OF_INDEXES;
	memcpy(indx->chunk_id, strl->tag, 4);
}

void quicktime_update_indx(quicktime_t *file, 
	quicktime_indx_t *indx, 
	quicktime_ix_t *ix)
{
	quicktime_indxtable_t *indx_table;

/* Allocate */
	if(indx->table_size >= indx->table_allocation)
	{
		quicktime_indxtable_t *old_table = indx->table;
		int new_allocation = indx->table_allocation * 2;
		if(new_allocation < 1) new_allocation = 1;
		indx->table = calloc(1, sizeof(quicktime_indxtable_t) * new_allocation);
		if(old_table)
		{
			memcpy(indx->table, old_table, sizeof(quicktime_indxtable_t) * indx->table_size);
			free(old_table);
		}
		indx->table_allocation = new_allocation;
	}

/* Append */
	indx_table = &indx->table[indx->table_size++];
	indx_table->index_offset = ix->atom.start - 8;
	indx_table->index_size = ix->atom.size;
	indx_table->duration = ix->table_size;
}



void quicktime_finalize_indx(quicktime_t *file)
{
	int i, j;
	quicktime_riff_t *riff = file->riff[0];
	quicktime_hdrl_t *hdrl = &riff->hdrl;
	quicktime_strl_t *strl;
	quicktime_indx_t *indx;
	quicktime_atom_t junk_atom;
	int junk_size;


	for(i = 0; i < file->moov.total_tracks; i++)
	{
		strl = hdrl->strl[i];
		indx = &strl->indx;

/* Write indx */
		quicktime_set_position(file, strl->indx_offset);
		quicktime_atom_write_header(file, &indx->atom, "indx");
/* longs per entry */
		quicktime_write_int16_le(file, indx->longs_per_entry);
/* index sub type */
		quicktime_write_char(file, indx->index_subtype);
/* index type */
		quicktime_write_char(file, indx->index_type);
/* entries in use */
		quicktime_write_int32_le(file, indx->table_size);
/* chunk ID */
		quicktime_write_char32(file, indx->chunk_id);
/* reserved */
		quicktime_write_int32_le(file, 0);
		quicktime_write_int32_le(file, 0);
		quicktime_write_int32_le(file, 0);

/* table */
		for(j = 0; j < indx->table_size; j++)
		{
			quicktime_indxtable_t *indx_table = &indx->table[j];
			quicktime_write_int64_le(file, indx_table->index_offset);
			quicktime_write_int32_le(file, indx_table->index_size);
			quicktime_write_int32_le(file, indx_table->duration);
		}

		quicktime_atom_write_footer(file, &indx->atom);



/* Rewrite JUNK less indx size and indx header size */
		junk_size = strl->padding_size - indx->atom.size - 8;
/*
 * 		quicktime_atom_write_header(file, &junk_atom, "JUNK");
 * 		for(j = 0; j < junk_size; j += 4)
 * 			quicktime_write_int32_le(file, 0);
 * 		quicktime_atom_write_footer(file, &junk_atom);
 */
	}
}









void quicktime_read_indx(quicktime_t *file, 
	quicktime_strl_t *strl, 
	quicktime_atom_t *parent_atom)
{
	quicktime_indx_t *indx = &strl->indx;
	quicktime_indxtable_t *indx_table;
	quicktime_ix_t *ix;
	int i;
	int64_t offset;

	indx->longs_per_entry = quicktime_read_int16_le(file);
	indx->index_subtype = quicktime_read_char(file);
	indx->index_type = quicktime_read_char(file);
	indx->table_size = quicktime_read_int32_le(file);
	quicktime_read_char32(file, indx->chunk_id);
	quicktime_read_int32_le(file);
	quicktime_read_int32_le(file);
	quicktime_read_int32_le(file);

//printf("quicktime_read_indx 1\n");
/* Read indx entries */
	indx->table = calloc(indx->table_size, sizeof(quicktime_indxtable_t));
	for(i = 0; i < indx->table_size; i++)
	{
		indx_table = &indx->table[i];
		indx_table->index_offset = quicktime_read_int64_le(file);
		indx_table->index_size = quicktime_read_int32_le(file);
		indx_table->duration = quicktime_read_int32_le(file);
		offset = quicktime_position(file);

		indx_table->ix = calloc(indx->table_size, sizeof(quicktime_ix_t*));

/* Now read the partial index */
		ix = indx_table->ix = calloc(1, sizeof(quicktime_ix_t));
		quicktime_set_position(file, indx_table->index_offset);
		quicktime_read_ix(file, ix);
		quicktime_set_position(file, offset);
	}
//printf("quicktime_read_indx 100\n");

}




