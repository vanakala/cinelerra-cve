#include "libmpeg3.h"
#include "mpeg3io.h"
#include "mpeg3protos.h"
#include "workarounds.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ABS(x) ((x) >= 0 ? (x) : -(x))

/* Don't advance pointer */
static inline unsigned char packet_next_char(mpeg3_demuxer_t *demuxer)
{
//printf(__FUNCTION__ " called\n");
	return demuxer->raw_data[demuxer->raw_offset];
}

/* Advance pointer */
static unsigned char packet_read_char(mpeg3_demuxer_t *demuxer)
{
	unsigned char result = demuxer->raw_data[demuxer->raw_offset++];
//printf(__FUNCTION__ " called\n");
	return result;
}

static inline unsigned int packet_read_int16(mpeg3_demuxer_t *demuxer)
{
	unsigned int a, b, result;
//printf(__FUNCTION__ " called\n");
	a = demuxer->raw_data[demuxer->raw_offset++];
	b = demuxer->raw_data[demuxer->raw_offset++];
	result = (a << 8) | b;

	return result;
}

static inline unsigned int packet_next_int24(mpeg3_demuxer_t *demuxer)
{
	unsigned int a, b, c, result;
//printf(__FUNCTION__ " called\n");
	a = demuxer->raw_data[demuxer->raw_offset];
	b = demuxer->raw_data[demuxer->raw_offset + 1];
	c = demuxer->raw_data[demuxer->raw_offset + 2];
	result = (a << 16) | (b << 8) | c;

	return result;
}

static inline unsigned int packet_read_int24(mpeg3_demuxer_t *demuxer)
{
	unsigned int a, b, c, result;
//printf(__FUNCTION__ " called\n");
	a = demuxer->raw_data[demuxer->raw_offset++];
	b = demuxer->raw_data[demuxer->raw_offset++];
	c = demuxer->raw_data[demuxer->raw_offset++];
	result = (a << 16) | (b << 8) | c;

	return result;
}

static inline unsigned int packet_read_int32(mpeg3_demuxer_t *demuxer)
{
	unsigned int a, b, c, d, result;
	a = demuxer->raw_data[demuxer->raw_offset++];
	b = demuxer->raw_data[demuxer->raw_offset++];
	c = demuxer->raw_data[demuxer->raw_offset++];
	d = demuxer->raw_data[demuxer->raw_offset++];
	result = (a << 24) | (b << 16) | (c << 8) | d;

	return result;
}

static inline unsigned int packet_skip(mpeg3_demuxer_t *demuxer, int length)
{
	demuxer->raw_offset += length;
	return 0;
}

static int get_adaptation_field(mpeg3_demuxer_t *demuxer)
{
	int length;
	int pcr_flag;

//printf("get_adaptation_field %d\n", demuxer->adaptation_field_control);
	demuxer->adaptation_fields++;
/* get adaptation field length */
	length = packet_read_char(demuxer);

	if(length > 0)
	{
/* get first byte */
  		pcr_flag = (packet_read_char(demuxer) >> 4) & 1;           

		if(pcr_flag)
		{
    		unsigned int clk_ref_base = packet_read_int32(demuxer);
    		unsigned int clk_ref_ext = packet_read_int16(demuxer);

			if (clk_ref_base > 0x7fffffff)
			{   
/* correct for invalid numbers */
				clk_ref_base = 0;               /* ie. longer than 32 bits when multiplied by 2 */
				clk_ref_ext = 0;                /* multiplied by 2 corresponds to shift left 1 (<<=1) */
			}
			else 
			{
/* Create space for bit */
				clk_ref_base <<= 1; 
				clk_ref_base |= (clk_ref_ext >> 15);          /* Take bit */
				clk_ref_ext &= 0x01ff;                        /* Only lower 9 bits */
			}
			demuxer->time = ((double)clk_ref_base + clk_ref_ext / 300) / 90000;
	    	if(length) packet_skip(demuxer, length - 7);

			if(demuxer->dump)
			{
				printf(" pcr_flag=%x time=%f\n", pcr_flag, demuxer->time);
			}
		}
		else
			packet_skip(demuxer, length - 1);
	}

	return 0;
}

static int get_program_association_table(mpeg3_demuxer_t *demuxer)
{
	demuxer->program_association_tables++;
	demuxer->table_id = packet_read_char(demuxer);
	demuxer->section_length = packet_read_int16(demuxer) & 0xfff;
	demuxer->transport_stream_id = packet_read_int16(demuxer);
	packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
	if(demuxer->dump)
	{
		printf(" table_id=0x%x section_length=%d transport_stream_id=0x%x\n",
			demuxer->table_id,
			demuxer->section_length,
			demuxer->transport_stream_id);
	}
	return 0;
}

static int get_transport_payload(mpeg3_demuxer_t *demuxer, 
	int is_audio, 
	int is_video)
{
	int bytes = demuxer->raw_size - demuxer->raw_offset;

	if(bytes < 0)
	{
		printf("get_transport_payload: got negative payload size!\n");
		return 1;
	}
/*
 * 	if(demuxer->data_size + bytes > MPEG3_RAW_SIZE)
 * 		bytes = MPEG3_RAW_SIZE - demuxer->data_size;
 */

	if(demuxer->read_all && is_audio)
	{
		memcpy(demuxer->audio_buffer + demuxer->audio_size,
			demuxer->raw_data + demuxer->raw_offset,
			bytes);
		demuxer->audio_size += bytes;
	}
	else
	if(demuxer->read_all && is_video)
	{
		memcpy(demuxer->video_buffer + demuxer->video_size,
			demuxer->raw_data + demuxer->raw_offset,
			bytes);
		demuxer->video_size += bytes;
	}
	else
	{
		memcpy(demuxer->data_buffer + demuxer->data_size,
			demuxer->raw_data + demuxer->raw_offset,
			bytes);
		demuxer->data_size += bytes;
	}

	demuxer->raw_offset += bytes;
	return 0;
}

static int get_pes_packet_header(mpeg3_demuxer_t *demuxer, 
	unsigned int *pts, 
	unsigned int *dts)
{
	unsigned int pes_header_bytes = 0;
	unsigned int pts_dts_flags;
	int pes_header_data_length;

/* drop first 8 bits */
	packet_read_char(demuxer);  
	pts_dts_flags = (packet_read_char(demuxer) >> 6) & 0x3;
	pes_header_data_length = packet_read_char(demuxer);


/* Get Presentation Time stamps and Decoding Time Stamps */
	if(pts_dts_flags == 2)
	{
		*pts = (packet_read_char(demuxer) >> 1) & 7;  /* Only low 4 bits (7==1111) */
		*pts <<= 15;
		*pts |= (packet_read_int16(demuxer) >> 1);
		*pts <<= 15;
		*pts |= (packet_read_int16(demuxer) >> 1);
		pes_header_bytes += 5;
	}
	else 
	if(pts_dts_flags == 3)
	{
		*pts = (packet_read_char(demuxer) >> 1) & 7;  /* Only low 4 bits (7==1111) */
		*pts <<= 15;
		*pts |= (packet_read_int16(demuxer) >> 1);
		*pts <<= 15;
		*pts |= (packet_read_int16(demuxer) >> 1);
		*dts = (packet_read_char(demuxer) >> 1) & 7;  /* Only low 4 bits (7==1111) */
		*dts <<= 15;
		*dts |= (packet_read_int16(demuxer) >> 1);
		*dts <<= 15;
		*dts |= (packet_read_int16(demuxer) >> 1);
		pes_header_bytes += 10;
  	}

	demuxer->time = (double)*pts / 90000;

	if(demuxer->dump)
	{
		printf(" pts_dts_flags=0x%02x pts=%f dts=%f\n",
			pts_dts_flags,
			(double)*pts / 90000,
			(double)*dts / 90000);
	}


/* extract other stuff here! */
	packet_skip(demuxer, pes_header_data_length - pes_header_bytes);
	return 0;
}

