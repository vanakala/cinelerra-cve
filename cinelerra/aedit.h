#ifndef AEDIT_H
#define AEDIT_H


#include "guicast.h"
#include "edit.h"
#include "filexml.inc"
#include "aedits.inc"

// UNITS ARE SAMPLES

class AEdit : public Edit
{
public:
	AEdit(EDL *edl, Edits *edits);
	
	
	
	
	
	
	
	
	
	
	
	~AEdit();

	int load_properties_derived(FileXML *xml);


// ========================================= editing

	int copy_properties_derived(FileXML *xml, int64_t length_in_selection);
	int dump_derived();
	int64_t get_source_end(int64_t default_);

private:

	AEdits *aedits;
};

#endif
