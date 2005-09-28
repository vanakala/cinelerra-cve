#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static uint32_t read_int32(unsigned char *buffer, int *position)
{
	uint32_t temp;

	if(MPEG3_LITTLE_ENDIAN)
	{
		((unsigned char*)&temp)[3] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[0] = buffer[(*position)++];
	}
	else
	{
		((unsigned char*)&temp)[0] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[3] = buffer[(*position)++];
	}
	
	return temp;
}

static uint64_t read_int64(unsigned char *buffer, int *position)
{
	uint64_t temp;

	if(MPEG3_LITTLE_ENDIAN)
	{
		((unsigned char*)&temp)[7] = buffer[(*position)++];
		((unsigned char*)&temp)[6] = buffer[(*position)++];
		((unsigned char*)&temp)[5] = buffer[(*position)++];
		((unsigned char*)&temp)[4] = buffer[(*position)++];
		((unsigned char*)&temp)[3] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[0] = buffer[(*position)++];
	}
	else
	{
		((unsigned char*)&temp)[0] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[3] = buffer[(*position)++];
		((unsigned char*)&temp)[4] = buffer[(*position)++];
		((unsigned char*)&temp)[5] = buffer[(*position)++];
		((unsigned char*)&temp)[6] = buffer[(*position)++];
		((unsigned char*)&temp)[7] = buffer[(*position)++];
	}

	return temp;
}

static int read_data(unsigned char *buffer, 
	int *position, 
	unsigned char *output, 
	int bytes)
{
	memcpy(output, buffer + *position, bytes);
	*position += bytes;
}

