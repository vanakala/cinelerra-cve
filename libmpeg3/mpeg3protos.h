#ifndef MPEG3PROTOS_H
#define MPEG3PROTOS_H


/* CSS */

mpeg3_css_t* mpeg3_new_css();


/* Workarounds */

int64_t mpeg3io_tell_gcc(mpeg3_fs_t *fs);
double mpeg3_add_double_gcc(double x, double y);
double mpeg3_divide_double_gcc(double x, double y);
int64_t mpeg3_total_bytes_gcc(mpeg3_title_t *title);
int64_t mpeg3io_path_total_bytes(char *path);
int64_t mpeg3io_get_total_bytes(mpeg3_fs_t *fs);




/* TITLE */

mpeg3_title_t* mpeg3_new_title(mpeg3_t *file, char *path);
void mpeg3_new_cell(mpeg3_title_t *title, 
		long start_byte, 
		double start_time,
		long end_byte,
		double end_time,
		int program);
mpeg3demux_cell_t* mpeg3_append_cell(mpeg3_demuxer_t *demuxer, 
		mpeg3_title_t *title, 
		long prev_byte, 
		double prev_time, 
		long start_byte, 
		double start_time,
		int dont_store,
		int program);
/* Called by mpeg3_open for a single file */
int mpeg3demux_create_title(mpeg3_demuxer_t *demuxer, 
		int cell_search, 
		FILE *toc);


/* ATRACK */

mpeg3_atrack_t* mpeg3_new_atrack(mpeg3_t *file, 
	int stream_id, 
	int is_ac3, 
	mpeg3_demuxer_t *demuxer,
	int number);
int mpeg3_delete_atrack(mpeg3_t *file, mpeg3_atrack_t *atrack);


/* These return 1 on failure and 0 on success */
int mpeg3_next_header();

/* VTRACK */

mpeg3_vtrack_t* mpeg3_new_vtrack(mpeg3_t *file, 
	int stream_id, 
	mpeg3_demuxer_t *demuxer,
	int number);
int mpeg3_delete_vtrack(mpeg3_t *file, mpeg3_vtrack_t *vtrack);

/* AUDIO */
mpeg3audio_t* mpeg3audio_new(mpeg3_t *file, mpeg3_atrack_t *track, int is_ac3);
int mpeg3audio_delete(mpeg3audio_t *audio);
/* Read header and store common parameters in audio structure */
int mpeg3audio_read_header(mpeg3audio_t *audio);


/* Audio consists of many possible formats, each packetized into frames. */
/* Each format has a constructor, header decoder, frame decoder, and destructor. */
/* The function set is determined by the audio format. */

/* To decode audio, for each frame read the header sized number of bytes. */
/* Call the header decoder for the format. */
/* Call the frame decoder for the format. */

int mpeg3_new_decode_tables(mpeg3_layer_t *audio);
int mpeg3_init_layer3(mpeg3_layer_t *audio);
int mpeg3_init_layer2(mpeg3_layer_t *audio);

/* Create a new layer decoder */
mpeg3_layer_t* mpeg3_new_layer();

/* Create a new ac3 decoder */
mpeg3_ac3_t* mpeg3_new_ac3();

/* Create a new pcm decoder */
mpeg3_pcm_t* mpeg3_new_pcm();


/* Delete a new layer decoder */
void mpeg3_delete_layer(mpeg3_layer_t *audio);

/* Delete a new ac3 decoder */
void mpeg3_delete_ac3(mpeg3_ac3_t *audio);

/* Delete a new pcm decoder */
void mpeg3_delete_pcm(mpeg3_pcm_t *audio);


/* Return 1 if the data isn't a header */
int mpeg3_layer_check(unsigned char *data);
int mpeg3_ac3_check(unsigned char *header);
int mpeg3_pcm_check(unsigned char *header);

/* These return the size of the next frame including the header */
/* or 0 if it wasn't a header. */
/* Decode a layer header */
int mpeg3_layer_header(mpeg3_layer_t *audio, unsigned char *data);


/* Decode an AC3 header */
int mpeg3_ac3_header(mpeg3_ac3_t *audio, unsigned char *header);

/* Decode an PCM header */
int mpeg3_pcm_header(mpeg3_pcm_t *audio, unsigned char *header);


