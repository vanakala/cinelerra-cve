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
