#include "asset.h"
#include "edits.h"
#include "aedit.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "indexfile.h"
#include "mwindow.h"
#include "patch.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"


AEdit::AEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
}



AEdit::~AEdit() { }

int AEdit::load_properties_derived(FileXML *xml)
{
	channel = xml->tag.get_property("CHANNEL", (int32_t)0);
	return 0;
}

// ========================================== editing

int AEdit::copy_properties_derived(FileXML *xml, int64_t length_in_selection)
{
	return 0;
}


int AEdit::dump_derived()
{
	//printf("	channel %d\n", channel);
}


int64_t AEdit::get_source_end(int64_t default_)
{
	if(!asset) return default_;   // Infinity

	return (int64_t)((double)asset->audio_length / asset->sample_rate * edl->session->sample_rate + 0.5);
}