/* Reset after a seek */
void mpeg3_layer_reset(mpeg3_layer_t *audio);

/* Decode a frame of layer 3 audio. */
/* The output is linear, one buffer for every channel. */
/* The user should get the channel count from one of the header commands */
/* The output must be big enough to hold the largest frame of audio */
/* These functions return the number of samples rendered */
int mpeg3audio_dolayer3(mpeg3_layer_t *audio, 
	char *frame, 
	int frame_size, 
	float **output, 
	int render);

/* Decode a frame of layer 2 audio */
int mpeg3audio_dolayer2(mpeg3_layer_t *audio, 
	char *frame, 
	int frame_size, 
	float **output,
	int render);

/* Decode a frame of ac3 audio */
int mpeg3audio_doac3(mpeg3_ac3_t *audio, 
	char *frame, 
	int frame_size, 
	float **output,
	int render);


/* Decode a frame of ac3 audio */
int mpeg3audio_dopcm(mpeg3_pcm_t *audio, 
	char *frame, 
	int frame_size, 
	float **output,
	int render);


/* VIDEO */
mpeg3video_t* mpeg3video_new(mpeg3_t *file, mpeg3_vtrack_t *track);
int mpeg3video_delete(mpeg3video_t *video);
int mpeg3video_read_frame(mpeg3video_t *video, 
		long frame_number, 
		unsigned char **output_rows,
		int in_x, 
		int in_y, 
		int in_w, 
		int in_h, 
		int out_w, 
		int out_h, 
		int color_model);
void mpeg3video_dump(mpeg3video_t *video);
int mpeg3video_prev_code(mpeg3_demuxer_t *demuxer, unsigned int code);
int mpeg3video_next_code(mpeg3_bits_t* stream, unsigned int code);



/* FILESYSTEM */

mpeg3_fs_t* mpeg3_new_fs(char *path);
int mpeg3_delete_fs(mpeg3_fs_t *fs);
int mpeg3io_open_file(mpeg3_fs_t *fs);
int mpeg3io_close_file(mpeg3_fs_t *fs);
int mpeg3io_seek(mpeg3_fs_t *fs, int64_t byte);
int mpeg3io_read_data(unsigned char *buffer, long bytes, mpeg3_fs_t *fs);
int mpeg3io_next_code(mpeg3_fs_t *fs, uint32_t code, int count);
int mpeg3io_prev_code(mpeg3_fs_t *fs, uint32_t code, int count);















// Demuxer



mpeg3_demuxer_t* mpeg3_new_demuxer(mpeg3_t *file, 
	int do_audio, 
	int do_video, 
	int stream_id);
int mpeg3_delete_demuxer(mpeg3_demuxer_t *demuxer);
mpeg3_demuxer_t* mpeg3_get_demuxer(mpeg3_t *file);
int mpeg3demux_read_data(mpeg3_demuxer_t *demuxer, 
	unsigned char *output, 
	long size);
unsigned int mpeg3demux_read_int32(mpeg3_demuxer_t *demuxer);
unsigned int mpeg3demux_read_int24(mpeg3_demuxer_t *demuxer);
unsigned int mpeg3demux_read_int16(mpeg3_demuxer_t *demuxer);

/* Total number of seconds in all titles */
double mpeg3demux_length(mpeg3_demuxer_t *demuxer);

/* Give total number of bytes in all titles */
int64_t mpeg3demux_movie_size(mpeg3_demuxer_t *demuxer);

/* Number of bytes in the current title */
int64_t mpeg3demux_title_bytes(mpeg3_demuxer_t *demuxer);

/* Return the current title */
int mpeg3demux_tell_title(mpeg3_demuxer_t *demuxer);

/* Give byte offset relative to current title */
int64_t mpeg3demux_tell_relative(mpeg3_demuxer_t *demuxer);

/* Give byte offset relative to start of movie */
int64_t mpeg3demux_tell_byte(mpeg3_demuxer_t *demuxer);


