#include "libmpeg3.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


int mpeg3_major()
{
	return MPEG3_MAJOR;
}

int mpeg3_minor()
{
	return MPEG3_MINOR;
}

int mpeg3_release()
{
	return MPEG3_RELEASE;
}



mpeg3_t* mpeg3_new(char *path)
{
	int i;
	mpeg3_t *file = calloc(1, sizeof(mpeg3_t));
	file->cpus = 1;
	file->fs = mpeg3_new_fs(path);
// Late compilers don't produce usable code.
	file->demuxer = mpeg3_new_demuxer(file, 0, 0, -1);
	file->seekable = 1;
	file->index_bytes = 0x300000;
	return file;
}

mpeg3_index_t* mpeg3_new_index()
{
	mpeg3_index_t *index = calloc(1, sizeof(mpeg3_index_t));
	index->index_zoom = 1;
	return index;
}

void mpeg3_delete_index(mpeg3_index_t *index)
{
	int i;
	for(i = 0;i < index->index_channels; i++)
		free(index->index_data[i]);
	free(index->index_data);
	free(index);
}

int mpeg3_delete(mpeg3_t *file)
{
	int i;
	const int debug = 0;

if(debug) printf("mpeg3_delete 1\n");
	for(i = 0; i < file->total_vstreams; i++)
		mpeg3_delete_vtrack(file, file->vtrack[i]);
if(debug) printf("mpeg3_delete 1\n");

	for(i = 0; i < file->total_astreams; i++)
		mpeg3_delete_atrack(file, file->atrack[i]);

if(debug) printf("mpeg3_delete 1\n");
	mpeg3_delete_fs(file->fs);
	mpeg3_delete_demuxer(file->demuxer);

if(debug) printf("mpeg3_delete 1\n");
	if(file->frame_offsets)
	{
		for(i = 0; i < file->total_vstreams; i++)
		{
			free(file->frame_offsets[i]);
			free(file->keyframe_numbers[i]);
		}

if(debug) printf("mpeg3_delete 1\n");
		free(file->frame_offsets);
		free(file->keyframe_numbers);
		free(file->total_frame_offsets);
		free(file->total_keyframe_numbers);
	}

if(debug) printf("mpeg3_delete 1\n");
	if(file->sample_offsets)
	{
		for(i = 0; i < file->total_astreams; i++)
			free(file->sample_offsets[i]);

		free(file->sample_offsets);
		free(file->total_sample_offsets);
	}

if(debug) printf("mpeg3_delete 1\n");

	if(file->channel_counts)
		free(file->channel_counts);

if(debug) printf("mpeg3_delete 2\n");
	if(file->indexes)
	{
		for(i = 0; i < file->total_indexes; i++)
			mpeg3_delete_index(file->indexes[i]);
if(debug) printf("mpeg3_delete 3\n");
			
		free(file->indexes);
	}

if(debug) printf("mpeg3_delete 4\n");
	free(file);
if(debug) printf("mpeg3_delete 5\n");
	return 0;
}

int mpeg3_check_sig(char *path)
{
	mpeg3_fs_t *fs;
	u_int32_t bits;
	char *ext;
	int result = 0;

	fs = mpeg3_new_fs(path);
	if(mpeg3io_open_file(fs))
	{
/* File not found */
		return 0;
	}

	bits = mpeg3io_read_int32(fs);
/* Test header */
	if(bits == MPEG3_TOC_PREFIX)
	{
		result = 1;
	}
	else
	if((((bits >> 24) & 0xff) == MPEG3_SYNC_BYTE) ||
		(bits == MPEG3_PACK_START_CODE) ||
		((bits & 0xfff00000) == 0xfff00000) ||
		((bits & 0xffff0000) == 0xffe30000) ||
		(bits == MPEG3_SEQUENCE_START_CODE) ||
		(bits == MPEG3_PICTURE_START_CODE) ||
		(((bits & 0xffff0000) >> 16) == MPEG3_AC3_START_CODE) ||
		((bits >> 8) == MPEG3_ID3_PREFIX) ||
		(bits == MPEG3_RIFF_CODE) ||
        (bits == MPEG3_IFO_PREFIX))
	{
		result = 1;

		ext = strrchr(path, '.');
		if(ext)
		{
/* Test file extension. */
			if(strncasecmp(ext, ".ifo", 4) && 
            	strncasecmp(ext, ".mp2", 4) && 
				strncasecmp(ext, ".mp3", 4) &&
				strncasecmp(ext, ".m1v", 4) &&
				strncasecmp(ext, ".m2v", 4) &&
				strncasecmp(ext, ".m2s", 4) &&
				strncasecmp(ext, ".mpg", 4) &&
				strncasecmp(ext, ".vob", 4) &&
				strncasecmp(ext, ".mpeg", 4) &&
				strncasecmp(ext, ".ac3", 4))
				result = 0;
		}
	}

	mpeg3io_close_file(fs);
	mpeg3_delete_fs(fs);
	return result;
}










