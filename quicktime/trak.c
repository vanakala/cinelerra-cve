#include "funcprotos.h"
#include "quicktime.h"




int quicktime_trak_init(quicktime_trak_t *trak)
{
	quicktime_tkhd_init(&(trak->tkhd));
	quicktime_edts_init(&(trak->edts));
	quicktime_mdia_init(&(trak->mdia));
	return 0;
}

int quicktime_trak_init_video(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int frame_w, 
							int frame_h, 
							float frame_rate,
							char *compressor)
{

	quicktime_tkhd_init_video(file, 
		&(trak->tkhd), 
		frame_w, 
		frame_h);
	quicktime_mdia_init_video(file, 
		&(trak->mdia), 
		frame_w, 
		frame_h, 
		frame_rate, 
		compressor);
	quicktime_edts_init_table(&(trak->edts));

	return 0;
}

int quicktime_trak_init_audio(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int channels, 
							int sample_rate, 
							int bits, 
							char *compressor)
{
	quicktime_mdia_init_audio(file, &(trak->mdia), 
		channels, 
		sample_rate, 
		bits, 
		compressor);
	quicktime_edts_init_table(&(trak->edts));
	return 0;
}

int quicktime_trak_delete(quicktime_trak_t *trak)
{
	quicktime_mdia_delete(&(trak->mdia));
	quicktime_edts_delete(&(trak->edts));
	quicktime_tkhd_delete(&(trak->tkhd));
	return 0;
}


int quicktime_trak_dump(quicktime_trak_t *trak)
{
	printf(" track\n");
	quicktime_tkhd_dump(&(trak->tkhd));
	quicktime_edts_dump(&(trak->edts));
	quicktime_mdia_dump(&(trak->mdia));

	return 0;
}

// Used when reading a file
quicktime_trak_t* quicktime_add_trak(quicktime_t *file)
{
	quicktime_moov_t *moov = &(file->moov);
	if(moov->total_tracks < MAXTRACKS)
	{
		moov->trak[moov->total_tracks] = calloc(1, sizeof(quicktime_trak_t));
		quicktime_trak_init(moov->trak[moov->total_tracks]);
		moov->total_tracks++;
	}
	return moov->trak[moov->total_tracks - 1];
}

int quicktime_delete_trak(quicktime_moov_t *moov)
{
	if(moov->total_tracks)
	{
		moov->total_tracks--;
		quicktime_trak_delete(moov->trak[moov->total_tracks]);
		free(moov->trak[moov->total_tracks]);
	}
	return 0;
}


int quicktime_read_trak(quicktime_t *file, quicktime_trak_t *trak, quicktime_atom_t *trak_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

//printf("quicktime_read_trak %llx %llx\n", quicktime_position(file), quicktime_ftell(file));
/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "tkhd"))
			{ quicktime_read_tkhd(file, &(trak->tkhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "mdia"))
			{ quicktime_read_mdia(file, &(trak->mdia), &leaf_atom); }
		else
/* optional */
		if(quicktime_atom_is(&leaf_atom, "clip"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "matt"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "edts"))
			{ quicktime_read_edts(file, &(trak->edts), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "load"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "tref"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "imap"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "udta"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
			quicktime_atom_skip(file, &leaf_atom);
//printf("quicktime_read_trak %llx %llx\n", quicktime_position(file), leaf_atom.end);
	}while(quicktime_position(file) < trak_atom->end);

	return 0;
}

int quicktime_write_trak(quicktime_t *file, 
	quicktime_trak_t *trak, 
	long moov_time_scale)
{
	long duration;
	long timescale;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "trak");
	quicktime_trak_duration(trak, &duration, &timescale);

/*printf("quicktime_write_trak duration %d\n", duration); */
/* get duration in movie's units */
	trak->tkhd.duration = (long)((float)duration / timescale * moov_time_scale);
	trak->mdia.mdhd.duration = duration;
	trak->mdia.mdhd.time_scale = timescale;

	quicktime_write_tkhd(file, &(trak->tkhd));
	quicktime_write_edts(file, &(trak->edts), trak->tkhd.duration);
	quicktime_write_mdia(file, &(trak->mdia));

	quicktime_atom_write_footer(file, &atom);

	return 0;
}

