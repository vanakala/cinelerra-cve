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

	int copy_properties_derived(FileXML *xml, long length_in_selection);
	int dump_derived();
	long get_source_end(long default_);

private:

	AEdits *aedits;
};

#endif