static int is_toc(uint32_t bits)
{
	return (bits == MPEG3_TOC_PREFIX);
}


static int is_ifo(uint32_t bits)
{
	return (bits == MPEG3_IFO_PREFIX);
}

static int is_transport(uint32_t bits)
{
	return (((bits >> 24) & 0xff) == MPEG3_SYNC_BYTE);
}

static int is_program(uint32_t bits)
{
	return (bits == MPEG3_PACK_START_CODE);
}

static int is_mpeg_audio(uint32_t bits)
{
	return (bits & 0xfff00000) == 0xfff00000 ||
		(bits & 0xffff0000) == 0xffe30000 ||
		((bits >> 8) == MPEG3_ID3_PREFIX) ||
		(bits == MPEG3_RIFF_CODE);
}

static int is_mpeg_video(uint32_t bits)
{
	return (bits == MPEG3_SEQUENCE_START_CODE ||
			bits == MPEG3_PICTURE_START_CODE);
}

static int is_ac3(uint32_t bits)
{
	return (((bits & 0xffff0000) >> 16) == MPEG3_AC3_START_CODE);
}

static int calculate_packet_size(int is_transport,
	int is_program,
	int is_audio,
	int is_video)
{
	if(is_transport)
		return MPEG3_TS_PACKET_SIZE;
	else
	if(is_program)
		return 0;
	else
	if(is_audio)
		return MPEG3_DVD_PACKET_SIZE;
	else
	if(is_video)
		return MPEG3_DVD_PACKET_SIZE;
}

int mpeg3_get_file_type(mpeg3_t *file, 
	mpeg3_t *old_file,
	int *toc_atracks,
	int *toc_vtracks)
{
	uint32_t bits = mpeg3io_read_int32(file->fs);

/* TOC  */
	if(is_toc(bits))   
	{
/* Table of contents for another title set */
		if(!old_file)
		{
			if(toc_atracks && toc_vtracks)
			{
				if(mpeg3_read_toc(file, toc_atracks, toc_vtracks))
				{
					mpeg3io_close_file(file->fs);
					mpeg3_delete(file);
					return 1;
				}
			}
			else
			{
				mpeg3io_close_file(file->fs);
				mpeg3_delete(file);
				return 1;
			}
		}
		mpeg3io_close_file(file->fs);
	}
	else
/* IFO file */
    if(is_ifo(bits))
    {
    	if(!old_file)
		{
			file->is_program_stream = 1;
			if(mpeg3_read_ifo(file, 0))
        	{
				mpeg3_delete(file);
				mpeg3io_close_file(file->fs);
				return 1;
        	}
		}
		file->is_ifo_file = 1;
		mpeg3io_close_file(file->fs);
    }
    else
	if(is_transport(bits))
	{
/* Transport stream */
		file->is_transport_stream = 1;
	}
	else
	if(is_program(bits))
	{
/* Program stream */
/* Determine packet size empirically */
		file->is_program_stream = 1;
	}
	else
	if(is_mpeg_audio(bits))
	{
/* MPEG Audio only */
		file->is_audio_stream = 1;
	}
	else
	if(is_mpeg_video(bits))
	{
/* Video only */
		file->is_video_stream = 1;
	}
	else
	if(is_ac3(bits))
	{
/* AC3 Audio only */
		file->is_audio_stream = 1;
	}
	else
	{
		mpeg3_delete(file);
		fprintf(stderr, "mpeg3_get_file_type: not a readable stream.\n");
		return 1;
	}

/*
 * printf("mpeg3_open 2 %p %d %d %d %d\n", 
 * old_file, 
 * file->is_transport_stream, 
 * file->is_program_stream, 
 * file->is_video_stream, 
 * file->is_audio_stream);
 */


	file->packet_size = calculate_packet_size(file->is_transport_stream,
		file->is_program_stream,
		file->is_audio_stream,
		file->is_video_stream);

	return 0;
}


