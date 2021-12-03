// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru at gmail dot com>

#include <libgen.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bcsignals.h"
#include "filebase.h"
#include "filetoc.h"
#include "indexfile.h"
#include "language.h"
#include "mwindow.h"
#include "mainerror.h"
#include "mainprogress.h"


void FileTOC::put_int32(uint32_t x)
{
	if(fwrite(&x, sizeof(uint32_t), 1, tocfile) != 1)
		fileio_err = 1;
}

void FileTOC::put_int64(uint64_t x)
{
	if(fwrite(&x, sizeof(uint64_t), 1, tocfile) != 1)
		fileio_err = 1;
}

uint32_t FileTOC::read_int32()
{
	uint32_t temp;

	if(fread(&temp, sizeof(uint32_t), 1, tocfile) != 1)
	{
		fileio_err = 1;
		return 0;
	}
	return temp;
}

uint64_t FileTOC::read_int64()
{
	uint64_t temp;

	if(fread(&temp, sizeof(uint64_t), 1, tocfile) != 1)
	{
		fileio_err = 1;
		return 0;
	}
	return temp;
}

FileTOC::FileTOC(FileBase *media, const char *dirpath, const char *filepath,
		off_t length, time_t mtime)
{
	char source_filename[BCTEXTLEN];
	char *p;
	this->media = media;
	this->length = length;
	this->mtime = mtime;
	this->filepath = filepath;
	fileio_err = 0;
	progress = 0;

	IndexFile::get_index_filename(source_filename, dirpath,
		toc_filename, filepath, -1, TOCFILE_EXTENSION);

	items_allocated = 0;
	items = 0;
	num_streams = 0;
	memset(toc_streams, 0, sizeof(toc_streams));
}

FileTOC::~FileTOC()
{
	if(items)
		delete items;
	for(int i = 0; i < num_streams; i++)
	{
		if(toc_streams[i].items)
			delete [] toc_streams[i].items;
	}
}

