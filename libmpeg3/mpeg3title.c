#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "mpeg3title.h"


#include <stdlib.h>
#include <string.h>

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
		dst->cell_table = calloc(1, sizeof(mpeg3_cell_t) * dst->cell_table_allocation);

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
	
	printf("mpeg3_dump_title path %s %llx-%llx cell_table_size %d\n", 
		title->fs->path, 
		title->start_byte,
		title->end_byte,
		title->cell_table_size);
	for(i = 0; i < title->cell_table_size; i++)
	{
		printf("%llx-%llx %llx-%llx %x\n", 
			title->cell_table[i].title_start, 
			title->cell_table[i].title_end, 
			title->cell_table[i].program_start, 
			title->cell_table[i].program_end, 
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
		mpeg3_cell_t *new_table;
		int i;

		new_allocation = title->cell_table_allocation ? 
			title->cell_table_size * 2 : 
			64;
		new_table = calloc(1, sizeof(mpeg3_cell_t) * new_allocation);

		if(title->cell_table)
		{
			memcpy(new_table, 
				title->cell_table, 
				sizeof(mpeg3_cell_t) * title->cell_table_allocation);
			free(title->cell_table);
		}
		title->cell_table = new_table;
		title->cell_table_allocation = new_allocation;
	}
}

void mpeg3_new_cell(mpeg3_title_t *title, 
		int64_t program_start, 
		int64_t program_end,
		int64_t title_start,
		int64_t title_end,
		int program)
{
	mpeg3_cell_t *new_cell;

	extend_cell_table(title);
	new_cell = &title->cell_table[title->cell_table_size];
	
	new_cell->program_start = program_start;
	new_cell->program_end = program_end;
	new_cell->title_start = title_start;
	new_cell->title_end = title_end;
	new_cell->program = program;
	title->cell_table_size++;
}

/* Create a title and get PID's by scanning first few bytes. */
int mpeg3_create_title(mpeg3_demuxer_t *demuxer, 
	FILE *toc)
{
	int result = 0, done = 0, counter_start, counter;
	mpeg3_t *file = demuxer->file;
	int64_t next_byte, prev_byte;
	double next_time, prev_time, absolute_time;
	long i;
	mpeg3_title_t *title;
	u_int32_t test_header = 0;

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

// Create default cell
	mpeg3_new_cell(title, 
		0, 
		title->end_byte,
		0,
		title->end_byte,
		0);


/* Get PID's and tracks */
	if(file->is_transport_stream || file->is_program_stream)
	{
		mpeg3io_seek(title->fs, 0);
		while(!done && !result && !mpeg3io_eof(title->fs))
		{
			next_byte = mpeg3io_tell(title->fs);
			result = mpeg3_read_next_packet(demuxer);

/* Just get the first bytes if not building a toc to get the stream ID's. */
			if(next_byte > 0x1000000 && !toc) done = 1;
		}
	}

	mpeg3io_seek(title->fs, 0);
	demuxer->read_all = 0;
	return 0;
}

int mpeg3demux_print_cells(mpeg3_title_t *title, FILE *output)
{
	mpeg3_cell_t *cell;
	mpeg3_t *file = title->file;
	int i;

	if(title->cell_table)
	{
		for(i = 0; i < title->cell_table_size; i++)
		{
			cell = &title->cell_table[i];

			fprintf(output, "REGION: %llx-%llx %llx-%llx %f %f %d\n",
				cell->program_start,
				cell->program_end,
				cell->title_start,
				cell->title_end,
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
