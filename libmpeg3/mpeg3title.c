#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "mpeg3title.h"


#include <stdlib.h>


mpeg3_title_t* mpeg3_new_title(mpeg3_t *file, char *path)
{
	mpeg3_title_t *title = calloc(1, sizeof(mpeg3_title_t));
	title->fs = mpeg3_new_fs(path);
	title->file = file;
	return title;
}

int mpeg3_delete_title(mpeg3_title_t *title)
{
	mpeg3_delete_fs(title->fs);
	if(title->cell_table_size)
	{
		free(title->cell_table);
	}
	free(title);
	return 0;
}


int mpeg3_copy_title(mpeg3_title_t *dst, mpeg3_title_t *src)
{
	int i;

	mpeg3_copy_fs(dst->fs, src->fs);
	dst->total_bytes = src->total_bytes;
	dst->start_byte = src->start_byte;
	dst->end_byte = src->end_byte;

	if(src->cell_table_size)
	{
		dst->cell_table_allocation = src->cell_table_allocation;
		dst->cell_table_size = src->cell_table_size;
		dst->cell_table = calloc(1, sizeof(mpeg3demux_cell_t) * dst->cell_table_allocation);

		for(i = 0; i < dst->cell_table_size; i++)
		{
			dst->cell_table[i] = src->cell_table[i];
		}
	}
	return 0;
}

int mpeg3_dump_title(mpeg3_title_t *title)
{
	int i;
	
	printf("mpeg3_dump_title path %s cell_table_size %d\n", title->fs->path, title->cell_table_size);
	for(i = 0; i < title->cell_table_size; i++)
	{
		printf("%.02f: %x - %x %.02f %.02f %x\n", 
			title->cell_table[i].absolute_start_time, 
			title->cell_table[i].start_byte, 
			title->cell_table[i].end_byte, 
			title->cell_table[i].start_time, 
			title->cell_table[i].end_time, 
			title->cell_table[i].program);
	}
	return 0;
}


// Realloc doesn't work for some reason.
static void extend_cell_table(mpeg3_title_t *title)
{
	if(!title->cell_table || 
		title->cell_table_allocation <= title->cell_table_size)
	{
		long new_allocation;
		mpeg3demux_cell_t *new_table;
		int i;

//printf("extend_cell_table 1\n");
		new_allocation = title->cell_table_allocation ? 
			title->cell_table_size * 2 : 
			64;
		new_table = calloc(1, sizeof(mpeg3demux_cell_t) * new_allocation);
//printf("extend_cell_table 1\n");
		memcpy(new_table, 
			title->cell_table, 
			sizeof(mpeg3demux_cell_t) * title->cell_table_allocation);
//printf("extend_cell_table 1 %p %d %d\n", title->cell_table, title->cell_table_allocation,
//	(new_allocation - title->cell_table_allocation));
		free(title->cell_table);
		title->cell_table = new_table;
//printf("extend_cell_table 2\n");
		title->cell_table_allocation = new_allocation;
//printf("extend_cell_table 2\n");
	}
}

void mpeg3_new_cell(mpeg3_title_t *title, 
		long start_byte, 
		double start_time,
		long end_byte,
		double end_time,
		int program)
{
	mpeg3demux_cell_t *new_cell;

	extend_cell_table(title);
	new_cell = &title->cell_table[title->cell_table_size];
	
	new_cell->start_byte = start_byte;
	new_cell->start_time = start_time;
	new_cell->end_byte = end_byte;
	new_cell->end_time = end_time;
	new_cell->absolute_start_time = 0;
	new_cell->program = program;
	title->cell_table_size++;
}

mpeg3demux_cell_t* mpeg3_append_cell(mpeg3_demuxer_t *demuxer, 
		mpeg3_title_t *title, 
		long prev_byte, 
		double prev_time, 
		long start_byte, 
		double start_time,
		int dont_store,
		int program)
{
	mpeg3demux_cell_t *new_cell, *old_cell;
	long i;

	extend_cell_table(title);
/*
 * printf("mpeg3_append_cell 1 %d %f %d %f %d %d\n", prev_byte, 
 * 		prev_time, 
 * 		start_byte, 
 * 		start_time,
 * 		dont_store,
 * 		program);
 */

	new_cell = &title->cell_table[title->cell_table_size];
	if(!dont_store)
	{
		new_cell->start_byte = start_byte;
		new_cell->start_time = start_time;
		new_cell->absolute_start_time = 0;

		if(title->cell_table_size > 0)
		{
			old_cell = &title->cell_table[title->cell_table_size - 1];
			old_cell->end_byte = prev_byte;
			old_cell->end_time = prev_time;
			new_cell->absolute_start_time = 
				prev_time - 
				old_cell->start_time + 
				old_cell->absolute_start_time;
			new_cell->absolute_end_time = start_time;
		}
	}

	title->cell_table_size++;
	return new_cell;
}

