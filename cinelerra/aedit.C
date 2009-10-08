
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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