double mpeg3demux_get_time(mpeg3_demuxer_t *demuxer);
int mpeg3demux_eof(mpeg3_demuxer_t *demuxer);
int mpeg3demux_bof(mpeg3_demuxer_t *demuxer);
void mpeg3demux_start_reverse(mpeg3_demuxer_t *demuxer);
void mpeg3demux_start_forward(mpeg3_demuxer_t *demuxer);
int mpeg3demux_open_title(mpeg3_demuxer_t *demuxer, int title_number);
int mpeg3demux_seek_byte(mpeg3_demuxer_t *demuxer, int64_t byte);
unsigned char mpeg3demux_read_char_packet(mpeg3_demuxer_t *demuxer);
unsigned char mpeg3demux_read_prev_char_packet(mpeg3_demuxer_t *demuxer);
int mpeg3demux_read_program(mpeg3_demuxer_t *demuxer);

/* Get last pts read */
double mpeg3demux_audio_pts(mpeg3_demuxer_t *demuxer);

double mpeg3demux_video_pts(mpeg3_demuxer_t *demuxer);

/* Set the last pts read to -1 for audio and video */
void mpeg3demux_reset_pts(mpeg3_demuxer_t *demuxer);

/* scan forward for next pts.  Used in byte seeking to synchronize */
double mpeg3demux_scan_pts(mpeg3_demuxer_t *demuxer);

/* seek using sequential search to the pts given.  Used in byte seeking. */
int mpeg3demux_goto_pts(mpeg3_demuxer_t *demuxer, double pts);


unsigned char mpeg3demux_read_char_packet(mpeg3_demuxer_t *demuxer);
unsigned char mpeg3demux_read_prev_char_packet(mpeg3_demuxer_t *demuxer);

#define mpeg3demux_error(demuxer) (((mpeg3_demuxer_t *)(demuxer))->error_flag)

#define mpeg3demux_time_offset(demuxer) (((mpeg3_demuxer_t *)(demuxer))->time_offset)

#define mpeg3demux_current_time(demuxer) (((mpeg3_demuxer_t *)(demuxer))->time + ((mpeg3_demuxer_t *)(demuxer))->time_offset)

static unsigned char mpeg3demux_read_char(mpeg3_demuxer_t *demuxer)
{
//printf("mpeg3demux_read_char %lx %lx\n", demuxer->data_position, demuxer->data_size);
	if(demuxer->data_position < demuxer->data_size)
	{
		return demuxer->data_buffer[demuxer->data_position++];
	}
	else
	{
		return mpeg3demux_read_char_packet(demuxer);
	}
}



static unsigned char mpeg3demux_read_prev_char(mpeg3_demuxer_t *demuxer)
{
	if(demuxer->data_position != 0)
	{
		return demuxer->data_buffer[demuxer->data_position--];
	}
	else
	{
		return mpeg3demux_read_prev_char_packet(demuxer);
	}
}













// Bitstream

mpeg3_bits_t* mpeg3bits_new_stream(mpeg3_t *file, mpeg3_demuxer_t *demuxer);
int mpeg3bits_delete_stream(mpeg3_bits_t* stream);
int mpeg3bits_seek_byte(mpeg3_bits_t* stream, int64_t position);
int mpeg3bits_open_title(mpeg3_bits_t* stream, int title);
/* Give absolute byte offset in all titles. */
int64_t mpeg3bits_tell(mpeg3_bits_t* stream);
/* Reset bit bucket */
void mpeg3bits_reset(mpeg3_bits_t *stream);






#define mpeg3bits_packet_time(stream) mpeg3demux_current_time((stream)->demuxer)

#define mpeg3bits_time_offset(stream) mepg2demux_time_offset((stream)->demuxer)

#define mpeg3bits_error(stream) mpeg3demux_error((stream)->demuxer)

#define mpeg3bits_eof(stream) mpeg3demux_eof((stream)->demuxer)

#define mpeg3bits_bof(stream) mpeg3demux_bof((stream)->demuxer)

/* Read bytes backward from the file until the reverse_bits is full. */
static void mpeg3bits_fill_reverse_bits(mpeg3_bits_t* stream, int bits)
{
// Right justify
	while(stream->bit_number > 7)
	{
		stream->bfr >>= 8;
		stream->bfr_size -= 8;
		stream->bit_number -= 8;
	}

// Insert bytes before bfr_size
	while(stream->bfr_size - stream->bit_number < bits)
	{
		if(stream->input_ptr)
			stream->bfr |= (unsigned int)(*--stream->input_ptr) << stream->bfr_size;
		else
			stream->bfr |= (unsigned int)mpeg3demux_read_prev_char(stream->demuxer) << stream->bfr_size;
		stream->bfr_size += 8;
	}
}