static int get_unknown_data(mpeg3_demuxer_t *demuxer)
{
	int bytes = demuxer->raw_size - demuxer->raw_offset;
	memcpy(demuxer->data_buffer + demuxer->data_size,
			demuxer->raw_data + demuxer->raw_offset,
			bytes);
	demuxer->data_size += bytes;
	demuxer->raw_offset += bytes;
	return 0;
}



static int get_pes_packet_data(mpeg3_demuxer_t *demuxer)
{
	unsigned int pts = 0, dts = 0;
	get_pes_packet_header(demuxer, &pts, &dts);


	if(demuxer->stream_id == 0xbd)
	{
// AC3 audio
// Don't know if the next byte is the true stream id like in program stream
		demuxer->stream_id = 0x0;
		demuxer->got_audio = 1;
		demuxer->custom_id = demuxer->pid;

		if(demuxer->read_all)
			demuxer->astream_table[demuxer->custom_id] = AUDIO_AC3;
		if(demuxer->astream == -1)
		    demuxer->astream = demuxer->custom_id;

		if(demuxer->dump)
		{
			printf(" 0x%x bytes AC3 audio\n", demuxer->raw_size - demuxer->raw_offset);
		}

    	if((demuxer->custom_id == demuxer->astream && 
			demuxer->do_audio) ||
			demuxer->read_all)
		{
			demuxer->pes_audio_time = (double)pts / 90000;
			demuxer->audio_pid = demuxer->pid;
			return get_transport_payload(demuxer, 1, 0);
    	}

	}
	else
	if((demuxer->stream_id >> 4) == 12 || (demuxer->stream_id >> 4) == 13)
	{
// MPEG audio
		demuxer->custom_id = demuxer->pid;
		demuxer->got_audio = 1;

/* Just pick the first available stream if no ID is set */
		if(demuxer->read_all)
			demuxer->astream_table[demuxer->custom_id] = AUDIO_MPEG;
		if(demuxer->astream == -1)
		    demuxer->astream = demuxer->custom_id;

		if(demuxer->dump)
		{
			printf(" 0x%x bytes MP2 audio\n", demuxer->raw_size - demuxer->raw_offset);
		}

    	if((demuxer->custom_id == demuxer->astream && 
			demuxer->do_audio) ||
			demuxer->read_all)
		{
			demuxer->pes_audio_time = (double)pts / 90000;
			demuxer->audio_pid = demuxer->pid;

			return get_transport_payload(demuxer, 1, 0);
    	}
	}
	else 
	if((demuxer->stream_id >> 4) == 14)
	{
// Video
		demuxer->custom_id = demuxer->pid;
		demuxer->got_video = 1;


/* Just pick the first available stream if no ID is set */
		if(demuxer->read_all)
			demuxer->vstream_table[demuxer->custom_id] = 1;
		else
		if(demuxer->vstream == -1)
			demuxer->vstream = demuxer->custom_id;


		if(demuxer->dump)
		{
			printf(" 0x%x bytes video data\n", demuxer->raw_size - demuxer->raw_offset);
		}

		if((demuxer->custom_id == demuxer->vstream && 
			demuxer->do_video) ||
			demuxer->read_all)
		{
			demuxer->pes_video_time = (double)pts / 90000;
			demuxer->video_pid = demuxer->pid;
/*
 * printf("get_pes_packet_data video %04x %llx\n", 
 * stream_id, 
 * mpeg3io_tell(demuxer->titles[demuxer->current_title]->fs));
 * int i;
 * for(i = demuxer->raw_offset; i < demuxer->raw_size; i++)
 * printf("%02x ", demuxer->raw_data[i], stdout);
 * printf("\n");
 */


			return get_transport_payload(demuxer, 0, 1);
		}
	}

	packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);

	return 0;
}

static int get_pes_packet(mpeg3_demuxer_t *demuxer)
{
	demuxer->pes_packets++;



/* Skip startcode */
	packet_read_int24(demuxer);
	demuxer->stream_id = packet_read_char(demuxer);


	if(demuxer->dump)
	{
		printf(" stream_id=0x%02x\n", demuxer->stream_id);
	}

/* Skip pes packet length */
	packet_read_int16(demuxer);

	if(demuxer->stream_id != MPEG3_PRIVATE_STREAM_2 && 
		demuxer->stream_id != MPEG3_PADDING_STREAM)
	{
		return get_pes_packet_data(demuxer);
	}
	else
	if(demuxer->stream_id == MPEG3_PRIVATE_STREAM_2)
	{
/* Dump private data! */
		fprintf(stderr, "stream_id == MPEG3_PRIVATE_STREAM_2\n");
		packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
		return 0;
	}
	else
	if(demuxer->stream_id == MPEG3_PADDING_STREAM)
	{
		packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
		return 0;
	}
	else
	{
    	fprintf(stderr, "unknown stream_id in pes packet");
		return 1;
	}
	return 0;
}

static int get_payload(mpeg3_demuxer_t *demuxer)
{
//printf("get_payload 1 %x %d\n", demuxer->pid, demuxer->payload_unit_start_indicator);
	if(demuxer->payload_unit_start_indicator)
	{
    	if(demuxer->pid == 0) 
			get_program_association_table(demuxer);
    	else 
		if(packet_next_int24(demuxer) == MPEG3_PACKET_START_CODE_PREFIX) 
			get_pes_packet(demuxer);
    	else 
			packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
	}
	else
	{
		if(demuxer->dump)
		{
			printf(" 0x%x bytes elementary data\n", demuxer->raw_size - demuxer->raw_offset);
		}

    	if(demuxer->pid == demuxer->audio_pid && 
			(demuxer->do_audio ||
			demuxer->read_all))
		{
			if(demuxer->do_audio) demuxer->got_audio = 1;
			get_transport_payload(demuxer, 1, 0);
//printf("get_payload 1 %x\n", demuxer->custom_id);
		}
    	else 
		if(demuxer->pid == demuxer->video_pid && 
			(demuxer->do_video ||
			demuxer->read_all))
		{
			if(demuxer->do_video) demuxer->got_video = 1;
			get_transport_payload(demuxer, 0, 1);
		}
    	else
		{
			packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
		}
	}
	return 0;
}

