#include "amodule.h"
#include "arender.h"
#include "atrack.h"
#include "atrack.h"
#include "audiodevice.h"
#include "auto.h"
#include "autos.h"
#include "cache.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "mainsession.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualconsole.h"
#include "virtualnode.h"

ARender::ARender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
// Clear output buffers
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		audio_out[i] = 0;
		level_history[i] = 0;
	}
	level_samples = 0;
	total_peaks = 0;

	data_type = TRACK_AUDIO;
}

ARender::~ARender()
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
//printf("ARender::~ARender %p %p\n", audio_out[i], level_history[i]);
		if(audio_out[i]) delete [] audio_out[i];
		if(level_history[i]) delete [] level_history[i];
	}
	if(level_samples) delete [] level_samples;
}

void ARender::arm_command()
{
//printf("ARender::arm_command  1\n");
	CommonRender::arm_command();
//printf("ARender::arm_command  1\n");
	asynchronous = 1;
//printf("ARender::arm_command  1\n");
	init_meters();
//printf("ARender::arm_command  2\n");
}


int ARender::get_total_tracks()
{
	return renderengine->edl->tracks->total_audio_tracks();
}

Module* ARender::new_module(Track *track)
{
//printf("VirtualAConsole::new_module %p\n", track);
	return new AModule(renderengine, this, 0, track);
}

int ARender::history_size()
{
	if(total_peaks > 0) 
		return total_peaks;
	else
	{
		meter_render_fragment = renderengine->edl->session->audio_module_fragment;
		while(meter_render_fragment > renderengine->edl->session->sample_rate / 10) 
			meter_render_fragment /= 2;
		total_peaks = 2 * 
			renderengine->edl->session->audio_read_length / 
			meter_render_fragment;
		return total_peaks;
	}
}

int ARender::init_meters()
{
// not providing enough peaks results in peaks that are ahead of the sound
//printf("ARender::init_meters 1\n");
	if(level_samples) delete [] level_samples;
	level_samples = new int64_t[history_size()];
	for(int i = 0; i < MAXCHANNELS;i++)
	{
		current_level[i] = 0;
		if(audio_out[i] && !level_history[i]) level_history[i] = new double[total_peaks];
	}

//printf("ARender::init_meters 2\n");
	for(int i = 0; i < history_size(); i++)
	{
		level_samples[i] = -1;
	}
	
	for(int j = 0; j < MAXCHANNELS; j++)
	{
		if(audio_out[j]) 
			for(int i = 0; i < history_size(); i++)
				level_history[j][i] = 0;
	}
	return 0;
}

void ARender::init_output_buffers()
{
//printf("ARender::init_output_buffers %d\n", renderengine->adjusted_fragment_len);
	if(renderengine->command->realtime)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
// Reset the output buffers in case speed changed
			if(audio_out[i])
			{
				delete [] audio_out[i];
				audio_out[i] = 0;
			}

//printf("ARender::init_output_buffers %d %d\n", i, renderengine->config->aconfig->do_channel[i]);
			if(renderengine->config->aconfig->do_channel[i])
			{
				audio_out[i] = new double[renderengine->adjusted_fragment_len];
			}
		}
	}
}


VirtualConsole* ARender::new_vconsole_object() 
{ 
	return new VirtualAConsole(renderengine, this);
}

int64_t ARender::tounits(double position, int round)
{
	if(round)
		return Units::round(position * renderengine->edl->session->sample_rate);
	else
		return (int64_t)(position * renderengine->edl->session->sample_rate);
}

double ARender::fromunits(int64_t position)
{
	return (double)position / renderengine->edl->session->sample_rate;
}


int ARender::process_buffer(double **buffer_out, 
	int64_t input_len, 
	int64_t input_position, 
	int last_buffer)
{
	int result = 0;
//printf("ARender::process_buffer 1\n");
	this->last_playback = last_buffer;
	int64_t fragment_position = 0;
	int64_t fragment_len = input_len;
	int reconfigure = 0;
	current_position = input_position;

//printf("ARender::process_buffer 2\n");
	while(fragment_position < input_len)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(buffer_out[i])
				this->audio_out[i] = buffer_out[i] + fragment_position;
			else
				this->audio_out[i] = 0;
		}

		fragment_len = input_len;
		if(fragment_position + fragment_len > input_len)
			fragment_len = input_len - fragment_position;

//printf("ARender::process_buffer 3 %d\n", fragment_len);
		reconfigure = vconsole->test_reconfigure(input_position, 
			fragment_len,
			last_playback);

//printf("ARender::process_buffer 4 %d %d %d\n", input_position, fragment_len, reconfigure);


		if(reconfigure) restart_playback();
//printf("ARender::process_buffer 6\n");

		result = process_buffer(fragment_len, input_position);
//printf("ARender::process_buffer 7\n");

		fragment_position += fragment_len;
		input_position += fragment_len;
		current_position = input_position;
	}

// Don't delete audio_out on completion
	bzero(this->audio_out, sizeof(double*) * MAXCHANNELS);
// printf("ARender::process_buffer 1 %.2f %.2f %.2f %.2f %.2f\n", 
// 	buffer_out[0][0], 
// 	buffer_out[0][1], 
// 	buffer_out[0][2], 
// 	buffer_out[0][3], 
// 	buffer_out[0][4]);
//printf("ARender::process_buffer 8\n");
	return result;
	return 0;
}


