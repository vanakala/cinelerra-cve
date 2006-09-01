#include <byteswap.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "ifo.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"

typedef struct
{
// Bytes relative to start of stream.
	int64_t start_byte;
	int64_t end_byte;
// Used in final table
	int program;
// Used in cell play info
	int cell_type;
// Used in cell addresses
	int vob_id;
	int cell_id;
} mpeg3ifo_cell_t;

typedef struct
{
	mpeg3ifo_cell_t *cells;
	long total_cells;
	long cells_allocated;
} mpeg3ifo_celltable_t;


#define CADDR_HDR_LEN 8

typedef struct {
	u_short	num		: 16;	// Number of Video Objects
	u_short unknown		: 16;	// don't know
	u_int	len		: 32;	// length of table
} cell_addr_hdr_t;

typedef struct {
	u_int	foo		: 16;	// ???
	u_int	num		: 16;	// number of subchannels
} audio_hdr_t;

#define AUDIO_HDR_LEN 4

typedef struct {
	u_short id		: 16;	// Language
	u_short			: 16;	// don't know
	u_int	start		: 32;	// Start of unit
} pgci_sub_t;

#define PGCI_SUB_LEN 8

#define PGCI_COLOR_LEN 4


static u_int get4bytes(u_char *buf)
{
	return bswap_32 (*((u_int32_t *)buf));
}

static u_int get2bytes(u_char *buf)
{
	return bswap_16 (*((u_int16_t *)buf));
}

static int ifo_read(int fd, long pos, long count, unsigned char *data)
{
	if((pos = lseek(fd, pos, SEEK_SET)) < 0)
	{
    	perror("ifo_read");
    	return -1;
    }

	return read(fd, data, count); 
}

#define OFF_PTT get4bytes (ifo->data[ID_MAT] + 0xC8)
#define OFF_TITLE_PGCI get4bytes (ifo->data[ID_MAT] + 0xCC)
#define OFF_MENU_PGCI get4bytes (ifo->data[ID_MAT] + 0xD0)
#define OFF_TMT get4bytes (ifo->data[ID_MAT] + 0xD4)
#define OFF_MENU_CELL_ADDR get4bytes (ifo->data[ID_MAT] + 0xD8)
#define OFF_MENU_VOBU_ADDR_MAP get4bytes (ifo->data[ID_MAT] + 0xDC)
#define OFF_TITLE_CELL_ADDR get4bytes (ifo->data[ID_MAT] + 0xE0)
#define OFF_TITLE_VOBU_ADDR_MAP get4bytes (ifo->data[ID_MAT] + 0xE4)

#define OFF_VMG_TSP get4bytes (ifo->data[ID_MAT] + 0xC4)
#define OFF_VMG_MENU_PGCI get4bytes (ifo->data[ID_MAT] + 0xC8)
#define OFF_VMG_TMT get4bytes (ifo->data[ID_MAT] + 0xD0)


static int ifo_vts(ifo_t *ifo)
{
    if(!strncmp((char*)ifo->data[ID_MAT], "DVDVIDEO-VTS", 12)) 
		return 0;

	return -1;
}


static int ifo_vmg(ifo_t *ifo)
{
    if(!strncmp((char*)ifo->data[ID_MAT], "DVDVIDEO-VMG", 12))
		return 0;

	return -1;
}

