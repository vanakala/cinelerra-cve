#include "funcprotos.h"
#include "quicktime.h"


void quicktime_read_riff(quicktime_t *file, quicktime_atom_t *parent_atom)
{
	quicktime_riff_t *riff = quicktime_new_riff(file);
	quicktime_atom_t leaf_atom;
	int result = 0;
	int i;
	char data[5];

	riff->atom = *parent_atom;

// AVI
	quicktime_read_data(file, data, 4);
//printf("quicktime_read_riff 1 %llx\n", quicktime_position(file));

// Certain AVI parameters must be copied over to quicktime objects:
// hdrl -> moov
// movi -> mdat
// idx1 -> moov
	do
	{
		result = quicktime_atom_read_header(file, &leaf_atom);

/*
 * printf("quicktime_read_riff 1 %llx %llx %c%c%c%c\n", 
 * leaf_atom.start,
 * leaf_atom.size,
 * leaf_atom.type[0], 
 * leaf_atom.type[1], 
 * leaf_atom.type[2], 
 * leaf_atom.type[3]);
 */
		if(!result)
		{
			if(quicktime_atom_is(&leaf_atom, "LIST"))
			{
				data[4] = 0;


				result = !quicktime_read_data(file, data, 4);


				if(!result)
				{
// Got LIST 'hdrl'
					if(quicktime_match_32(data, "hdrl"))
					{

// No size here.
//printf("quicktime_read_riff 10 %llx\n", quicktime_position(file));
						quicktime_read_hdrl(file, &riff->hdrl, &leaf_atom);
//printf("quicktime_read_riff 20 %llx\n", quicktime_position(file));
					}
					else
// Got LIST 'movi'
					if(quicktime_match_32(data, "movi"))
					{
//printf("quicktime_read_riff 30 %llx\n", quicktime_position(file));
						quicktime_read_movi(file, &leaf_atom, &riff->movi);
//printf("quicktime_read_riff 40 %llx\n", quicktime_position(file));
					}
				}

// Skip it
				quicktime_atom_skip(file, &leaf_atom);

			}
			else
// Got 'movi'
			if(quicktime_atom_is(&leaf_atom, "movi"))
			{
				quicktime_read_movi(file, &leaf_atom, &riff->movi);

			}
			else
// Got 'idx1' original index
			if(quicktime_atom_is(&leaf_atom, "idx1"))
			{

//printf("quicktime_read_riff 50 %llx\n", quicktime_position(file));
// Preload idx1 here
				int64_t start_position = quicktime_position(file);
				long temp_size = leaf_atom.end - start_position;
				unsigned char *temp = malloc(temp_size);
				quicktime_set_preload(file, 
					(temp_size < 0x100000) ? 0x100000 : temp_size);
				quicktime_read_data(file, temp, temp_size);
				quicktime_set_position(file, start_position);
				free(temp);

// Read idx1
				quicktime_read_idx1(file, riff, &leaf_atom);
//printf("quicktime_read_riff 60 %llx\n", quicktime_position(file));

			}
			else
/* Skip it */
			{

				quicktime_atom_skip(file, &leaf_atom);

			}
		}
	}while(!result && quicktime_position(file) < parent_atom->end);

//printf("quicktime_read_riff 10\n");


}


quicktime_riff_t* quicktime_new_riff(quicktime_t *file)
{
	if(file->total_riffs >= MAX_RIFFS)
	{
		fprintf(stderr, "quicktime_new_riff file->total_riffs >= MAX_RIFFS\n");
		return 0;
	}
	else
	{
		quicktime_riff_t *riff = calloc(1, sizeof(quicktime_riff_t));
		file->riff[file->total_riffs++] = riff;
		return riff;
	}
}



void quicktime_delete_riff(quicktime_t *file, quicktime_riff_t *riff)
{
	int i = 0;
	quicktime_delete_hdrl(file, &riff->hdrl);
	quicktime_delete_movi(file, &riff->movi);
	quicktime_delete_idx1(&riff->idx1);
	free(riff);
}

void quicktime_init_riff(quicktime_t *file)
{

// Create new RIFF
	quicktime_riff_t *riff = quicktime_new_riff(file);

// Write riff header
// RIFF 'AVI '
	quicktime_atom_write_header(file, &riff->atom, "RIFF");
	quicktime_write_char32(file, "AVI ");

// Write header list in first RIFF only
	if(file->total_riffs < 2)
	{
		quicktime_init_hdrl(file, &riff->hdrl);
		riff->have_hdrl = 1;
	}

	quicktime_init_movi(file, riff);
}

void quicktime_finalize_riff(quicktime_t *file, quicktime_riff_t *riff)
{
// Write partial indexes
	quicktime_finalize_movi(file, &riff->movi);
	if(riff->have_hdrl)
	{
printf("quicktime_finalize_riff 1\n");
		quicktime_finalize_hdrl(file, &riff->hdrl);
printf("quicktime_finalize_riff 10\n");
// Write original index for first RIFF
		quicktime_write_idx1(file, &riff->idx1);
printf("quicktime_finalize_riff 100\n");
	}
	quicktime_atom_write_footer(file, &riff->atom);
}




