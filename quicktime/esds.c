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


void quicktime_read_esds(quicktime_t *file, 
	quicktime_atom_t *parent_atom, 
	quicktime_stsd_table_t *table)
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
				table->mpeg4_header_size = decode_length(file);
				if(!table->mpeg4_header_size) return;

				table->mpeg4_header = calloc(1, table->mpeg4_header_size);
// Get mpeg4 sequence header
				quicktime_read_data(file, 
					table->mpeg4_header, 
					table->mpeg4_header_size);
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
	quicktime_stsd_table_t *table,
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
	quicktime_write_int16(file, 0);
// stream priority
	if(do_video)
		quicktime_write_char(file, 0x1f);
	else
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
		quicktime_write_int24(file, 0x007b00);
// max bitrate
		quicktime_write_int32(file, 0x00014800);
// average bitrate
		quicktime_write_int32(file, 0x00014800);
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
	quicktime_write_data(file, table->mpeg4_header, table->mpeg4_header_size);


	int64_t current_length2 = quicktime_position(file) - length2 - 4;
	int64_t current_length3 = quicktime_position(file) - length3 - 4;

// unknown tag, length and data
	quicktime_write_char(file, 0x06);
//	quicktime_write_int32(file, 0x80808001);
	quicktime_write_char(file, 0x01);
	quicktime_write_char(file, 0x02);


// write lengths
	int64_t current_length1 = quicktime_position(file) - length1 - 4;
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

