// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "asset.h"
#include "audiorender.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "file.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "packagerenderer.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "renderengine.h"
#include "sighandler.h"
#include "tmpframecache.h"
#include "transportcommand.h"
#include "vframe.h"
#include "videodevice.h"
#include "videorender.h"


RenderPackage::RenderPackage()
{
	audio_start_pts = 0;
	audio_end_pts = 0;
	video_start_pts = 0;
	video_end_pts = 0;
	count = 0;
	path[0] = 0;
	done = 0;
	use_brender = 0;
	audio_do = 0;
	video_do = 0;
}

void RenderPackage::dump(int indent)
{
	printf("%*sRenderPackage %p dump:\n", indent, "", this);
	indent += 2;
	if(path[0])
		printf("%*spath '%s'\n", indent, "", path);
	else
		printf("%*spath is empty\n", indent, "");
	printf("%*saudio start %.3f, end %.3f  video start %.3f end %.3f\n", indent, "",
		audio_start_pts, audio_end_pts, video_start_pts, video_end_pts);
	printf("%*suse_brender %d audio_do %d video_do %d count %d\n", indent, "",
		use_brender, audio_do, video_do, count);
}


// Used by RenderFarm and in the future, Render, to do packages.
PackageRenderer::PackageRenderer()
{
	command = 0;
	aconfig = 0;
	vconfig = 0;
	pkg_error[0] = 0;
}

PackageRenderer::~PackageRenderer()
{
	delete command;
}

// PackageRenderer::initialize happens only once for every node when doing rendering session
// This is not called for each package!

void PackageRenderer::initialize(EDL *edl,
	Preferences *preferences,
	Asset *default_asset)
{
	this->edl = edl;
	this->preferences = preferences;
	this->default_asset = default_asset;
	if(!preferences_global)
		preferences_global = preferences;

	command = new TransportCommand;
	command->command = NORMAL_FWD;
	command->set_edl(edl);
	command->set_playback_range();

	aconfig = 0;
	vconfig = 0;
}

int PackageRenderer::create_output()
{
	int result;

	file = new File;

	file->set_processors(preferences->processors);

	result = file->open_file(default_asset, FILE_OPEN_WRITE, 0, package->path);

	if(mwindow_global)
	{
		if(result)
			errormsg(_("Couldn't open output file %s"), package->path);
		mwindow_global->sighandler->push_file(file);
	}
	return result;
}

void PackageRenderer::create_engine()
{
	audio_read_length = AUDIO_BUFFER_SIZE;

	render_engine = new RenderEngine(0, 0);

	aconfig = render_engine->config->aconfig;
	vconfig = render_engine->config->vconfig;
	render_engine->arm_command(command);

	if(package->use_brender)
	{
		audio_preroll = round((ptstime)preferences->brender_preroll *
			edlsession->frame_rate);
		video_preroll = round((ptstime)preferences->brender_preroll *
			edlsession->frame_rate);
	}
	else
	{
		audio_preroll = preferences->render_preroll;
		video_preroll = preferences->render_preroll;
	}

	if((audio_pts = package->audio_start_pts - audio_preroll) < 0)
	{
		audio_preroll += audio_pts;
		audio_pts = 0;
	}
	if((video_pts = package->video_start_pts - video_preroll) < 0)
	{
		video_preroll += video_pts;
		video_pts = 0;
	}

// Create output buffers
	if(default_asset->stream_count(STRDSC_AUDIO))
	{
		file->start_audio_thread(audio_read_length, 
			preferences->processors > 1 ? 2 : 1);
	}

	if(default_asset->stream_count(STRDSC_VIDEO))
	{
// The write length needs to correlate with the processor count because
// it is passed to the file handler which usually processes frames simultaneously.
		video_write_length = preferences->processors;
		video_write_position = 0;
		file->start_video_thread(video_write_length,
			edlsession->color_model,
			preferences->processors > 1 ? 2 : 1);

		if(mwindow_global)
		{
			video_device = new VideoDevice;
			video_device->open_output(vconfig,
				edlsession->output_w,
				edlsession->output_h,
				edlsession->color_model,
				mwindow_global->cwindow->gui->canvas,
				0);
		}
	}
}