int mpeg3_read_toc(mpeg3_t *file, int *atracks_return, int *vtracks_return)
{
	unsigned char *buffer;
	int file_type;
	int position = 4;
	int stream_type;
	int i, j;
	int is_vfs = 0;
	int vfs_len = strlen(RENDERFARM_FS_PREFIX);
	int toc_version;
	int64_t current_byte = 0;

//printf("read_toc 1\n");
// Fix title paths for Cinelerra VFS
	if(!strncmp(file->fs->path, RENDERFARM_FS_PREFIX, vfs_len))
		is_vfs = 1;

	buffer = malloc(mpeg3io_total_bytes(file->fs));
	mpeg3io_seek(file->fs, 0);
	mpeg3io_read_data(buffer, mpeg3io_total_bytes(file->fs), file->fs);

// Test version
	if((toc_version = buffer[position++]) != MPEG3_TOC_VERSION)
	{
		fprintf(stderr,
			"read_toc: invalid TOC version %x\n", 
			toc_version);
		return 1;
	}

//printf("read_toc %lld\n", mpeg3io_total_bytes(file->fs));

// File type
	file_type = buffer[position++];
	switch(file_type)
	{
		case FILE_TYPE_PROGRAM:
			file->is_program_stream = 1;
			break;
		case FILE_TYPE_TRANSPORT:
			file->is_transport_stream = 1;
			break;
		case FILE_TYPE_AUDIO:
			file->is_audio_stream = 1;
			break;
		case FILE_TYPE_VIDEO:
			file->is_video_stream = 1;
			break;
	}

//printf("read_toc 10\n");

// Stream ID's
	while((stream_type = buffer[position]) != TITLE_PATH)
	{
		int offset;
		int stream_id;

//printf("read_toc %d %x\n", position, buffer[position]);
		position++;
		offset = read_int32(buffer, &position);
		stream_id = read_int32(buffer, &position);

		if(stream_type == STREAM_AUDIO)
		{
			file->demuxer->astream_table[offset] = stream_id;
		}

		if(stream_type == STREAM_VIDEO)
		{
			file->demuxer->vstream_table[offset] = stream_id;
		}
	}



//printf("read_toc 10\n");


// Titles
	while(buffer[position] == TITLE_PATH)
	{
		char string[MPEG3_STRLEN];
		int string_len = 0;
		mpeg3_title_t *title;
		FILE *test_fd;

// Construct title path from VFS prefix and path.
		position++;
		if(is_vfs)
		{
			strcpy(string, RENDERFARM_FS_PREFIX);
			string_len = vfs_len;
		}
		while(buffer[position] != 0) string[string_len++] = buffer[position++];
		string[string_len++] = 0;
		position++;

// Test title availability
		test_fd = fopen(string, "r");
		if(test_fd) 
		{
			fclose(test_fd);
		}
		else
		{
// Try concatenating title and toc directory if title is not absolute and
// toc path has a directory section.
			if((!is_vfs && string[0] != '/') ||
				(is_vfs && string[vfs_len] != '/'))
			{
// Get toc filename without path
				char *ptr = strrchr(file->fs->path, '/');
				if(ptr)
				{
					char string2[MPEG3_STRLEN];

// Stack filename on toc path
					strcpy(string2, file->fs->path);
					if(!is_vfs)
						strcpy(&string2[ptr - file->fs->path + 1], string);
					else
						strcpy(&string2[ptr - file->fs->path + 1], string + vfs_len);

					test_fd = fopen(string2, "r");
					if(test_fd)
					{
						fclose(test_fd);
						strcpy(string, string2);
					}
					else
					{
						fprintf(stderr, 
							"read_toc: failed to open %s or %s\n",
							string,
							string2);
						return 1;
					}
				}
				else
				{
					fprintf(stderr,
						"read_toc: failed to open %s\n", 
						string);
					return 1;
				}
			}
			else
			{
				fprintf(stderr, 
					"read_toc: failed to open %s\n", 
					string);
				return 1;
			}
		}

		title = 
			file->demuxer->titles[file->demuxer->total_titles++] = 
			mpeg3_new_title(file, string);

		title->total_bytes = read_int64(buffer, &position);
		title->start_byte = current_byte;
		title->end_byte = title->start_byte + title->total_bytes;
		current_byte = title->end_byte;

// Cells
		title->cell_table_size = 
			title->cell_table_allocation = 
			read_int32(buffer, &position);
		title->cell_table = calloc(title->cell_table_size, sizeof(mpeg3_cell_t));
		for(i = 0; i < title->cell_table_size; i++)
		{
			mpeg3_cell_t *cell = &title->cell_table[i];
			cell->title_start = read_int64(buffer, &position);
			cell->title_end = read_int64(buffer, &position);
			cell->program_start = read_int64(buffer, &position);
			cell->program_end = read_int64(buffer, &position);
			cell->program = read_int32(buffer, &position);
		}
	}
//printf("read_toc 10\n");



// Audio streams
// Skip ATRACK_COUNT
	position++;
	*atracks_return = read_int32(buffer, &position);
//printf("read_toc 1 %d\n", *atracks_return);

// Skip VTRACK_COUNT
	position++;
	*vtracks_return = read_int32(buffer, &position);
//printf("read_toc 2 %d\n", *vtracks_return);


	if(*atracks_return)
	{
		file->channel_counts = calloc(sizeof(int), *atracks_return);
		file->sample_offsets = calloc(sizeof(int64_t*), *atracks_return);
		file->total_sample_offsets = calloc(sizeof(int*), *atracks_return);
		file->audio_eof = calloc(sizeof(int64_t), *atracks_return);

		for(i = 0; i < *atracks_return; i++)
		{
			file->audio_eof[i] = read_int64(buffer, &position);
			file->channel_counts[i] = read_int32(buffer, &position);
			file->total_sample_offsets[i] = read_int32(buffer, &position);
			file->sample_offsets[i] = malloc(file->total_sample_offsets[i] * sizeof(int64_t));
			for(j = 0; j < file->total_sample_offsets[i]; j++)
			{
				file->sample_offsets[i][j] = read_int64(buffer, &position);
			}
//printf("samples %d %x %llx\n", i, file->sample_offsets[i][0]);
		}
	}
//printf("read_toc 10\n");

	if(*vtracks_return)
	{
		file->frame_offsets = calloc(sizeof(int64_t*), *vtracks_return);
		file->total_frame_offsets = calloc(sizeof(int*), *vtracks_return);
		file->keyframe_numbers = calloc(sizeof(int64_t*), *vtracks_return);
		file->total_keyframe_numbers = calloc(sizeof(int*), *vtracks_return);
		file->video_eof = calloc(sizeof(int64_t), *vtracks_return);

		for(i = 0; i < *vtracks_return; i++)
		{
			file->video_eof[i] = read_int64(buffer, &position);
			file->total_frame_offsets[i] = read_int32(buffer, &position);
			file->frame_offsets[i] = malloc(file->total_frame_offsets[i] * sizeof(int64_t));
			for(j = 0; j < file->total_frame_offsets[i]; j++)
			{
				file->frame_offsets[i][j] = read_int64(buffer, &position);
//printf("frame %llx\n", file->frame_offsets[i][j]);
			}


			file->total_keyframe_numbers[i] = read_int32(buffer, &position);
			file->keyframe_numbers[i] = malloc(file->total_keyframe_numbers[i] * sizeof(int64_t));
			for(j = 0; j < file->total_keyframe_numbers[i]; j++)
			{
				file->keyframe_numbers[i][j] = read_int64(buffer, &position);
			}
		}
	}



// Indexes.  ATracks aren't created yet so we need to put the indexes somewhere
// else.
	file->indexes = calloc(sizeof(mpeg3_index_t*), *atracks_return);
	file->total_indexes = *atracks_return;
	for(i = 0; i < *atracks_return; i++)
	{
		mpeg3_index_t *index = file->indexes[i] = mpeg3_new_index();
		
		index->index_size = read_int32(buffer, &position);
		index->index_zoom = read_int32(buffer, &position);
//printf("mpeg3_read_toc %d %d %d\n", i, index->index_size, index->index_zoom);
		int channels = index->index_channels = file->channel_counts[i];
		if(channels)
		{
			index->index_data = calloc(sizeof(float*), channels);
			for(j = 0; j < channels; j++)
			{
				index->index_data[j] = calloc(sizeof(float), 
					index->index_size * 2);
				read_data(buffer,
					&position,
					(unsigned char*)index->index_data[j], 
					sizeof(float) * index->index_size * 2);
			}
		}
	}





	free(buffer);
//printf("read_toc 10\n");



//printf("read_toc 1\n");
	mpeg3demux_open_title(file->demuxer, 0);
//printf("read_toc 10\n");

	return 0;
}


