#ifndef VEDIT_H
#define VEDIT_H

#include "guicast.h"
#include "bezierauto.inc"
#include "cache.inc"
#include "edit.h"
#include "vedits.inc"
#include "vframe.inc"

// UNITS ARE FRAMES

class VEdit : public Edit
{
public:
	VEdit(EDL *edl, Edits *edits);
	~VEdit();
	
	
	
	
	
	int read_frame(VFrame *video_out, 
			int64_t input_position, 
			int direction,
			CICache *cache);
	
	
	
	
	
	

	int load_properties_derived(FileXML *xml);

// ========================================= editing

	int copy_properties_derived(FileXML *xml, int64_t length_in_selection);


	int dump_derived();
	int64_t get_source_end(int64_t default_);

private:
	VEdits *vedits;
};




#endif