static int ifo_table(ifo_t *ifo, int64_t offset, unsigned long tbl_id)
{
	unsigned char *data;
	int64_t len = 0;
	int i;
	u_int32_t *ptr;

	if(!offset) return -1;

	data = (u_char *)calloc(1, DVD_VIDEO_LB_LEN);

	if(ifo_read(ifo->fd, ifo->pos + offset * DVD_VIDEO_LB_LEN, DVD_VIDEO_LB_LEN, data) <= 0) 
	{
		perror("ifo_table");
		return -1;
	}

	switch(tbl_id)
	{
		case ID_TITLE_VOBU_ADDR_MAP:
		case ID_MENU_VOBU_ADDR_MAP:
			len = get4bytes(data) + 1;
			break;

		default: 
		{
			ifo_hdr_t *hdr = (ifo_hdr_t *)data;
			len = bswap_32(hdr->len) + 1;
		}
	}

	if(len > DVD_VIDEO_LB_LEN) 
	{
		data = (u_char *)realloc((void *)data, len);
		bzero(data, len);
		ifo_read(ifo->fd, ifo->pos + offset * DVD_VIDEO_LB_LEN, len, data);
	}

	ifo->data[tbl_id] = data;
	ptr = (u_int32_t*)data;
	len /= 4;

	if(tbl_id == ID_TMT) 
		for (i = 0; i < len; i++)
			ptr[i] = bswap_32(ptr[i]);

	return 0;
}

static ifo_t* ifo_open(int fd, long pos)
{
	ifo_t *ifo;

	ifo = (ifo_t *)calloc(sizeof(ifo_t), 1);

	ifo->data[ID_MAT] = (unsigned char *)calloc(DVD_VIDEO_LB_LEN, 1);

	ifo->pos = pos; 
	ifo->fd = fd;

	if(ifo_read(fd, pos, DVD_VIDEO_LB_LEN, ifo->data[ID_MAT]) < 0) 
	{
		perror("ifo_open");
		free(ifo->data[ID_MAT]);
		free(ifo);
		return NULL;
	}

	ifo->num_menu_vobs	= get4bytes(ifo->data[ID_MAT] + 0xC0);
	ifo->vob_start		= get4bytes(ifo->data[ID_MAT] + 0xC4);

#ifdef DEBUG
	printf ("num of vobs: %x vob_start %x\n", ifo->num_menu_vobs, ifo->vob_start);
#endif

	if(!ifo_vts(ifo)) 
	{
		ifo_table(ifo, OFF_PTT, ID_PTT);
    	ifo_table(ifo, OFF_TITLE_PGCI, ID_TITLE_PGCI);
		ifo_table(ifo, OFF_MENU_PGCI, ID_MENU_PGCI);
		ifo_table(ifo, OFF_TMT, ID_TMT);
		ifo_table(ifo, OFF_MENU_CELL_ADDR, ID_MENU_CELL_ADDR);
		ifo_table(ifo, OFF_MENU_VOBU_ADDR_MAP, ID_MENU_VOBU_ADDR_MAP);
    	ifo_table(ifo, OFF_TITLE_CELL_ADDR, ID_TITLE_CELL_ADDR);
		ifo_table(ifo, OFF_TITLE_VOBU_ADDR_MAP, ID_TITLE_VOBU_ADDR_MAP);
	} 
	else 
	if(!ifo_vmg(ifo)) 
	{
		ifo_table(ifo, OFF_VMG_TSP, ID_TSP);
		ifo_table(ifo, OFF_VMG_MENU_PGCI, ID_MENU_PGCI);
		ifo_table(ifo, OFF_VMG_TMT, ID_TMT);
		ifo_table(ifo, OFF_TITLE_CELL_ADDR, ID_TITLE_CELL_ADDR);
		ifo_table(ifo, OFF_TITLE_VOBU_ADDR_MAP, ID_TITLE_VOBU_ADDR_MAP);
	}

	return ifo;
}


static int ifo_close(ifo_t *ifo)
{
	if(ifo->data[ID_MAT]) free(ifo->data[ID_MAT]);
	if(ifo->data[ID_PTT]) free(ifo->data[ID_PTT]);
	if(ifo->data[ID_TITLE_PGCI]) free(ifo->data[ID_TITLE_PGCI]);
	if(ifo->data[ID_MENU_PGCI]) free(ifo->data[ID_MENU_PGCI]);
	if(ifo->data[ID_TMT]) free(ifo->data[ID_TMT]);
	if(ifo->data[ID_MENU_CELL_ADDR]) free(ifo->data[ID_MENU_CELL_ADDR]);
	if(ifo->data[ID_MENU_VOBU_ADDR_MAP]) free(ifo->data[ID_MENU_VOBU_ADDR_MAP]);
	if(ifo->data[ID_TITLE_CELL_ADDR]) free(ifo->data[ID_TITLE_CELL_ADDR]);
	if(ifo->data[ID_TITLE_VOBU_ADDR_MAP]) free(ifo->data[ID_TITLE_VOBU_ADDR_MAP]);

	free(ifo);

	return 0;
}

