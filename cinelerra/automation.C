#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "atrack.inc"
#include "bezierautos.h"
#include "colors.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "maskautos.h"
#include "panauto.h"
#include "panautos.h"
#include "intautos.h"
#include "track.h"
#include "transportque.inc"


Automation::Automation(EDL *edl, Track *track)
{
	this->edl = edl;
	this->track = track;
	camera_autos = 0;
	projector_autos = 0;
	pan_autos = 0;
	mute_autos = 0;
	fade_autos = 0;
	mode_autos = 0;
	mask_autos = 0;
	czoom_autos = 0;
	pzoom_autos = 0;
}

Automation::~Automation()
{
	if(mute_autos) delete mute_autos;

	if(mode_autos) delete mode_autos;
	if(fade_autos) delete fade_autos;
	if(pan_autos) delete pan_autos;
	if(camera_autos) delete camera_autos;
	if(projector_autos) delete projector_autos;
	if(mask_autos) delete mask_autos;
	if(czoom_autos) delete czoom_autos;
	if(pzoom_autos) delete pzoom_autos;
}

int Automation::create_objects()
{
	mute_autos = new IntAutos(edl, track);
	mute_autos->create_objects();
	((IntAuto*)mute_autos->default_auto)->value = 0;
	return 0;
}

Automation& Automation::operator=(Automation& automation)
{
	copy_from(&automation);
	return *this;
}

void Automation::equivalent_output(Automation *automation, long *result)
{
	mute_autos->equivalent_output(automation->mute_autos, 0, result);
	if(camera_autos) camera_autos->equivalent_output(automation->camera_autos, 0, result);
	if(projector_autos) projector_autos->equivalent_output(automation->projector_autos, 0, result);
	if(fade_autos) fade_autos->equivalent_output(automation->fade_autos, 0, result);
	if(pan_autos) pan_autos->equivalent_output(automation->pan_autos, 0, result);
	if(mode_autos) mode_autos->equivalent_output(automation->mode_autos, 0, result);
	if(mask_autos) mask_autos->equivalent_output(automation->mask_autos, 0, result);
	if(czoom_autos) czoom_autos->equivalent_output(automation->czoom_autos, 0, result);
	if(pzoom_autos) pzoom_autos->equivalent_output(automation->pzoom_autos, 0, result);
}

void Automation::copy_from(Automation *automation)
{
	mute_autos->copy_from(automation->mute_autos);
	if(camera_autos) camera_autos->copy_from(automation->camera_autos);
	if(projector_autos) projector_autos->copy_from(automation->projector_autos);
	if(fade_autos) fade_autos->copy_from(automation->fade_autos);
	if(pan_autos) pan_autos->copy_from(automation->pan_autos);
	if(mode_autos) mode_autos->copy_from(automation->mode_autos);
	if(mask_autos) mask_autos->copy_from(automation->mask_autos);
	if(czoom_autos) czoom_autos->copy_from(automation->czoom_autos);
	if(pzoom_autos) pzoom_autos->copy_from(automation->pzoom_autos);
}

int Automation::load(FileXML *file)
{
	if(file->tag.title_is("MUTEAUTOS") && mute_autos)
	{
		mute_autos->load(file);
	}
	else
	if(file->tag.title_is("FADEAUTOS") && fade_autos)
	{
		fade_autos->load(file);
	}
	else
	if(file->tag.title_is("PANAUTOS") && pan_autos)
	{
		pan_autos->load(file);
	}
	else
	if(file->tag.title_is("CAMERAAUTOS") && camera_autos)
	{
		camera_autos->load(file);
	}
	else
	if(file->tag.title_is("PROJECTORAUTOS") && projector_autos)
	{
		projector_autos->load(file);
	}
	else
	if(file->tag.title_is("MODEAUTOS") && mode_autos)
	{
		mode_autos->load(file);
	}
	else
	if(file->tag.title_is("MASKAUTOS") && mask_autos)
	{
		mask_autos->load(file);
	}
	else
	if(file->tag.title_is("CZOOMAUTOS") && czoom_autos)
	{
		czoom_autos->load(file);
	}
	else
	if(file->tag.title_is("PZOOMAUTOS") && pzoom_autos)
	{
		pzoom_autos->load(file);
	}
	return 0;
}

