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
			long input_position, 
			int direction,
			CICache *cache);
	
	
	
	
	
	

	int load_properties_derived(FileXML *xml);

// ========================================= editing

	int copy_properties_derived(FileXML *xml, long length_in_selection);


	int dump_derived();
	long get_source_end(long default_);

private:
	VEdits *vedits;
};




#endif