/* Read a transport packet */
static int read_transport(mpeg3_demuxer_t *demuxer)
{
demuxer->dump = 0;
	mpeg3_t *file = (mpeg3_t*)demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	int result = 0;
	unsigned int bits;
	int table_entry;

/* Packet size is known for transport streams */
	demuxer->raw_size = file->packet_size;
	demuxer->raw_offset = 0;
	demuxer->stream_id = 0;
	demuxer->got_audio = 0;
	demuxer->got_video = 0;
	demuxer->custom_id = -1;


	if(result)
	{
		perror("read_transport");
		return 1;
	}

//printf("read transport 1 %llx %llx\n", title->fs->current_byte, title->fs->total_bytes);
// Search for Sync byte */
	do
	{
		bits = mpeg3io_read_char(title->fs);
	}while(!mpeg3io_eof(title->fs) && !result && bits != MPEG3_SYNC_BYTE);

/* Hit EOF */
	if(mpeg3io_eof(title->fs) || result) return 1;
/*
 * printf("read transport 2 bits=%x tell=%llx packet_size=%x\n", 
 * bits, 
 * mpeg3io_tell(title->fs), 
 * file->packet_size);
 */

	if(bits == MPEG3_SYNC_BYTE && !result)
	{
		demuxer->raw_data[0] = MPEG3_SYNC_BYTE;
		result = mpeg3io_read_data(demuxer->raw_data + 1, 
			file->packet_size - 1, 
			title->fs);
	}
	else
	{
// Assume transport streams are all one program
		demuxer->program_byte = mpeg3io_tell(title->fs) + 
			title->start_byte;
		return 1;
	}

	packet_read_char(demuxer);
    bits =  packet_read_int24(demuxer) & 0x00ffffff;
//printf("read transport 3 tell=%x bits=%x\n", mpeg3io_tell(title->fs), bits);
    demuxer->transport_error_indicator = (bits >> 23) & 0x1;
    demuxer->payload_unit_start_indicator = (bits >> 22) & 0x1;
    demuxer->pid = demuxer->custom_id = (bits >> 8) & 0x00001fff;

    demuxer->transport_scrambling_control = (bits >> 6) & 0x3;
    demuxer->adaptation_field_control = (bits >> 4) & 0x3;
    demuxer->continuity_counter = bits & 0xf;

//printf("read_transport 1 %x\n", demuxer->pid);

// This caused an audio track to not be created.
	if(demuxer->transport_error_indicator)
	{
		fprintf(stderr, 
			"demuxer->transport_error_indicator at %llx\n", 
			mpeg3io_tell(title->fs));
		demuxer->program_byte = mpeg3io_tell(title->fs) + 
			title->start_byte;
		return 0;
	}

//printf("read_transport 5 %x\n", demuxer->pid);
    if (demuxer->pid == 0x1fff)
	{
		demuxer->is_padding = 1;  /* padding; just go to next */
    }
	else
	{
		demuxer->is_padding = 0;
	}

//printf("read_transport 6 %x\n", demuxer->pid);
/* Get pid from table */
	for(table_entry = 0, result = 0; 
		table_entry < demuxer->total_pids; 
		table_entry++)
	{
		if(demuxer->pid == demuxer->pid_table[table_entry])
		{
			result = 1;
			break;
		}
	}

//printf("read_transport 7 %x\n", demuxer->pid);



/* Not in pid table */
	if(!result)
	{
		demuxer->pid_table[table_entry] = demuxer->pid;
		demuxer->continuity_counters[table_entry] = demuxer->continuity_counter;  /* init */
		demuxer->total_pids++;
	}
	result = 0;

	if(demuxer->dump)
	{
		printf("0x%llx pid=0x%02x continuity=0x%02x padding=%d adaptation=%d unit_start=%d\n", 
			demuxer->program_byte,
			demuxer->pid,
			demuxer->continuity_counter,
			demuxer->is_padding,
			demuxer->adaptation_field_control,
			demuxer->payload_unit_start_indicator);
	}


/* Abort if padding.  Should abort after demuxer->pid == 0x1fff for speed. */
	if(demuxer->is_padding)
	{
		demuxer->program_byte = mpeg3io_tell(title->fs) + 
			title->start_byte;
		return 0;
	}


#if 0
/* Check counters */
    if(demuxer->pid != MPEG3_PROGRAM_ASSOCIATION_TABLE && 
		demuxer->pid != MPEG3_CONDITIONAL_ACCESS_TABLE &&
        (demuxer->adaptation_field_control == 1 || demuxer->adaptation_field_control == 3))
	{
		if(demuxer->continuity_counters[table_entry] != demuxer->continuity_counter)
		{
//			fprintf(stderr, "demuxer->continuity_counters[table_entry] != demuxer->continuity_counter\n");
/* Reset it */
			demuxer->continuity_counters[table_entry] = demuxer->continuity_counter;
		}
		if(++(demuxer->continuity_counters[table_entry]) > 15) demuxer->continuity_counters[table_entry] = 0;
	}
#endif







    if(demuxer->adaptation_field_control & 0x2)
    	result = get_adaptation_field(demuxer);

// Need to enter in astream and vstream table:
// PID ored with stream_id
    if(demuxer->adaptation_field_control & 0x1)
    	result = get_payload(demuxer);

	demuxer->program_byte = mpeg3io_tell(title->fs) + 
		title->start_byte;
	return result;
}

static int get_system_header(mpeg3_demuxer_t *demuxer)
{
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	int length = mpeg3io_read_int16(title->fs);
	mpeg3io_seek_relative(title->fs, length);
	return 0;
}

static unsigned int get_timestamp(mpeg3_demuxer_t *demuxer)
{
	unsigned int timestamp;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

/* Only low 4 bits (7==1111) */
	timestamp = (mpeg3io_read_char(title->fs) >> 1) & 7;  
	timestamp <<= 15;
	timestamp |= (mpeg3io_read_int16(title->fs) >> 1);
	timestamp <<= 15;
	timestamp |= (mpeg3io_read_int16(title->fs) >> 1);
	return timestamp;
}

static int get_pack_header(mpeg3_demuxer_t *demuxer)
{
	unsigned int i, j;
	unsigned int clock_ref, clock_ref_ext;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

/* Get the time code */
	if((mpeg3io_next_char(title->fs) >> 4) == 2)
	{
/* MPEG-1 */
			demuxer->time = (double)get_timestamp(demuxer) / 90000;
/* Skip 3 bytes */
			mpeg3io_read_int24(title->fs);
	}
	else
	if(mpeg3io_next_char(title->fs) & 0x40)
	{
		i = mpeg3io_read_int32(title->fs);
		j = mpeg3io_read_int16(title->fs);

		if(i & 0x40000000 || (i >> 28) == 2)
		{
    		clock_ref = ((i & 0x38000000) << 3);
    		clock_ref |= ((i & 0x03fff800) << 4);
    		clock_ref |= ((i & 0x000003ff) << 5);
    		clock_ref |= ((j & 0xf800) >> 11);
    		clock_ref_ext = (j >> 1) & 0x1ff;

   			demuxer->time = (double)(clock_ref + clock_ref_ext / 300) / 90000;
/* Skip 3 bytes */
			mpeg3io_read_int24(title->fs);
			i = mpeg3io_read_char(title->fs) & 0x7;

/* stuffing */
			mpeg3io_seek_relative(title->fs, i);  
		}
	}
	else
	{
		mpeg3io_seek_relative(title->fs, 2);
	}
	return 0;
}



static int get_program_payload(mpeg3_demuxer_t *demuxer,
	int bytes,
	int is_audio,
	int is_video)
{
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

	if(demuxer->read_all && is_audio)
	{
		mpeg3io_read_data(demuxer->audio_buffer + demuxer->audio_size, 
			bytes, 
			title->fs);
		demuxer->audio_size += bytes;
	}
	else
	if(demuxer->read_all && is_video)
	{
		mpeg3io_read_data(demuxer->video_buffer + demuxer->video_size, 
			bytes, 
			title->fs);
		demuxer->video_size += bytes;
	}
	else
	{
		mpeg3io_read_data(demuxer->data_buffer + demuxer->data_size, 
			bytes, 
			title->fs);
		demuxer->data_size += bytes;
	}

	
	return 0;
}






static int handle_scrambling(mpeg3_demuxer_t *demuxer, 
	int decryption_offset)
{
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

// Advance 2048 bytes if scrambled.  We might pick up a spurrius
// packet start code in the scrambled data otherwise.
	if(demuxer->last_packet_start + 0x800 > mpeg3io_tell(title->fs))
	{
		mpeg3io_seek_relative(title->fs, 
			demuxer->last_packet_start + 0x800 - mpeg3io_tell(title->fs));
	}



// Descramble if desired.
	if(demuxer->data_size ||
		demuxer->audio_size ||
		demuxer->video_size)
	{
		unsigned char *buffer_ptr = 0;
		if(demuxer->data_size) buffer_ptr = demuxer->data_buffer;
		else
		if(demuxer->audio_size) buffer_ptr = demuxer->audio_buffer;
		else
		if(demuxer->video_size) buffer_ptr = demuxer->video_buffer;


//printf(__FUNCTION__ " data_size=%x decryption_offset=%x\n", demuxer->data_size, decryption_offset);
		if(mpeg3_decrypt_packet(title->fs->css, 
			buffer_ptr,
			decryption_offset))
		{
			fprintf(stderr, "handle_scrambling: Decryption not available\n");
			return 1;
		}
	}

	return 0;
}






