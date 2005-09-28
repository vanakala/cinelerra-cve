#include "funcprotos.h"
#include "quicktime.h"


static int decode_length(quicktime_t *file)
{
	int bytes = 0;
	int result = 0;
	int byte;
	do
	{
		byte = quicktime_read_char(file);
		result = (result << 7) + (byte & 0x7f);
		bytes++;
	}while((byte & 0x80) && bytes < 4);
	return result;
}

void quicktime_delete_esds(quicktime_esds_t *esds)
{
	if(esds->mpeg4_header) free(esds->mpeg4_header);
}

void quicktime_esds_samplerate(quicktime_stsd_table_t *table, 
	quicktime_esds_t *esds)
{
// Straight out of ffmpeg
	if(esds->mpeg4_header_size > 1 &&
		quicktime_match_32(table->format, QUICKTIME_MP4A))
	{
		static int samplerate_table[] = 
		{
             96000, 88200, 64000, 48000, 44100, 32000, 
             24000, 22050, 16000, 12000, 11025, 8000,
             7350, 0, 0, 0
        };

		unsigned char *ptr = esds->mpeg4_header;
		int samplerate_index = ((ptr[0] & 7) << 1) + ((ptr[1] >> 7) & 1);
		table->channels = (ptr[1] >> 3) & 0xf;
		table->sample_rate = 
			samplerate_table[samplerate_index];
// Faad decodes 1/2 the requested samplerate if the samplerate is <= 22050.
	}
}

void quicktime_read_esds(quicktime_t *file, 
	quicktime_atom_t *parent_atom, 
	quicktime_esds_t *esds)
{

// version
	quicktime_read_char(file);
// flags
	quicktime_read_int24(file);
// elementary stream descriptor tag

	if(quicktime_read_char(file) == 0x3)
	{
		int len = decode_length(file);
// elementary stream id
		quicktime_read_int16(file);
// stream priority
		quicktime_read_char(file);
// decoder configuration descripton tab
		if(quicktime_read_char(file) == 0x4)
		{
			int len2 = decode_length(file);
// object type id
			quicktime_read_char(file);
// stream type
			quicktime_read_char(file);
// buffer size
			quicktime_read_int24(file);
// max bitrate
			quicktime_read_int32(file);
// average bitrate
			quicktime_read_int32(file);

// decoder specific description tag
			if(quicktime_read_char(file) == 0x5)
			{
				esds->mpeg4_header_size = decode_length(file);
				if(!esds->mpeg4_header_size) return;

// Need padding for FFMPEG
				esds->mpeg4_header = calloc(1, 
					esds->mpeg4_header_size + 1024);
// Get extra data for decoder
				quicktime_read_data(file, 
					esds->mpeg4_header, 
					esds->mpeg4_header_size);

// skip rest
				quicktime_atom_skip(file, parent_atom);
				return;
			}
			else
			{
// error
				quicktime_atom_skip(file, parent_atom);
				return;
			}
		}
		else
		{
// error
			quicktime_atom_skip(file, parent_atom);
			return;
		}
	}
	else
	{
// error
		quicktime_atom_skip(file, parent_atom);
		return;
	}
}



void quicktime_write_esds(quicktime_t *file, 
	quicktime_esds_t *esds,
	int do_video,
	int do_audio)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "esds");
// version
	quicktime_write_char(file, 0);
// flags
	quicktime_write_int24(file, 0);

// elementary stream descriptor tag
	quicktime_write_char(file, 0x3);

// length placeholder
	int64_t length1 = quicktime_position(file);
//	quicktime_write_int32(file, 0x80808080);
	quicktime_write_char(file, 0x00);

// elementary stream id
	quicktime_write_int16(file, 0x1);

// stream priority
/*
 * 	if(do_video)
 * 		quicktime_write_char(file, 0x1f);
 * 	else
 */
		quicktime_write_char(file, 0);


// decoder configuration description tab
	quicktime_write_char(file, 0x4);
// length placeholder
	int64_t length2 = quicktime_position(file);
//	quicktime_write_int32(file, 0x80808080);
	quicktime_write_char(file, 0x00);

// video
	if(do_video)
	{
// object type id
		quicktime_write_char(file, 0x20);
// stream type
		quicktime_write_char(file, 0x11);
// buffer size
//		quicktime_write_int24(file, 0x007b00);
		quicktime_write_int24(file, 0x000000);
// max bitrate
//		quicktime_write_int32(file, 0x00014800);
		quicktime_write_int32(file, 0x000030d40);
// average bitrate
//		quicktime_write_int32(file, 0x00014800);
		quicktime_write_int32(file, 0x00000000);
	}
	else
	{
// object type id
		quicktime_write_char(file, 0x40);
// stream type
		quicktime_write_char(file, 0x15);
// buffer size
		quicktime_write_int24(file, 0x001800);
// max bitrate
		quicktime_write_int32(file, 0x00004e20);
// average bitrate
		quicktime_write_int32(file, 0x00003e80);
	}

// decoder specific description tag
	quicktime_write_char(file, 0x05);
// length placeholder
	int64_t length3 = quicktime_position(file);
//	quicktime_write_int32(file, 0x80808080);
	quicktime_write_char(file, 0x00);

// mpeg4 sequence header
	quicktime_write_data(file, esds->mpeg4_header, esds->mpeg4_header_size);


	int64_t current_length2 = quicktime_position(file) - length2 - 1;
	int64_t current_length3 = quicktime_position(file) - length3 - 1;

// unknown tag, length and data
	quicktime_write_char(file, 0x06);
//	quicktime_write_int32(file, 0x80808001);
	quicktime_write_char(file, 0x01);
	quicktime_write_char(file, 0x02);


// write lengths
	int64_t current_length1 = quicktime_position(file) - length1 - 1;
	quicktime_atom_write_footer(file, &atom);
	int64_t current_position = quicktime_position(file);
//	quicktime_set_position(file, length1 + 3);
	quicktime_set_position(file, length1);
	quicktime_write_char(file, current_length1);
//	quicktime_set_position(file, length2 + 3);
	quicktime_set_position(file, length2);
	quicktime_write_char(file, current_length2);
//	quicktime_set_position(file, length3 + 3);
	quicktime_set_position(file, length3);
	quicktime_write_char(file, current_length3);
	quicktime_set_position(file, current_position);
}

void quicktime_esds_dump(quicktime_esds_t *esds)
{
	printf("       elementary stream description\n");
	printf("        mpeg4_header_size=0x%x\n", esds->mpeg4_header_size);
	printf("        mpeg4_header=");
	int i;
	for(i = 0; i < esds->mpeg4_header_size; i++)
		printf("%02x ", (unsigned char)esds->mpeg4_header[i]);
	printf("\n");
}


