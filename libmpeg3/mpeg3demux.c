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
//printf(__FUNCTION__ " called\n");
	a = demuxer->raw_data[demuxer->raw_offset++];
	b = demuxer->raw_data[demuxer->raw_offset++];
	c = demuxer->raw_data[demuxer->raw_offset++];
	d = demuxer->raw_data[demuxer->raw_offset++];
	result = (a << 24) | (b << 16) | (c << 8) | d;

	return result;
}

static inline unsigned int packet_skip(mpeg3_demuxer_t *demuxer, long length)
{
//printf(__FUNCTION__ " called\n");
	demuxer->raw_offset += length;
	return 0;
}

static int get_adaptation_field(mpeg3_demuxer_t *demuxer)
{
	long length;
	int pcr_flag;

	demuxer->adaptation_fields++;
/* get adaptation field length */
	length = packet_read_char(demuxer);
	
	if(length > 0)
	{
/* get first byte */
  		pcr_flag = (packet_read_char(demuxer) >> 4) & 1;           

		if(pcr_flag)
		{
    		unsigned long clk_ref_base = packet_read_int32(demuxer);
    		unsigned int clk_ref_ext = packet_read_int16(demuxer);

			if (clk_ref_base > 0x7fffffff)
			{   /* correct for invalid numbers */
				clk_ref_base = 0;               /* ie. longer than 32 bits when multiplied by 2 */
				clk_ref_ext = 0;                /* multiplied by 2 corresponds to shift left 1 (<<=1) */
			}
			else 
			{
				clk_ref_base <<= 1; /* Create space for bit */
				clk_ref_base |= (clk_ref_ext >> 15);          /* Take bit */
				clk_ref_ext &= 0x01ff;                        /* Only lower 9 bits */
			}
			demuxer->time = ((double)clk_ref_base + clk_ref_ext / 300) / 90000;
	    	if(length) packet_skip(demuxer, length - 7);
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
	return 0;
}

static int get_data_buffer(mpeg3_demuxer_t *demuxer)
{
	while(demuxer->raw_offset < demuxer->raw_size && 
		demuxer->data_size < MPEG3_RAW_SIZE)
	{
		demuxer->data_buffer[demuxer->data_size++] = 
			demuxer->raw_data[demuxer->raw_offset++];
	}
	return 0;
}

static int get_pes_packet_header(mpeg3_demuxer_t *demuxer, unsigned long *pts, unsigned long *dts)
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
	else if(pts_dts_flags == 3)
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
//printf("get_pes_packet_header %f\n", demuxer->time);

/* extract other stuff here! */
	packet_skip(demuxer, pes_header_data_length - pes_header_bytes);
	return 0;
}

static int get_unknown_data(mpeg3_demuxer_t *demuxer)
{
	packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
	return 0;
}

// Combine the pid and the stream id into one unit
#define CUSTOM_ID(pid, stream_id) (((pid << 8) | stream_id) & 0xffff)


static int get_pes_packet_data(mpeg3_demuxer_t *demuxer, unsigned int stream_id)
{
	unsigned long pts = 0, dts = 0;
	get_pes_packet_header(demuxer, &pts, &dts);


//printf("get_pes_packet_data %x\n", CUSTOM_ID(demuxer->pid, stream_id));
	if(stream_id == 0xbd)
	{
// Don't know if the next byte is the true stream id like in program stream
		stream_id = 0x0;

		if(demuxer->read_all)
			demuxer->astream_table[CUSTOM_ID(demuxer->pid, stream_id)] = AUDIO_AC3;
		if(demuxer->astream == -1)
		    demuxer->astream = CUSTOM_ID(demuxer->pid, stream_id);

    	if(CUSTOM_ID(demuxer->pid, stream_id) == demuxer->astream && 
			demuxer->do_audio)
		{
			demuxer->pes_audio_time = (double)pts / 90000;
			demuxer->audio_pid = demuxer->pid;
			return get_data_buffer(demuxer);
    	}
	}
	else
	if((stream_id >> 4) == 12 || (stream_id >> 4) == 13)
	{
/* Just pick the first available stream if no ID is set */
		if(demuxer->read_all)
			demuxer->astream_table[CUSTOM_ID(demuxer->pid, stream_id)] = AUDIO_MPEG;
		if(demuxer->astream == -1)
		    demuxer->astream = CUSTOM_ID(demuxer->pid, stream_id);

    	if(CUSTOM_ID(demuxer->pid, stream_id) == demuxer->astream && 
			demuxer->do_audio)
		{
			demuxer->pes_audio_time = (double)pts / 90000;
			demuxer->audio_pid = demuxer->pid;

			return get_data_buffer(demuxer);
    	}
	}
	else 
	if((stream_id >> 4) == 14)
	{
//printf("get_pes_packet_data video %x\n", stream_id);
/* Just pick the first available stream if no ID is set */
		if(demuxer->read_all)
			demuxer->vstream_table[CUSTOM_ID(demuxer->pid, stream_id)] = 1;
		else
		if(demuxer->vstream == -1)
			demuxer->vstream = (CUSTOM_ID(demuxer->pid, stream_id));


//printf("get_pes_packet_data %x %d\n", stream_id, demuxer->payload_unit_start_indicator);
		if(CUSTOM_ID(demuxer->pid, stream_id) == demuxer->vstream && 
			demuxer->do_video)
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


			return get_data_buffer(demuxer);
		}
	}
	else 
	{
		return get_unknown_data(demuxer);
	}

	packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);

	return 0;
}