static int handle_pcm(mpeg3_demuxer_t *demuxer, int bytes)
{
/* Synthesize PCM header and delete MPEG header for PCM data */
/* Must be done after decryption */
	unsigned char code;
	int bits_code;
	int bits;
	int samplerate_code;
	int samplerate;
	unsigned char *output = 0;
	unsigned char *data_buffer = 0;
	int data_start = 0;
	int *data_size = 0;
	int i, j;




	if(demuxer->read_all && demuxer->audio_size)
	{
		output = demuxer->audio_buffer + demuxer->audio_start;
		data_buffer = demuxer->audio_buffer;
		data_start = demuxer->audio_start;
		data_size = &demuxer->audio_size;
	}
	else
	{
		output = demuxer->data_buffer + demuxer->data_start;
		data_buffer = demuxer->data_buffer;
		data_start = demuxer->data_start;
		data_size = &demuxer->data_size;
	}




/* Shift audio back */
	code = output[1];
	for(i = *data_size - 1, j = *data_size + PCM_HEADERSIZE - 3 - 1; 
		i >= data_start;
		i--, j--)
		*(data_buffer + j) = *(data_buffer + i);
	*data_size += PCM_HEADERSIZE - 3;

	bits_code = (code >> 6) & 3;
	samplerate_code = (code & 0x10);


	output[0] = 0x7f;
	output[1] = 0x7f;
	output[2] = 0x80;
	output[3] = 0x7f;
/* Samplerate */
	switch(samplerate_code)
	{
		case 1:
			samplerate = 96000;
			break;
		default:
			samplerate = 48000;
			break;
	}

	*(int32_t*)(output + 4) = samplerate;
/* Bits */
	switch(bits_code)
	{
		case 0: bits = 16; break;
		case 1: bits = 20; break;
		case 2: bits = 24; break;
		default: bits = 16; break;						
	}
	*(int32_t*)(output + 8) = bits;
/* Channels */
	*(int32_t*)(output + 12) = (code & 0x7) + 1;
/* Framesize */
	*(int32_t*)(output + 16) = bytes - 
		3 + 
		PCM_HEADERSIZE;



	
//printf(__FUNCTION__ " %02x%02x%02x %d\n", code1, code, code2, pes_packet_length - 3);
}






/* Program packet reading core */
static int get_ps_pes_packet(mpeg3_demuxer_t *demuxer, unsigned int header)
{
	unsigned int pts = 0, dts = 0;
	int pes_packet_length;
	int64_t pes_packet_start;
	int decryption_offset;
	int i;
	mpeg3_t *file = demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	int scrambling = 0;
/* Format if audio */
	int do_pcm = 0;

	demuxer->data_start = demuxer->data_size;
	demuxer->audio_start = demuxer->audio_size;
	demuxer->video_start = demuxer->video_size;

	demuxer->stream_id = header & 0xff;
	pes_packet_length = mpeg3io_read_int16(title->fs);
	pes_packet_start = mpeg3io_tell(title->fs);



/*
 * printf(__FUNCTION__ " pes_packet_start=%llx pes_packet_length=%x data_size=%x\n", 
 * pes_packet_start, 
 * pes_packet_length,
 * demuxer->data_size);
 */





	if(demuxer->stream_id != MPEG3_PRIVATE_STREAM_2 &&
		demuxer->stream_id != MPEG3_PADDING_STREAM)
	{
		if((mpeg3io_next_char(title->fs) >> 6) == 0x02)
		{
/* Get MPEG-2 packet */
			int pes_header_bytes = 0;
    		int pts_dts_flags;
			int pes_header_data_length;


			demuxer->last_packet_decryption = mpeg3io_tell(title->fs);
			scrambling = mpeg3io_read_char(title->fs) & 0x30;
//scrambling = 1;
/* Reset scrambling bit for the mpeg3cat utility */
//			if(scrambling) demuxer->raw_data[demuxer->raw_offset - 1] &= 0xcf;
// Force packet length if scrambling
			if(scrambling) pes_packet_length = 0x800 - 
				pes_packet_start + 
				demuxer->last_packet_start;


    		pts_dts_flags = (mpeg3io_read_char(title->fs) >> 6) & 0x3;
			pes_header_data_length = mpeg3io_read_char(title->fs);



/* Get Presentation and Decoding Time Stamps */
			if(pts_dts_flags == 2)
			{
				pts = get_timestamp(demuxer);
				if(demuxer->dump)
				{
					printf("pts=%d\n", pts);
				}
				pes_header_bytes += 5;
			}
    		else 
			if(pts_dts_flags == 3)
			{
				pts = get_timestamp(demuxer);
				dts = get_timestamp(demuxer);
				if(demuxer->dump)
				{
					printf("pts=%d dts=%d\n", pts, dts);
				}
/*
 *         		pts = (mpeg3io_read_char(title->fs) >> 1) & 7;
 *         		pts <<= 15;
 *         		pts |= (mpeg3io_read_int16(title->fs) >> 1);
 *         		pts <<= 15;
 *         		pts |= (mpeg3io_read_int16(title->fs) >> 1);
 */

/*
 *         		dts = (mpeg3io_read_char(title->fs) >> 1) & 7;
 *         		dts <<= 15;
 *         		dts |= (mpeg3io_read_int16(title->fs) >> 1);
 *         		dts <<= 15;
 *         		dts |= (mpeg3io_read_int16(title->fs) >> 1);
 */
        		pes_header_bytes += 10;
    		}

//printf("get_ps_pes_packet do_audio=%d do_video=%d pts=%x dts=%x\n", 
//	demuxer->do_audio, demuxer->do_video, pts, dts);

/* Skip unknown */
        	mpeg3io_seek_relative(title->fs, 
				pes_header_data_length - pes_header_bytes);
		}
		else
		{
			int pts_dts_flags;
/* Get MPEG-1 packet */
			while(mpeg3io_next_char(title->fs) == 0xff)
			{
				mpeg3io_read_char(title->fs);
			}

/* Skip STD buffer scale */
			if((mpeg3io_next_char(title->fs) & 0x40) == 0x40)
			{
				mpeg3io_seek_relative(title->fs, 2);
			}

/* Decide which timestamps are available */
			pts_dts_flags = mpeg3io_next_char(title->fs);

			if(pts_dts_flags >= 0x30)
			{
/* Get the presentation and decoding time stamp */
				pts = get_timestamp(demuxer);
				dts = get_timestamp(demuxer);
			}
			else
			if(pts_dts_flags >= 0x20)
			{
/* Get just the presentation time stamp */
				pts = get_timestamp(demuxer);
			}
			else
			if(pts_dts_flags == 0x0f)
			{
/* End of timestamps */
				mpeg3io_read_char(title->fs);
			}
			else
			{
				return 1;     /* Error */
			}
		}

/* Now extract the payload. */
		if((demuxer->stream_id >> 4) == 0xc || (demuxer->stream_id >> 4) == 0xd)
		{
/* Audio data */
/* Take first stream ID if -1 */
			pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;

			demuxer->got_audio = 1;
			demuxer->custom_id = demuxer->stream_id & 0x0f;


			if(demuxer->read_all)
				demuxer->astream_table[demuxer->custom_id] = AUDIO_MPEG;
			else
			if(demuxer->astream == -1) 
				demuxer->astream = demuxer->custom_id;




			if(pts > 0) demuxer->pes_audio_time = (double)pts / 60000;


			if(demuxer->custom_id == demuxer->astream && 
				demuxer->do_audio ||
				demuxer->read_all)
			{
				decryption_offset = mpeg3io_tell(title->fs) - demuxer->last_packet_start;
				if(demuxer->dump)
				{
					printf(" MP2 audio data offset=%llx custom_id=%x size=%x\n", 
						demuxer->program_byte, 
						demuxer->custom_id,
						pes_packet_length);
				}

				get_program_payload(demuxer, pes_packet_length, 1, 0);
		  	}
			else 
			{
    			mpeg3io_seek_relative(title->fs, pes_packet_length);
			}
		}
    	else 
		if((demuxer->stream_id >> 4) == 0xe)
		{
/* Video data */
/* Take first stream ID if -1 */
			pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;

			demuxer->got_video = 1;
			demuxer->custom_id = demuxer->stream_id & 0x0f;


			if(demuxer->read_all) 
				demuxer->vstream_table[demuxer->custom_id] = 1;
			else
			if(demuxer->vstream == -1) 
				demuxer->vstream = demuxer->custom_id;

			if(pts > 0) demuxer->pes_video_time = (double)pts / 60000;



    	    if(demuxer->custom_id == demuxer->vstream && 
				demuxer->do_video ||
				demuxer->read_all)
			{
				decryption_offset = mpeg3io_tell(title->fs) - demuxer->last_packet_start;
				if(demuxer->dump)
				{
					printf(" video offset=%llx custom_id=%x size=%x\n", 
						demuxer->program_byte,
						demuxer->custom_id,
						pes_packet_length);
				}


				get_program_payload(demuxer,
					pes_packet_length,
					0,
					1);
    	  	}
    		else 
			{
				if(demuxer->dump)
				{
					printf(" skipping video size=%x\n", pes_packet_length);
				}
    			mpeg3io_seek_relative(title->fs, pes_packet_length);
    		}
    	}
    	else 
		if((demuxer->stream_id == 0xbd || demuxer->stream_id == 0xbf) && 
			mpeg3io_next_char(title->fs) != 0xff &&
			((mpeg3io_next_char(title->fs) & 0xf0) != 0x20))
		{
			int format;
/* DVD audio data */
/* Get the audio format */
			if((mpeg3io_next_char(title->fs) & 0xf0) == 0xa0)
				format = AUDIO_PCM;
			else
				format = AUDIO_AC3;


// Picks up bogus data if (& 0xf) or (& 0x7f)
			demuxer->stream_id = mpeg3io_next_char(title->fs);
			if(pts > 0) demuxer->pes_audio_time = (double)pts / 60000;

			demuxer->got_audio = 1;
			demuxer->custom_id = demuxer->stream_id;


/* Take first stream ID if not building TOC. */
			if(demuxer->read_all)
				demuxer->astream_table[demuxer->custom_id] = format;
			else
			if(demuxer->astream == -1)
				demuxer->astream = demuxer->custom_id;







      		if(demuxer->custom_id == demuxer->astream && 
				demuxer->do_audio ||
				demuxer->read_all)
			{
				demuxer->aformat = format;
				mpeg3io_read_int32(title->fs);

				pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
				decryption_offset = mpeg3io_tell(title->fs) - demuxer->last_packet_start;

				if(format == AUDIO_PCM) do_pcm = 1;
//printf("get_ps_pes_packet 5 %x\n", decryption_offset);

				if(demuxer->dump)
				{
					printf(" AC3 audio data size=%x\n", pes_packet_length);
				}

				get_program_payload(demuxer,
					pes_packet_length,
					1,
					0);
      		}
      		else
			{
				pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
    			mpeg3io_seek_relative(title->fs, pes_packet_length);
      		}
//printf("get_ps_pes_packet 6 %d\n", demuxer->astream_table[0x20]);
    	}
    	else 
		if(demuxer->stream_id == 0xbc || 1)
		{
			pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
    		mpeg3io_seek_relative(title->fs, pes_packet_length);
    	}
	}
  	else 
	if(demuxer->stream_id == MPEG3_PRIVATE_STREAM_2 || demuxer->stream_id == MPEG3_PADDING_STREAM)
	{
		pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
    	mpeg3io_seek_relative(title->fs, pes_packet_length);
  	}




	if(scrambling) handle_scrambling(demuxer, decryption_offset);



	if(do_pcm) handle_pcm(demuxer, pes_packet_length);








	return 0;
}

