#include "asset.h"
#include "cache.h"
#include "condition.h"
#include "datatype.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "strategies.inc"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"
#include "videodevice.h"
#include "virtualconsole.h"
#include "virtualvconsole.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"





VRender::VRender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
	data_type = TRACK_VIDEO;
	transition_temp = 0;
	overlayer = new OverlayFrame(renderengine->preferences->processors);
	input_temp = 0;
}

VRender::~VRender()
{
	if(input_temp) delete input_temp;
	if(transition_temp) delete transition_temp;
	if(overlayer) delete overlayer;
}


VirtualConsole* VRender::new_vconsole_object() 
{
	return new VirtualVConsole(renderengine, this);
}

int VRender::get_total_tracks()
{
	return renderengine->edl->tracks->total_video_tracks();
}

Module* VRender::new_module(Track *track)
{
	return new VModule(renderengine, this, 0, track);
}

int VRender::flash_output()
{
	return renderengine->video->write_buffer(video_out, renderengine->edl);
}

int VRender::process_buffer(VFrame **video_out, 
	int64_t input_position, 
	int last_buffer)
{
// process buffer for non realtime
	int i, j;
	int64_t render_len = 1;
	int reconfigure = 0;


	for(i = 0; i < MAX_CHANNELS; i++)
		this->video_out[i] = video_out[i];
	this->last_playback = last_buffer;

	current_position = input_position;

// test for automation configuration and shorten the fragment len if necessary
	reconfigure = vconsole->test_reconfigure(input_position, 
		render_len,
		last_playback);

	if(reconfigure) restart_playback();
	return process_buffer(input_position);
}


int VRender::process_buffer(int64_t input_position)
{
	Edit *playable_edit = 0;
	int colormodel;
	int use_vconsole = 1;
	int use_brender = 0;
	int result = 0;

//sleep(1);

// Determine the rendering strategy for this frame.
	use_vconsole = get_use_vconsole(playable_edit, 
		input_position,
		use_brender);


// Negotiate color model
	colormodel = get_colormodel(playable_edit, use_vconsole, use_brender);

// Get output buffer from device
	if(renderengine->command->realtime)
		renderengine->video->new_output_buffers(video_out, colormodel);

// Read directly from file to video_out
	if(!use_vconsole)
	{

		if(use_brender)
		{
			Asset *asset = renderengine->preferences->brender_asset;
			File *file = renderengine->get_vcache()->check_out(asset);
			if(file)
			{
				int64_t corrected_position = current_position;
				if(renderengine->command->get_direction() == PLAY_REVERSE)
					corrected_position--;

				file->set_video_position(corrected_position, 
					renderengine->edl->session->frame_rate);
// Cache single frames only
				if(renderengine->command->single_frame())
					file->set_cache_frames(1);
				file->read_frame(video_out[0]);
				if(renderengine->command->single_frame())
					file->set_cache_frames(0);
				renderengine->get_vcache()->check_in(asset);
			}
		}
		else
		if(playable_edit)
		{
			result = ((VEdit*)playable_edit)->read_frame(video_out[0], 
				current_position, 
				renderengine->command->get_direction(),
				renderengine->get_vcache(),
				1,
				renderengine->command->single_frame());
		}
	}
	else
// Read into virtual console
	{

// process this buffer now in the virtual console
		result = ((VirtualVConsole*)vconsole)->process_buffer(input_position);

	}


	return result;
}