static int get_pes_packet(mpeg3_demuxer_t *demuxer)
{
	unsigned int stream_id;

	demuxer->pes_packets++;



/* Skip startcode */
	packet_read_int24(demuxer);
	stream_id = packet_read_char(demuxer);



/* Skip pes packet length */
	packet_read_int16(demuxer);

	if(stream_id != MPEG3_PRIVATE_STREAM_2 && 
		stream_id != MPEG3_PADDING_STREAM)
	{
		return get_pes_packet_data(demuxer, stream_id);
	}
	else
	if(stream_id == MPEG3_PRIVATE_STREAM_2)
	{
/* Dump private data! */
		fprintf(stderr, "stream_id == MPEG3_PRIVATE_STREAM_2\n");
		packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
		return 0;
	}
	else
	if(stream_id == MPEG3_PADDING_STREAM)
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
    	if(demuxer->pid==0) 
			get_program_association_table(demuxer);
    	else 
		if(packet_next_int24(demuxer) == MPEG3_PACKET_START_CODE_PREFIX) 
			get_pes_packet(demuxer);
    	else 
			packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
	}
	else
	{
//printf("get_payload 2\n");
    	if(demuxer->pid == demuxer->audio_pid && demuxer->do_audio)
		{
			get_data_buffer(demuxer);
		}
    	else 
		if(demuxer->pid == demuxer->video_pid && demuxer->do_video)
		{
			get_data_buffer(demuxer);
		}
    	else 
			packet_skip(demuxer, demuxer->raw_size - demuxer->raw_offset);
	}
	return 0;
}

/* Read a transport packet */
static int read_transport(mpeg3_demuxer_t *demuxer)
{
	mpeg3_t *file = (mpeg3_t*)demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	int result = 0;
	unsigned int bits;
	int table_entry;

/* Packet size is known for transport streams */
	demuxer->raw_size = file->packet_size;
	demuxer->raw_offset = 0;

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
		demuxer->absolute_byte = mpeg3io_tell(title->fs) + 
			title->start_byte;
		return 1;
	}

	packet_read_char(demuxer);
    bits =  packet_read_int24(demuxer) & 0x00ffffff;
//printf("read transport 3 tell=%x bits=%x\n", mpeg3io_tell(title->fs), bits);
    demuxer->transport_error_indicator = (bits >> 23) & 0x1;
    demuxer->payload_unit_start_indicator = (bits >> 22) & 0x1;
    demuxer->pid = (bits >> 8) & 0x00001fff;
    demuxer->transport_scrambling_control = (bits >> 6) & 0x3;
    demuxer->adaptation_field_control = (bits >> 4) & 0x3;
    demuxer->continuity_counter = bits & 0xf;
//printf("read_transport 1 %llx %08x %d\n", mpeg3io_tell(title->fs), bits, demuxer->payload_unit_start_indicator);