int mpeg3demux_read_program(mpeg3_demuxer_t *demuxer)
{
	int result = 0;
	int count = 0;
	mpeg3_t *file = demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	unsigned int header = 0;
	int pack_count = 0;

	demuxer->got_audio = 0;
	demuxer->got_video = 0;
	demuxer->stream_id = 0;
	demuxer->custom_id = -1;

	if(mpeg3io_eof(title->fs)) return 1;

//printf("mpeg3demux_read_program 2 %d %x %llx\n", result, title->fs->current_byte, title->fs->total_bytes);







/* Search for next header */
/* Parse packet until the next packet start code */
	while(!result && !mpeg3io_eof(title->fs))
	{
		header = mpeg3io_read_int32(title->fs);


		if(header == MPEG3_PACK_START_CODE)
		{


// Second start code in this call.  Don't read it.
			if(pack_count)
			{
				mpeg3io_seek_relative(title->fs, -4);
				break;
			}

			demuxer->last_packet_start = mpeg3io_tell(title->fs) - 4;
			result = get_pack_header(demuxer);
			pack_count++;



		}
		else
		if(header == MPEG3_SYSTEM_START_CODE && pack_count)
		{



 			result = get_system_header(demuxer);



		}
		else
		if((header >> 8) == MPEG3_PACKET_START_CODE_PREFIX && pack_count)
		{


			result = get_ps_pes_packet(demuxer, header);



		}
		else
		{
// Try again.
			mpeg3io_seek_relative(title->fs, -3);
		}
	}







// Ignore errors in the parsing.  Just quit if eof.
	result = 0;






	demuxer->last_packet_end = mpeg3io_tell(title->fs);

/*
 * if(demuxer->last_packet_end - demuxer->last_packet_start != 0x800)
 * printf(__FUNCTION__ " packet_size=%llx data_size=%x pack_count=%d packet_start=%llx packet_end=%llx\n",
 * demuxer->last_packet_end - demuxer->last_packet_start,
 * demuxer->data_size,
 * pack_count,
 * demuxer->last_packet_start,
 * demuxer->last_packet_end);
 */
//printf("mpeg3demux_read_program 5 %d\n", result);

//printf("read_program 3\n");
//	if(!result) result = mpeg3io_eof(title->fs);

	demuxer->program_byte = 
		mpeg3_absolute_to_program(demuxer, mpeg3io_tell(title->fs) +
			title->start_byte);
	return result;
}




// Point the current title and current cell to the program byte.
static int get_current_cell(mpeg3_demuxer_t *demuxer)
{
	int got_it = 0;
	int result = 0;

/* Find first cell on or after current position */
	if(demuxer->reverse)
	{
		for(demuxer->current_title = demuxer->total_titles - 1;
			demuxer->current_title >= 0;
			demuxer->current_title--)
		{
			mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
			for(demuxer->title_cell = title->cell_table_size - 1;
				demuxer->title_cell >= 0;
				demuxer->title_cell--)
			{
				mpeg3_cell_t *cell = &title->cell_table[demuxer->title_cell];
				if(cell->program_start < demuxer->program_byte &&
					cell->program == demuxer->current_program)
				{
					got_it = 1;
					if(demuxer->program_byte > cell->program_end)
						demuxer->program_byte = cell->program_end;
					break;
				}
			}
			if(got_it) break;
		}

		if(!got_it)
		{
			demuxer->current_title = 0;
			demuxer->title_cell = 0;
			result = 1;
		}
	}
	else
	{
		for(demuxer->current_title = 0;
			demuxer->current_title < demuxer->total_titles;
			demuxer->current_title++)
		{
			mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
			for(demuxer->title_cell = 0;
				demuxer->title_cell < title->cell_table_size;
				demuxer->title_cell++)
			{
				mpeg3_cell_t *cell = &title->cell_table[demuxer->title_cell];
				if(cell->program_end > demuxer->program_byte &&
					cell->program == demuxer->current_program)
				{
					got_it = 1;
					if(demuxer->program_byte < cell->program_start)
						demuxer->program_byte = cell->program_start;
					break;
				}
			}
			if(got_it) break;
		}

		if(!got_it)
		{
			demuxer->current_title = demuxer->total_titles - 1;
			mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
			demuxer->title_cell = title->cell_table_size - 1;
			result = 1;
		}
	}


//printf("get_current_cell 2 %p %d %d\n", demuxer, demuxer->current_title, demuxer->title_cell);
	return result;
}