int FileTOC::init_tocfile(int ftype)
{
	int res, i, j, k, streams;
	uint32_t *ptr;
	uint64_t val1, val2;

	if(!((tocfile = fopen(toc_filename, "r")) && (i = read_int32()) == TOCFILE_PREFIX &&
			(j = read_int32()) == TOCFILE_VERSION &&
			(file_type = read_int32()) == ftype))
	{
		fileio_err = 0;
		if(tocfile)
		{
			fclose(tocfile);

			if(confirmbox(_("TOC file already exists.\nOverwrite?")))
				return 1;
			unlink(toc_filename);
		}
reindex:
		if((tocfile = fopen(toc_filename, "w")) == NULL)
		{
			errorbox("Unable to create tocfile: %m");
			return 1;
		}
		canceled = 0;
		put_int32(TOCFILE_PREFIX);
		put_int32(TOCFILE_VERSION);
		put_int32(ftype);
		put_int32(TOCFILE_INFO);
		put_int64(mtime);
		put_int64(length);
		j = strlen(filepath) + 4;
		j &= ~3;
		put_int32(j);
		k = fprintf(tocfile, "%s", filepath);
		for(; k < j; k++)
			fputc(0, tocfile);
		// Stream IDs: stream_type, stream_id
		if(streams = media->get_streamcount())
		{
			if(mwindow_global && length > TOC_PROGRESS_LENGTH)
			{
				canceled = 0;
				char fnbuf[BCTEXTLEN];
				strcpy(fnbuf, filepath);
				progress = mwindow_global->mainprogress->start_progress(_("Generating TOC"), streams * length);
				progress->update_title(_("Generating TOC for %s"), basename(fnbuf));
			}
			put_int32(TOCFILE_STREAMCOUNT);
			put_int32(streams);
			// New data fields put here
			// Stream data
			for(i = 0; i < streams; i++)
			{
				stream_params *td;
				max_items = 0;
				streamnum = i;
				if(td = media->get_track_data(i))
				{
					if(canceled)
						goto toc_canceled;
					put_int32(TOCFILE_STREAM);
					put_int32(td->type);
					put_int32(td->id);
					put_int32(td->layers);
					put_int32(td->rate_num);
					put_int32(td->rate_denom);
					put_int32(td->data1);
					put_int32(td->data2);
					put_int64(td->min_index);
					put_int64(td->max_index);
					put_int64(td->min_offset);
					put_int64(td->max_offset);
					put_int32(max_items);
					for(j = 0; j < max_items; j++)
					{
						put_int64(items[j].index + td->index_correction);
						put_int64(items[j].offset);
					}
				}
				if(canceled)
					goto toc_canceled;
			}
			if(progress)
			{
				progress->stop_progress();
				delete progress;
				progress = 0;
			}
		}
		media->toc_is_made(canceled); // cleanup in media
		items_allocated = 0;
		delete items;
		items = 0;
		if(fclose(tocfile) || fileio_err)
		{
			strcpy(toc_filename, filepath);
			errorbox(_("Failed to write tocfile for '%s'"), basename(toc_filename));
			return 1;
		}
		if(!(tocfile = fopen(toc_filename, "r")))
		{
			errorbox("Can't reopen TOC file");
			return 1;
		}
		read_int32(); // prefix
		read_int32(); // version
		file_type = read_int32();
	}

	memset(toc_streams, 0, sizeof(toc_streams));
	num_streams = 0;

	for(;;)
	{
		k = read_int32();
		if(fileio_err)
			goto toc_eof;

		switch(k)
		{
		case TOCFILE_INFO:
			val1 = read_int64();
			val2 = read_int64();

			if(fileio_err)
				goto toc_err;
			if(val1 != mtime || val2 != length)
			{
				fclose(tocfile);
				unlink(toc_filename);
				fileio_err = 0;
				goto reindex;
			}
			i = read_int32();
			// skip filepath
			if(fseeko(tocfile, i, SEEK_CUR))
				goto toc_err;
			break;
		case TOCFILE_STREAMCOUNT:
			streams = read_int32();
			if(fileio_err)
				goto toc_err;
			break;
		case TOCFILE_STREAM:
			toc_streams[num_streams].type = read_int32();
			toc_streams[num_streams].id = read_int32();
			toc_streams[num_streams].layers = read_int32();
			toc_streams[num_streams].rate_num = read_int32();
			toc_streams[num_streams].rate_denom = read_int32();
			toc_streams[num_streams].data1 = read_int32();
			toc_streams[num_streams].data2 = read_int32();
			toc_streams[num_streams].min_index = read_int64();
			toc_streams[num_streams].max_index = read_int64();
			toc_streams[num_streams].min_offset = read_int64();
			toc_streams[num_streams].max_offset = read_int64();
			max_items = toc_streams[num_streams].max_items = read_int32();
			toc_streams[num_streams].toc_start = ftello(tocfile);
			if(fileio_err)
				goto toc_err;
			// skip items
			for(i = 0; i < max_items; i++)
			{
				read_int64();
				read_int64();
			}
			if(fileio_err)
				goto toc_err;
			num_streams++;
			break;
		default:
			errorbox("Failed to decode TOC.\nFile corrupted?");
			goto toc_fail;
		}
	}
toc_err:
	errorbox("Unexpected error while loading TOC.\nFile corrupted?");

toc_fail:
	fclose(tocfile);
	return 1;

toc_canceled:
	fclose(tocfile);
	if(progress)
	{
		progress->stop_progress();
		delete progress;
		progress = 0;
	}
	if(items)
	{
		delete items;
		max_items = 0;
		items = 0;
	}
	unlink(toc_filename);
	media->toc_is_made(canceled); // cleanup in media
	return 1;

toc_eof:
	if(num_streams != streams)
		goto toc_err;
	fclose(tocfile);
	return 0;
}