mpeg3_t* mpeg3_open_copy(char *path, mpeg3_t *old_file)
{
	mpeg3_t *file = 0;
	int i, done;
/* The table of contents may have fewer tracks than are in the demuxer */
/* This limits the track count */
	int toc_atracks = 0x7fffffff;
	int toc_vtracks = 0x7fffffff;

/* Initialize the file structure */
	file = mpeg3_new(path);



/* Need to perform authentication before reading a single byte. */
	if(mpeg3io_open_file(file->fs))
	{
		mpeg3_delete(file);
		return 0;
	}





/* =============================== Create the title objects ========================= */
	if(mpeg3_get_file_type(file, 
		old_file,
		&toc_atracks, 
		&toc_vtracks)) return 0;











/* Create titles */
/* Copy timecodes from an old demuxer */
	if(old_file && mpeg3_get_demuxer(old_file))
	{
		mpeg3demux_copy_titles(file->demuxer, mpeg3_get_demuxer(old_file));
		file->is_transport_stream = old_file->is_transport_stream;
		file->is_program_stream = old_file->is_program_stream;
	}
	else
/* Start from scratch */
	if(!file->demuxer->total_titles)
	{
		mpeg3_create_title(file->demuxer, 0);
	}














/* Generate tracks */
	if(file->is_transport_stream || file->is_program_stream)
	{
/* Create video tracks */
		for(i = 0; 
			i < MPEG3_MAX_STREAMS && file->total_vstreams < toc_vtracks; 
			i++)
		{
			if(file->demuxer->vstream_table[i])
			{
				file->vtrack[file->total_vstreams] = 
					mpeg3_new_vtrack(file, 
						i, 
						file->demuxer, 
						file->total_vstreams);
				if(file->vtrack[file->total_vstreams]) 
					file->total_vstreams++;

			}
		}

/* Create audio tracks */
		for(i = 0; i < MPEG3_MAX_STREAMS && file->total_astreams < toc_atracks; i++)
		{
			if(file->demuxer->astream_table[i])
			{
				file->atrack[file->total_astreams] = mpeg3_new_atrack(file, 
					i, 
					file->demuxer->astream_table[i], 
					file->demuxer,
					file->total_astreams);
				if(file->atrack[file->total_astreams]) file->total_astreams++;
			}
		}
	}
	else
	if(file->is_video_stream)
	{
/* Create video tracks */
		file->vtrack[0] = mpeg3_new_vtrack(file, 
			-1, 
			file->demuxer, 
			0);
		if(file->vtrack[0]) file->total_vstreams++;
	}
	else
	if(file->is_audio_stream)
	{
/* Create audio tracks */

		file->atrack[0] = mpeg3_new_atrack(file, 
			-1, 
			AUDIO_UNKNOWN, 
			file->demuxer,
			0);
		if(file->atrack[0]) file->total_astreams++;
	}





	mpeg3io_close_file(file->fs);

	return file;
}

mpeg3_t* mpeg3_open(char *path)
{
	return mpeg3_open_copy(path, 0);
}

int mpeg3_close(mpeg3_t *file)
{
/* File is closed in the same procedure it is opened in. */
	mpeg3_delete(file);
	return 0;
}

int mpeg3_set_cpus(mpeg3_t *file, int cpus)
{
	int i;
	file->cpus = cpus;
	for(i = 0; i < file->total_vstreams; i++)
		mpeg3video_set_cpus(file->vtrack[i]->video, cpus);
	return 0;
}

int mpeg3_has_audio(mpeg3_t *file)
{
	return file->total_astreams > 0;
}

int mpeg3_total_astreams(mpeg3_t *file)
{
	return file->total_astreams;
}

int mpeg3_audio_channels(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->channels;
	return -1;
}

int mpeg3_sample_rate(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->sample_rate;
	return -1;
}

long mpeg3_get_sample(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->current_position;
	return -1;
}

int mpeg3_set_sample(mpeg3_t *file, 
		long sample,
		int stream)
{
	if(file->total_astreams)
	{
//printf(__FUNCTION__ " 1 %d %d\n", file->atrack[stream]->current_position, sample);
		file->atrack[stream]->current_position = sample;
		mpeg3audio_seek_sample(file->atrack[stream]->audio, sample);
		return 0;
	}
	return -1;
}

long mpeg3_audio_samples(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->total_samples;
	return -1;
}

char* mpeg3_audio_format(mpeg3_t *file, int stream)
{
	if(stream < file->total_astreams)
	{
		switch(file->atrack[stream]->format)
		{
			case AUDIO_UNKNOWN: return "Unknown"; break;
			case AUDIO_MPEG:    return "MPEG"; break;
			case AUDIO_AC3:     return "AC3"; break;
			case AUDIO_PCM:     return "PCM"; break;
			case AUDIO_AAC:     return "AAC"; break;
			case AUDIO_JESUS:   return "Vorbis"; break;
		}
	}
	return "";
}