/* Read bytes forward from the file until the forward_bits is full. */
static void mpeg3bits_fill_bits(mpeg3_bits_t* stream, int bits)
{
	while(stream->bit_number < bits)
	{
		stream->bfr <<= 8;
		if(stream->input_ptr)
		{
			stream->bfr |= *stream->input_ptr++;
		}
		else
		{
			stream->bfr |= mpeg3demux_read_char(stream->demuxer);
		}
		stream->bit_number += 8;
		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;
	}
}

/* Return 8 bits, advancing the file position. */
static unsigned int mpeg3bits_getbyte_noptr(mpeg3_bits_t* stream)
{
	if(stream->bit_number < 8)
	{
		stream->bfr <<= 8;
		if(stream->input_ptr)
			stream->bfr |= *stream->input_ptr++;
		else
			stream->bfr |= mpeg3demux_read_char(stream->demuxer);

		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;

		return (stream->bfr >> stream->bit_number) & 0xff;
	}
	return (stream->bfr >> (stream->bit_number -= 8)) & 0xff;
}

static unsigned int mpeg3bits_getbit_noptr(mpeg3_bits_t* stream)
{
	if(!stream->bit_number)
	{
		stream->bfr <<= 8;
		stream->bfr |= mpeg3demux_read_char(stream->demuxer);

		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;

		stream->bit_number = 7;

		return (stream->bfr >> 7) & 0x1;
	}
	return (stream->bfr >> (--stream->bit_number)) & (0x1);
}

/* Return n number of bits, advancing the file position. */
/* Use in place of flushbits */
static unsigned int mpeg3bits_getbits(mpeg3_bits_t* stream, int bits)
{
	if(bits <= 0) return 0;
	mpeg3bits_fill_bits(stream, bits);
	return (stream->bfr >> (stream->bit_number -= bits)) & (0xffffffff >> (32 - bits));
}

static unsigned int mpeg3bits_showbits24_noptr(mpeg3_bits_t* stream)
{
	while(stream->bit_number < 24)
	{
		stream->bfr <<= 8;
		stream->bfr |= mpeg3demux_read_char(stream->demuxer);
		stream->bit_number += 8;
		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;
	}
	return (stream->bfr >> (stream->bit_number - 24)) & 0xffffff;
}

static unsigned int mpeg3bits_showbits32_noptr(mpeg3_bits_t* stream)
{
	while(stream->bit_number < 32)
	{
		stream->bfr <<= 8;
		stream->bfr |= mpeg3demux_read_char(stream->demuxer);
		stream->bit_number += 8;
		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;
	}
	return stream->bfr;
}

static unsigned int mpeg3bits_showbits(mpeg3_bits_t* stream, int bits)
{
	mpeg3bits_fill_bits(stream, bits);
	return (stream->bfr >> (stream->bit_number - bits)) & (0xffffffff >> (32 - bits));
}

static unsigned int mpeg3bits_getbits_reverse(mpeg3_bits_t* stream, int bits)
{
	unsigned int result;
	mpeg3bits_fill_reverse_bits(stream, bits);
	result = (stream->bfr >> stream->bit_number) & (0xffffffff >> (32 - bits));
	stream->bit_number += bits;
	return result;
}

static unsigned int mpeg3bits_showbits_reverse(mpeg3_bits_t* stream, int bits)
{
	unsigned int result;
	mpeg3bits_fill_reverse_bits(stream, bits);
	result = (stream->bfr >> stream->bit_number) & (0xffffffff >> (32 - bits));
	return result;
}









// I/O
// I/O must be character based so the buffer doesn't get overrun







void mpeg3io_read_buffer(mpeg3_fs_t *fs);

#define mpeg3io_tell(fs) (((mpeg3_fs_t *)(fs))->current_byte)

// End of file
#define mpeg3io_eof(fs) (((mpeg3_fs_t *)(fs))->current_byte >= ((mpeg3_fs_t *)(fs))->total_bytes)