//printf("read_transport 4 %x\n", demuxer->pid);
	if(demuxer->transport_error_indicator)
	{
		fprintf(stderr, "demuxer->transport_error_indicator\n");
		demuxer->absolute_byte = mpeg3io_tell(title->fs) + 
			title->start_byte;
		return 1;
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

/* Abort if padding.  Should abort after demuxer->pid == 0x1fff for speed. */
	if(demuxer->is_padding)
	{
		demuxer->absolute_byte = mpeg3io_tell(title->fs) + 
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







    if(demuxer->adaptation_field_control == 2 || 
		demuxer->adaptation_field_control == 3)
    	result = get_adaptation_field(demuxer);

// Need to enter in astream and vstream table:
// PID ored with stream_id
    if(demuxer->adaptation_field_control == 1 || 
		demuxer->adaptation_field_control == 3)
    	result = get_payload(demuxer);

	demuxer->absolute_byte = mpeg3io_tell(title->fs) + 
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

static unsigned long get_timestamp(mpeg3_demuxer_t *demuxer)
{
	unsigned long timestamp;
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
	unsigned long i, j;
	unsigned long clock_ref, clock_ref_ext;
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

/* Program packet reading core */
static int get_ps_pes_packet(mpeg3_demuxer_t *demuxer, unsigned int header)
{
	unsigned long pts = 0, dts = 0;
	int stream_id;
	int pes_packet_length;
	int64_t pes_packet_start;
	long decryption_offset;
	int i;
	mpeg3_t *file = demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
	int scrambling = 0;
	int data_start = demuxer->data_size;
/* Format if audio */
	int do_pcm = 0;

	stream_id = header & 0xff;
	pes_packet_length = mpeg3io_read_int16(title->fs);
	pes_packet_start = mpeg3io_tell(title->fs);



/*
 * printf(__FUNCTION__ " pes_packet_start=%llx pes_packet_length=%x data_size=%x\n", 
 * pes_packet_start, 
 * pes_packet_length,
 * demuxer->data_size);
 */





	if(stream_id != MPEG3_PRIVATE_STREAM_2 &&
		stream_id != MPEG3_PADDING_STREAM)
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
				pts = (mpeg3io_read_char(title->fs) >> 1) & 7;  /* Only low 4 bits (7==1111) */
				pts <<= 15;
				pts |= (mpeg3io_read_int16(title->fs) >> 1);
				pts <<= 15;
				pts |= (mpeg3io_read_int16(title->fs) >> 1);
				pes_header_bytes += 5;
			}
    		else 
			if(pts_dts_flags == 3)
			{
        		pts = (mpeg3io_read_char(title->fs) >> 1) & 7;  /* Only low 4 bits (7==1111) */
        		pts <<= 15;
        		pts |= (mpeg3io_read_int16(title->fs) >> 1);
        		pts <<= 15;
        		pts |= (mpeg3io_read_int16(title->fs) >> 1);

        		dts = (mpeg3io_read_char(title->fs) >> 1) & 7;  /* Only low 4 bits (7==1111) */
        		dts <<= 15;
        		dts |= (mpeg3io_read_int16(title->fs) >> 1);
        		dts <<= 15;
        		dts |= (mpeg3io_read_int16(title->fs) >> 1);
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
		if((stream_id >> 4) == 0xc || (stream_id >> 4) == 0xd)
		{
/* Audio data */
/* Take first stream ID if -1 */
			pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;




			if(demuxer->read_all)
				demuxer->astream_table[stream_id & 0x0f] = AUDIO_MPEG;
			else
			if(demuxer->astream == -1) 
				demuxer->astream = stream_id & 0x0f;




			if(pts > 0) demuxer->pes_audio_time = (double)pts / 60000;


			if((stream_id & 0x0f) == demuxer->astream && demuxer->do_audio)
			{
				decryption_offset = mpeg3io_tell(title->fs) - demuxer->last_packet_start;
				mpeg3io_read_data(demuxer->data_buffer + demuxer->data_size, 
					pes_packet_length, 
					title->fs);
				demuxer->data_size += pes_packet_length;
		  	}
			else 
			{
    			mpeg3io_seek_relative(title->fs, pes_packet_length);
			}
		}
    	else 
		if((stream_id >> 4) == 0xe)
		{
/* Video data */
/* Take first stream ID if -1 */
			pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;



			if(demuxer->read_all) 
				demuxer->vstream_table[stream_id & 0x0f] = 1;
			else
			if(demuxer->vstream == -1) 
				demuxer->vstream = stream_id & 0x0f;

			if(pts > 0) demuxer->pes_video_time = (double)pts / 60000;



    	    if((stream_id & 0x0f) == demuxer->vstream && demuxer->do_video)
			{
//printf(__FUNCTION__ " stream_id=%x size=%x\n", stream_id, pes_packet_length);
				decryption_offset = mpeg3io_tell(title->fs) - demuxer->last_packet_start;
				mpeg3io_read_data(demuxer->data_buffer + demuxer->data_size, 
					pes_packet_length, 
					title->fs);

				demuxer->data_size += pes_packet_length;
    	  	}
    		else 
			{
    			mpeg3io_seek_relative(title->fs, pes_packet_length);
    		}
    	}
    	else 
		if((stream_id == 0xbd || stream_id == 0xbf) && 
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
			stream_id = mpeg3io_next_char(title->fs);
			if(pts > 0) demuxer->pes_audio_time = (double)pts / 60000;

//printf("get_ps_pes_packet %x\n", stream_id);
/* Take first stream ID if not building TOC. */
			if(demuxer->read_all)
				demuxer->astream_table[stream_id] = format;
			else
			if(demuxer->astream == -1)
				demuxer->astream = stream_id;







//printf("get_ps_pes_packet 5 %x\n", format);
      		if(stream_id == demuxer->astream && demuxer->do_audio)
			{
				demuxer->aformat = format;
				mpeg3io_read_int32(title->fs);

				pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
				decryption_offset = mpeg3io_tell(title->fs) - demuxer->last_packet_start;

				if(format == AUDIO_PCM) do_pcm = 1;
//printf("get_ps_pes_packet 5 %x\n", decryption_offset);



				mpeg3io_read_data(demuxer->data_buffer + demuxer->data_size, 
					pes_packet_length, 
					title->fs);
				demuxer->data_size += pes_packet_length;
      		}
      		else
			{
				pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
    			mpeg3io_seek_relative(title->fs, pes_packet_length);
      		}
//printf("get_ps_pes_packet 6 %d\n", demuxer->astream_table[0x20]);
    	}
    	else 
		if(stream_id == 0xbc || 1)
		{
			pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
    		mpeg3io_seek_relative(title->fs, pes_packet_length);
    	}
	}
  	else 
	if(stream_id == MPEG3_PRIVATE_STREAM_2 || stream_id == MPEG3_PADDING_STREAM)
	{
		pes_packet_length -= mpeg3io_tell(title->fs) - pes_packet_start;
    	mpeg3io_seek_relative(title->fs, pes_packet_length);
//printf(__FUNCTION__ " 2 %llx\n", mpeg3io_tell(title->fs));
  	}





// Advance 2048 bytes if scrambled.  We might pick up a spurrius
// packet start code in the scrambled data otherwise.
	if(scrambling && 
		demuxer->last_packet_start + 0x800 > mpeg3io_tell(title->fs))
	{
		mpeg3io_seek_relative(title->fs, 
			demuxer->last_packet_start + 0x800 - mpeg3io_tell(title->fs));
	}





// Descramble if desired
	if(demuxer->data_size && scrambling)
	{
//printf(__FUNCTION__ " data_size=%x decryption_offset=%x\n", demuxer->data_size, decryption_offset);
		if(mpeg3_decrypt_packet(title->fs->css, 
			demuxer->data_buffer,
			decryption_offset))
		{
			fprintf(stderr, "get_ps_pes_packet: Decryption not available\n");
			return 1;
		}
	}






/* Synthesize PCM header and delete MPEG header for PCM data */
/* Must be done after decryption */
	if(do_pcm)
	{
		unsigned char code;
		int bits_code;
		int bits;
		int samplerate_code;
		int samplerate;
		unsigned char *output = demuxer->data_buffer + data_start;
		int j;

/* Shift audio back */
		code = output[1];
		for(i = demuxer->data_size - 1, j = demuxer->data_size + PCM_HEADERSIZE - 3 - 1; 
			i >= data_start;
			i--, j--)
			*(demuxer->data_buffer + j) = *(demuxer->data_buffer + i);
		demuxer->data_size += PCM_HEADERSIZE - 3;

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
		*(int32_t*)(output + 16) = pes_packet_length - 
			3 + 
			PCM_HEADERSIZE;
//printf(__FUNCTION__ " %02x%02x%02x %d\n", code1, code, code2, pes_packet_length - 3);
	}








//printf(__FUNCTION__ " 3 %llx %x\n", mpeg3io_tell(title->fs), demuxer->data_size);


//if(mpeg3io_tell(title->fs) - demuxer->last_packet_start != 0x800)
//printf(__FUNCTION__ " packet size == %llx\n", mpeg3io_tell(title->fs) - demuxer->last_packet_start);

//printf(__FUNCTION__ " pes_audio_time=%f pes_video_time=%f\n", demuxer->pes_audio_time, demuxer->pes_video_time);

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

	demuxer->data_size = 0;
//printf("mpeg3demux_read_program 1 %d %llx %llx\n", result, title->fs->current_byte, title->fs->total_bytes);

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
//printf("mpeg3demux_read_program MPEG3_PACK_START_CODE %d\n", result);
			pack_count++;



		}
		else
		if(header == MPEG3_SYSTEM_START_CODE && pack_count)
		{



 			result = get_system_header(demuxer);
//printf("mpeg3demux_read_program MPEG3_SYSTEM_START_CODE %d\n", result);



		}
		else
		if((header >> 8) == MPEG3_PACKET_START_CODE_PREFIX && pack_count)
		{


			result = get_ps_pes_packet(demuxer, header);
//printf("mpeg3demux_read_program MPEG3_PACKET_START_CODE_PREFIX %d %08x\n", result, header);



		}
		else
		{
// Try again.
//printf(__FUNCTION__ " %llx %08x\n", mpeg3io_tell(title->fs), header);
			mpeg3io_seek_relative(title->fs, -3);
		}
	}
//printf("mpeg3demux_read_program 3 %d %x %llx\n", result, title->fs->current_byte, title->fs->total_bytes);







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

	demuxer->absolute_byte = mpeg3io_tell(title->fs) +
		title->start_byte;
	return result;
}







