mpeg3_t* mpeg3_start_toc(char *path, char *toc_path, int64_t *total_bytes)
{
	*total_bytes = 0;
	mpeg3_t *file = mpeg3_new(path);

	file->toc_fd = fopen(toc_path, "w");
	if(!file->toc_fd)
	{
		printf("mpeg3_start_toc: can't open \"%s\".  %s\n",
			toc_path,
			strerror(errno));
		mpeg3_delete(file);
		return 0;
	}
	
	

	file->seekable = 0;

/* Authenticate encryption before reading a single byte */
	if(mpeg3io_open_file(file->fs))
	{
		mpeg3_delete(file);
		return 0;
	}

// Determine file type
	int toc_atracks = 0, toc_vtracks = 0;
	if(mpeg3_get_file_type(file, 0, 0, 0)) return 0;



// Create title without scanning for tracks
	if(!file->demuxer->total_titles)
	{
		mpeg3_title_t *title;
		title = file->demuxer->titles[0] = mpeg3_new_title(file, file->fs->path);
		file->demuxer->total_titles = 1;
		mpeg3demux_open_title(file->demuxer, 0);
		title->total_bytes = mpeg3io_total_bytes(title->fs);
		title->start_byte = 0;
		title->end_byte = title->total_bytes;
		mpeg3_new_cell(title, 
			0, 
			title->end_byte,
			0,
			title->end_byte,
			0);
	}

//	mpeg3demux_seek_byte(file->demuxer, 0x1734e4800LL);
	mpeg3demux_seek_byte(file->demuxer, 0);
	file->demuxer->read_all = 1;
	*total_bytes = mpeg3demux_movie_size(file->demuxer);

	return file;
}

void mpeg3_set_index_bytes(mpeg3_t *file, int64_t bytes)
{
	file->index_bytes = bytes;
}