int64_t quicktime_track_end(quicktime_trak_t *trak)
{
/* get the byte endpoint of the track in the file */
	int64_t size = 0;
	int64_t chunk, chunk_offset, chunk_samples, sample;
	quicktime_stsz_t *stsz = &(trak->mdia.minf.stbl.stsz);
	quicktime_stsz_table_t *table = stsz->table;
	quicktime_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	quicktime_stco_t *stco;

/* get the last chunk offset */
/* the chunk offsets contain the HEADER_LENGTH themselves */
	stco = &(trak->mdia.minf.stbl.stco);
	chunk = stco->total_entries;
	size = chunk_offset = stco->table[chunk - 1].offset;

/* get the number of samples in the last chunk */
	chunk_samples = stsc->table[stsc->total_entries - 1].samples;

/* get the size of last samples */
	if(stsz->sample_size)
	{
/* assume audio so calculate the sample size */
		size += chunk_samples * stsz->sample_size
			* trak->mdia.minf.stbl.stsd.table[0].channels 
			* trak->mdia.minf.stbl.stsd.table[0].sample_size / 8;
	}
	else
	{
/* assume video */
		for(sample = stsz->total_entries - chunk_samples; 
			sample < stsz->total_entries; sample++)
		{
			size += stsz->table[sample].size;
		}
	}

	return size;
}

long quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak)
{
/*printf("file->rd %d file->wr %d\n", file->rd, file->wr); */
	if(file->wr)
	{
/* get the sample count when creating a new file */
 		quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
		long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
		long chunk = trak->mdia.minf.stbl.stco.total_entries;
		long sample;

		if(chunk)
		{
			sample = quicktime_sample_of_chunk(trak, chunk);
			sample += table[total_entries - 1].samples;
		}
		else 
			sample = 0;

		return sample;
	}
	else
	{
/* get the sample count when reading only */
		quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
		int i;
		int64_t total = 0;

		for(i = 0; i < stts->total_entries; i++)
		{
			total += stts->table[i].sample_count *
				stts->table[i].sample_duration;
		}

/* Convert to samples */
		if(trak->mdia.minf.is_audio)
			return total;
		else
		if(trak->mdia.minf.is_video)
			return total /
				stts->table[0].sample_duration;
		return total;
	}
}

long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk)
{
	quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
	long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
	long chunk1entry, chunk2entry;
	long chunk1, chunk2, chunks, total = 0;

	for(chunk1entry = total_entries - 1, chunk2entry = total_entries; 
		chunk1entry >= 0; 
		chunk1entry--, chunk2entry--)
	{
		chunk1 = table[chunk1entry].chunk;

		if(chunk > chunk1)
		{
			if(chunk2entry < total_entries)
			{
				chunk2 = table[chunk2entry].chunk;

				if(chunk < chunk2) chunk2 = chunk;
			}
			else
				chunk2 = chunk;

			chunks = chunk2 - chunk1;

			total += chunks * table[chunk1entry].samples;
		}
	}

	return total;
}

// For AVI
int quicktime_avg_chunk_samples(quicktime_t *file, quicktime_trak_t *trak)
{
	int i, chunk = trak->mdia.minf.stbl.stco.total_entries - 1;
	long total_samples;

	if(chunk >= 0)
	{
		total_samples = quicktime_sample_of_chunk(trak, chunk);
		return total_samples / (chunk + 1);
	}
	else
	{
		total_samples = quicktime_track_samples(file, trak);
		return total_samples;
	}
}

int quicktime_chunk_of_sample(int64_t *chunk_sample, 
	int64_t *chunk, 
	quicktime_trak_t *trak, 
	long sample)
{
	quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
	long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
	long chunk2entry;
	long chunk1, chunk2, chunk1samples, range_samples, total = 0;

	chunk1 = 1;
	chunk1samples = 0;
	chunk2entry = 0;

	if(!total_entries)
	{
		*chunk_sample = 0;
		*chunk = 0;
		return 0;
	}

	do
	{
		chunk2 = table[chunk2entry].chunk;
		*chunk = chunk2 - chunk1;
		range_samples = *chunk * chunk1samples;

		if(sample < total + range_samples) break;

		chunk1samples = table[chunk2entry].samples;
		chunk1 = chunk2;

		if(chunk2entry < total_entries)
		{
			chunk2entry++;
			total += range_samples;
		}
	}while(chunk2entry < total_entries);

	if(chunk1samples)
		*chunk = (sample - total) / chunk1samples + chunk1;
	else
		*chunk = 1;

	*chunk_sample = total + (*chunk - chunk1) * chunk1samples;
	return 0;
}