int PackageRenderer::do_audio()
{
	AFrame *audio_output_ptr[MAX_CHANNELS];
	AFrame **audio_output;
	AFrame *af;
	ptstime buffer_duration;
	int result;
	int read_length = audio_read_length;
	int stream;

// Do audio data
	if((stream = default_asset->get_stream_ix(STRDSC_AUDIO)) >= 0)
	{
		audio_output = file->get_audio_buffer();
// Zero unused channels in output vector
		for(int i = 0; i < MAX_CHANNELS; i++)
		{
			if(i < default_asset->streams[stream].channels)
			{
				af = audio_output[i];
				if(af)
				{
					af->init_aframe(audio_pts, audio_read_length,
						default_asset->streams[stream].sample_rate);
					af->set_fill_request(audio_pts, audio_read_length);
				}
			}
			else
				af = 0;
			audio_output_ptr[i] = af;
		}
// Call render engine
		if(result = render_engine->arender->process_buffer(audio_output_ptr))
			return result;
// Fix buffers for preroll
		read_length = audio_output_ptr[0]->get_length();
		int output_length = read_length;

		if(audio_preroll > 0)
		{
			int preroll_len = round(audio_preroll *
				default_asset->streams[stream].sample_rate);

			if(preroll_len >= output_length)
				output_length = 0;
			else
			{
				output_length -= preroll_len;
				for(int i = 0; i < MAX_CHANNELS; i++)
				{
					if(audio_output_ptr[i])
					{
						for(int j = 0; j < output_length; j++)
							audio_output_ptr[i]->buffer[j] = audio_output_ptr[i]->buffer[j + audio_read_length - output_length];
						audio_output_ptr[i]->set_length(output_length);
						audio_output_ptr[i]->set_duration(
							audio_output_ptr[i]->get_duration() -
							audio_preroll);
					}
				}
			}
			audio_preroll -= (ptstime)read_length /
				default_asset->streams[stream].sample_rate;
		}
// Must perform writes even if 0 length so get_audio_buffer doesn't block
		result |= file->write_audio_buffer(output_length);
	}
	audio_pts += (ptstime)read_length / default_asset->streams[stream].sample_rate;
	return 0;
}

