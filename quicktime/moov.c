#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"


#include <zlib.h>


int quicktime_moov_init(quicktime_moov_t *moov)
{
	int i;

	moov->total_tracks = 0;
	for(i = 0 ; i < MAXTRACKS; i++) moov->trak[i] = 0;
	quicktime_mvhd_init(&(moov->mvhd));
	quicktime_udta_init(&(moov->udta));
	quicktime_ctab_init(&(moov->ctab));
	return 0;
}

int quicktime_moov_delete(quicktime_moov_t *moov)
{
	int i;
	while(moov->total_tracks) quicktime_delete_trak(moov);
	quicktime_mvhd_delete(&(moov->mvhd));
	quicktime_udta_delete(&(moov->udta));
	quicktime_ctab_delete(&(moov->ctab));
	return 0;
}

void quicktime_moov_dump(quicktime_moov_t *moov)
{
	int i;
	printf("movie\n");
	quicktime_mvhd_dump(&(moov->mvhd));
	quicktime_udta_dump(&(moov->udta));
	for(i = 0; i < moov->total_tracks; i++)
		quicktime_trak_dump(moov->trak[i]);
	quicktime_ctab_dump(&(moov->ctab));
}

static void* zalloc(void *opaque, unsigned int items, unsigned int size)
{
	return calloc(items, size);
}

static void zfree(void *opaque, void *ptr)
{
	free(ptr);
}

static int read_cmov(quicktime_t *file,
	quicktime_atom_t *parent_atom,
	quicktime_atom_t *moov_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

/* Algorithm used to compress */
		if(quicktime_atom_is(&leaf_atom, "dcom"))
		{
			char data[5];
//printf("read_cmov 1 %lld\n", quicktime_position(file));
			quicktime_read_data(file, data, 4);
			data[4] = 0;
			if(strcmp(data, "zlib"))
			{
				fprintf(stderr, 
					"read_cmov: compression '%c%c%c%c' not zlib.  Giving up and going to a movie.\n",
					data[0],
					data[1],
					data[2],
					data[3]);
				return 1;
			}

			quicktime_atom_skip(file, &leaf_atom);
		}
		else
/* Size of uncompressed data followed by compressed data */
		if(quicktime_atom_is(&leaf_atom, "cmvd"))
		{
/* Size of uncompressed data */
			int uncompressed_size = quicktime_read_int32(file);
/* Read compressed data */
			int compressed_size = leaf_atom.end - quicktime_position(file);
			if(compressed_size > uncompressed_size)
			{
				fprintf(stderr, 
					"read_cmov: FYI compressed_size=%d uncompressed_size=%d\n",
					compressed_size,
					uncompressed_size);
			}

			unsigned char *data_in = calloc(1, compressed_size);
			quicktime_read_data(file, data_in, compressed_size);
/* Decompress to another buffer */
			unsigned char *data_out = calloc(1, uncompressed_size + 0x400);
			z_stream zlib;
			zlib.zalloc = zalloc;
			zlib.zfree = zfree;
			zlib.opaque = NULL;
			zlib.avail_out = uncompressed_size + 0x400;
			zlib.next_out = data_out;
			zlib.avail_in = compressed_size;
			zlib.next_in = data_in;
			inflateInit(&zlib);
			inflate(&zlib, Z_PARTIAL_FLUSH);
			inflateEnd(&zlib);
			free(data_in);
			file->moov_data = data_out;

/*
 * FILE *test = fopen("/tmp/test", "w");
 * fwrite(data_out, uncompressed_size, 1, test);
 * fclose(test);
 * exit(0);
 */


/* Trick library into reading temporary buffer for the moov */
			file->moov_end = moov_atom->end;
			file->moov_size = moov_atom->size;
			moov_atom->end = moov_atom->start + uncompressed_size;
			moov_atom->size = uncompressed_size;
			file->old_preload_size = file->preload_size;
			file->old_preload_buffer = file->preload_buffer;
			file->old_preload_start = file->preload_start;
			file->old_preload_end = file->preload_end;
			file->old_preload_ptr = file->preload_ptr;
			file->preload_size = uncompressed_size;
			file->preload_buffer = data_out;
			file->preload_start = moov_atom->start;
			file->preload_end = file->preload_start + uncompressed_size;
			file->preload_ptr = 0;
			quicktime_set_position(file, file->preload_start + 8);
/* Try again */
			if(quicktime_read_moov(file, 
				&file->moov, 
				moov_atom))
				return 1;
/* Exit the compressed state */
			moov_atom->size = file->moov_size;
			moov_atom->end = file->moov_end;
			file->preload_size = file->old_preload_size;
			file->preload_buffer = file->old_preload_buffer;
			file->preload_start = file->old_preload_start;
			file->preload_end = file->old_preload_end;
			file->preload_ptr = file->old_preload_ptr;
			quicktime_set_position(file, moov_atom->end);
		}
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);
	return 0;
}


