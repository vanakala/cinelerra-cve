#include "funcprotos.h"
#include "quicktime.h"



typedef struct
{
	char tag[4];
	int32_t flags;
	int32_t offset;
	int32_t size;
} avi_tag_t;


static char** new_tags(quicktime_t *file)
{
	char **tags = calloc(sizeof(char*), file->moov.total_tracks);
	int i;

// Construct tags to look for
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		tags[i] = calloc(4, 1);
		if(file->moov.trak[i]->mdia.minf.is_audio)
		{
			tags[i][0] = i / 10 + '0';
			tags[i][1] = i % 10 + '0';
			tags[i][2] = 'w';
			tags[i][3] = 'b';
//printf("quicktime_read_idx1 2 %c%c%c%c\n", tags[i][0], tags[i][1], tags[i][2], tags[i][3]);
		}
		else
		if(file->moov.trak[i]->mdia.minf.is_video)
		{
			tags[i][0] = i / 10 + '0';
			tags[i][1] = i % 10 + '0';
			tags[i][2] = 'd';
			tags[i][3] = 'c';
//printf("quicktime_read_idx1 3 %c%c%c%c\n", tags[i][0], tags[i][1], tags[i][2], tags[i][3]);
		}
	}

	return tags;
}

static void delete_tags(quicktime_t *file, char **tags)
{
	int i;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		free(tags[i]);
	}

	free(tags);
}

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


void quicktime_read_idx1(quicktime_t *file, quicktime_atom_t *parent_atom)
{
	int i;
	char **tags;
	char data[4];


	

//printf("quicktime_read_idx1 1\n");
	tags = new_tags(file);

//printf("quicktime_read_idx1 2\n");



	while(quicktime_position(file) <= parent_atom->end - 0x10)
	{
		int current_vtrack = 0;
		int current_atrack = 0;
		quicktime_read_data(file, data, 4);

		for(i = 0; i < file->moov.total_tracks; i++)
		{
			if(quicktime_match_24(data, tags[i]))
			{
// Flags
				int flags = quicktime_read_int32_le(file);
				longest offset = quicktime_read_int32_le(file);

// Offset from start of movi
//printf("quicktime_read_idx1 3 %d\n", file->moov.trak[i]->mdia.minf.stbl.stco.total_entries);
				quicktime_update_stco(&file->moov.trak[i]->mdia.minf.stbl.stco, 
					file->moov.trak[i]->mdia.minf.stbl.stco.total_entries + 1, 
					offset);
//printf("quicktime_read_idx1 4\n");

				if(file->moov.trak[i]->mdia.minf.is_audio)
				{
					long chunk_size;

// Bytes in chunk
					chunk_size = quicktime_read_int32_le(file);


// Update samples per chunk table if PCM
					if(file->moov.trak[i]->mdia.minf.stbl.stsd.table[0].packet_size > 0)
					{
						quicktime_update_stsc(&file->moov.trak[i]->mdia.minf.stbl.stsc, 
							file->moov.trak[i]->mdia.minf.stbl.stsc.total_entries + 1, 
							chunk_size / 
								file->moov.trak[i]->mdia.minf.stbl.stsd.table[0].packet_size);
					}
				}
				else
				{
					long sample_size;

// Bytes in chunk
					sample_size = quicktime_read_int32_le(file);

//printf("quicktime_read_idx1 5 %d %d\n", file->moov.trak[i]->mdia.minf.stbl.stsz.total_entries, current_vtrack);

// Update keyframe table
					if(flags & 0x10)
						quicktime_insert_keyframe(file, 
							file->moov.trak[i]->mdia.minf.stbl.stsz.total_entries, 
							current_vtrack);
//printf("quicktime_read_idx1 6\n");

// Update samplesize table
					quicktime_update_stsz(&file->moov.trak[i]->mdia.minf.stbl.stsz, 
						file->moov.trak[i]->mdia.minf.stbl.stsz.total_entries, 
						sample_size);
//printf("quicktime_read_idx1 7\n");
				}
			}

			if(file->moov.trak[i]->mdia.minf.is_audio)
				current_atrack++;
			else
				current_vtrack++;
		}
	}

	quicktime_atom_skip(file, parent_atom);
	

// Delete tags
	delete_tags(file, tags);
}

void quicktime_write_idx1(quicktime_t *file)
{
	int i, j;
	quicktime_atom_t idx1_atom;
	char *temp;
	int table_size = 0;
	avi_tag_t *table;
	int current_offset = 0;
	int done = 0;
//printf(__FUNCTION__ " 1 %lld\n", file->total_length);

//printf(__FUNCTION__ " 1\n");

	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_trak_t *trak = file->moov.trak[i];
		table_size += trak->mdia.minf.stbl.stco.total_entries;
	}
//printf(__FUNCTION__ " 1\n");

	table = calloc(table_size, sizeof(avi_tag_t));
//printf(__FUNCTION__ " 1\n");

// Fill table with all tracks
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_trak_t *trak = file->moov.trak[i];
		int offsets = trak->mdia.minf.stbl.stco.total_entries;
		for(j = 0; j < offsets; j++)
		{
			avi_tag_t *entry = &table[current_offset];
			if(trak->mdia.minf.is_audio)
			{
				entry->tag[0] = i / 10 + '0';
				entry->tag[1] = i % 10 + '0';
				entry->tag[2] = 'w';
				entry->tag[3] = 'b';
				entry->flags = 0x10;
			}
			else
			{
				entry->tag[0] = i / 10 + '0';
				entry->tag[1] = i % 10 + '0';
				entry->tag[2] = 'd';
				entry->tag[3] = 'c';
				if(is_keyframe(trak, j))
					entry->flags = 0x10;
				else
					entry->flags = 0x0;
			}

			entry->offset = 
				trak->mdia.minf.stbl.stco.table[j].offset - 
				file->mdat.atom.start;
			entry->size = 
				trak->chunksizes[j];
			current_offset++;
		}
	}
//printf(__FUNCTION__ " 1\n");


// Sort table
	while(!done)
	{
		done = 1;
		for(i = 0; i < table_size - 1; i++)
		{
			if(table[i].offset > table[i + 1].offset)
			{
				avi_tag_t temp;
				memcpy(&temp, &table[i], sizeof(avi_tag_t));
				memcpy(&table[i], &table[i + 1], sizeof(avi_tag_t));
				memcpy(&table[i + 1], &temp, sizeof(avi_tag_t));
				done = 0;
			}
		}
	}

//printf(__FUNCTION__ " 1\n");

// Write table
	quicktime_atom_write_header(file, &idx1_atom, "idx1");
//printf(__FUNCTION__ " 2 %lld\n", file->total_length);

	for(i = 0; i < table_size; i++)
	{
		avi_tag_t *entry = &table[i];
		quicktime_write_char32(file, entry->tag);
		quicktime_write_int32_le(file, entry->flags);
		quicktime_write_int32_le(file, entry->offset);
		quicktime_write_int32_le(file, entry->size);
	}

//printf(__FUNCTION__ " 3 %lld\n", file->total_length);

	quicktime_atom_write_footer(file, &idx1_atom);
//printf(__FUNCTION__ " 1\n");
	if(table) free(table);
//printf(__FUNCTION__ " 2 %d %d\n", table_size, current_offset);
}