static void divide_index(mpeg3_t *file, int track_number)
{
	if(file->total_indexes <= track_number) return;


	int i, j;
	mpeg3_atrack_t *atrack = file->atrack[track_number];
	mpeg3_index_t *index = file->indexes[track_number];


	index->index_size /= 2;
	index->index_zoom *= 2;
	for(i = 0; i < index->index_channels; i++)
	{
		float *current_channel = index->index_data[i];
		float *out = current_channel;
		float *in = current_channel;

		for(j = 0; j < index->index_size; j++)
		{
			float max = MAX(in[0], in[2]);
			float min = MIN(in[1], in[3]);
			*out++ = max;
			*out++ = min;
			in += 4;
		}
	}

}




int mpeg3_update_index(mpeg3_t *file, 
	int track_number,
	int flush)
{
	int i, j, k;
	mpeg3_atrack_t *atrack = file->atrack[track_number];
	mpeg3_index_t *index = file->indexes[track_number];


	while((flush && atrack->audio->output_size) ||
		(!flush && atrack->audio->output_size > MPEG3_AUDIO_CHUNKSIZE))
	{
		int fragment = MPEG3_AUDIO_CHUNKSIZE;
		if(atrack->audio->output_size < fragment)
			fragment = atrack->audio->output_size;

		int index_fragments = fragment / 
			index->index_zoom;
		if(flush) index_fragments++;

		int new_index_samples;
		new_index_samples = index_fragments + 
			index->index_size;

// Update number of channels
		if(index->index_allocated && 
			index->index_channels < atrack->channels)
		{
			float **new_index_data = calloc(sizeof(float*), atrack->channels);
			for(i = 0; i < index->index_channels; i++)
			{
				new_index_data[i] = index->index_data[i];
			}
			for(i = index->index_channels; i < atrack->channels; i++)
			{
				new_index_data[i] = calloc(sizeof(float), 
					index->index_allocated * 2);
			}
			index->index_channels = atrack->channels;
			free(index->index_data);
			index->index_data = new_index_data;
		}

// Allocate index buffer
		if(new_index_samples > index->index_allocated)
		{
// Double current number of samples
			index->index_allocated = new_index_samples * 2;
			if(!index->index_data)
			{
				index->index_data = calloc(sizeof(float*), atrack->channels);
			}

// Allocate new size in high and low pairs
			for(i = 0; i < atrack->channels; i++)
				index->index_data[i] = realloc(index->index_data[i], 
					index->index_allocated * sizeof(float) * 2);
			index->index_channels = atrack->channels;
		}



// Calculate new index chunk
		for(i = 0; i < atrack->channels; i++)
		{
			float *in_channel = atrack->audio->output[i];
			float *out_channel = index->index_data[i] + 
				index->index_size * 2;
			float min = 0;
			float max = 0;

// Calculate index frames
			for(j = 0; j < index_fragments; j++)
			{
				int remaining_fragment = fragment - j * index->index_zoom;
// Incomplete index frame
				if(remaining_fragment < index->index_zoom)
				{
					for(k = 0; k < remaining_fragment; k++)
					{
						if(k == 0)
						{
							min = max = *in_channel++;
						}
						else
						{
							if(*in_channel > max) max = *in_channel;
							else
							if(*in_channel < min) min = *in_channel;
							in_channel++;
						}
					}
				}
				else
				{
					min = max = *in_channel++;
					for(k = 1; k < index->index_zoom; k++)
					{
						if(*in_channel > max) max = *in_channel;
						else
						if(*in_channel < min) min = *in_channel;
						in_channel++;
					}
				}
				*out_channel++ = max;
				*out_channel++ = min;
			}
		}

		index->index_size = new_index_samples;

// Shift audio buffer
		mpeg3_shift_audio(atrack->audio, fragment);


// Create new toc entry
		mpeg3_append_samples(atrack, atrack->prev_offset);
		

		atrack->current_position += fragment;
	}

// Divide index by 2 and increase zoom
	if(index->index_size * atrack->channels * sizeof(float) * 2 > 
			file->index_bytes && 
		!(index->index_size % 2))
	{
		divide_index(file, track_number);
	}
}