// Determine if virtual console is needed
int VRender::get_use_vconsole(Edit* &playable_edit, 
	int64_t position,
	int &use_brender)
{
	Track *playable_track;


// Background rendering completed
	if((use_brender = renderengine->brender_available(position, 
		renderengine->command->get_direction())) != 0) 
		return 0;



// Total number of playable tracks is 1
	if(vconsole->total_entry_nodes != 1) return 1;

	playable_track = vconsole->playable_tracks->values[0];

// Test mutual conditions between render.C and this.
	if(!playable_track->direct_copy_possible(position, 
		renderengine->command->get_direction(),
		1))
		return 1;

	playable_edit = playable_track->edits->editof(position, 
		renderengine->command->get_direction(),
		1);
// No edit at current location
	if(!playable_edit) return 1;

// Edit is silence
	if(!playable_edit->asset) return 1;

// Asset and output device must have the same dimensions
	if(playable_edit->asset->width != renderengine->edl->session->output_w ||
		playable_edit->asset->height != renderengine->edl->session->output_h)
		return 1;

// If we get here the frame is going to be directly copied.  Whether it is
// decompressed in hardware depends on the colormodel.
	return 0;
}

int VRender::get_colormodel(Edit* &playable_edit, 
	int use_vconsole,
	int use_brender)
{
	int colormodel = renderengine->edl->session->color_model;

//printf("VRender::get_colormodel 1\n");
	if(!use_vconsole && !renderengine->command->single_frame())
	{
// Get best colormodel supported by the file
		int driver = renderengine->config->vconfig->driver;
		File *file;
		Asset *asset;

		if(use_brender)
		{
			asset = renderengine->preferences->brender_asset;
		}
		else
		{
			asset = playable_edit->asset;
		}

//printf("VRender::get_colormodel 1\n");
		file = renderengine->get_vcache()->check_out(asset);
//printf("VRender::get_colormodel 10\n");

		if(file)
		{
//printf("VRender::get_colormodel 20n");
			colormodel = file->get_best_colormodel(driver);
//printf("VRender::get_colormodel 30\n");
			renderengine->get_vcache()->check_in(asset);
//printf("VRender::get_colormodel 40\n");
		}
	}
//printf("VRender::get_colormodel 100\n");
	return colormodel;
}