void quicktime_import_avi(quicktime_t *file)
{
	int i, j, k;
	quicktime_riff_t *first_riff = file->riff[0];
	quicktime_idx1_t *idx1 = &first_riff->idx1;
	quicktime_hdrl_t *hdrl = &first_riff->hdrl;



/* Determine whether to use idx1 or indx indexes for offsets. */
/* idx1 must always be used for keyframes but it also must be */
/* ignored for offsets if indx exists. */


//printf("quicktime_import_avi 1\n");
/* Convert idx1 to keyframes and load offsets and sizes */
	for(i = 0; i < idx1->table_size; i++)
	{
		quicktime_idx1table_t *idx1table = idx1->table + i;
		char *tag = idx1table->tag;
		int track_number = (tag[0] - '0') * 10 + (tag[1] - '0');
		if(track_number < file->moov.total_tracks)
		{
			quicktime_trak_t *trak = file->moov.trak[track_number];
			int is_audio = trak->mdia.minf.is_audio;
			int is_video = trak->mdia.minf.is_video;


/* Chunk offset */
			quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
/* Sample size */
			quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
/* Samples per chunk */
			quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
/* Sample description */
			quicktime_stsd_t *stsd = &trak->mdia.minf.stbl.stsd;


/* Enter the offset and size no matter what so the sample counts */
/* can be used to set keyframes */
			quicktime_update_stco(stco, 
					stco->total_entries + 1, 
					idx1table->offset + first_riff->movi.atom.start);

			if(is_video)
			{
/* Just get the keyframe flag.  don't call quicktime_insert_keyframe because */
/* that updates idx1 and we don't have a track map. */
				int is_keyframe = (idx1table->flags & AVI_KEYFRAME) == AVI_KEYFRAME;
				if(is_keyframe)
				{
					quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
/* This is done before the image size table so this value is right */
					int frame = stsz->total_entries;

/* Expand table */
					if(stss->entries_allocated <= stss->total_entries)
					{
						stss->entries_allocated *= 2;
						stss->table = realloc(stss->table, 
							sizeof(quicktime_stss_table_t) * stss->entries_allocated);
					}
					stss->table[stss->total_entries++].sample = frame;
				}

/* Set image size */
				quicktime_update_stsz(stsz, 
							stsz->total_entries, 
							idx1table->size);
			}
			else
			if(is_audio)
			{
/* Set samples per chunk if PCM */
				if(stsd->table[0].sample_size > 0)
				{
					quicktime_update_stsc(stsc, 
						stsc->total_entries + 1, 
						idx1table->size * 
							8 / 
							stsd->table[0].sample_size / 
							stsd->table[0].channels);
				}
			}
		}
	}

//printf("quicktime_import_avi 10\n");


/* Convert super indexes into Quicktime indexes. */
/* Append to existing entries if idx1 exists. */
/* No keyframes here. */
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_strl_t *strl = first_riff->hdrl.strl[i];

		if(strl->have_indx)
		{
			quicktime_indx_t *indx = &strl->indx;
			quicktime_trak_t *trak = file->moov.trak[i];
			quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
			quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
			quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
			quicktime_stsd_t *stsd = &trak->mdia.minf.stbl.stsd;
/* Get existing chunk count from the idx1 */
			int existing_chunks = stco->total_entries;

/* Read each indx entry */
			for(j = 0; j < indx->table_size; j++)
			{
				quicktime_indxtable_t *indx_table = &indx->table[j];
				quicktime_ix_t *ix = indx_table->ix;

				for(k = 0; k < ix->table_size; k++)
				{
/* Copy from existing chunk count to end of ix table */
					if(existing_chunks <= 0)
					{
						quicktime_ixtable_t *ixtable = &ix->table[k];

/* Do the same things that idx1 did to the chunk tables */
/* Subtract the super indexes by size of the header.  McRoweSoft seems to */
/* want the header before the ix offset but after the idx1 offset. */
						quicktime_update_stco(stco, 
							stco->total_entries + 1, 
							ixtable->relative_offset + ix->base_offset - 8);
						if(strl->is_video)
						{
							quicktime_update_stsz(stsz, 
								stsz->total_entries, 
								ixtable->size);
						}
						else
						if(strl->is_audio)
						{
							if(stsd->table[0].sample_size > 0)
							{
								quicktime_update_stsc(stsc, 
									stsc->total_entries + 1, 
									ixtable->size * 
									8 / 
									stsd->table[0].sample_size / 
									stsd->table[0].channels);
							}
						}
					}
					else
						existing_chunks--;
				}
			}
		}
	}


//printf("quicktime_import_avi 20\n");




/* Set total samples, time to sample, for audio */
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_trak_t *trak = file->moov.trak[i];
		quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
		quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
		quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
		quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;

		if(trak->mdia.minf.is_audio)
		{
 			quicktime_stsc_table_t *stsc_table = stsc->table;
			long total_entries = stsc->total_entries;
			long chunk = stco->total_entries;
			long sample = 0;

			if(chunk > 0)
			{
				sample = quicktime_sample_of_chunk(trak, chunk) + 
					stsc_table[total_entries - 1].samples;
			}

			stsz->sample_size = 1;
			stsz->total_entries = sample;
			stts->table[0].sample_count = sample;
		}
		else
		if(trak->mdia.minf.is_video)
		{
			stsc->total_entries = 1;
/* stts has 1 allocation by default */
			stts->table[0].sample_count = stco->total_entries;
		}
	}

//printf("quicktime_import_avi 30\n");

}









