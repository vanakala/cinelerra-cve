#include "aedit.h"
#include "aedits.h"
#include "amodule.h"
#include "apluginset.h"
#include "assets.h"
#include "atrack.h"
#include "autoconf.h"
#include "aautomation.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "cache.h"
#include "clip.h"
#include "datatype.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "localsession.h"
#include "mainsession.h"
#include "panautos.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"

#include <string.h>

ATrack::ATrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_AUDIO;
}

ATrack::~ATrack()
{
}

// Used by PlaybackEngine
void ATrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);

	ATrack *atrack = (ATrack*)track;
}

int ATrack::copy_settings(Track *track)
{
	Track::copy_settings(track);

	ATrack *atrack = (ATrack*)track;
	return 0;
}


int ATrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "AUDIO");
	return 0;
}

int ATrack::save_derived(FileXML *file)
{
	char string[BCTEXTLEN];
	file->append_newline();
	return 0;
}

int ATrack::load_header(FileXML *file, uint32_t load_flags)
{
	return 0;
}


int ATrack::load_derived(FileXML *file, uint32_t load_flags)
{
	return 0;
}

int ATrack::create_objects()
{
	Track::create_objects();
	automation = new AAutomation(edl, this);
	automation->create_objects();
	edits = new AEdits(edl, this);
	return 0;
}

int ATrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	if(expand_view)
	{
		patch_h += theme->title_h + theme->play_h + theme->fade_h + theme->meter_h + theme->pan_h;
	}
	return MAX(track_h, patch_h);
}

PluginSet* ATrack::new_plugins()
{
	return new APluginSet(edl, this);
}

int ATrack::load_defaults(Defaults *defaults)
{
	Track::load_defaults(defaults);
	return 0;
}

void ATrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO) i++;
	}
	sprintf(title, "Audio %d", i);
}

int64_t ATrack::to_units(double position, int round)
{
	if(round)
		return Units::round(position * edl->session->sample_rate);
	else
		return Units::to_int64(position * edl->session->sample_rate);
}

double ATrack::to_doubleunits(double position)
{
	return position * edl->session->sample_rate;
}

double ATrack::from_units(int64_t position)
{
	return (double)position / edl->session->sample_rate;
}


int ATrack::identical(int64_t sample1, int64_t sample2)
{
// Units of samples
	if(labs(sample1 - sample2) <= 1) return 1; else return 0;
}
























int64_t ATrack::length()
{
	return edits->length();
}

int ATrack::get_dimensions(double &view_start, 
	double &view_units, 
	double &zoom_units)
{
	view_start = (double)edl->local_session->view_start * edl->session->sample_rate;
	view_units = (double)0;
//	view_units = (double)tracks->view_samples();
	zoom_units = (double)edl->local_session->zoom_sample;
}







int ATrack::paste_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int &current_channel)
{
	if(!strcmp(xml->tag.get_title(), "PANAUTOS"))
	{
		current_channel = xml->tag.get_property("CHANNEL", current_channel);
//		pan_autos->paste(start, end, total_length, xml, "/PANAUTOS", mwindow->session->autos_follow_edits);
		return 1;
	}
	return 0;
}