int mpeg3_advance_cell(mpeg3_demuxer_t *demuxer)
{
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

	mpeg3io_seek(title->fs, demuxer->absolute_byte - title->start_byte);

//printf("mpeg3_advance_cell %llx %llx\n",demuxer->absolute_byte , title->start_byte);
/* Don't do anything when constructing cell table */
	if(!title->cell_table || 
		!title->cell_table_size || 
		demuxer->read_all)
	{
		return 0;
	}

	mpeg3demux_cell_t *cell = &title->cell_table[demuxer->title_cell];


//printf("mpeg3_advance_cell 1\n");
/* Don't do anything if we're in the cell and it's the right program */
	if(demuxer->reverse)
	{
/*
 * printf("mpeg3_advance_cell 1 %llx %llx %llx %llx %llx\n", 
 * demuxer->absolute_byte,
 * cell->start_byte,
 * cell->end_byte,
 * title->start_byte,
 * title->end_byte);fflush(stdout);
 */
		if(demuxer->absolute_byte > cell->start_byte + title->start_byte &&
			demuxer->absolute_byte <= cell->end_byte + title->start_byte &&
			cell->program == demuxer->current_program)
		{
			return 0;
		}
	}
	else
	{
		if(demuxer->absolute_byte >= cell->start_byte + title->start_byte &&
			demuxer->absolute_byte < cell->end_byte + title->start_byte &&
			cell->program == demuxer->current_program)
			return 0;
	}

//printf("mpeg3_advance_cell 10\n");
	int result = 0;
	int last_cell = demuxer->title_cell;
	int last_title = demuxer->current_title;
	int64_t last_byte = demuxer->absolute_byte;
	int got_it = 0;

/*
 * printf("mpeg3_advance_cell 2 %llx %llx %llx %llx %llx\n", 
 * demuxer->absolute_byte,
 * cell->start_byte + title->start_byte,
 * cell->end_byte + title->start_byte,
 * title->start_byte,
 * title->end_byte);fflush(stdout);
 */

/* Find first cell on or after current position */
	if(demuxer->reverse)
	{
		for(demuxer->current_title = demuxer->total_titles - 1;
			demuxer->current_title >= 0;
			demuxer->current_title--)
		{
			title = demuxer->titles[demuxer->current_title];
			if(title->start_byte < demuxer->absolute_byte)
			{
				for(demuxer->title_cell = title->cell_table_size - 1;
					demuxer->title_cell >= 0;
					demuxer->title_cell--)
				{
					cell = &title->cell_table[demuxer->title_cell];
					if(cell->start_byte + title->start_byte < demuxer->absolute_byte &&
						cell->program == demuxer->current_program)
					{
						got_it = 1;
						if(demuxer->absolute_byte > cell->end_byte + title->start_byte)
							demuxer->absolute_byte = cell->end_byte + title->start_byte;
						break;
					}
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
//printf("mpeg3_advance_cell 10\n");
	}
	else
	{
//printf("mpeg3_advance_cell 20\n");
		for(demuxer->current_title = 0;
			demuxer->current_title < demuxer->total_titles;
			demuxer->current_title++)
		{
			title = demuxer->titles[demuxer->current_title];
			if(title->end_byte > demuxer->absolute_byte)
			{
				for(demuxer->title_cell = 0;
					demuxer->title_cell < title->cell_table_size;
					demuxer->title_cell++)
				{
					cell = &title->cell_table[demuxer->title_cell];
					if(cell->end_byte + title->start_byte > demuxer->absolute_byte &&
						cell->program == demuxer->current_program)
					{
						got_it = 1;
						if(demuxer->absolute_byte < cell->start_byte + title->start_byte)
							demuxer->absolute_byte = cell->start_byte + title->start_byte;
						break;
					}
				}
			}
			if(got_it) break;
		}

		if(!got_it)
		{
			demuxer->current_title = demuxer->total_titles - 1;
			title = demuxer->titles[demuxer->current_title];
			demuxer->title_cell = title->cell_table_size - 1;
			result = 1;
		}
//printf("mpeg3_advance_cell 30\n");
	}


/*
 * printf("mpeg3_advance_cell 40 %llx %llx %llx %llx %llx\n", 
 * demuxer->absolute_byte,
 * cell->start_byte + title->start_byte,
 * cell->end_byte + title->start_byte,
 * title->start_byte,
 * title->end_byte);fflush(stdout);
 */


	if(demuxer->current_title != last_title)
	{
		mpeg3demux_open_title(demuxer, demuxer->current_title);
	}

	title = demuxer->titles[demuxer->current_title];
	if(demuxer->absolute_byte != title->start_byte + mpeg3io_tell(title->fs))
	{
		mpeg3io_seek(title->fs, demuxer->absolute_byte - title->start_byte);
	}

//printf("mpeg3_advance_cell 40\n", demuxer->absolute_byte);
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
		demuxer->absolute_byte++;
		error = mpeg3_advance_cell(demuxer);
	}
	return error;
}










/* Read packet in the forward direction */
int mpeg3_read_next_packet(mpeg3_demuxer_t *demuxer)
{
	int result = 0;
	long current_position;
	mpeg3_t *file = demuxer->file;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
/*
 * printf("mpeg3_read_next_packet 1 %d %llx\n", result, demuxer->absolute_byte);
 * fflush(stdout);
 */

/* Reset output buffer */
	demuxer->data_size = 0;
	demuxer->data_position = 0;

/* Switch to forward direction. */
	if(demuxer->reverse)
	{
/* Don't reread anything if we're at the beginning of the file. */
		if(demuxer->absolute_byte < 0)
		{
			demuxer->absolute_byte = 0;
			result = mpeg3_advance_cell(demuxer);
/* First character was the -1 byte which brings us to 0 after this function. */
			result = 1;
		}
		else
/* Transport or elementary stream */
		if(file->packet_size > 0)
		{
			demuxer->absolute_byte += file->packet_size;
			result = mpeg3_advance_cell(demuxer);
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






//printf("mpeg3_read_next_packet 2 %llx\n", demuxer->absolute_byte);

/* Read packets until the output buffer is full */
	if(!result)
	{
		do
		{
			title = demuxer->titles[demuxer->current_title];

			if(!result)
			{
				if(file->is_transport_stream)
				{
					result = read_transport(demuxer);
					if(!result) result = mpeg3_advance_cell(demuxer);
				}
				else
				if(file->is_program_stream)
				{
					result = mpeg3demux_read_program(demuxer);
					if(!result) result = mpeg3_advance_cell(demuxer);
				}
				else
				{
/* Read elementary stream. */
					result = mpeg3io_read_data(demuxer->data_buffer, 
						file->packet_size, title->fs);
					if(!result)
					{
						demuxer->data_size = file->packet_size;
						demuxer->absolute_byte += file->packet_size;
						result = mpeg3_advance_cell(demuxer);
					}
				}
			}
		}while(!result && 
			demuxer->data_size == 0 && 
			(demuxer->do_audio || demuxer->do_video));
	}
//printf("mpeg3_read_next_packet 3\n");

/*
 * printf("mpeg3_read_next_packet 2 %d %llx\n", result, demuxer->absolute_byte);
 * fflush(stdout);
 */
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
		demuxer->absolute_byte > 0 && 
		!error)
	{
		result >>= 8;
		title = demuxer->titles[demuxer->current_title];
		mpeg3io_seek(title->fs, demuxer->absolute_byte - title->start_byte - 1LL);
//printf("1 %llx %llx\n", demuxer->absolute_byte, title->start_byte);fflush(stdout);
		result |= ((uint32_t)mpeg3io_read_char(title->fs)) << 24;
//printf("2\n");fflush(stdout);
		demuxer->absolute_byte--;
		error = mpeg3_advance_cell(demuxer);
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

/*
 * printf("mpeg3_read_prev_packet 1 %d %d %d %d %llx\n", 
 * demuxer->total_titles,
 * demuxer->current_title, 
 * title->cell_table_size,
 * demuxer->title_cell,
 * demuxer->absolute_byte);fflush(stdout);
 */
 

//printf("mpeg3_read_prev_packet 1 %d %d %llx\n", result, demuxer->current_title, mpeg3io_tell(title->fs));
/* Switch to reverse direction */
	if(!demuxer->reverse)
	{
		demuxer->reverse = 1;

/* Transport stream or elementary stream case */
		if(file->packet_size > 0)
		{
			demuxer->absolute_byte -= file->packet_size;
			result = mpeg3_advance_cell(demuxer);
		}
		else
/* Program stream */
		{
//printf("mpeg3_read_prev_packet 1 %d %llx\n", demuxer->current_title, demuxer->absolute_byte);
			result = prev_code(demuxer, 
				MPEG3_PACK_START_CODE);
//printf("mpeg3_read_prev_packet 2 %d %llx\n", demuxer->current_title, demuxer->absolute_byte);
		}

	}







/* Go to beginning of previous packet */
	do
	{
		title = demuxer->titles[demuxer->current_title];

/*
 * printf("mpeg3_read_prev_packet 2 %d %llx %llx\n", 
 * demuxer->current_title, 
 * demuxer->absolute_byte,
 * title->start_byte);
 */

/* Transport stream or elementary stream case */
		if(file->packet_size > 0)
		{
			demuxer->absolute_byte -= file->packet_size;
			result = mpeg3_advance_cell(demuxer);
		}
		else
		{
			if(!result) result = prev_code(demuxer, 
				MPEG3_PACK_START_CODE);
		}

//printf("mpeg3_read_prev_packet 3 %d %llx\n", demuxer->current_title, demuxer->absolute_byte);


/* Read packet and then rewind it */
		title = demuxer->titles[demuxer->current_title];
		if(file->is_transport_stream && !result)
		{
			result = read_transport(demuxer);

			if(demuxer->absolute_byte > 0)
			{
				demuxer->absolute_byte -= file->packet_size;
				result = mpeg3_advance_cell(demuxer);
			}
		}
		else
		if(file->is_program_stream && !result)
		{
			int64_t current_position = demuxer->absolute_byte;

/* Read packet */
			result = mpeg3demux_read_program(demuxer);

//printf("mpeg3_read_prev_packet 5 %d %llx\n", demuxer->current_title, demuxer->absolute_byte);
/* Rewind packet */
			while(demuxer->absolute_byte > current_position &&
				!result)
			{
				result = prev_code(demuxer, 
					MPEG3_PACK_START_CODE);
			}
//printf("mpeg3_read_prev_packet 6 %d %llx\n", demuxer->current_title, demuxer->absolute_byte);
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
				result = mpeg3io_seek(title->fs, demuxer->absolute_byte);
			}
		}
//printf("mpeg3_read_prev_packet 4 %d %llx\n", demuxer->current_title, demuxer->absolute_byte);
	}while(!result && 
		demuxer->data_size == 0 && 
		(demuxer->do_audio || demuxer->do_video));


//printf("mpeg3_read_prev_packet 5 %d %d %llx\n", result, demuxer->current_title, demuxer->absolute_byte);fflush(stdout);
	return result;
}


/* For audio */
int mpeg3demux_read_data(mpeg3_demuxer_t *demuxer, 
		unsigned char *output, 
		long size)
{
	int result = 0;
	demuxer->error_flag = 0;

//printf("mpeg3demux_read_data 1 %d\n", demuxer->data_position);
	if(demuxer->data_position >= 0)
	{
/* Read forwards */
		long i;
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
		int64_t current_position;
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

	if(demuxer->data_position >= demuxer->data_size)
		demuxer->error_flag = mpeg3_read_next_packet(demuxer);
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
	demuxer->next_char = demuxer->data_buffer[demuxer->data_position];
	return demuxer->next_char;
}



int mpeg3demux_open_title(mpeg3_demuxer_t *demuxer, int title_number)
{
	mpeg3_title_t *title;

	if(title_number < demuxer->total_titles)
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


	return demuxer->error_flag;
}

/* Assign program numbers to interleaved programs */
int mpeg3demux_assign_programs(mpeg3_demuxer_t *demuxer)
{
	int current_program = 0;
	int current_title = 0;
	int title_cell = 0;
	double current_time;
	mpeg3demux_cell_t *cell;
	int total_programs = 1;
	int i, j;
	int program_exists, last_program_assigned = 0;
	int total_cells;
	mpeg3_title_t **titles = demuxer->titles;

	for(i = 0, total_cells = 0; i < demuxer->total_titles; i++)
	{
		total_cells += demuxer->titles[i]->cell_table_size;
		for(j = 0; j < demuxer->titles[i]->cell_table_size; j++)
		{
			cell = &demuxer->titles[i]->cell_table[j];
			if(cell->program > total_programs - 1)
				total_programs = cell->program + 1;
		}
	}



	demuxer->current_program = 0;
	return 0;
}

int mpeg3demux_copy_titles(mpeg3_demuxer_t *dst, mpeg3_demuxer_t *src)
{
	long i;
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
		dst_title = dst->titles[i] = mpeg3_new_title(file, src->titles[i]->fs->path);
		mpeg3_copy_title(dst_title, src_title);
	}

	mpeg3demux_open_title(dst, src->current_title);
	dst->title_cell = 0;
	return 0;
}

/* ==================================================================== */
/*                            Entry points */
/* ==================================================================== */

mpeg3_demuxer_t* mpeg3_new_demuxer(mpeg3_t *file, int do_audio, int do_video, int stream_id)
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
/* System specific variables */
	demuxer->audio_pid = stream_id;
	demuxer->video_pid = stream_id;
	demuxer->astream = stream_id;
	demuxer->vstream = stream_id;
	demuxer->current_title = -1;
	demuxer->pes_audio_time = -1;
	demuxer->pes_video_time = -1;
//printf("mpeg3_new_demuxer %f\n", demuxer->time);


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

	free(demuxer->data_buffer);
	free(demuxer->raw_data);
	free(demuxer);
	return 0;
}


int mpeg3demux_eof(mpeg3_demuxer_t *demuxer)
{
	if(demuxer->current_title >= 0)
	{
		if(mpeg3io_eof(demuxer->titles[demuxer->current_title]->fs) &&
			demuxer->current_title >= demuxer->total_titles - 1)
			return 1;
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

	demuxer->absolute_byte = byte;
	demuxer->data_position = 0;
	demuxer->data_size = 0;

/* Get on a packet boundary only for transport streams. */
	if(file->is_transport_stream &&
		file->packet_size)
	{
		demuxer->absolute_byte -= demuxer->absolute_byte % file->packet_size;
	}

	int result = mpeg3_advance_cell(demuxer);

	return result;










	int64_t current_position;
	mpeg3_title_t *title = demuxer->titles[demuxer->current_title];

	demuxer->data_position = 0;
	demuxer->data_size = 0;
	demuxer->error_flag = mpeg3io_seek(title->fs, byte);

	if(!demuxer->error_flag && 
		file->is_transport_stream &&
		file->packet_size)
	{
/* Get on a packet boundary only for transport streams. */
		current_position = mpeg3io_tell(title->fs);
		if(byte % file->packet_size)
		{
			demuxer->error_flag |= mpeg3io_seek(title->fs, 
				current_position - 
				(current_position % file->packet_size));
		}
	}

	demuxer->absolute_byte = title->start_byte + mpeg3io_tell(title->fs);

// Get current cell
	for(demuxer->title_cell = 0; 
		demuxer->title_cell < title->cell_table_size; 
		demuxer->title_cell++)
	{
		if(title->cell_table[demuxer->title_cell].start_byte <= byte &&
			title->cell_table[demuxer->title_cell].end_byte > byte)
		{
			break;
		}
	}

	if(demuxer->title_cell >= title->cell_table_size)
		demuxer->title_cell = title->cell_table_size - 1;


	return demuxer->error_flag;
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
	return demuxer->absolute_byte;



	int i;
	int64_t result = 0;
	for(i = 0; i < demuxer->current_title; i++)
	{
		result += demuxer->titles[i]->total_bytes;
	}
	result += mpeg3io_tell(demuxer->titles[demuxer->current_title]->fs);
	return result;
}


int64_t mpeg3demux_movie_size(mpeg3_demuxer_t *demuxer)
{
	int64_t result = 0;
	int i;
	for(i = 0; i < demuxer->total_titles; i++)
		result += demuxer->titles[i]->total_bytes;
	return result;
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