int ARender::process_buffer(int64_t input_len, int64_t input_position)
{
//printf("ARender::process_buffer 1\n");
	int result = ((VirtualAConsole*)vconsole)->process_buffer(input_len,
		input_position,
		last_playback,
		session_position);






// advance counters
	session_position += input_len;
	return result;
}

int ARender::restart_playback()
{
// Use for rebuilding the virtual console during playback.
// Send last buffer to old thread.
	if(vconsole)
	{
		send_reconfigure_buffer();
		vconsole->wait_for_completion();
	}

	CommonRender::restart_playback();
	return 0;
}

int ARender::get_history_number(int64_t *table, int64_t position)
{
// Get the entry closest to position
	int result = 0;
	int64_t min_difference = 0x7fffffff;
	for(int i = 0; i < history_size(); i++)
	{
//printf("%d %d ", i, table[i]);
		if(labs(table[i] - position) < min_difference)
		{
//printf("\n");
			min_difference = labs(table[i] - position);
			result = i;
		}
	}
//printf("\n");
//printf("ARender::get_history_number %ld %d %d\n", position, result, min_difference);
	return result;
}


int ARender::wait_device_completion()
{
// audio device should be entirely cleaned up by vconsole
	renderengine->audio->wait_for_completion();
	return 0;
}

void ARender::run()
{
	int64_t current_input_length;
	int reconfigure = 0;
//printf("ARender::run 1\n");

//sleep(1);
	start_lock.unlock();

//printf("ARender::run 2 %d %d %d\n", done, interrupt, last_playback);
	while(!done && !interrupt && !last_playback)
	{
		current_input_length = renderengine->edl->session->audio_read_length;
//printf("ARender::run 2 %d\n", current_input_length);
		get_boundaries(current_input_length);
//printf("ARender::run 3 %d\n", current_input_length);

		if(current_input_length)
		{
//printf("ARender::run 4 %d\n", current_input_length);
			reconfigure = vconsole->test_reconfigure(current_position, 
				current_input_length,
				last_playback);
//printf("ARender::run 5 %d %d\n", reconfigure, current_input_length);
			
			
			
			
			
// 			test_virtualnodes(current_position, 
// 				current_input_length, 
// 				get_datatype());
			if(reconfigure) restart_playback();
		}
//printf("ARender::run 4 %d %d %d\n", current_position, current_input_length, reconfigure);


// Update tracking if no video is playing.
		if(renderengine->command->realtime && 
			renderengine->playback_engine &&
			!renderengine->do_video)
		{
			double position = (double)renderengine->audio->current_position() / 
				renderengine->edl->session->sample_rate * 
				renderengine->command->get_speed();

			if(renderengine->command->get_direction() == PLAY_FORWARD) 
				position += renderengine->command->playbackstart;
			else
				position = renderengine->command->playbackstart - position;

//printf("ARender::run 6 %f\n", renderengine->audio->current_position());
// This number is not compensated for looping.  It's compensated in 
// PlaybackEngine::get_tracking_position when interpolation also happens.
			renderengine->playback_engine->update_tracking(position);
		}
//printf("ARender::run 7 %d\n", current_position);






		process_buffer(current_input_length, current_position);
//printf("ARender::run 8 %d %d %d\n", done, interrupt, last_playback);

		advance_position(get_render_length(current_input_length));
		if(vconsole->interrupt) interrupt = 1;
//printf("ARender::run 9 %d %d %d\n", done, interrupt, last_playback);
	}

//printf("ARender::run 10\n");
	if(renderengine->command->realtime) wait_device_completion();
//printf("ARender::run 11\n");
	vconsole->stop_rendering(0);
//printf("ARender::run 12\n");
}


















int ARender::get_datatype()
{
	return TRACK_AUDIO;
}

int ARender::arm_playback(int64_t current_position,
			int64_t input_length, 
			int64_t amodule_render_fragment, 
			int64_t playback_buffer, 
			int64_t output_length)
{
	this->current_position = current_position;
	this->input_length = input_length;
	session_position = 0;
	startup_lock.lock();

	source_length = renderengine->end_position - renderengine->start_position;


	if(renderengine->command->realtime)
	{
		Thread::set_realtime(renderengine->edl->session->real_time_playback);
		init_meters();
	}


// start reading input and sending to arenderthread
// only if there's an audio device
	if(renderengine->command->realtime)	
	{
		set_synchronous(1);
		start();
	}
	return 0;
}

int ARender::send_reconfigure_buffer()
{
	if(renderengine->command->realtime)
	{
		vconsole->output_lock[vconsole->current_input_buffer]->lock();

		vconsole->input_len[vconsole->current_input_buffer] = 0;
		vconsole->input_position[vconsole->current_input_buffer] = 0;
		vconsole->last_playback[vconsole->current_input_buffer] = 0;
		vconsole->last_reconfigure[vconsole->current_input_buffer] = 1;

		vconsole->input_lock[vconsole->current_input_buffer]->unlock();
		vconsole->swap_input_buffer();
	}
	return 0;
}

int ARender::reverse_buffer(double *buffer, int64_t len)
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

int ARender::get_next_peak(int current_peak)
{
	current_peak++;
	if(current_peak == total_peaks) current_peak = 0;
	return current_peak;
}

int64_t ARender::get_render_length(int64_t current_render_length)
{
	return current_render_length;
}

int ARender::wait_for_startup()
{
	startup_lock.lock();
	startup_lock.unlock();
}