int64_t quicktime_chunk_to_offset(quicktime_t *file, quicktime_trak_t *trak, long chunk)
{
	quicktime_stco_table_t *table = trak->mdia.minf.stbl.stco.table;
	int64_t result = 0;

	if(trak->mdia.minf.stbl.stco.total_entries && 
		chunk > trak->mdia.minf.stbl.stco.total_entries)
		result = table[trak->mdia.minf.stbl.stco.total_entries - 1].offset;
	else
	if(trak->mdia.minf.stbl.stco.total_entries)
		result = table[chunk - 1].offset;
	else
		result = HEADER_LENGTH * 2;

// Skip chunk header for AVI.  Skip it here instead of in read_chunk because some
// codecs can't use read_chunk
	if(file->use_avi)
	{
//printf("quicktime_chunk_to_offset 1 %llx %llx\n", result, file->mdat.atom.start);
		result += 8 + file->mdat.atom.start;
	}
	return result;
}

long quicktime_offset_to_chunk(int64_t *chunk_offset, 
	quicktime_trak_t *trak, 
	int64_t offset)
{
	quicktime_stco_table_t *table = trak->mdia.minf.stbl.stco.table;
	int i;

	for(i = trak->mdia.minf.stbl.stco.total_entries - 1; i >= 0; i--)
	{
		if(table[i].offset <= offset)
		{
			*chunk_offset = table[i].offset;
			return i + 1;
		}
	}
	*chunk_offset = HEADER_LENGTH * 2;
	return 1;
}

int quicktime_chunk_bytes(quicktime_t *file, 
	int64_t *chunk_offset,
	int chunk, 
	quicktime_trak_t *trak)
{
	int result;
	*chunk_offset = quicktime_chunk_to_offset(file, trak, chunk);
	quicktime_set_position(file, *chunk_offset - 4);
	result = quicktime_read_int32_le(file);
	return result;
}

int64_t quicktime_sample_range_size(quicktime_trak_t *trak, 
	long chunk_sample, 
	long sample)
{
	quicktime_stsz_table_t *table = trak->mdia.minf.stbl.stsz.table;
	int64_t i, total;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
/* assume audio */
		return quicktime_samples_to_bytes(trak, sample - chunk_sample);
	}
	else
	{
/* video or vbr audio */
		for(i = chunk_sample, total = 0; 
			i < sample && i < trak->mdia.minf.stbl.stsz.total_entries; 
			i++)
		{
			total += trak->mdia.minf.stbl.stsz.table[i].size;
		}
	}
	return total;
}

int64_t quicktime_sample_to_offset(quicktime_t *file, 
	quicktime_trak_t *trak, 
	long sample)
{
	int64_t chunk, chunk_sample, chunk_offset1, chunk_offset2;

	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
	chunk_offset1 = quicktime_chunk_to_offset(file, trak, chunk);
	chunk_offset2 = chunk_offset1 + 
		quicktime_sample_range_size(trak, chunk_sample, sample);
	return chunk_offset2;
}

long quicktime_offset_to_sample(quicktime_trak_t *trak, int64_t offset)
{
	int64_t chunk_offset;
	int64_t chunk = quicktime_offset_to_chunk(&chunk_offset, trak, offset);
	int64_t chunk_sample = quicktime_sample_of_chunk(trak, chunk);
	int64_t sample, sample_offset;
	quicktime_stsz_table_t *table = trak->mdia.minf.stbl.stsz.table;
	int64_t total_samples = trak->mdia.minf.stbl.stsz.total_entries;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		sample = chunk_sample + (offset - chunk_offset) / 
			trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	for(sample = chunk_sample, sample_offset = chunk_offset; 
		sample_offset < offset && sample < total_samples; )
	{
		sample_offset += table[sample].size;
		if(sample_offset < offset) sample++;
	}
	
	return sample;
}