static int ifo_audio(char *_hdr, char **ptr)
{
	audio_hdr_t *hdr = (audio_hdr_t *)_hdr;

	if(!_hdr) return -1;

	*ptr = _hdr + AUDIO_HDR_LEN;

	return bswap_16(hdr->num);
}


static int pgci(ifo_hdr_t *hdr, int title, char **ptr)
{
	pgci_sub_t *pgci_sub;

	*ptr = (char *) hdr;

	if(!*ptr) return -1;

	if(title > hdr->num) return -1;

	*ptr += IFO_HDR_LEN;

	pgci_sub = (pgci_sub_t *)*ptr + title;

	*ptr = (char *)hdr + bswap_32(pgci_sub->start);

	return 0;
}

static int program_map(mpeg3_t *file, char *pgc, unsigned char **ptr)
{
	int num;
	int i;
	*ptr = pgc;

	if (!pgc)
		return -1;

	*ptr += 2;	
	num = **ptr;

	*ptr += 10;
	*ptr += 8 * 2;			// AUDIO
	*ptr += 32 * 4;			// SUBPICTURE
	*ptr += 8;
// subtitle color palette
//	*ptr += 16 * PGCI_COLOR_LEN;

	if(!file->have_palette)
	{
		for(i = 0; i < 16; i++)
		{
			int r = (int)*(*ptr)++;
			int g = (int)*(*ptr)++;
			int b = (int)*(*ptr)++;
			(*ptr)++;

			int y = (int)(0.29900 * r  + 0.58700 * g  + 0.11400 * b);
			int u = (int)(-0.16874 * r + -0.33126 * g + 0.50000 * b + 0x80);
			int v = (int)(0.50000 * r  + -0.41869 * g + -0.08131 * b + 0x80);
			CLAMP(y, 0, 0xff);
			CLAMP(u, 0, 0xff);
			CLAMP(v, 0, 0xff);

			file->palette[i * 4] = y;
			file->palette[i * 4 + 1] = u;
			file->palette[i * 4 + 2] = v;
//printf("color %02d: 0x%02x 0x%02x 0x%02x\n", i, y, u, v);
		}

		file->have_palette = 1;
		

	}
	else
	{
		(*ptr) += 16 * 4;
	}

	*ptr += 2;

	*ptr = get2bytes((unsigned char*)*ptr) + pgc;

	return num;
}


static u_int get_cellplayinfo(u_char *pgc, u_char **ptr)
{
	u_int num;
	*ptr = pgc;

	if (!pgc)
		return -1;

	*ptr += 3;	
	num = **ptr;

	*ptr += 9;
	*ptr += 8 * 2;			// AUDIO
	*ptr += 32 * 4;			// SUBPICTURE
	*ptr += 8;
	*ptr += 16 * PGCI_COLOR_LEN;	// CLUT
	*ptr += 4;

	*ptr =  get2bytes(*ptr) + pgc;

	return num;
}