int mpeg3_toc_audio(mpeg3_t *file, 
	int track_number)
{
	int i, j, k;
	mpeg3_atrack_t *atrack = file->atrack[track_number];

// Assume last packet of stream
	atrack->audio_eof = mpeg3demux_tell_byte(file->demuxer);

// Append demuxed data to track buffer
	mpeg3demux_append_data(atrack->demuxer,
		file->demuxer->audio_buffer,
		file->demuxer->audio_size);


// Decode samples
	mpeg3audio_decode_audio(atrack->audio, 
		0, 
		0, 
		0,
		MPEG3_AUDIO_HISTORY);

// When a chunk is available, 
// add downsampled samples to the index buffer and create toc entry.
	mpeg3_update_index(file, track_number, 0);

	return 0;
}

int mpeg3_toc_video(mpeg3_t *file, 
	mpeg3_vtrack_t *vtrack)
{
	mpeg3video_t *video = vtrack->video;

/*
 * static FILE *test = 0;
 * if(!test) test = fopen("test.m2v", "w");
 * fwrite(file->demuxer->data_buffer, 1, file->demuxer->data_size, test);
 * static int counter = 0;
 * printf("%x %d\n", vtrack->pid, counter++);
 */
// Assume last packet of stream
	vtrack->video_eof = mpeg3demux_tell_byte(file->demuxer);

// Append demuxed data to track buffer
	mpeg3demux_append_data(vtrack->demuxer,
		file->demuxer->video_buffer,
		file->demuxer->video_size);


	if(vtrack->demuxer->data_size - vtrack->demuxer->data_position <
		MPEG3_VIDEO_STREAM_SIZE) return 0;

// Scan for a start code a certain number of bytes from the end of the 
// buffer.  Then scan the header using the video decoder to get the 
// repeat count.
	unsigned char *ptr = &vtrack->demuxer->data_buffer[
		vtrack->demuxer->data_position];
	uint32_t code = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | (ptr[3]);
	ptr += 4;
	while(vtrack->demuxer->data_size - vtrack->demuxer->data_position > 
		MPEG3_VIDEO_STREAM_SIZE)
	{
		if(code == MPEG3_SEQUENCE_START_CODE ||
			code == MPEG3_GOP_START_CODE ||
			code == MPEG3_PICTURE_START_CODE)
		{

// Use video decoder to get repeat count and field type.  Should never hit EOF in here.
// This rereads up to the current ptr since data_position isn't updated by
// mpeg3_toc_video.
			if(!mpeg3video_get_header(video, 0))
			{
				if(video->pict_struct == BOTTOM_FIELD ||
					video->pict_struct == FRAME_PICTURE ||
					!video->pict_struct)
				{
					int is_keyframe = (video->pict_type == I_TYPE);

// Add entry for every repeat count.
					mpeg3_append_frame(vtrack, vtrack->prev_offset, is_keyframe);
					video->current_repeat += 100;
					while(video->repeat_count - video->current_repeat >= 100)
					{
						mpeg3_append_frame(vtrack, vtrack->prev_offset, is_keyframe);
						video->current_repeat += 100;
					}

					ptr = &vtrack->demuxer->data_buffer[
						vtrack->demuxer->data_position];
					code = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | (ptr[3]);
					ptr += 4;

// Shift out data from before frame
					mpeg3demux_shift_data(vtrack->demuxer, vtrack->demuxer->data_position);
				}
			}
			else
			{
// Try this offset again with more data
				break;
			}
		}
		else
		{
			vtrack->demuxer->data_position++;
			code <<= 8;
			code |= *ptr++;
		}
	}
	vtrack->demuxer->data_position -= 4;


	return 0;
}

