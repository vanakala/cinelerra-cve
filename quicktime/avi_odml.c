#include "funcprotos.h"
#include "quicktime.h"



void quicktime_read_odml(quicktime_t *file, quicktime_atom_t *parent_atom)
{
}


void quicktime_init_odml(quicktime_t *file, quicktime_hdrl_t *hdrl)
{
	quicktime_atom_t list_atom, dmlh_atom;


// LIST 'odml'
	quicktime_atom_write_header(file, &list_atom, "LIST");
	quicktime_write_char32(file, "odml");
// 'dmlh'
	quicktime_atom_write_header(file, &dmlh_atom, "dmlh");

// Placeholder for total frames in all RIFF objects
	hdrl->total_frames_offset = quicktime_position(file);
	quicktime_write_int32_le(file, 0);

	quicktime_atom_write_footer(file, &dmlh_atom);
	quicktime_atom_write_footer(file, &list_atom);
}

void quicktime_finalize_odml(quicktime_t *file, quicktime_hdrl_t *hdrl)
{
// Get length in frames
	quicktime_set_position(file, hdrl->total_frames_offset);
//	quicktime_write_int32_le(file, );
}