static void get_ifo_playlist(mpeg3_t *file, mpeg3_demuxer_t *demuxer)
{
	DIR *dirstream;
	char directory[MPEG3_STRLEN];
	char filename[MPEG3_STRLEN];
	char complete_path[MPEG3_STRLEN];
	char title_path[MPEG3_STRLEN];
	char vob_prefix[MPEG3_STRLEN];
	struct dirent *new_filename;
	char *ptr;
	int64_t total_bytes = 0;
	int done = 0, i;

// Get titles matching ifo file
	mpeg3io_complete_path(complete_path, file->fs->path);
	mpeg3io_get_directory(directory, complete_path);
	mpeg3io_get_filename(filename, complete_path);
	strncpy(vob_prefix, filename, 6);

	dirstream = opendir(directory);
	while(new_filename = readdir(dirstream))
	{
		if(!strncasecmp(new_filename->d_name, vob_prefix, 6))
		{
			ptr = strrchr(new_filename->d_name, '.');
			if(ptr && !strncasecmp(ptr, ".vob", 4))
			{
// Got a title
				if(atol(&new_filename->d_name[7]) > 0)
				{
					mpeg3_title_t *title;

					mpeg3io_joinpath(title_path, directory, new_filename->d_name);
					title = demuxer->titles[demuxer->total_titles++] = 
						mpeg3_new_title(file, title_path);
					title->total_bytes = mpeg3io_path_total_bytes(title_path);
					title->start_byte = total_bytes;
					title->end_byte = total_bytes + title->total_bytes;
					total_bytes += title->total_bytes;
					
					mpeg3_new_cell(title, 
						0, 
						title->end_byte,
						0,
						title->end_byte,
						0);
	
//printf("%s\n", title_path);
				}
			}
		}
	}
	closedir(dirstream);


// Alphabetize titles.  Only problematic for guys who rip entire DVD's
// to their hard drives while retaining the file structure.
	while(!done)
	{
		done = 1;
		for(i = 0; i < demuxer->total_titles - 1; i++)
		{
			if(strcmp(demuxer->titles[i]->fs->path, demuxer->titles[i + 1]->fs->path) > 0)
			{
				mpeg3_title_t *temp = demuxer->titles[i];
				demuxer->titles[i] = demuxer->titles[i + 1];
				demuxer->titles[i + 1] = temp;
				done = 0;
			}
		}
	}
	
	
}




// IFO parsing
static void get_ifo_header(mpeg3_demuxer_t *demuxer, ifo_t *ifo)
{
	int i;
// Video header
	demuxer->vstream_table[0] = 1;

// Audio header
	if(!ifo_vts(ifo))
	{
		ifo_audio_t *audio;
		int result = 0;
// Doesn't detect number of tracks.
		int atracks = ifo_audio((char*)ifo->data[ID_MAT] + IFO_OFFSET_AUDIO, (char**)&audio);
		int atracks_empirical = 0;

// Collect stream id's
#define TEST_START 0x1000000
#define TEST_LEN   0x1000000
		mpeg3demux_open_title(demuxer, 0);
		mpeg3demux_seek_byte(demuxer, TEST_START);
		while(!result && 
			!mpeg3demux_eof(demuxer) && 
			mpeg3demux_tell_byte(demuxer) < TEST_START + TEST_LEN)
		{
			result = mpeg3_read_next_packet(demuxer);
		}
		mpeg3demux_seek_byte(demuxer, 0);

		for(i = 0; i < MPEG3_MAX_STREAMS; i++)
		{
			if(demuxer->astream_table[i]) atracks_empirical++;
		}

// Doesn't detect PCM audio or total number of tracks
/*
 * 		if(atracks && !atracks_empirical)
 * 			for(i = 0; i < atracks; i++)
 * 			{
 * 				int audio_mode = AUDIO_AC3;
 * 				switch(audio->coding_mode)
 * 				{
 * 					case 0: audio_mode = AUDIO_AC3;  break;
 * 					case 1: audio_mode = AUDIO_MPEG; break;
 * 					case 2: audio_mode = AUDIO_MPEG; break;
 * 					case 3: audio_mode = AUDIO_PCM;  break;
 * 				}
 * 				if(!demuxer->astream_table[i + 0x80]) demuxer->astream_table[i + 0x80] = audio_mode;
 * 			}
 */
	}
	else
	if(!ifo_vmg(ifo))
	{
	}
}