int mpeg3_seek_phys(mpeg3_demuxer_t *demuxer)
{
	int result = 0;

// This happens if the video header is bigger than the buffer size in a table
// of contents scan.
	if(demuxer->current_title < 0 || 
		demuxer->current_title >= demuxer->total_titles)
	{
		printf("mpeg3_seek_phys demuxer=%p read_all=%d do_audio=%d do_video=%d demuxer->current_title=%d\n",
			demuxer,
			demuxer->read_all,
			demuxer->do_audio,
			demuxer->do_video,
			demuxer->current_title);
		return 1;
	}

	if(!demuxer->titles) return 1;

//printf("%d %d\n", demuxer->current_title, demuxer->title_cell);
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

	if(!title->cell_table) return 1;

	mpeg3_cell_t *cell = &title->cell_table[demuxer->title_cell];


/* Don't do anything if we're in the cell and it's the right program */
	if(demuxer->reverse)
	{
		if(demuxer->program_byte > cell->program_start &&
			demuxer->program_byte <= cell->program_end &&
			cell->program == demuxer->current_program)
		{
			goto do_phys_seek;
		}
	}
	else
	{
// End of stream
		if(demuxer->stream_end > 0 &&
			demuxer->program_byte >= demuxer->stream_end) return 1;

// In same cell
		if(demuxer->program_byte >= cell->program_start &&
			demuxer->program_byte < cell->program_end &&
			cell->program == demuxer->current_program)
			goto do_phys_seek;
	}



// Need to change cells if we get here.
	int last_cell = demuxer->title_cell;
	int last_title = demuxer->current_title;
	int64_t last_byte = demuxer->program_byte;
	int got_it = 0;

	result = get_current_cell(demuxer);
// End of file
	if(result) return 1;

	if(demuxer->current_title != last_title)
	{
		mpeg3demux_open_title(demuxer, demuxer->current_title);
	}

	title = demuxer->titles[demuxer->current_title];
	cell = &title->cell_table[demuxer->title_cell];


do_phys_seek:
	mpeg3io_seek(title->fs, 
		demuxer->program_byte - cell->program_start + cell->title_start);

	return result;
}







static int next_code(mpeg3_demuxer_t *demuxer,
	uint32_t code)
{
	uint32_t result = 0;
	int error = 0;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	mpeg3_fs_t *fd = title->fs;

	while(result != code &&
		!error)
	{
		title = demuxer->titles[demuxer->current_title];
		result <<= 8;
		result |= (unsigned char)mpeg3io_read_char(title->fs);
		demuxer->program_byte++;
		error = mpeg3_seek_phys(demuxer);
	}
	return error;
}










/* Read packet in the forward direction */
int mpeg3_read_next_packet(mpeg3_demuxer_t *demuxer)
{
	if(demuxer->current_title < 0) return 1;

	int result = 0;
	int current_position;
	mpeg3_t *file = demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];


//printf("mpeg3_read_next_packet 1 %lld\n", demuxer->program_byte);
/* Reset output buffer */
	demuxer->data_size = 0;
	demuxer->data_position = 0;
	demuxer->audio_size = 0;
	demuxer->video_size = 0;

/* Switch to forward direction. */
	if(demuxer->reverse)
	{
/* Don't reread anything if we're at the beginning of the file. */
		if(demuxer->program_byte < 0)
		{
			demuxer->program_byte = 0;
			result = mpeg3_seek_phys(demuxer);
/* First character was the -1 byte which brings us to 0 after this function. */
			result = 1;
		}
		else
/* Transport or elementary stream */
		if(file->packet_size > 0)
		{
			demuxer->program_byte += file->packet_size;
			result = mpeg3_seek_phys(demuxer);
		}
		else
		{
/* Packet just read */
			if(!result) result = next_code(demuxer, 
				MPEG3_PACK_START_CODE);
/* Next packet */
			if(!result) result = next_code(demuxer, 
				MPEG3_PACK_START_CODE);
		}

		demuxer->reverse = 0;
	}







/* Read packets until the output buffer is full. */
/* Read a single packet if not fetching audio or video. */
	if(!result)
	{
		do
		{
			title = demuxer->titles[demuxer->current_title];

			if(!result)
			{
				if(file->is_transport_stream)
				{
					result = mpeg3_seek_phys(demuxer);
					if(!result) result = read_transport(demuxer);
				}
				else
				if(file->is_program_stream)
				{
					result = mpeg3_seek_phys(demuxer);
					if(!result) result = mpeg3demux_read_program(demuxer);
				}
				else
				if(demuxer->read_all && file->is_audio_stream)
				{
/* Read elementary stream. */
					result = mpeg3io_read_data(demuxer->audio_buffer, 
						file->packet_size, title->fs);
					demuxer->audio_size = file->packet_size;
					demuxer->program_byte += file->packet_size;
					result |= mpeg3_seek_phys(demuxer);
				}
				else
				if(demuxer->read_all && file->is_video_stream)
				{
/* Read elementary stream. */
					result = mpeg3io_read_data(demuxer->video_buffer, 
						file->packet_size, title->fs);
						demuxer->video_size = file->packet_size;
						demuxer->program_byte += file->packet_size;
						result |= mpeg3_seek_phys(demuxer);
				}
				else
				{
					result = mpeg3io_read_data(demuxer->data_buffer, 
						file->packet_size, title->fs);
					demuxer->data_size = file->packet_size;
					demuxer->program_byte += file->packet_size;
					result |= mpeg3_seek_phys(demuxer);
				}
			}
		}while(!result && 
			demuxer->data_size == 0 && 
			(demuxer->do_audio || demuxer->do_video));
	}

	return result;
}




static int prev_code(mpeg3_demuxer_t *demuxer,
	uint32_t code)
{
	uint32_t result = 0;
	int error = 0;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	mpeg3_fs_t *fd = title->fs;


	while(result != code &&
		demuxer->program_byte > 0 && 
		!error)
	{
		result >>= 8;
		title = demuxer->titles[demuxer->current_title];
		mpeg3io_seek(title->fs, demuxer->program_byte - title->start_byte - 1LL);
		result |= ((uint32_t)mpeg3io_read_char(title->fs)) << 24;
		demuxer->program_byte--;
		error = mpeg3_seek_phys(demuxer);
	}
	return error;
}





