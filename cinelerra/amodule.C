#include "aattachmentpoint.h"
#include "aedit.h"
#include "amodule.h"
#include "aplugin.h"
#include "arender.h"
#include "assets.h"
#include "atrack.h"
#include "cache.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "module.h"
#include "patch.h"
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "sharedlocation.h"
#include "theme.h"
#include "transition.h"
#include "transportque.h"
#include <string.h>


AModule::AModule(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_AUDIO;
	transition_temp = 0;
	level_history = 0;
	current_level = 0;
}




AModule::~AModule()
{
	if(transition_temp) delete [] transition_temp;
	if(level_history)
	{
		delete [] level_history;
		delete [] level_samples;
	}
}

AttachmentPoint* AModule::new_attachment(Plugin *plugin)
{
	return new AAttachmentPoint(renderengine, plugin);
}


void AModule::create_objects()
{
	Module::create_objects();
// Not needed in pluginarray
	if(commonrender)
	{
		level_history = new double[((ARender*)commonrender)->history_size()];
		level_samples = new int64_t[((ARender*)commonrender)->history_size()];
		current_level = 0;

		for(int i = 0; i < ((ARender*)commonrender)->history_size(); i++)
		{
			level_history[i] = 0;
			level_samples[i] = -1;
		}
	}
}

int AModule::get_buffer_size()
{
//printf("AModule::get_buffer_size 1 %p\n", get_edl());
	return get_edl()->session->audio_module_fragment;
}

void AModule::reverse_buffer(double *buffer, int64_t len)
{
	register int64_t start, end;
	double temp;

	for(start = 0, end = len - 1; end > start; start++, end--)
	{
		temp = buffer[start];
		buffer[start] = buffer[end];
		buffer[end] = temp;
	}
}


CICache* AModule::get_cache()
{
	if(renderengine) 
		return renderengine->get_acache();
	else
		return cache;
}