int quicktime_read_moov(quicktime_t *file, 
	quicktime_moov_t *moov, 
	quicktime_atom_t *parent_atom)
{
/* mandatory mvhd */
	quicktime_atom_t leaf_atom;

/* AVI translation: */
/* strh -> mvhd */

	do
	{
//printf("quicktime_read_moov 1 %llx\n", quicktime_position(file));
		quicktime_atom_read_header(file, &leaf_atom);

/*
 * printf("quicktime_read_moov 2 %c%c%c%c\n", 
 * leaf_atom.type[0],
 * leaf_atom.type[1],
 * leaf_atom.type[2],
 * leaf_atom.type[3]);
 */

		if(quicktime_atom_is(&leaf_atom, "cmov"))
		{
			file->compressed_moov = 1;
			if(read_cmov(file, &leaf_atom, parent_atom)) return 1;
/* Now were reading the compressed moov atom from the beginning. */
		}
		else
		if(quicktime_atom_is(&leaf_atom, "mvhd"))
		{
			quicktime_read_mvhd(file, &(moov->mvhd), &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "clip"))
		{
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "trak"))
		{
			quicktime_trak_t *trak = quicktime_add_trak(file);
			quicktime_read_trak(file, trak, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "udta"))
		{
			quicktime_read_udta(file, &(moov->udta), &leaf_atom);
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "ctab"))
		{
			quicktime_read_ctab(file, &(moov->ctab));
		}
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);

	return 0;
}

void quicktime_write_moov(quicktime_t *file, quicktime_moov_t *moov)
{
	quicktime_atom_t atom;
	int i;
	long int64_t_duration = 0;
	long duration, timescale;
	int result;


// Try moov header immediately
	file->mdat.atom.end = quicktime_position(file);
	result = quicktime_atom_write_header(file, &atom, "moov");

// Disk full.  Rewind and try again
	if(result)
	{
		quicktime_set_position(file, file->mdat.atom.end - (int64_t)0x100000);
		file->mdat.atom.end = quicktime_position(file);
		quicktime_atom_write_header(file, &atom, "moov");
	}

/* get the duration from the int64_t track in the mvhd's timescale */
	for(i = 0; i < moov->total_tracks; i++)
	{
		quicktime_trak_fix_counts(file, moov->trak[i]);
		quicktime_trak_duration(moov->trak[i], &duration, &timescale);

		duration = (long)((float)duration / timescale * moov->mvhd.time_scale);

		if(duration > int64_t_duration)
		{
			int64_t_duration = duration;
		}
	}
	moov->mvhd.duration = int64_t_duration;
	moov->mvhd.selection_duration = int64_t_duration;

	quicktime_write_mvhd(file, &(moov->mvhd));
	quicktime_write_udta(file, &(moov->udta));
	for(i = 0; i < moov->total_tracks; i++)
	{
		quicktime_write_trak(file, moov->trak[i], moov->mvhd.time_scale);
	}
	/*quicktime_write_ctab(file, &(moov->ctab)); */

	quicktime_atom_write_footer(file, &atom);
// Rewind to end of mdat
	quicktime_set_position(file, file->mdat.atom.end);
}

void quicktime_update_durations(quicktime_moov_t *moov)
{
	
}

int quicktime_shift_offsets(quicktime_moov_t *moov, int64_t offset)
{
	int i;
	for(i = 0; i < moov->total_tracks; i++)
	{
		quicktime_trak_shift_offsets(moov->trak[i], offset);
	}
	return 0;
}