// Beginning of file
#define mpeg3io_bof(fs)	(((mpeg3_fs_t *)(fs))->current_byte < 0)

#define mpeg3io_get_fd(fs) (fileno(((mpeg3_fs_t *)(fs))->fd))

#define mpeg3io_total_bytes(fs) (((mpeg3_fs_t *)(fs))->total_bytes)

static int mpeg3io_sync_buffer(mpeg3_fs_t *fs)
{
// Reposition buffer offset
	if(fs->buffer_position + fs->buffer_offset != fs->current_byte)
	{
		fs->buffer_offset = fs->current_byte - fs->buffer_position;
	}

// Load new buffer
	if(fs->current_byte < fs->buffer_position ||
		fs->current_byte >= fs->buffer_position + fs->buffer_size)
	{
/*
 * printf("mpeg3io_sync_buffer 1 %llx %llx %llx\n",
 * fs->buffer_position,
 * fs->current_byte,
 * fs->buffer_position + fs->buffer_size);
 */
		mpeg3io_read_buffer(fs);
	}

	return !fs->buffer_size;
}

static unsigned int mpeg3io_read_char(mpeg3_fs_t *fs)
{
	unsigned int result;
	mpeg3io_sync_buffer(fs);
	result = fs->buffer[fs->buffer_offset++];
	fs->current_byte++;
	return result;
}

static unsigned char mpeg3io_next_char(mpeg3_fs_t *fs)
{
	unsigned char result;
	mpeg3io_sync_buffer(fs);
	result = fs->buffer[fs->buffer_offset];
	return result;
}

static uint32_t mpeg3io_read_int32(mpeg3_fs_t *fs)
{
	int a, b, c, d;
	uint32_t result;
/* Do not fread.  This breaks byte ordering. */
	a = mpeg3io_read_char(fs);
	b = mpeg3io_read_char(fs);
	c = mpeg3io_read_char(fs);
	d = mpeg3io_read_char(fs);
	result = (a << 24) |
					(b << 16) |
					(c << 8) |
					(d);
	return result;
}

static uint32_t mpeg3io_read_int24(mpeg3_fs_t *fs)
{
	int b, c, d;
	uint32_t result;
/* Do not fread.  This breaks byte ordering. */
	b = mpeg3io_read_char(fs);
	c = mpeg3io_read_char(fs);
	d = mpeg3io_read_char(fs);
	result = (b << 16) |
					(c << 8) |
					(d);
	return result;
}

static uint16_t mpeg3io_read_int16(mpeg3_fs_t *fs)
{
	int c, d;
	uint16_t result;
/* Do not fread.  This breaks byte ordering. */
	c = mpeg3io_read_char(fs);
	d = mpeg3io_read_char(fs);
	result = (c << 8) |
					(d);
	return result;
}









// More bitstream









#define mpeg3slice_fillbits(buffer, nbits) \
	while(((mpeg3_slice_buffer_t*)(buffer))->bits_size < (nbits)) \
	{ \
		if(((mpeg3_slice_buffer_t*)(buffer))->current_position < ((mpeg3_slice_buffer_t*)(buffer))->buffer_size) \
		{ \
			((mpeg3_slice_buffer_t*)(buffer))->bits <<= 8; \
			((mpeg3_slice_buffer_t*)(buffer))->bits |= ((mpeg3_slice_buffer_t*)(buffer))->data[((mpeg3_slice_buffer_t*)(buffer))->current_position++]; \
		} \
		((mpeg3_slice_buffer_t*)(buffer))->bits_size += 8; \
	}

#define mpeg3slice_flushbits(buffer, nbits) \
	{ \
		mpeg3slice_fillbits((buffer), (nbits)); \
		((mpeg3_slice_buffer_t*)(buffer))->bits_size -= (nbits); \
	}

#define mpeg3slice_flushbit(buffer) \
{ \
	if(((mpeg3_slice_buffer_t*)(buffer))->bits_size) \
		((mpeg3_slice_buffer_t*)(buffer))->bits_size--; \
	else \
	if(((mpeg3_slice_buffer_t*)(buffer))->current_position < ((mpeg3_slice_buffer_t*)(buffer))->buffer_size) \
	{ \
		((mpeg3_slice_buffer_t*)(buffer))->bits = \
			((mpeg3_slice_buffer_t*)(buffer))->data[((mpeg3_slice_buffer_t*)(buffer))->current_position++]; \
		((mpeg3_slice_buffer_t*)(buffer))->bits_size = 7; \
	} \
}