int mpeg3_has_video(mpeg3_t *file)
{
	return file->total_vstreams > 0;
}

int mpeg3_total_vstreams(mpeg3_t *file)
{
	return file->total_vstreams;
}

int mpeg3_video_width(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->width;
	return -1;
}

int mpeg3_video_height(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->height;
	return -1;
}

float mpeg3_aspect_ratio(mpeg3_t *file, int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->aspect_ratio;
	return 0;
}

double mpeg3_frame_rate(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->frame_rate;
	return -1;
}

long mpeg3_video_frames(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->total_frames;
	return -1;
}

long mpeg3_get_frame(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->current_position;
	return -1;
}

int mpeg3_set_frame(mpeg3_t *file, 
		long frame,
		int stream)
{
	if(file->total_vstreams)
	{
		file->vtrack[stream]->current_position = frame;
		mpeg3video_seek_frame(file->vtrack[stream]->video, frame);
		return 0;
	}
	return -1;
}

int mpeg3_seek_byte(mpeg3_t *file, int64_t byte)
{
	int i;

//	file->percentage_pts = -1;
	for(i = 0; i < file->total_vstreams; i++)
	{
		file->vtrack[i]->current_position = 0;
		mpeg3video_seek_byte(file->vtrack[i]->video, byte);
	}

	for(i = 0; i < file->total_astreams; i++)
	{
		file->atrack[i]->current_position = 0;
		mpeg3audio_seek_byte(file->atrack[i]->audio, byte);
	}

	return 0;
}

/*
 * double mpeg3_get_percentage_pts(mpeg3_t *file)
 * {
 * 	return file->percentage_pts;
 * }
 * 
 * void mpeg3_set_percentage_pts(mpeg3_t *file, double pts)
 * {
 * }
 */


int mpeg3_previous_frame(mpeg3_t *file, int stream)
{
	file->last_type_read = 2;
	file->last_stream_read = stream;

	if(file->total_vstreams)
		return mpeg3video_previous_frame(file->vtrack[stream]->video);

	return 0;
}

int64_t mpeg3_tell_byte(mpeg3_t *file)
{
	int64_t result = 0;
	if(file->last_type_read == 1)
	{
		result = mpeg3demux_tell_byte(file->atrack[file->last_stream_read]->demuxer);
	}

	if(file->last_type_read == 2)
	{
		result = mpeg3demux_tell_byte(file->vtrack[file->last_stream_read]->demuxer);
	}
	return result;
}

int64_t mpeg3_get_bytes(mpeg3_t *file)
{
	return mpeg3demux_movie_size(file->demuxer);
}

double mpeg3_get_time(mpeg3_t *file)
{
	double atime = 0, vtime = 0;

	if(file->is_transport_stream || file->is_program_stream)
	{
/* Timecode only available in transport stream */
		if(file->last_type_read == 1)
		{
			atime = mpeg3demux_get_time(file->atrack[file->last_stream_read]->demuxer);
		}
		else
		if(file->last_type_read == 2)
		{
			vtime = mpeg3demux_get_time(file->vtrack[file->last_stream_read]->demuxer);
		}
	}
	else
	{
/* Use percentage and total time */
		if(file->total_astreams)
		{
			atime = mpeg3demux_tell_byte(file->atrack[0]->demuxer) * 
						mpeg3_audio_samples(file, 0) / 
						mpeg3_sample_rate(file, 0) /
						mpeg3_get_bytes(file);
		}

		if(file->total_vstreams)
		{
			vtime = mpeg3demux_tell_byte(file->vtrack[0]->demuxer) *
						mpeg3_video_frames(file, 0) / 
						mpeg3_frame_rate(file, 0) /
						mpeg3_get_bytes(file);
		}
	}

	return MAX(atime, vtime);
}

int mpeg3_end_of_audio(mpeg3_t *file, int stream)
{
	int result = 0;
	if(!file->atrack[stream]->channels) return 1;
	result = mpeg3demux_eof(file->atrack[stream]->demuxer);
	return result;
}

int mpeg3_end_of_video(mpeg3_t *file, int stream)
{
	int result = 0;
	result = mpeg3demux_eof(file->vtrack[stream]->demuxer);
	return result;
}