int FileTOC::append_item(posnum index, off_t offset, off_t mdoffs)
{
	int itmx;

	if(items_allocated == 0)
	{
		items = new stream_item[ITEM_BLOCK];
		items_allocated = ITEM_BLOCK;
	}
	if(max_items >= items_allocated)
	{
		int new_alloc = items_allocated + ITEM_BLOCK;
		stream_item *new_itms = new stream_item[new_alloc];
		memcpy(new_itms, items,
			items_allocated * sizeof(stream_item));
		delete [] items;
		items = new_itms;
		items_allocated = new_alloc;
	}
	// keep indexes in order
	for(itmx = max_items - 1; itmx >= 0 && items[itmx].index > index; itmx--);
	itmx++;
	if(itmx < max_items - 1)
	{
		// Not good if we need to reorder here
		for(int i = max_items; i > itmx; i--)
			items[i] = items[i - 1];
		items[itmx].index = index;
		items[itmx].offset = offset;
	}
	else
	{
		items[max_items].index = index;
		items[max_items].offset = offset;
	}
	if(progress && mdoffs > 0)
		canceled = progress->update(mdoffs + streamnum * length);

	max_items++;
	return canceled;
}

stream_item *FileTOC::get_item(int stream, posnum ix, stream_item **nxitem)
{
	int j;

	*nxitem = 0;
	if(stream < 0 || stream >= num_streams)
		return 0;

	if(toc_streams[stream].items == 0)
	{
		if((tocfile = fopen(toc_filename, "r")) == NULL)
		{
			toc_streams[stream].max_items = 0;
			return 0;
		}
		fileio_err = 0;
		fseeko(tocfile, toc_streams[stream].toc_start, SEEK_SET);
		max_items = toc_streams[stream].max_items;
		toc_streams[stream].items = new stream_item[max_items];
		for(int i = 0; i < max_items; i++)
		{
			toc_streams[stream].items[i].index = read_int64();
			toc_streams[stream].items[i].offset = read_int64();
		}
		fclose(tocfile);
		tocfile = 0;
		if(fileio_err)
		{
			delete toc_streams[stream].items;
			toc_streams[stream].items = 0;
			toc_streams[stream].max_items = 0;
			return 0;
		}
	}

	if(ix < toc_streams[stream].min_index)
	{
		if(toc_streams[stream].max_items > 1)
			*nxitem = &toc_streams[stream].items[1];
		return &toc_streams[stream].items[0];
	}
	if(ix >= toc_streams[stream].max_index)
		return &toc_streams[stream].items[toc_streams[stream].max_items - 1];

	for(j = 1; j < toc_streams[stream].max_items; j++)
	{
		if(toc_streams[stream].items[j].index > ix)
		{
			if(j < toc_streams[stream].max_items)
				*nxitem = &toc_streams[stream].items[j];
			return &toc_streams[stream].items[j - 1];
		}
	}
	return &toc_streams[stream].items[j - 1];
}

int FileTOC::id_to_index(int id)
{
	int i;

	for(i = 0; i < num_streams; i++)
	{
		if(toc_streams[i].id == id)
			return i;
	}
	return -1;
}

void FileTOC::dump(int offsets)
{
	printf("FileTOC %p dump\n", this);
	printf("  toc: %s\n", toc_filename);
	printf("  file_type %d streams %d\n", file_type, num_streams);
	for(int i = 0; i < num_streams; i++)
	{
		printf("  Stream %d: type %d id %d layers %d\n", i,
			toc_streams[i].type, toc_streams[i].id, toc_streams[i].layers);
		printf("    rate num %d, denom %d, data1 %d data2 %d corr %" PRId64 "\n",
			toc_streams[i].rate_num, toc_streams[i].rate_denom,
			toc_streams[i].data1, toc_streams[i].data2, toc_streams[i].index_correction);
		printf("    index %" PRId64 "..%" PRId64 " offset %" PRId64 "..%" PRId64 ", toc start %" PRId64 " max_items %d items %p\n",
			toc_streams[i].min_index, toc_streams[i].max_index,
			toc_streams[i].min_offset, toc_streams[i].max_offset,
			toc_streams[i].toc_start, toc_streams[i].max_items,
			toc_streams[i].items);
		if(offsets && toc_streams[i].items)
		{
			for(int j = 0; j < toc_streams[i].max_items; j++)
				printf("      %5d. %6" PRId64 " : %8" PRId64 "\n", j,
					toc_streams[i].items[j].index,
					toc_streams[i].items[j].offset);
		}
	}
}