static unsigned int mpeg3slice_getbit(mpeg3_slice_buffer_t *buffer)
{
	if(buffer->bits_size)
		return (buffer->bits >> (--buffer->bits_size)) & 0x1;
	else
	if(buffer->current_position < buffer->buffer_size)
	{
		buffer->bits = buffer->data[buffer->current_position++];
		buffer->bits_size = 7;
		return (buffer->bits >> 7) & 0x1;
	}
	return 0;
}


static unsigned int mpeg3slice_getbits2(mpeg3_slice_buffer_t *buffer)
{
	if(buffer->bits_size >= 2)
		return (buffer->bits >> (buffer->bits_size -= 2)) & 0x3;
	else
	if(buffer->current_position < buffer->buffer_size)
	{
		buffer->bits <<= 8;
		buffer->bits |= buffer->data[buffer->current_position++];
		buffer->bits_size += 6;
		return (buffer->bits >> buffer->bits_size)  & 0x3;
	}
	return 0;
}

static unsigned int mpeg3slice_getbyte(mpeg3_slice_buffer_t *buffer)
{
	if(buffer->bits_size >= 8)
		return (buffer->bits >> (buffer->bits_size -= 8)) & 0xff;
	else
	if(buffer->current_position < buffer->buffer_size)
	{
		buffer->bits <<= 8;
		buffer->bits |= buffer->data[buffer->current_position++];
		return (buffer->bits >> buffer->bits_size) & 0xff;
	}
	return 0;
}


static unsigned int mpeg3slice_getbits(mpeg3_slice_buffer_t *slice_buffer, int bits)
{
	if(bits == 1) return mpeg3slice_getbit(slice_buffer);
	mpeg3slice_fillbits(slice_buffer, bits);
	return (slice_buffer->bits >> (slice_buffer->bits_size -= bits)) & (0xffffffff >> (32 - bits));
}

static unsigned int mpeg3slice_showbits16(mpeg3_slice_buffer_t *buffer)
{
	if(buffer->bits_size >= 16)
		return (buffer->bits >> (buffer->bits_size - 16)) & 0xffff;
	else
	if(buffer->current_position < buffer->buffer_size)
	{
		buffer->bits <<= 16;
		buffer->bits_size += 16;
		buffer->bits |= (unsigned int)buffer->data[buffer->current_position++] << 8;
		buffer->bits |= buffer->data[buffer->current_position++];
		return (buffer->bits >> (buffer->bits_size - 16)) & 0xffff;
	}
	return 0;
}

static unsigned int mpeg3slice_showbits9(mpeg3_slice_buffer_t *buffer)
{
	if(buffer->bits_size >= 9)
		return (buffer->bits >> (buffer->bits_size - 9)) & 0x1ff;
	else
	if(buffer->current_position < buffer->buffer_size)
	{
		buffer->bits <<= 16;
		buffer->bits_size += 16;
		buffer->bits |= (unsigned int)buffer->data[buffer->current_position++] << 8;
		buffer->bits |= buffer->data[buffer->current_position++];
		return (buffer->bits >> (buffer->bits_size - 9)) & 0x1ff;
	}
	return 0;
}

static unsigned int mpeg3slice_showbits5(mpeg3_slice_buffer_t *buffer)
{
	if(buffer->bits_size >= 5)
		return (buffer->bits >> (buffer->bits_size - 5)) & 0x1f;
	else
	if(buffer->current_position < buffer->buffer_size)
	{
		buffer->bits <<= 8;
		buffer->bits_size += 8;
		buffer->bits |= buffer->data[buffer->current_position++];
		return (buffer->bits >> (buffer->bits_size - 5)) & 0x1f;
	}
	return 0;
}

static unsigned int mpeg3slice_showbits(mpeg3_slice_buffer_t *slice_buffer, int bits)
{
	mpeg3slice_fillbits(slice_buffer, bits);
	return (slice_buffer->bits >> (slice_buffer->bits_size - bits)) & (0xffffffff >> (32 - bits));
}






#endif