int AModule::render(double *buffer, 
	int64_t input_len, 
	int64_t input_position,
	int direction)
{
	AEdit *playable_edit;
	int64_t start_project = input_position;
	int64_t end_project = input_position + input_len;
	int64_t buffer_offset = 0;
	int result = 0;


//printf("AModule::render 1 %d %d\n", input_position, input_len);

// Flip range around so start_project < end_project
	if(direction == PLAY_REVERSE)
	{
		start_project -= input_len;
		end_project -= input_len;
	}

	bzero(buffer, input_len * sizeof(double));

//printf("AModule::render 2\n");

// Get first edit containing range
	for(playable_edit = (AEdit*)track->edits->first; 
		playable_edit;
		playable_edit = (AEdit*)playable_edit->next)
	{
		double edit_start = playable_edit->startproject;
		double edit_end = playable_edit->startproject + playable_edit->length;

		if(start_project < edit_end && start_project + input_len > edit_start)
		{
			break;
		}
	}





//printf("AModule::render 3\n");


// Fill output one fragment at a time
	while(start_project < end_project)
	{
		int64_t fragment_len = input_len;

//printf("AModule::render 4 %d\n", fragment_len);
		if(fragment_len + start_project > end_project)
			fragment_len = end_project - start_project;

//printf("AModule::render 5 %d\n", fragment_len);
		update_transition(start_project, PLAY_FORWARD);


		if(playable_edit)
		{
//printf("AModule::render 6\n");


// Trim fragment_len
			if(fragment_len + start_project > 
				playable_edit->startproject + playable_edit->length)
				fragment_len = playable_edit->startproject + 
					playable_edit->length - start_project;

//printf("AModule::render 8 %d\n", fragment_len);

			if(playable_edit->asset)
			{
				File *source;

//printf("AModule::render 9\n");


				if(!(source = get_cache()->check_out(playable_edit->asset)))
				{
// couldn't open source file / skip the edit
					result = 1;
					printf("VirtualAConsole::load_track Couldn't open %s.\n", playable_edit->asset->path);
				}
				else
				{
					int result = 0;

//printf("AModule::render 10\n");

//printf("AModule::load_track 7 %d\n", start_project - 
//							playable_edit->startproject + 
//							playable_edit->startsource);

					result = source->set_audio_position(start_project - 
							playable_edit->startproject + 
							playable_edit->startsource, 
						get_edl()->session->sample_rate);
//printf("AModule::render 10\n");

					if(result) printf("AModule::render start_project=%d playable_edit->startproject=%d playable_edit->startsource=%d\n"
						"source=%p playable_edit=%p edl=%p edlsession=%p sample_rate=%d\n",
						start_project, playable_edit->startproject, playable_edit->startsource, 
						source, playable_edit, get_edl(), get_edl()->session, get_edl()->session->sample_rate);

					source->set_channel(playable_edit->channel);

// printf("AModule::render 11 %p %p %d %d\n", 
// 	source, 
// 	buffer, 
// 	buffer_offset, 
// 	fragment_len);
//printf("AModule::render 10 %p %p\n", source, playable_edit);
					source->read_samples(buffer + buffer_offset, 
						fragment_len,
						get_edl()->session->sample_rate);

//printf("AModule::render 12 %d %d\n", fragment_len, get_edl()->session->sample_rate);
					get_cache()->check_in(playable_edit->asset);
//printf("AModule::render 13\n");
				}
			}






// Read transition into temp and render
			AEdit *previous_edit = (AEdit*)playable_edit->previous;
			if(transition && previous_edit)
			{

// Read into temp buffers
// Temp + master or temp + temp ? temp + master
				if(!transition_temp)
				{
					transition_temp = new double[get_edl()->session->audio_read_length];
				}



// Trim transition_len
				int transition_len = fragment_len;
				if(fragment_len + start_project > 
					playable_edit->startproject + transition->length)
					fragment_len = playable_edit->startproject + 
						playable_edit->transition->length - 
						start_project;

				if(transition_len > 0)
				{
					if(previous_edit->asset)
					{
						File *source;
						if(!(source = get_cache()->check_out(previous_edit->asset)))
						{
// couldn't open source file / skip the edit
							printf("VirtualAConsole::load_track Couldn't open %s.\n", playable_edit->asset->path);
							result = 1;
						}
						else
						{
							int result = 0;

							result = source->set_audio_position(start_project - 
									previous_edit->startproject + 
									previous_edit->startsource, 
								get_edl()->session->sample_rate);

							if(result) printf("AModule::render start_project=%d playable_edit->startproject=%d playable_edit->startsource=%d\n"
								"source=%p playable_edit=%p edl=%p edlsession=%p sample_rate=%d\n",
								start_project, 
								previous_edit->startproject, 
								previous_edit->startsource, 
								source, 
								playable_edit, 
								get_edl(), 
								get_edl()->session, 
								get_edl()->session->sample_rate);

							source->set_channel(previous_edit->channel);

							source->read_samples(transition_temp, 
								transition_len,
								get_edl()->session->sample_rate);

							get_cache()->check_in(previous_edit->asset);
						}
					}
					else
					{
						bzero(transition_temp, transition_len * sizeof(double));
					}

					double *output = buffer + buffer_offset;
					transition_server->process_realtime(
						&transition_temp,
						&output,
						start_project - playable_edit->startproject,
						transition_len,
						transition->length);
				}
			}
		}

//printf("AModule::render 13\n");
		buffer_offset += fragment_len;
		start_project += fragment_len;
		if(playable_edit &&
			start_project >= playable_edit->startproject + playable_edit->length)
			playable_edit = (AEdit*)playable_edit->next;
	}

//printf("AModule::render 14\n");

// Reverse buffer here so plugins always render forward.
	if(direction == PLAY_REVERSE) 
		reverse_buffer(buffer, input_len);

//printf("AModule::render 15\n");
	return result;
}