/* Read the packet right before the packet we're currently on. */
int mpeg3_read_prev_packet(mpeg3_demuxer_t *demuxer)
{
	int result = 0;
	mpeg3_t *file = demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

	demuxer->data_size = 0;
	demuxer->data_position = 0;

 

//printf("mpeg3_read_prev_packet 1 %d %d %llx\n", result, demuxer->current_title, mpeg3io_tell(title->fs));
/* Switch to reverse direction */
	if(!demuxer->reverse)
	{
		demuxer->reverse = 1;

/* Transport stream or elementary stream case */
		if(file->packet_size > 0)
		{
			demuxer->program_byte -= file->packet_size;
			result = mpeg3_seek_phys(demuxer);
		}
		else
/* Program stream */
		{
			result = prev_code(demuxer, 
				MPEG3_PACK_START_CODE);
		}

	}







/* Go to beginning of previous packet */
	do
	{
		title = demuxer->titles[demuxer->current_title];

/* Transport stream or elementary stream case */
		if(file->packet_size > 0)
		{
			demuxer->program_byte -= file->packet_size;
			result = mpeg3_seek_phys(demuxer);
		}
		else
		{
			if(!result) result = prev_code(demuxer, 
				MPEG3_PACK_START_CODE);
		}



/* Read packet and then rewind it */
		title = demuxer->titles[demuxer->current_title];
		if(file->is_transport_stream && !result)
		{
			result = read_transport(demuxer);

			if(demuxer->program_byte > 0)
			{
				demuxer->program_byte -= file->packet_size;
				result = mpeg3_seek_phys(demuxer);
			}
		}
		else
		if(file->is_program_stream && !result)
		{
			int64_t current_position = demuxer->program_byte;

/* Read packet */
			result = mpeg3demux_read_program(demuxer);

/* Rewind packet */
			while(demuxer->program_byte > current_position &&
				!result)
			{
				result = prev_code(demuxer, 
					MPEG3_PACK_START_CODE);
			}
		}
		else
		if(!result)
		{
/* Elementary stream */
/* Read the packet forwards and seek back to the start */
			result = mpeg3io_read_data(demuxer->data_buffer, 
				file->packet_size, 
				title->fs);

			if(!result)
			{
				demuxer->data_size = file->packet_size;
				result = mpeg3io_seek(title->fs, demuxer->program_byte);
			}
		}
	}while(!result && 
		demuxer->data_size == 0 && 
		(demuxer->do_audio || demuxer->do_video));


	return result;
}


/* For audio */
int mpeg3demux_read_data(mpeg3_demuxer_t *demuxer, 
		unsigned char *output, 
		int size)
{
	int result = 0;
	demuxer->error_flag = 0;

	if(demuxer->data_position >= 0)
	{
/* Read forwards */
		int i;
		for(i = 0; i < size && !result; )
		{
			int fragment_size = size - i;
			if(fragment_size > demuxer->data_size - demuxer->data_position)
				fragment_size = demuxer->data_size - demuxer->data_position;
			memcpy(output + i, demuxer->data_buffer + demuxer->data_position, fragment_size);
			demuxer->data_position += fragment_size;
			i += fragment_size;

			if(i < size)
			{
				result = mpeg3_read_next_packet(demuxer);
			}
		}
	}
	else
	{
		int current_position;
/* Read backwards a full packet. */
/* Only good for reading less than the size of a full packet, but */
/* this routine should only be used for searching for previous markers. */
		current_position = demuxer->data_position;
		result = mpeg3_read_prev_packet(demuxer);
		if(!result) demuxer->data_position = demuxer->data_size + current_position;
		memcpy(output, demuxer->data_buffer + demuxer->data_position, size);
		demuxer->data_position += size;
	}

//printf("mpeg3demux_read_data 2\n");
	demuxer->error_flag = result;
	return result;
}

unsigned char mpeg3demux_read_char_packet(mpeg3_demuxer_t *demuxer)
{
	demuxer->error_flag = 0;
	demuxer->next_char = -1;

	if(demuxer->data_position >= demuxer->data_size)
	{
		demuxer->error_flag = mpeg3_read_next_packet(demuxer);
	}

	if(!demuxer->error_flag) 
		demuxer->next_char = demuxer->data_buffer[demuxer->data_position++];

	return demuxer->next_char;
}

unsigned char mpeg3demux_read_prev_char_packet(mpeg3_demuxer_t *demuxer)
{
	demuxer->error_flag = 0;
	demuxer->data_position--;
	if(demuxer->data_position < 0)
	{
		demuxer->error_flag = mpeg3_read_prev_packet(demuxer);
		if(!demuxer->error_flag) demuxer->data_position = demuxer->data_size - 1;
	}

	if(demuxer->data_position >= 0)
	{
		demuxer->next_char = demuxer->data_buffer[demuxer->data_position];
	}
	return demuxer->next_char;
}



int mpeg3demux_open_title(mpeg3_demuxer_t *demuxer, int title_number)
{
	mpeg3_title_t *title;

	if(title_number < demuxer->total_titles &&
		title_number >= 0)
	{
		if(demuxer->current_title >= 0)
		{
			mpeg3io_close_file(demuxer->titles[demuxer->current_title]->fs);
			demuxer->current_title = -1;
		}

		title = demuxer->titles[title_number];

		if(mpeg3io_open_file(title->fs))
		{
			demuxer->error_flag = 1;
			fprintf(stderr, "mpeg3demux_open_title %s: %s", title->fs->path, strerror(errno));
		}
		else
		{
			demuxer->current_title = title_number;
		}
	}
	else
	{
		fprintf(stderr, "mpeg3demux_open_title title_number = %d\n",
			title_number);
	}


	return demuxer->error_flag;
}

int mpeg3demux_copy_titles(mpeg3_demuxer_t *dst, mpeg3_demuxer_t *src)
{
	int i;
	mpeg3_t *file = dst->file;
	mpeg3_title_t *dst_title, *src_title;

	dst->total_titles = src->total_titles;
	dst->total_programs = src->total_programs;
	for(i = 0; i < MPEG3_MAX_STREAMS; i++)
	{
		dst->astream_table[i] = src->astream_table[i];
		dst->vstream_table[i] = src->vstream_table[i];
	}
	for(i = 0; i < src->total_titles; i++)
	{
		src_title = src->titles[i];
		dst_title = dst->titles[i] = mpeg3_new_title(file, 
			src->titles[i]->fs->path);
		mpeg3_copy_title(dst_title, src_title);
	}

	mpeg3demux_open_title(dst, src->current_title);
	dst->title_cell = 0;
	return 0;
}

/* ==================================================================== */
/*                            Entry points */
/* ==================================================================== */

mpeg3_demuxer_t* mpeg3_new_demuxer(mpeg3_t *file, int do_audio, int do_video, int custom_id)
{
	mpeg3_demuxer_t *demuxer = calloc(1, sizeof(mpeg3_demuxer_t));
	int i;

/* The demuxer will change the default packet size for its own use. */
	demuxer->file = file;
	demuxer->do_audio = do_audio;
	demuxer->do_video = do_video;

/* Allocate buffer + padding */
	demuxer->raw_data = calloc(1, MPEG3_RAW_SIZE);
	demuxer->data_buffer = calloc(1, MPEG3_RAW_SIZE);
	demuxer->data_allocated = MPEG3_RAW_SIZE;

	demuxer->audio_buffer = calloc(1, MPEG3_RAW_SIZE);
	demuxer->audio_allocated = MPEG3_RAW_SIZE;

	demuxer->video_buffer = calloc(1, MPEG3_RAW_SIZE);
	demuxer->video_allocated = MPEG3_RAW_SIZE;

/* System specific variables */
	demuxer->audio_pid = custom_id;
	demuxer->video_pid = custom_id;
	demuxer->astream = custom_id;
	demuxer->vstream = custom_id;
	demuxer->current_title = -1;
	demuxer->pes_audio_time = -1;
	demuxer->pes_video_time = -1;
//printf("mpeg3_new_demuxer %f\n", demuxer->time);
	demuxer->stream_end = -1;

	return demuxer;
}