static mpeg3ifo_cell_t* append_cell(mpeg3ifo_celltable_t *table)
{
	if(!table->cells || table->total_cells >= table->cells_allocated)
	{
		long new_allocation;
		mpeg3ifo_cell_t *new_cells;

		new_allocation = table->cells_allocated ? table->cells_allocated * 2 : 64;
		new_cells = calloc(1, sizeof(mpeg3ifo_cell_t) * new_allocation);
		if(table->cells)
		{
			memcpy(new_cells, table->cells, sizeof(mpeg3ifo_cell_t) * table->total_cells);
			free(table->cells);
		}
		table->cells = new_cells;
		table->cells_allocated = new_allocation;
	}

	return &table->cells[table->total_cells++];
}

static void delete_celltable(mpeg3ifo_celltable_t *table)
{
	if(table->cells) free(table->cells);
	free(table);
}

static void cellplayinfo(mpeg3_t *file, ifo_t *ifo, mpeg3ifo_celltable_t *cells)
{
	int i, j;
	char *cell_hdr, *cell_hdr_start, *cell_info;
	ifo_hdr_t *hdr = (ifo_hdr_t*)ifo->data[ID_TITLE_PGCI];
	int program_chains = bswap_16(hdr->num);
	long total_cells;

//printf("cellplayinfo\n");
	for(j = 0; j < program_chains; j++)
	{
// Cell header
// Program Chain Information
		pgci(hdr, j, &cell_hdr);

		cell_hdr_start = cell_hdr;
// Unknown
		cell_hdr += 2;
// Num programs
		cell_hdr += 2;
// Chain Time
		cell_hdr += 4;
// Unknown
		cell_hdr += 4;
// Subaudio streams
		for(i = 0; i < 8; i++) cell_hdr += 2;
// Subpictures
		for(i = 0; i < 32; i++) cell_hdr += 4;
// Unknown
		for(i = 0; i < 8; i++) cell_hdr++;
// Skip CLUT
// Skip PGC commands
// Program map
		if(program_map(file, cell_hdr_start, &cell_hdr))
			;

// Cell Positions
		if(total_cells = get_cellplayinfo((unsigned char*)cell_hdr_start, (unsigned char**)&cell_hdr))
		{
//printf("cellplayinfo %d %d\n", j, total_cells);
			cell_info = cell_hdr;
			for(i = 0; i < total_cells; i++)
			{
				ifo_pgci_cell_addr_t *cell_addr = (ifo_pgci_cell_addr_t *)cell_info;
				int64_t start_byte = bswap_32(cell_addr->vobu_start);
				int64_t end_byte = bswap_32(cell_addr->vobu_last_end);
				int cell_type = cell_addr->chain_info;

				if(!cells->total_cells && start_byte > 0)
					start_byte = 0;

				if(!cells->total_cells ||
					end_byte >= cells->cells[cells->total_cells - 1].end_byte)
				{
					mpeg3ifo_cell_t *cell = append_cell(cells);

					cell->start_byte = start_byte;
					cell->end_byte = end_byte;
					cell->cell_type = cell_type;
//printf("cellplayinfo start: %llx end: %llx type: %x\n", 
//	(int64_t)cell->start_byte * 0x800, (int64_t)cell->end_byte * 0x800, cell->cell_type);
				}
				cell_info += PGCI_CELL_ADDR_LEN;
			}
		}
	}
}