/* Create a title. */
/* Build a table of cells contained in the program stream. */
/* If toc is 0 just read the first and last cell. */
int mpeg3demux_create_title(mpeg3_demuxer_t *demuxer, 
		int cell_search, 
		FILE *toc)
{
	int result = 0, done = 0, counter_start, counter;
	mpeg3_t *file = demuxer->file;
	long next_byte, prev_byte;
	double next_time, prev_time, absolute_time;
	long i;
	mpeg3_title_t *title;
	u_int32_t test_header = 0;
	mpeg3demux_cell_t *cell = 0;

	demuxer->error_flag = 0;
	demuxer->read_all = 1;

/* Create a single title */
	if(!demuxer->total_titles)
	{
		demuxer->titles[0] = mpeg3_new_title(file, file->fs->path);
		demuxer->total_titles = 1;
		mpeg3demux_open_title(demuxer, 0);
	}

	title = demuxer->titles[0];
	title->total_bytes = mpeg3io_total_bytes(title->fs);
	title->start_byte = 0;
	title->end_byte = title->total_bytes;







/* Get cells for the title */
	if(file->is_transport_stream || file->is_program_stream)
	{
		mpeg3io_seek(title->fs, 0);
		while(!done && !result && !mpeg3io_eof(title->fs))
		{
			next_byte = mpeg3io_tell(title->fs);
			result = mpeg3_read_next_packet(demuxer);

			if(!result)
			{
				next_time = demuxer->time;
				if(next_time < prev_time || 
					next_time - prev_time > MPEG3_CONTIGUOUS_THRESHOLD ||
					!title->cell_table_size)
				{
/* Discontinuous */
					cell = mpeg3_append_cell(demuxer, 
						title, 
						prev_byte, 
						prev_time, 
						next_byte, 
						next_time,
						0,
						0);
/*
 * printf("cell: %ld %ld %f %f\n",
 * 				cell->start_byte,
 * 				cell->end_byte,
 * 				cell->start_time,
 * 				cell->end_time);
 */

					counter_start = next_time;
				}
				


// Breaks transport stream decoding	
// Kai Strieder
//				if (prev_time == next_time)
//				{
//					done = 1;
//				}


				prev_time = next_time;
				prev_byte = next_byte;
				counter = next_time;
			}

/* Just get the first bytes if not building a toc to get the stream ID's. */
			if(next_byte > 0x100000 && 
				(!cell_search || !toc)) done = 1;
		}

/* Get the last cell */
		if(!toc || !cell_search)
		{
			demuxer->read_all = 0;
			result = mpeg3io_seek(title->fs, title->total_bytes);
			if(!result) result = mpeg3_read_prev_packet(demuxer);
		}

		if(title->cell_table && cell)
		{
			cell->end_byte = title->total_bytes;
//			cell->end_byte = mpeg3io_tell(title->fs)/*  + demuxer->packet_size */;
			cell->end_time = demuxer->time;
			cell->absolute_end_time = cell->end_time - cell->start_time;
		}
	}

	mpeg3io_seek(title->fs, 0);
	demuxer->read_all = 0;
	return 0;
}

int mpeg3demux_print_cells(mpeg3_title_t *title, FILE *output)
{
	mpeg3demux_cell_t *cell;
	mpeg3_t *file = title->file;
	int i;

	if(title->cell_table)
	{
		for(i = 0; i < title->cell_table_size; i++)
		{
			cell = &title->cell_table[i];

			fprintf(output, "REGION: %ld %ld %f %f %d\n",
				cell->start_byte,
				cell->end_byte,
				cell->start_time,
				cell->end_time,
				cell->program);
		}
	}
	return 0;
}

int mpeg3demux_print_streams(mpeg3_demuxer_t *demuxer, FILE *toc)
{
	int i;
/* Print the stream information */
	for(i = 0; i < MPEG3_MAX_STREAMS; i++)
	{
		if(demuxer->astream_table[i])
			fprintf(toc, "ASTREAM: %d %d\n", i, demuxer->astream_table[i]);

		if(demuxer->vstream_table[i])
			fprintf(toc, "VSTREAM: %d %d\n", i, demuxer->vstream_table[i]);
	}
	return 0;
}