void VRender::run()
{
	int reconfigure;

// Want to know how many samples rendering each frame takes.
// Then use this number to predict the next frame that should be rendered.
// Be suspicious of frames that render late so have a countdown
// before we start dropping.
	int64_t current_sample, start_sample, end_sample; // Absolute counts.
	int64_t next_frame;  // Actual position.
	int64_t last_delay = 0;  // delay used before last frame
	int64_t skip_countdown = VRENDER_THRESHOLD;    // frames remaining until drop
	int64_t delay_countdown = VRENDER_THRESHOLD;  // Frames remaining until delay
// Number of frames before next reconfigure
	int64_t current_input_length;
// Number of frames to skip.
	int64_t frame_step = 1;

	first_frame = 1;

// Number of frames since start of rendering
	session_frame = 0;
	framerate_counter = 0;
	framerate_timer.update();

	start_lock->unlock();


	while(!done && 
		!renderengine->video->interrupt && 
		!last_playback)
	{
// Perform the most time consuming part of frame decompression now.
// Want the condition before, since only 1 frame is rendered 
// and the number of frames skipped after this frame varies.
		current_input_length = 1;

		reconfigure = vconsole->test_reconfigure(current_position, 
			current_input_length,
			last_playback);

		if(reconfigure) restart_playback();

		process_buffer(current_position);

		if(renderengine->command->single_frame())
		{
			flash_output();
			frame_step = 1;
			done = 1;
		}
		else
// Perform synchronization
		{
// Determine the delay until the frame needs to be shown.
			current_sample = (int64_t)(renderengine->sync_position() * 
				renderengine->command->get_speed());
// latest sample at which the frame can be shown.
			end_sample = Units::tosamples(session_frame, 
				renderengine->edl->session->sample_rate, 
				renderengine->edl->session->frame_rate);
// earliest sample by which the frame needs to be shown.
			start_sample = Units::tosamples(session_frame - 1, 
				renderengine->edl->session->sample_rate, 
				renderengine->edl->session->frame_rate);

			if(first_frame || end_sample < current_sample)
			{
// Frame rendered late or this is the first frame.  Flash it now.
				flash_output();

				if(renderengine->edl->session->video_every_frame)
				{
// User wants every frame.
					frame_step = 1;
				}
				else
				if(skip_countdown > 0)
				{
// Maybe just a freak.
					frame_step = 1;
					skip_countdown--;
				}
				else
				{
// Get the frames to skip.
					delay_countdown = VRENDER_THRESHOLD;
					frame_step = 1;
					frame_step += (int64_t)Units::toframes(current_sample, 
							renderengine->edl->session->sample_rate, 
							renderengine->edl->session->frame_rate);
					frame_step -= (int64_t)Units::toframes(end_sample, 
								renderengine->edl->session->sample_rate, 
								renderengine->edl->session->frame_rate);
				}
//printf("VRender:run 11 frame_step %d\n", frame_step);
			}
			else
			{
// Frame rendered early or just in time.
				frame_step = 1;

				if(delay_countdown > 0)
				{
// Maybe just a freak
					delay_countdown--;
				}
				else
				{
					skip_countdown = VRENDER_THRESHOLD;
					if(start_sample > current_sample)
					{
						int64_t delay_time = (int64_t)((float)(start_sample - current_sample) * 
							1000 / 
							renderengine->edl->session->sample_rate);
						timer.delay(delay_time);
//printf("VRender:run 10 %lld\n", delay_time);
					}
					else
					{
// Came after the earliest sample so keep going
					}
				}

// Flash frame now.
				flash_output();
			}
		}

// Trigger audio to start
		if(first_frame)
		{
			renderengine->first_frame_lock->unlock();
			first_frame = 0;
			renderengine->reset_sync_position();
		}

		session_frame += frame_step;

// advance position in project
		current_input_length = frame_step;


// Subtract frame_step in a loop to allow looped playback to drain
		while(frame_step && current_input_length && !last_playback)
		{
// set last_playback if necessary and trim current_input_length to range
			get_boundaries(current_input_length);
// advance 1 frame
			advance_position(current_input_length);
			frame_step -= current_input_length;
			current_input_length = frame_step;
		}

// Update tracking.
		if(renderengine->command->realtime &&
			renderengine->playback_engine &&
			renderengine->command->command != CURRENT_FRAME)
		{
			renderengine->playback_engine->update_tracking(fromunits(current_position));
		}

// Calculate the framerate counter
		framerate_counter++;
		if(framerate_counter >= renderengine->edl->session->frame_rate && 
			renderengine->command->realtime)
		{
			renderengine->update_framerate((float)framerate_counter / 
				((float)framerate_timer.get_difference() / 1000));
			framerate_counter = 0;
			framerate_timer.update();
		}
	}

// In case we were interrupted before the first loop
	renderengine->first_frame_lock->unlock();

}
























VRender::VRender(MWindow *mwindow, RenderEngine *renderengine)
 : CommonRender(mwindow, renderengine)
{
	input_length = 0;
	vmodule_render_fragment = 0;
	playback_buffer = 0;
	session_frame = 0;
	asynchronous = 0;     // render 1 frame at a time
	framerate_counter = 0;
	video_out[0] = 0;
	render_strategy = -1;
}

int VRender::init_device_buffers()
{
// allocate output buffer if there is a video device
	if(renderengine->video)
	{
		video_out[0] = 0;
		render_strategy = -1;
	}
}

int VRender::get_datatype()
{
	return TRACK_VIDEO;
}


int VRender::start_playback()
{
// start reading input and sending to vrenderthread
// use a thread only if there's a video device
	if(renderengine->command->realtime)
	{
		start();
	}
}

int VRender::wait_for_startup()
{
}







int64_t VRender::tounits(double position, int round)
{
	if(round)
		return Units::round(position * renderengine->edl->session->frame_rate);
	else
		return Units::to_int64(position * renderengine->edl->session->frame_rate);
}

double VRender::fromunits(int64_t position)
{
	return (double)position / renderengine->edl->session->frame_rate;
}