int mpeg3_do_toc(mpeg3_t *file, int64_t *bytes_processed)
{
	int i, j, k;
// Starting byte before our packet read
	int64_t start_byte;

	start_byte = mpeg3demux_tell_byte(file->demuxer);

	int result = mpeg3_read_next_packet(file->demuxer);

// Determine program and stream id for current packet.  Should be only one.
	int program = mpeg3demux_tell_program(file->demuxer);


/*
 * if(start_byte > 0x1b0000 &&
 * start_byte < 0x1c0000)
 * printf("mpeg3_do_toc 1 start_byte=%llx custum_id=%x got_audio=%d got_video=%d audio_size=%d video_size=%d data_size=%d\n", 
 * start_byte, 
 * file->demuxer->custom_id,
 * file->demuxer->got_audio, 
 * file->demuxer->got_video,
 * file->demuxer->audio_size,
 * file->demuxer->video_size,
 * file->demuxer->data_size);
 */

// Only handle program 0
	if(program == 0)
	{
// Find current PID in tracks.
		int custom_id = file->demuxer->custom_id;
		int got_it = 0;


// In a transport stream the PID's are unique for all audio and video but in 
// other streams the PID's are shared.
		if(file->demuxer->got_audio || 
			file->is_transport_stream ||
			file->is_audio_stream)
		{
			for(i = 0; i < file->total_astreams && !got_it; i++)
			{
				mpeg3_atrack_t *atrack = file->atrack[i];
				if(custom_id == atrack->pid)
				{
//printf("mpeg3_do_toc 2 %x\n", atrack->pid);
// Update an audio track
					mpeg3_toc_audio(file, i);
					atrack->prev_offset = start_byte;
					got_it = 1;
					break;
				}
			}

			if(!got_it && ((file->demuxer->got_audio &&
				file->demuxer->astream_table[custom_id]) ||
				file->is_audio_stream))
			{
				mpeg3_atrack_t *atrack = 
					file->atrack[file->total_astreams] = 
						mpeg3_new_atrack(file, 
							custom_id, 
							file->demuxer->astream_table[custom_id], 
							file->demuxer,
							file->total_astreams);

				if(atrack)
				{
// Create index table
					file->total_indexes++;
					file->indexes = realloc(file->indexes, 
						file->total_indexes * sizeof(mpeg3_index_t*));
					file->indexes[file->total_indexes - 1] = 
						mpeg3_new_index();


					file->total_astreams++;
// Make the first offset correspond to the start of the first packet.
					mpeg3_append_samples(atrack, start_byte);
					mpeg3_toc_audio(file, file->total_astreams - 1);
					atrack->prev_offset = start_byte;
				}
			}

		}


		if(file->demuxer->got_video || 
			file->is_transport_stream ||
			file->is_video_stream)
		{
			got_it = 0;
			for(i = 0; i < file->total_vstreams && !got_it; i++)
			{
				mpeg3_vtrack_t *vtrack = file->vtrack[i];
				if(vtrack->pid == custom_id)
				{
// Update a video track
					mpeg3_toc_video(file, vtrack);
					vtrack->prev_offset = start_byte;
					got_it = 1;
					break;
				}
			}



			if(!got_it && ((file->demuxer->got_video &&
				file->demuxer->vstream_table[custom_id]) ||
				file->is_video_stream))
			{
				mpeg3_vtrack_t *vtrack = 
					file->vtrack[file->total_vstreams] = 
						mpeg3_new_vtrack(file, 
							custom_id, 
							file->demuxer, 
							file->total_vstreams);

// Make the first offset correspond to the start of the first packet.
				if(vtrack)
				{
					file->total_vstreams++;
// Create table entry for frame 0
					mpeg3_append_frame(vtrack, start_byte, 1);
					mpeg3_toc_video(file, vtrack);
					vtrack->prev_offset = start_byte;
				}
			}
		}
	}


// Make user value independant of data type in packet
	*bytes_processed = mpeg3demux_tell_byte(file->demuxer);
//printf("mpeg3_do_toc 1000 %llx\n", *bytes_processed);
}





#define PUT_INT32(x) \
{ \
	if(MPEG3_LITTLE_ENDIAN) \
	{ \
		fputc(((unsigned char*)&x)[3], file->toc_fd); \
		fputc(((unsigned char*)&x)[2], file->toc_fd); \
		fputc(((unsigned char*)&x)[1], file->toc_fd); \
		fputc(((unsigned char*)&x)[0], file->toc_fd); \
	} \
	else \
	{ \
		fputc(((unsigned char*)&x)[0], file->toc_fd); \
		fputc(((unsigned char*)&x)[1], file->toc_fd); \
		fputc(((unsigned char*)&x)[2], file->toc_fd); \
		fputc(((unsigned char*)&x)[3], file->toc_fd); \
	} \
}