static void celladdresses(ifo_t *ifo, mpeg3ifo_celltable_t *cell_addresses)
{
	int i;
	char *ptr = (char*)ifo->data[ID_TITLE_CELL_ADDR];
	int total_addresses;
	cell_addr_hdr_t *cell_addr_hdr = (cell_addr_hdr_t*)ptr;
	ifo_cell_addr_t *cell_addr = (ifo_cell_addr_t*)(ptr + CADDR_HDR_LEN);
	int done = 0;
//printf("celladdresses\n");

	if(total_addresses = bswap_32(cell_addr_hdr->len) / sizeof(ifo_cell_addr_t))
	{
		for(i = 0; i < total_addresses; i++)
		{
			mpeg3ifo_cell_t *cell;
			cell = append_cell(cell_addresses);
			cell->start_byte = (int64_t)bswap_32(cell_addr->start);
			cell->end_byte = (int64_t)bswap_32(cell_addr->end);
			cell->vob_id = bswap_16(cell_addr->vob_id);
			cell->cell_id = cell_addr->cell_id;
			cell_addr++;
		}
	}

// Sort addresses by address instead of vob id
	done = 0;
	while(!done)
	{
		done = 1;
		for(i = 0; i < total_addresses - 1; i++)
		{
			mpeg3ifo_cell_t *cell1, *cell2;
			cell1 = &cell_addresses->cells[i];
			cell2 = &cell_addresses->cells[i + 1];

			if(cell1->start_byte > cell2->start_byte)
			{
				mpeg3ifo_cell_t temp = *cell1;
				*cell1 = *cell2;
				*cell2 = temp;
				done = 0;
				break;
			}
		}
	}
	
	for(i = 0; i < total_addresses; i++)
	{
		mpeg3ifo_cell_t *cell = &cell_addresses->cells[i];
	}
}

static void finaltable(mpeg3ifo_celltable_t *final_cells, 
		mpeg3ifo_celltable_t *cells, 
		mpeg3ifo_celltable_t *cell_addresses)
{
	int input_cell = 0, current_address = 0;
	int output_cell = 0;
	int done;
	int i, j;
	int current_vobid;
// Start and end bytes of programs
	int64_t program_start_byte[256], program_end_byte[256];

	final_cells->total_cells = 0;
	final_cells->cells_allocated = cell_addresses->total_cells;
	final_cells->cells = calloc(1, sizeof(mpeg3ifo_cell_t) * final_cells->cells_allocated);

// Assign programs to cells
	current_vobid = -1;
	for(i = cell_addresses->total_cells - 1; i >= 0; i--)
	{
		mpeg3ifo_cell_t *input = &cell_addresses->cells[i];
		mpeg3ifo_cell_t *output = &final_cells->cells[i];
		if(current_vobid < 0) current_vobid = input->vob_id;

		*output = *input;
// Reduce current vobid
		if(input->vob_id < current_vobid)
			current_vobid = input->vob_id;
		else
// Get the current program number
		if(input->vob_id > current_vobid)
		{
			int current_program = input->vob_id - current_vobid;
			output->program = current_program;

// Get the last interleave by brute force
			for(j = i; 
				j < cell_addresses->total_cells && cell_addresses->cells[i].cell_id == cell_addresses->cells[j].cell_id; 
				j++)
			{
				int new_program = final_cells->cells[j].vob_id - current_vobid;
				if(new_program <= current_program)
					final_cells->cells[j].program = new_program;
			}
		}

		final_cells->total_cells++;
	}

// Expand byte position and remove duplicates
	for(i = 0; i < final_cells->total_cells; i++)
	{
		if(i < final_cells->total_cells - 1 &&
			final_cells->cells[i].start_byte == final_cells->cells[i + 1].start_byte)
		{
			for(j = i; j < final_cells->total_cells - 1; j++)
				final_cells->cells[j] = final_cells->cells[j + 1];

			final_cells->total_cells--;
		}

		final_cells->cells[i].start_byte *= (int64_t)2048;
		final_cells->cells[i].end_byte *= (int64_t)2048;
// End index seems to be inclusive
		final_cells->cells[i].end_byte += 2048;
	}

return;
// Debug
	printf("finaltable\n");
	for(i = 0; i < final_cells->total_cells; i++)
	{
		printf(" vob id: %x cell id: %x start: %llx end: %llx program: %x\n", 
			final_cells->cells[i].vob_id, final_cells->cells[i].cell_id, (int64_t)final_cells->cells[i].start_byte, (int64_t)final_cells->cells[i].end_byte, final_cells->cells[i].program);
	}
}