void quicktime_write_chunk_header(quicktime_t *file, 
	quicktime_trak_t *trak, 
	quicktime_atom_t *chunk)
{
	if(file->use_avi)
	{
/* Get tag from first riff strl */
		quicktime_riff_t *first_riff = file->riff[0];
		quicktime_hdrl_t *hdrl = &first_riff->hdrl;
		quicktime_strl_t *strl = hdrl->strl[trak->tkhd.track_id - 1];
		char *tag = strl->tag;

/* Create new RIFF object at 1 Gig mark */
		quicktime_riff_t *riff = file->riff[file->total_riffs - 1];
		if(quicktime_position(file) - riff->atom.start > 0x40000000)
		{
			quicktime_finalize_riff(file, riff);
			quicktime_init_riff(file);
		}

		

/* Write AVI header */
		quicktime_atom_write_header(file, chunk, tag);
	}
	else
	{
		chunk->start = quicktime_position(file);
	}
}

void quicktime_write_chunk_footer(quicktime_t *file, 
	quicktime_trak_t *trak,
	int current_chunk,
	quicktime_atom_t *chunk, 
	int samples)
{
	int64_t offset = chunk->start;
	int sample_size = quicktime_position(file) - offset;


// Write AVI footer
	if(file->use_avi)
	{
		quicktime_atom_write_footer(file, chunk);

// Save original index entry for first RIFF only
		if(file->total_riffs < 2)
		{
			quicktime_update_idx1table(file, 
				trak, 
				offset, 
				sample_size);
		}

// Save partial index entry
		quicktime_update_ixtable(file, 
			trak, 
			offset, 
			sample_size);
	}

	if(offset + sample_size > file->mdat.atom.size)
		file->mdat.atom.size = offset + sample_size;

	quicktime_update_stco(&(trak->mdia.minf.stbl.stco), 
		current_chunk, 
		offset);

	if(trak->mdia.minf.is_video)
		quicktime_update_stsz(&(trak->mdia.minf.stbl.stsz), 
		current_chunk - 1, 
		sample_size);

	quicktime_update_stsc(&(trak->mdia.minf.stbl.stsc), 
		current_chunk, 
		samples);
}


/*
 * int quicktime_update_tables(quicktime_t *file, 
 * 							quicktime_trak_t *trak, 
 * 							int64_t offset, 
 * 							int64_t chunk, 
 * 							int64_t sample, 
 * 							int64_t samples, 
 * 							int64_t sample_size)
 * {
 * //if(file->use_avi)
 * printf("quicktime_update_tables: replaced by quicktime_write_chunk_header and quicktime_write_chunk_footer\n");
 * 
 * 	if(offset + sample_size > file->mdat.atom.size) 
 * 		file->mdat.atom.size = offset + sample_size;
 * 
 * 	quicktime_update_stco(&(trak->mdia.minf.stbl.stco), chunk, offset);
 * 
 * 	if(trak->mdia.minf.is_video)
 * 		quicktime_update_stsz(&(trak->mdia.minf.stbl.stsz), sample, sample_size);
 * 
 * 
 * 	quicktime_update_stsc(&(trak->mdia.minf.stbl.stsc), chunk, samples);
 * 	return 0;
 * }
 */


/**
 * This is used for writing the header
 **/
int quicktime_trak_duration(quicktime_trak_t *trak, 
	long *duration, 
	long *timescale)
{
	quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
	int i;
	*duration = 0;

	for(i = 0; i < stts->total_entries; i++)
	{
		*duration += stts->table[i].sample_duration * 
			stts->table[i].sample_count;
	}

	*timescale = trak->mdia.mdhd.time_scale;
	return 0;
}

int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak)
{
	long samples = quicktime_track_samples(file, trak);

	trak->mdia.minf.stbl.stts.table[0].sample_count = samples;

	if(!trak->mdia.minf.stbl.stsz.total_entries)
	{
		trak->mdia.minf.stbl.stsz.sample_size = 1;
		trak->mdia.minf.stbl.stsz.total_entries = samples;
	}

	return 0;
}

long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk)
{
	long result, current_chunk;
	quicktime_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	long i = stsc->total_entries - 1;

	do
	{
		current_chunk = stsc->table[i].chunk;
		result = stsc->table[i].samples;
		i--;
	}while(i >= 0 && current_chunk > chunk);

	return result;
}

int quicktime_trak_shift_offsets(quicktime_trak_t *trak, int64_t offset)
{
	quicktime_stco_t *stco = &(trak->mdia.minf.stbl.stco);
	int i;

	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset += offset;
	}
	return 0;
}
