#include "assets.h"
#include "bezierauto.h"
#include "bezierautos.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "patch.h"
#include "preferences.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vtrack.h"

VEdit::VEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
}


VEdit::~VEdit() { }

int VEdit::load_properties_derived(FileXML *xml)
{
	channel = xml->tag.get_property("CHANNEL", (long)0);
	return 0;
}





// ================================================== editing



int VEdit::read_frame(VFrame *video_out, 
			long input_position, 
			int direction,
			CICache *cache)
{
//printf("VEdit::read_frame 1 %f %d %p %p %s\n", asset->frame_rate, asset->video_length, cache, asset, asset->path);
	File *file = cache->check_out(asset);
	int result = 0;

//printf("VEdit::read_frame 2\n");
	if(file)
	{

//printf("VEdit::read_frame 3\n");
		input_position = (direction == PLAY_FORWARD) ? input_position : (input_position - 1);

//printf("VEdit::read_frame 4\n");
		file->set_layer(channel);

//printf("VEdit::read_frame 5\n");
		file->set_video_position(input_position - startproject + startsource, edl->session->frame_rate);

//printf("VEdit::read_frame 6 channel: %d w: %d h: %d video_out: %p\n", channel, asset->width, asset->height, video_out);
//asset->dump();
		result = file->read_frame(video_out);

//printf("VEdit::read_frame 7 %d\n", video_out->get_compressed_size());
		cache->check_in(asset);

//printf("VEdit::read_frame 8\n");
	}
	else
		result = 1;
	
//for(int i = 0; i < video_out->get_w() * 3 * 20; i++) video_out->get_rows()[0][i] = 128;
	return result;
}

int VEdit::copy_properties_derived(FileXML *xml, long length_in_selection)
{
	return 0;
}

int VEdit::dump_derived()
{
	printf("	VEdit::dump_derived\n");
	printf("		startproject %ld\n", startproject);
	printf("		length %ld\n", length);
}

long VEdit::get_source_end(long default_)
{
	if(!asset) return default_;   // Infinity

	return (long)((double)asset->video_length / asset->frame_rate * edl->session->frame_rate + 0.5);
}