/* Read the title information from an ifo */
int mpeg3_read_ifo(mpeg3_t *file, 
	int read_cells)
{
	int64_t last_ifo_byte = 0, first_ifo_byte = 0;
	mpeg3ifo_celltable_t *cells, *cell_addresses, *final_cells;
	mpeg3_demuxer_t *demuxer = file->demuxer;
	int current_title = 0, current_cell = 0;
	int i, j;
	ifo_t *ifo;
    int fd = mpeg3io_get_fd(file->fs);
	int64_t title_start_byte = 0;
	int result;

	if(!(ifo = ifo_open(fd, 0)))
	{
		fprintf(stderr, "read_ifo: Error decoding ifo.\n");
		return 1;
	}

	demuxer->read_all = 1;
	cells = calloc(1, sizeof(mpeg3ifo_celltable_t));
	cell_addresses = calloc(1, sizeof(mpeg3ifo_celltable_t));
	final_cells = calloc(1, sizeof(mpeg3ifo_celltable_t));

	get_ifo_playlist(file, demuxer);
	get_ifo_header(demuxer, ifo);
	cellplayinfo(file, ifo, cells);
	celladdresses(ifo, cell_addresses);
	finaltable(final_cells, 
		cells, 
		cell_addresses);

// Get maximum program for program_bytes table
	int total_programs = 0;
	if(final_cells)
	{
		for(i = 0; i < final_cells->total_cells; i++)
		{
			mpeg3ifo_cell_t *cell = &final_cells->cells[i];
			if(cell->program > total_programs - 1)
				total_programs = cell->program + 1;
		}
	}
	int64_t *program_bytes = calloc(total_programs, sizeof(int64_t));


// Clear out old cells
	for(i = 0; i < demuxer->total_titles; i++)
	{
		mpeg3_title_t *title = demuxer->titles[i];
		if(title->cell_table)
		{
			for(j = 0; j < title->cell_table_size; j++)
			{
				free(title->cell_table);
				title->cell_table = 0;
			}
		}
	}


// Assign new cells to titles
	while(final_cells && current_cell < final_cells->total_cells)
	{
		mpeg3_title_t *title;
		mpeg3ifo_cell_t *cell;
		int64_t cell_start, cell_end;
		int64_t length = 1;

		title = demuxer->titles[current_title];
		cell = &final_cells->cells[current_cell];
		cell_start = cell->start_byte;
		cell_end = cell->end_byte;

// Cell may be split by a title so handle in fragments.
		while(cell_start < cell_end && length > 0)
		{
			length = cell_end - cell_start;

// Clamp length to end of current title
			if(cell_start + length - title_start_byte > title->total_bytes)
				length = title->total_bytes - cell_start + title_start_byte;

//printf("%llx %llx %llx %llx\n", cell_end, cell_start, title_start_byte, length);

// Should never fail.  If it does it means the length of the cells and the
// length of the titles don't match.  The title lengths must match or else
// the cells won't line up.
			if(length > 0)
			{
				int64_t program_start = program_bytes[cell->program];
				int64_t program_end = program_start + length;
				int64_t title_start = cell_start - title_start_byte;
				int64_t title_end = title_start + length;
				mpeg3_new_cell(title, 
					program_start, 
					program_end,
					title_start,
					title_end,
					cell->program);
				cell_start += length;
				program_bytes[cell->program] += length;
			}
			else
			{
				fprintf(stderr, 
					"read_ifo: cell length and title length don't match! title=%d cell=%d cell_start=%llx cell_end=%llx.\n",
					current_title,
					current_cell,
					cell_start - title_start_byte,
					cell_end - title_start_byte);

// Try this out.  It works for Contact where one VOB is 0x800 bytes longer than 
// the cells in it but the next cell aligns perfectly with the next VOB.
				if(current_title < demuxer->total_titles - 1) current_cell--;
			}

// Advance title
			if(cell_start - title_start_byte >= title->total_bytes && 
				current_title < demuxer->total_titles - 1)
			{
				title_start_byte += title->total_bytes;
				title = demuxer->titles[++current_title];
			}
		}
		current_cell++;
	}

	free(program_bytes);
	delete_celltable(cells);
	delete_celltable(cell_addresses);
	delete_celltable(final_cells);
	ifo_close(ifo);
	return 0;
}