#define PUT_INT64(x) \
{ \
	if(MPEG3_LITTLE_ENDIAN) \
	{ \
		fputc(((unsigned char*)&x)[7], file->toc_fd); \
		fputc(((unsigned char*)&x)[6], file->toc_fd); \
		fputc(((unsigned char*)&x)[5], file->toc_fd); \
		fputc(((unsigned char*)&x)[4], file->toc_fd); \
		fputc(((unsigned char*)&x)[3], file->toc_fd); \
		fputc(((unsigned char*)&x)[2], file->toc_fd); \
		fputc(((unsigned char*)&x)[1], file->toc_fd); \
		fputc(((unsigned char*)&x)[0], file->toc_fd); \
	} \
	else \
	{ \
		fwrite(&x, 1, 8, file->toc_fd); \
	} \
}





void mpeg3_stop_toc(mpeg3_t *file)
{
// Create final chunk for audio tracks to count the last samples.
	int i, j;
	for(i = 0; i < file->total_astreams; i++)
	{
		mpeg3_atrack_t *atrack = file->atrack[i];
		mpeg3_append_samples(atrack, atrack->prev_offset);
	}

// Flush audio indexes
	for(i = 0; i < file->total_astreams; i++)
		mpeg3_update_index(file, i, 1);

// Make all indexes the same scale
	int max_scale = 1;
	for(i = 0; i < file->total_astreams; i++)
	{
		mpeg3_atrack_t *atrack = file->atrack[i];
		mpeg3_index_t *index = file->indexes[i];
		if(index->index_data && index->index_zoom > max_scale)
			 	max_scale = index->index_zoom;
	}

	for(i = 0; i < file->total_astreams; i++)
	{
		mpeg3_atrack_t *atrack = file->atrack[i];
		mpeg3_index_t *index = file->indexes[i];
		if(index->index_data && index->index_zoom < max_scale)
		{
			while(index->index_zoom < max_scale)
				divide_index(file, i);
		}
	}




// Sort tracks by PID
	int done = 0;
	while(!done)
	{
		done = 1;
		for(i = 0; i < file->total_astreams - 1; i++)
		{
			mpeg3_atrack_t *atrack1 = file->atrack[i];
			mpeg3_atrack_t *atrack2 = file->atrack[i + 1];
			if(atrack1->pid > atrack2->pid)
			{
				done = 0;
				file->atrack[i + 1] = atrack1;
				file->atrack[i] = atrack2;
				mpeg3_index_t *index1 = file->indexes[i];
				mpeg3_index_t *index2 = file->indexes[i + 1];
				file->indexes[i] = index2;
				file->indexes[i + 1] = index1;
			}
		}
	}


	done = 0;
	while(!done)
	{
		done = 1;
		for(i = 0; i < file->total_vstreams - 1; i++)
		{
			mpeg3_vtrack_t *vtrack1 = file->vtrack[i];
			mpeg3_vtrack_t *vtrack2 = file->vtrack[i + 1];
			if(vtrack1->pid > vtrack2->pid)
			{
				done = 0;
				file->vtrack[i + 1] = vtrack1;
				file->vtrack[i] = vtrack2;
			}
		}
	}



// Output toc to file
// Write file type
	fputc('T', file->toc_fd);
	fputc('O', file->toc_fd);
	fputc('C', file->toc_fd);
	fputc(' ', file->toc_fd);

// Write version
	fputc(MPEG3_TOC_VERSION, file->toc_fd);

// Write stream type
	if(file->is_program_stream)
	{
		fputc(FILE_TYPE_PROGRAM, file->toc_fd);
	}
	else
	if(file->is_transport_stream)
	{
		fputc(FILE_TYPE_TRANSPORT, file->toc_fd);
	}
	else
	if(file->is_audio_stream)
	{
		fputc(FILE_TYPE_AUDIO, file->toc_fd);
	}
	else
	if(file->is_video_stream)
	{
		fputc(FILE_TYPE_VIDEO, file->toc_fd);
	}

// Write stream ID's
// Only program and transport streams have these
	for(i = 0; i < MPEG3_MAX_STREAMS; i++)
	{
		if(file->demuxer->astream_table[i])
		{
			fputc(STREAM_AUDIO, file->toc_fd);
			PUT_INT32(i);
			PUT_INT32(file->demuxer->astream_table[i]);
		}

		if(file->demuxer->vstream_table[i])
		{
			fputc(STREAM_VIDEO, file->toc_fd);
			PUT_INT32(i);
			PUT_INT32(file->demuxer->vstream_table[i]);
		}
	}

// Write titles
	for(i = 0; i < file->demuxer->total_titles; i++)
	{
		mpeg3_title_t *title = file->demuxer->titles[i];
// Path
		fputc(TITLE_PATH, file->toc_fd);
		fprintf(file->toc_fd, title->fs->path);
		fputc(0, file->toc_fd);
// Total bytes
		PUT_INT64(title->total_bytes);
// Byte offsets of cells
		PUT_INT32(file->demuxer->titles[i]->cell_table_size);
		for(j = 0; j < title->cell_table_size; j++)
		{
			mpeg3_cell_t *cell = &title->cell_table[j];
			PUT_INT64(cell->title_start);
			PUT_INT64(cell->title_end);
			PUT_INT64(cell->program_start);
			PUT_INT64(cell->program_end);
			PUT_INT32(cell->program);
		}
	}



	fputc(ATRACK_COUNT, file->toc_fd);
	PUT_INT32(file->total_astreams);

	fputc(VTRACK_COUNT, file->toc_fd);
	PUT_INT32(file->total_vstreams);

// Audio streams
	for(j = 0; j < file->total_astreams; j++)
	{
		mpeg3_atrack_t *atrack = file->atrack[j];
		PUT_INT64(atrack->audio_eof);
		PUT_INT32(atrack->channels);
		PUT_INT32(atrack->total_sample_offsets);
		for(i = 0; i < atrack->total_sample_offsets; i++)
		{
			PUT_INT64(atrack->sample_offsets[i]);
		}
	}





// Video streams
	for(j = 0; j < file->total_vstreams; j++)
	{
		mpeg3_vtrack_t *vtrack = file->vtrack[j];
		PUT_INT64(vtrack->video_eof);
		PUT_INT32(vtrack->total_frame_offsets);
		for(i = 0; i < vtrack->total_frame_offsets; i++)
		{
			PUT_INT64(vtrack->frame_offsets[i]);
		}

		PUT_INT32(vtrack->total_keyframe_numbers);
		for(i = 0; i < vtrack->total_keyframe_numbers; i++)
		{
			PUT_INT64(vtrack->keyframe_numbers[i]);
		}
	}


// Audio indexes
// Write indexes
	for(i = 0; i < file->total_astreams; i++)
	{
		mpeg3_atrack_t *atrack = file->atrack[i];
		mpeg3_index_t *index = file->indexes[i];
		if(index->index_data)
		{
			PUT_INT32(index->index_size);
			PUT_INT32(index->index_zoom);
			for(j = 0; j < atrack->channels; j++)
			{
				fwrite(index->index_data[j], 
					sizeof(float) * 2, 
					index->index_size,
					file->toc_fd);
			}
		}
	}




	fclose(file->toc_fd);


	mpeg3_delete(file);
}









int mpeg3_index_tracks(mpeg3_t *file)
{
	return file->total_indexes;
}

int mpeg3_index_channels(mpeg3_t *file, int track)
{
	if(!file->total_indexes) return 0;
	return file->indexes[track]->index_channels;
}

int mpeg3_index_zoom(mpeg3_t *file)
{
	if(!file->total_indexes) return 0;

	return file->indexes[0]->index_zoom;
}

int mpeg3_index_size(mpeg3_t *file, int track)
{
	if(!file->total_indexes) return 0;
	return file->indexes[track]->index_size;
}

float* mpeg3_index_data(mpeg3_t *file, int track, int channel)
{
	if(!file->total_indexes) return 0;
	return file->indexes[track]->index_data[channel];
}


int mpeg3_has_toc(mpeg3_t *file)
{
	if(file->frame_offsets || file->sample_offsets) return 1;
	return 0;
}