int PackageRenderer::do_video()
{
// Do video data
	int result;
	int stream;

	if((stream = default_asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
	{
		ptstime video_end = video_pts + video_read_length;
		ptstime duration;

		if(video_end > package->video_end_pts)
			video_end = package->video_end_pts;

		while(video_pts < video_end)
		{
// Try to use the rendering engine to write the frame.
// Get a buffer for background writing.
			if(video_write_position == 0)
				video_output = file->get_video_buffer();

// Construct layered output buffer
			video_output_ptr = video_output[0][video_write_position];
			video_output_ptr->set_pts(video_pts);
			video_output[0][video_write_position] = video_output_ptr =
				render_engine->vrender->process_buffer(video_output_ptr);
			duration = 1.0 / default_asset->streams[stream].frame_rate;
			video_output_ptr->set_pts(video_pts);
			video_output_ptr->set_duration(duration);
			if(mwindow_global && video_device->output_visible())
			{
// Vector for video device
				VFrame *preview_output =
					BC_Resources::tmpframes.clone_frame(video_output_ptr);

				preview_output->copy_from(video_output_ptr);
				video_device->write_buffer(preview_output, 
					command->get_edl());
			}

// Don't write to file
			if(video_preroll > 0)
			{
				video_preroll -= duration;
// Keep the write position at 0 until ready to write real frames
				if(result = file->write_video_buffer(0))
					return result;
				video_write_position = 0;
			}
			else
			{
// Set background rendering parameters
// Allow us to skip sections of the output file by setting the frame number.
// Used by background render and render farm.
				if(video_write_position == 0)
					brender_base = video_pts;
				video_output_ptr->set_frame_number(round(video_pts *
					default_asset->streams[stream].frame_rate));
				video_write_position++;
				if(video_write_position >= video_write_length)
				{
					result = file->write_video_buffer(video_write_position);

					if(result)
						return result;
// Update the brender map after writing the files.
					if(package->use_brender)
						set_video_map(brender_base, video_pts + duration);

					video_write_position = 0;
				}
			}
			package->count++;
			video_pts += duration;
			if(get_result())
				return 1;
			if(!package->use_brender && progress_cancelled())
				return 1;
		}
	}
	else
		video_pts += video_read_length;
	return 0;
}

void PackageRenderer::stop_engine()
{
	delete render_engine;
}

void PackageRenderer::stop_output()
{
	file->stop_audio_thread();

	if(default_asset->stream_count(STRDSC_VIDEO))
	{
		file->write_video_buffer(video_write_position);
		if(package->use_brender)
			set_video_map(brender_base, video_pts);

		video_write_position = 0;

		file->stop_video_thread();

		if(mwindow_global)
		{
			video_device->close_all();
			delete video_device;
		}
	}
}

void PackageRenderer::close_output()
{
	if(mwindow_global)
		mwindow_global->sighandler->pull_file(file);
	file->close_file();
	delete file;
}

// Aborts and returns 1 if an error is encountered.
int PackageRenderer::render_package(RenderPackage *package)
{
	int audio_done = 0;
	int video_done = 0;
	ptstime duration_rendered = 0;
	int result = 0;

	this->package = package;
	brender_base = -1;

// Command initalizes start positions from cursor position
//  what is irrelevant here. Get start and end from package.
	if(package->audio_do)
	{
		command->start_position = package->audio_start_pts;
		command->end_position = package->audio_end_pts;
	}

	if(package->video_do)
	{
		if(command->start_position > package->video_start_pts)
			command->start_position = package->video_start_pts;
		if(command->end_position < package->video_end_pts)
			command->end_position = package->video_end_pts;
	}
	command->playbackstart = command->start_position;

	if(create_output())
	{
		sprintf(pkg_error, _("Failed to create %s\n"), package->path);
		result = 1;
	}

// Create render engine
	if(!result)
	{
		int stream = default_asset->get_stream_ix(STRDSC_AUDIO);

		create_engine();
// Main loop
		while(!audio_done || !video_done && !result)
		{
			int need_audio = 0, need_video = 0;

// Calculate lengths to process.  Audio fragment is constant.
			if(!audio_done)
			{
				int samplerate =
					default_asset->streams[stream].sample_rate;
				samplenum audio_position = round(audio_pts * samplerate);
				samplenum audio_end = round(package->audio_end_pts * samplerate);

				if(audio_position + audio_read_length >= audio_end)
				{
					audio_done = 1;
					audio_read_length = audio_end - audio_position;
				}

				duration_rendered = (ptstime)audio_read_length / samplerate;
				need_audio = 1;
			}

			if(!video_done)
			{
				if(audio_done)
				{
					video_read_length = package->video_end_pts - video_pts;
// Packetize video length so progress gets updated
					video_read_length = MIN(1.0, video_read_length);
				}
				else
// Guide video with audio
				{
					video_read_length = audio_pts +
						(ptstime)audio_read_length /
						default_asset->streams[stream].sample_rate -
						video_pts;
				}

// Clamp length
				if(video_pts + video_read_length >= package->video_end_pts)
				{
					video_done = 1;
					video_read_length = package->video_end_pts - video_pts;
				}

// Calculate samples rendered for progress bar.
				if(audio_done)
					duration_rendered = video_read_length;

				need_video = 1;
			}

			if(need_video)
			{
				if(do_video())
				{
					sprintf(pkg_error, _("Failed to render video to %s\n"),
						package->path);
					result = 1;
					break;
				}
			}

			if(need_audio)
			{
				if(do_audio())
				{
					sprintf(pkg_error, _("Failed to render audio to %s\n"),
						package->path);
					result = 1;
					break;
				}
			}

			set_progress(duration_rendered);

			if(!package->use_brender && progress_cancelled())
			{
				sprintf(pkg_error, _("Rendering canelled"));
				result = 1;
				break;
			}

			result = get_result();
		}

		stop_engine();
		stop_output();
	}

	close_output();
	set_result(result, pkg_error);
	return result;
}