int mpeg3_delete_demuxer(mpeg3_demuxer_t *demuxer)
{
	int i;

	if(demuxer->current_title >= 0)
	{
		mpeg3io_close_file(demuxer->titles[demuxer->current_title]->fs);
	}

	for(i = 0; i < demuxer->total_titles; i++)
	{
		mpeg3_delete_title(demuxer->titles[i]);
	}

	if(demuxer->data_buffer) free(demuxer->data_buffer);
	if(demuxer->raw_data) free(demuxer->raw_data);
	if(demuxer->audio_buffer) free(demuxer->audio_buffer);
	if(demuxer->video_buffer) free(demuxer->video_buffer);
	free(demuxer);
	return 0;
}


int mpeg3demux_eof(mpeg3_demuxer_t *demuxer)
{
	mpeg3_t *file = demuxer->file;
	if(file->seekable)
	{
		if(demuxer->current_title >= 0)
		{
			if(mpeg3io_eof(demuxer->titles[demuxer->current_title]->fs) &&
				demuxer->current_title >= demuxer->total_titles - 1)
				return 1;
		}
	}
	else
	{
		if(demuxer->data_position >= demuxer->data_size) return 1;
	}

	return 0;
}

int mpeg3demux_bof(mpeg3_demuxer_t *demuxer)
{
	if(demuxer->current_title >= 0)
	{
		if(mpeg3io_bof(demuxer->titles[demuxer->current_title]->fs) &&
			demuxer->current_title <= 0)
			return 1;
	}
	return 0;
}

void mpeg3demux_start_reverse(mpeg3_demuxer_t *demuxer)
{
	demuxer->reverse = 1;
}

void mpeg3demux_start_forward(mpeg3_demuxer_t *demuxer)
{
	demuxer->reverse = 0;
}

/* Seek to absolute byte */
int mpeg3demux_seek_byte(mpeg3_demuxer_t *demuxer, int64_t byte)
{
	mpeg3_t *file = demuxer->file;

/*
 * int i;
 * for(i = 0; i < demuxer->total_titles; i++) mpeg3_dump_title(demuxer->titles[i]);
 */

	demuxer->program_byte = byte;
	demuxer->data_position = 0;
	demuxer->data_size = 0;

/* Get on a packet boundary only for transport streams. */
	if(file->is_transport_stream &&
		file->packet_size)
	{
		demuxer->program_byte -= demuxer->program_byte % file->packet_size;
	}

	int result = mpeg3_seek_phys(demuxer);
/*
 * printf("mpeg3demux_seek_byte 1 %d %d %lld %lld\n", 
 * demuxer->do_video, result, byte, demuxer->program_byte);
 */

	return result;
}








double mpeg3demux_get_time(mpeg3_demuxer_t *demuxer)
{
	return demuxer->time;
}

double mpeg3demux_audio_pts(mpeg3_demuxer_t *demuxer)
{
	return demuxer->pes_audio_time;
}

double mpeg3demux_video_pts(mpeg3_demuxer_t *demuxer)
{
	return demuxer->pes_video_time;
}

void mpeg3demux_reset_pts(mpeg3_demuxer_t *demuxer)
{
	demuxer->pes_audio_time = -1;
	demuxer->pes_video_time = -1;
}

double mpeg3demux_scan_pts(mpeg3_demuxer_t *demuxer)
{
	int64_t start_position = mpeg3demux_tell_byte(demuxer);
	int64_t end_position = start_position + MPEG3_PTS_RANGE;
	int64_t current_position = start_position;
	int result = 0;

	mpeg3demux_reset_pts(demuxer);
	while(!result && 
		current_position < end_position &&
		((demuxer->do_audio && demuxer->pes_audio_time < 0) ||
		(demuxer->do_video && demuxer->pes_video_time < 0)))
	{
		result = mpeg3_read_next_packet(demuxer);
		current_position = mpeg3demux_tell_byte(demuxer);
	}

// Seek back to starting point
	mpeg3demux_seek_byte(demuxer, start_position);

	if(demuxer->do_audio) return demuxer->pes_audio_time;
	if(demuxer->do_video) return demuxer->pes_video_time;
}

int mpeg3demux_goto_pts(mpeg3_demuxer_t *demuxer, double pts)
{
	int64_t start_position = mpeg3demux_tell_byte(demuxer);
	int64_t end_position = start_position + MPEG3_PTS_RANGE;
	int64_t current_position = start_position;
	int result = 0;

// Search forward for nearest pts
	mpeg3demux_reset_pts(demuxer);
	while(!result && current_position < end_position)
	{
		result = mpeg3_read_next_packet(demuxer);
		if(demuxer->pes_audio_time > pts) break;
		current_position = mpeg3demux_tell_byte(demuxer);
	}

// Search backward for nearest pts
	end_position = current_position - MPEG3_PTS_RANGE;
	mpeg3_read_prev_packet(demuxer);
	while(!result &&
		current_position > end_position)
	{
		result = mpeg3_read_prev_packet(demuxer);
		if(demuxer->pes_audio_time < pts) break;
		current_position = mpeg3demux_tell_byte(demuxer);
	}
}


int64_t mpeg3demux_tell_byte(mpeg3_demuxer_t *demuxer)
{
	return demuxer->program_byte;
}

int mpeg3demux_tell_program(mpeg3_demuxer_t *demuxer)
{
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	if(!title->cell_table || !title->cell_table_size) return 0;
	mpeg3_cell_t *cell = &title->cell_table[demuxer->title_cell];
	return cell->program;
}

int64_t mpeg3_absolute_to_program(mpeg3_demuxer_t *demuxer,
	int64_t byte)
{
	int i, j;

//fprintf(stderr, "%d\n", demuxer->data_size);
// Can only offset to current cell since we can't know what cell the 
// byte corresponds to.
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	mpeg3_cell_t *cell = &title->cell_table[demuxer->title_cell];
	return byte - 
		cell->title_start - 
		title->start_byte + 
		cell->program_start;
}

int64_t mpeg3demux_movie_size(mpeg3_demuxer_t *demuxer)
{
	if(!demuxer->total_bytes)
	{
		int64_t result = 0;
		int i, j;
		for(i = 0; i < demuxer->total_titles; i++)
		{
			mpeg3_title_t *title = demuxer->titles[i];
			for(j = 0; j < title->cell_table_size; j++)
			{
				mpeg3_cell_t *cell = &title->cell_table[j];
				if(cell->program == demuxer->current_program)
					result += cell->program_end - cell->program_start;
			}
//			result += demuxer->titles[i]->total_bytes;
		}
		demuxer->total_bytes = result;
	}
	return demuxer->total_bytes;
}

int64_t mpeg3demuxer_title_bytes(mpeg3_demuxer_t *demuxer)
{
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	return title->total_bytes;
}

mpeg3_demuxer_t* mpeg3_get_demuxer(mpeg3_t *file)
{
	if(file->is_program_stream || file->is_transport_stream)
	{
		if(file->total_astreams) return file->atrack[0]->demuxer;
		else
		if(file->total_vstreams) return file->vtrack[0]->demuxer;
	}
	return 0;
}

void mpeg3demux_append_data(mpeg3_demuxer_t *demuxer, 
	unsigned char *data, 
	int bytes)
{
	int new_data_size = demuxer->data_size + bytes;
	if(new_data_size >= demuxer->data_allocated)
	{
		demuxer->data_allocated = new_data_size * 2;
		demuxer->data_buffer = realloc(demuxer->data_buffer, 
			demuxer->data_allocated);
	}

	memcpy(demuxer->data_buffer + demuxer->data_size,
		data,
		bytes);
	demuxer->data_size += bytes;
}

void mpeg3demux_shift_data(mpeg3_demuxer_t *demuxer,
	int bytes)
{
	int i, j;
	for(i = 0, j = bytes; j < demuxer->data_size; i++, j++)
	{
		demuxer->data_buffer[i] = demuxer->data_buffer[j];
	}
	demuxer->data_size -= bytes;
	demuxer->data_position -= bytes;
}