void Automation::paste(long start, 
	long length, 
	double scale,
	FileXML *file, 
	int default_only,
	AutoConf *autoconf)
{
//printf("Automation::paste 1\n");
	if(!autoconf) autoconf = edl->session->auto_conf;
	if(file->tag.title_is("MUTEAUTOS") && mute_autos && autoconf->mute)
	{
		mute_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("FADEAUTOS") && fade_autos && autoconf->fade)
	{
		fade_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("PANAUTOS") && pan_autos && autoconf->pan)
	{
		pan_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("CAMERAAUTOS") && camera_autos && autoconf->camera)
	{
		camera_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("PROJECTORAUTOS") && projector_autos && autoconf->projector)
	{
		projector_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("MODEAUTOS") && mode_autos && autoconf->mode)
	{
		mode_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("MASKAUTOS") && mask_autos && autoconf->mask)
	{
		mask_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("CZOOMAUTOS") && czoom_autos && autoconf->czoom)
	{
		czoom_autos->paste(start, length, scale, file, default_only);
	}
	else
	if(file->tag.title_is("PZOOMAUTOS") && pzoom_autos && autoconf->pzoom)
	{
		pzoom_autos->paste(start, length, scale, file, default_only);
	}
}

int Automation::copy(long start, 
	long end, 
	FileXML *file, 
	int default_only,
	int autos_only)
{
// Always save these to save default
	if(mute_autos /* && mute_autos->total() */)
	{
		file->tag.set_title("MUTEAUTOS");
		file->append_tag();
		file->append_newline();
		mute_autos->copy(start, 
						end, 
						file, 
						default_only,
						autos_only);
		file->tag.set_title("/MUTEAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(fade_autos /* && fade_autos->total() */)
	{
		file->tag.set_title("FADEAUTOS");
		file->append_tag();
		file->append_newline();
		fade_autos->copy(start, 
						end, 
						file, 
						default_only,
						autos_only);
		file->tag.set_title("/FADEAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(camera_autos)
	{
		file->tag.set_title("CAMERAAUTOS");
		file->append_tag();
		file->append_newline();
		camera_autos->copy(start, 
						end, 
						file, 
						default_only,
						autos_only);
		file->tag.set_title("/CAMERAAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(projector_autos)
	{
		file->tag.set_title("PROJECTORAUTOS");
		file->append_tag();
		file->append_newline();
		projector_autos->copy(start, 
						end, 
						file, 
						default_only,
						autos_only);
		file->tag.set_title("/PROJECTORAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(pan_autos)
	{
		file->tag.set_title("PANAUTOS");
		file->append_tag();
		file->append_newline();
		pan_autos->copy(start, 
				end, 
				file, 
				default_only,
				autos_only);
		file->append_newline();
		file->tag.set_title("/PANAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(mode_autos)
	{
		file->tag.set_title("MODEAUTOS");
		file->append_tag();
		file->append_newline();
		mode_autos->copy(start, 
				end, 
				file, 
				default_only,
				autos_only);
		file->tag.set_title("/MODEAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(mask_autos)
	{
		file->tag.set_title("MASKAUTOS");
		file->append_tag();
		file->append_newline();
		mask_autos->copy(start, 
				end, 
				file, 
				default_only,
				autos_only);
		file->tag.set_title("/MASKAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(czoom_autos)
	{
		file->tag.set_title("CZOOMAUTOS");
		file->append_tag();
		file->append_newline();
		czoom_autos->copy(start, 
				end, 
				file, 
				default_only,
				autos_only);
		file->tag.set_title("/CZOOMAUTOS");
		file->append_tag();
		file->append_newline();
	}

	if(pzoom_autos)
	{
		file->tag.set_title("PZOOMAUTOS");
		file->append_tag();
		file->append_newline();
		pzoom_autos->copy(start, 
				end, 
				file, 
				default_only,
				autos_only);
		file->tag.set_title("/PZOOMAUTOS");
		file->append_tag();
		file->append_newline();
	}

	return 0;
}


void Automation::clear(long start, 
	long end, 
	AutoConf *autoconf, 
	int shift_autos)
{
	AutoConf *temp_autoconf = 0;

	if(!autoconf)
	{
		temp_autoconf = new AutoConf;
		temp_autoconf->set_all();
		autoconf = temp_autoconf;
	}

	if(autoconf->mute)
		mute_autos->clear(start, end, shift_autos);

	if(autoconf->fade)
		fade_autos->clear(start, end, shift_autos);

	if(camera_autos && autoconf->camera)
		camera_autos->clear(start, end, shift_autos);

	if(projector_autos && autoconf->projector)
		projector_autos->clear(start, end, shift_autos);

	if(pan_autos && autoconf->pan)
		pan_autos->clear(start, end, shift_autos);

	if(mode_autos && autoconf->mode)
		mode_autos->clear(start, end, shift_autos);

	if(mask_autos && autoconf->mask)
		mask_autos->clear(start, end, shift_autos);

	if(czoom_autos && autoconf->czoom)
		czoom_autos->clear(start, end, shift_autos);

	if(pzoom_autos && autoconf->pzoom)
		pzoom_autos->clear(start, end, shift_autos);

	if(temp_autoconf) delete temp_autoconf;
}

void Automation::paste_silence(long start, long end)
{
// Unit conversion done in calling routine
	mute_autos->paste_silence(start, end);
	fade_autos->paste_silence(start, end);

	if(camera_autos)
		camera_autos->paste_silence(start, end);
	if(projector_autos)
		projector_autos->paste_silence(start, end);
	if(pan_autos) 
		pan_autos->paste_silence(start, end);
	if(mode_autos) 
		mode_autos->paste_silence(start, end);
	if(mask_autos) 
		mask_autos->paste_silence(start, end);
	if(czoom_autos) 
		czoom_autos->paste_silence(start, end);
	if(pzoom_autos) 
		pzoom_autos->paste_silence(start, end);
}

// We don't replace it in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframe.
void Automation::insert_track(Automation *automation, 
	long start_unit, 
	long length_units,
	int replace_default)
{
	mute_autos->insert_track(automation->mute_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(camera_autos) camera_autos->insert_track(automation->camera_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(projector_autos) projector_autos->insert_track(automation->projector_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(fade_autos) fade_autos->insert_track(automation->fade_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(pan_autos) pan_autos->insert_track(automation->pan_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(mode_autos) mode_autos->insert_track(automation->mode_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(mask_autos) mask_autos->insert_track(automation->mask_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(czoom_autos) czoom_autos->insert_track(automation->czoom_autos, 
		start_unit, 
		length_units, 
		replace_default);
	if(pzoom_autos) pzoom_autos->insert_track(automation->pzoom_autos, 
		start_unit, 
		length_units, 
		replace_default);
}

void Automation::resample(double old_rate, double new_rate)
{
// Run resample for all the autos structures and all the keyframes
	mute_autos->resample(old_rate, new_rate);
	if(camera_autos) camera_autos->resample(old_rate, new_rate);
	if(projector_autos) projector_autos->resample(old_rate, new_rate);
	if(fade_autos) fade_autos->resample(old_rate, new_rate);
	if(pan_autos) pan_autos->resample(old_rate, new_rate);
	if(mode_autos) mode_autos->resample(old_rate, new_rate);
	if(mask_autos) mask_autos->resample(old_rate, new_rate);
	if(czoom_autos) czoom_autos->resample(old_rate, new_rate);
	if(pzoom_autos) pzoom_autos->resample(old_rate, new_rate);
}



int Automation::direct_copy_possible(long start, int direction)
{
	return 1;
}



long Automation::get_length()
{
	long length = 0;
	long total_length = 0;

	if(length > total_length) total_length = length;

	if(mute_autos) length = mute_autos->get_length();
	if(length > total_length) total_length = length;

	if(camera_autos) length = camera_autos->get_length();
	if(length > total_length) total_length = length;

	if(projector_autos) length = projector_autos->get_length();
	if(length > total_length) total_length = length;

	if(fade_autos) length = fade_autos->get_length();
	if(length > total_length) total_length = length;

	if(pan_autos) length = pan_autos->get_length();
	if(length > total_length) total_length = length;

	if(mode_autos) length = mode_autos->get_length();
	if(length > total_length) total_length = length;

	if(mask_autos) length = mask_autos->get_length();
	if(length > total_length) total_length = length;

	if(czoom_autos) length = czoom_autos->get_length();
	if(length > total_length) total_length = length;

	if(pzoom_autos) length = pzoom_autos->get_length();
	if(length > total_length) total_length = length;

	return total_length;
}



void Automation::dump()
{
	printf("   Automation: %p\n", this);
	printf("    mute_autos %p\n", mute_autos);
	mute_autos->dump();

	if(fade_autos)
	{
		printf("    fade_autos %p\n", fade_autos);
		fade_autos->dump();
	}

	if(pan_autos)
	{
		printf("    pan_autos %p\n", pan_autos);
		pan_autos->dump();
	}

	if(camera_autos)
	{
		printf("    camera_autos %p\n", camera_autos);
		camera_autos->dump();
	}

	if(projector_autos)
	{
		printf("    projector_autos %p\n", projector_autos);
		projector_autos->dump();
	}

	if(mode_autos)
	{
		printf("    mode_autos %p\n", mode_autos);
		mode_autos->dump();
	}

	if(mask_autos)
	{
		printf("    mask_autos %p\n", mask_autos);
		mask_autos->dump();
	}

	if(czoom_autos)
	{
		printf("    czoom_autos %p\n", czoom_autos);
		czoom_autos->dump();
	}

	if(pzoom_autos)
	{
		printf("    pzoom_autos %p\n", pzoom_autos);
		pzoom_autos->dump();
	}
}