int mpeg3_drop_frames(mpeg3_t *file, long frames, int stream)
{
	int result = -1;

	if(file->total_vstreams)
	{
		result = mpeg3video_drop_frames(file->vtrack[stream]->video, 
						frames);
		if(frames > 0) file->vtrack[stream]->current_position += frames;
		file->last_type_read = 2;
		file->last_stream_read = stream;
	}
	return result;
}

int mpeg3_colormodel(mpeg3_t *file, int stream)
{
	if(file->total_vstreams)
	{
		return mpeg3video_colormodel(file->vtrack[stream]->video);
	}
	return 0;
}

int mpeg3_set_rowspan(mpeg3_t *file, int bytes, int stream)
{
	if(file->total_vstreams)
	{
		file->vtrack[stream]->video->row_span = bytes;
	}
	return 0;
}


int mpeg3_read_frame(mpeg3_t *file, 
		unsigned char **output_rows, 
		int in_x, 
		int in_y, 
		int in_w, 
		int in_h, 
		int out_w, 
		int out_h, 
		int color_model,
		int stream)
{
	int result = -1;
//printf("mpeg3_read_frame 1 %d\n", file->vtrack[stream]->current_position);

	if(file->total_vstreams)
	{
		result = mpeg3video_read_frame(file->vtrack[stream]->video, 
					file->vtrack[stream]->current_position, 
					output_rows,
					in_x, 
					in_y, 
					in_w, 
					in_h, 
					out_w,
					out_h,
					color_model);
//printf(__FUNCTION__ " 2\n");
		file->last_type_read = 2;
		file->last_stream_read = stream;
		file->vtrack[stream]->current_position++;
	}

//printf("mpeg3_read_frame 2 %d\n", file->vtrack[stream]->current_position);
	return result;
}

int mpeg3_read_yuvframe(mpeg3_t *file,
		char *y_output,
		char *u_output,
		char *v_output,
		int in_x, 
		int in_y,
		int in_w,
		int in_h,
		int stream)
{
	int result = -1;

//printf("mpeg3_read_yuvframe 1\n");
	if(file->total_vstreams)
	{
		result = mpeg3video_read_yuvframe(file->vtrack[stream]->video, 
					file->vtrack[stream]->current_position, 
					y_output,
					u_output,
					v_output,
					in_x,
					in_y,
					in_w,
					in_h);
		file->last_type_read = 2;
		file->last_stream_read = stream;
		file->vtrack[stream]->current_position++;
	}
//printf("mpeg3_read_yuvframe 100\n");
	return result;
}

int mpeg3_read_yuvframe_ptr(mpeg3_t *file,
		char **y_output,
		char **u_output,
		char **v_output,
		int stream)
{
	int result = -1;

	if(file->total_vstreams)
	{
		result = mpeg3video_read_yuvframe_ptr(file->vtrack[stream]->video, 
					file->vtrack[stream]->current_position, 
					y_output,
					u_output,
					v_output);
		file->last_type_read = 2;
		file->last_stream_read = stream;
		file->vtrack[stream]->current_position++;
	}
	return result;
}

int mpeg3_read_audio(mpeg3_t *file, 
		float *output_f, 
		short *output_i, 
		int channel, 
		long samples,
		int stream)
{
	int result = -1;

	if(file->total_astreams)
	{
		result = mpeg3audio_decode_audio(file->atrack[stream]->audio, 
					output_f, 
					output_i, 
					channel, 
					samples);
		file->last_type_read = 1;
		file->last_stream_read = stream;
		file->atrack[stream]->current_position += samples;
	}

	return result;
}

int mpeg3_reread_audio(mpeg3_t *file, 
		float *output_f, 
		short *output_i, 
		int channel, 
		long samples,
		int stream)
{
	if(file->total_astreams)
	{
		mpeg3_set_sample(file, 
			file->atrack[stream]->current_position - samples,
			stream);
		file->last_type_read = 1;
		file->last_stream_read = stream;
		return mpeg3_read_audio(file, 
			output_f, 
			output_i, 
			channel, 
			samples,
			stream);
	}
	return -1;
}

int mpeg3_read_audio_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream)
{
	int result = 0;
	if(file->total_astreams)
	{
		result = mpeg3audio_read_raw(file->atrack[stream]->audio, output, size, max_size);
		file->last_type_read = 1;
		file->last_stream_read = stream;
	}
	return result;
}

int mpeg3_read_video_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream)
{
	int result = 0;
	if(file->total_vstreams)
	{
		result = mpeg3video_read_raw(file->vtrack[stream]->video, output, size, max_size);
		file->last_type_read = 2;
		file->last_stream_read = stream;
	}
	return result;
}



