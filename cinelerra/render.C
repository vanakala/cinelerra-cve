// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "batchrender.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "bctitle.h"
#include "cinelerra.h"
#include "clip.h"
#include "condition.h"
#include "cwindow.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "formattools.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "paramlist.h"
#include "preferences.h"
#include "renderfarm.h"
#include "render.h"
#include "timebar.h"
#include "transportcommand.h"
#include "units.h"
#include "vframe.h"
#include "renderprofiles.h"

#include <ctype.h>
#include <string.h>
#include <unistd.h>

RenderItem::RenderItem()
 : BC_MenuItem(_("Render..."), "Shift+R", 'R')
{
	set_shift(1);
}

int RenderItem::handle_event() 
{
	mwindow_global->render->start_interactive();
	return 1;
}


RenderProgress::RenderProgress(Render *render)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->render = render;
	last_value = 0;
}

RenderProgress::~RenderProgress()
{
	Thread::cancel();
	Thread::join();
}

void RenderProgress::run()
{
	Thread::disable_cancel();
	while(1)
	{
		if(render->total_rendered != last_value)
		{
			render->progress->update(render->total_rendered);
			last_value = render->total_rendered;
		}

		Thread::enable_cancel();
		sleep(1);
		Thread::disable_cancel();
	}
}


MainPackageRenderer::MainPackageRenderer(Render *render)
 : PackageRenderer()
{
	this->render = render;
}

int MainPackageRenderer::get_master()
{
	return 1;
}

int MainPackageRenderer::get_result()
{
	return render->result;
}

void MainPackageRenderer::set_result(int value)
{
	if(value)
		render->result = value;
}

void MainPackageRenderer::set_progress(ptstime value)
{
	render->counter_lock->lock("MainPackageRenderer::set_progress");
	render->total_rendered += value;

// If non interactive, print progress out
	if(!render->progress)
	{
		int64_t current_eta = render->progress_timer->get_scaled_difference(1000);
		if(current_eta - render->last_eta > 1000)
		{
			double eta = 0;

			if(render->total_rendered)
			{
				eta = current_eta /
					1000 *
					render->progress_max /
					render->total_rendered -
					current_eta /
					1000;
			}

			char string[BCTEXTLEN];
			Units::totext(string, 
				eta,
				TIME_HMS2);

			printf("\r%d%% ETA: %s      ", (int)(100 * 
					render->total_rendered / 
					render->progress_max),
				string);
			fflush(stdout);
			render->last_eta = current_eta;
		}
	}
	render->counter_lock->unlock();
}

int MainPackageRenderer::progress_cancelled()
{
	return (render->progress && render->progress->is_cancelled()) || 
		render->batch_cancelled;
}


Render::Render()
 : Thread()
{
	in_progress = 0;
	progress = 0;
	elapsed_time = 0.0;
	render_window = 0;
	package_lock = new Mutex("Render::package_lock");
	counter_lock = new Mutex("Render::counter_lock");
	completion = new Condition(0, "Render::completion");
	progress_timer = new Timer;
	range_type = RANGE_PROJECT;
	load_mode = LOADMODE_NEW_TRACKS;
	strategy = 0;
}

Render::~Render()
{
	delete package_lock;
	delete counter_lock;
	delete completion;
	delete progress_timer;
}

void Render::start_interactive()
{
	if(!Thread::running())
	{
		mode = Render::INTERACTIVE;
		this->jobs = 0;
		batch_cancelled = 0;
		completion->reset();
		Thread::start();
	}
	else
	{
		// raise the window if rendering hasn't started yet
		if(render_window && !in_progress)
			render_window->raise_window();
		else
			errorbox(_("Already rendering"));
	}
}

void Render::run_menueffects(Asset *asset, EDL *edl,
        int strategy, int range_type, int load_mode)
{
	int old_load_mode;

	mode = Render::EFFECT;
	old_load_mode = this->load_mode;
	this->load_mode = load_mode;
	this->jobs = 0;
	batch_cancelled = 0;
	completion->reset();
	render(1, asset, edl, strategy, range_type);
	this->load_mode = old_load_mode;
}

void Render::start_batches(ArrayList<BatchRenderJob*> *jobs)
{
	batch_cancelled = 0;
	if(!Thread::running())
	{
		mode = Render::BATCH;
		this->jobs = jobs;
		completion->reset();
		Thread::start();
	}
	else
		errorbox("Already rendering");
}

void Render::run_batches(ArrayList<BatchRenderJob*> *jobs)
{
	mode = Render::BATCH;
	batch_cancelled = 0;
	this->jobs = jobs;

	completion->reset();
	run();
}

void Render::stop_operation()
{
	if(Thread::running())
	{
		batch_cancelled = 1;
// Wait for completion
		completion->lock("Render::stop_operation");
		completion->reset();
	}
}

void Render::run()
{
	result = 0;

	if(mode == Render::INTERACTIVE)
	{
// Fix the asset for rendering
		Asset *asset = new Asset;
		load_defaults(asset);
		render_edl = new EDL(0);
		render_edl->copy_all(master_edl);
		render_edl->local_session->copy_from(master_edl->local_session);
		if(master_edl->this_edlsession)
		{
			render_edl->this_edlsession = new EDLSession();
			render_edl->this_edlsession->copy(master_edl->this_edlsession);
		}
		check_asset(render_edl, *asset);
// Get format from user
		render_window = new RenderWindow(this, asset);
		result = render_window->run_window();
		if(!result)
		{
			// add to recentlist only on OK
			render_window->format_tools->path_recent->add_item(ContainerSelection::container_prefix(asset->format), asset->path);
		}
		delete render_window;
		render_window = 0;

		if(!result)
		{
			if(!check_asset(render_edl, *asset))
			{
				asset->init_streams();
				save_defaults(asset);
				mwindow_global->save_defaults();

				if(!render_preferences)
					render_preferences = new Preferences;
				render_preferences->copy_from(preferences_global);

				if(asset->single_image)
					range_type = RANGE_SINGLEFRAME;

				render(1, asset, render_edl, strategy, range_type);
			}
			else
				errormsg(_("No audo or video to render"));
		}

		delete asset;
		delete render_edl->this_edlsession;
		delete render_edl;
		render_edl = 0;
	}
	else
	if(mode == Render::BATCH)
	{
		for(int i = 0; i < jobs->total && !result; i++)
		{
			BatchRenderJob *job = jobs->values[i];

			if(job->enabled)
			{
				if(mwindow_global)
				{
					mwindow_global->batch_render->update_active(i);
				}
				else
				{
					printf("Render::run: %s\n", job->edl_path);
				}

				FileXML *file = new FileXML;
				EDL *edl = new EDL(0);
				EDLSession *session = new EDLSession();
				file->read_from_file(job->edl_path);
				edl->load_xml(file, session);

				File assetfile;
				for(int i = 0; i < edl->assets->total; i++)
				{
					Asset *ap = edl->assets->values[i];

					if(assetfile.open_file(ap, FILE_OPEN_READ | FILE_OPEN_ALL) == FILE_OK)
						assetfile.close_file(0);
					else
					{
						errorbox("Failed to open '%s'", basename(ap->path));
						result = 1;
						break;
					}
				}
				if(!result && !check_asset(edl, *job->asset))
				{
					job->asset->init_streams();
					render(0, job->asset, edl, job->strategy, RANGE_PROJECT);
				}
				delete edl;
				delete file;

				if(!result)
				{
					if(mwindow_global)
						mwindow_global->batch_render->update_done(i, 1, elapsed_time);
					else
					{
						char string[BCTEXTLEN];
						elapsed_time = 
							(double)progress_timer->get_scaled_difference(1);
						Units::totext(string,
							elapsed_time,
							TIME_HMS2);
						printf("Render::run: done in %s\n", string);
					}
				}
				else
				{
					if(mwindow_global)
						mwindow_global->batch_render->update_active(-1);
					else
						printf("Render::run: failed\n");
				}
			}
		}

		if(mwindow_global)
		{
			mwindow_global->batch_render->update_active(-1);
			mwindow_global->batch_render->update_done(-1, 0, 0);
		}
	}
}

int Render::check_asset(EDL *edl, Asset &asset)
{
	if(asset.video_data && 
		edl->playable_tracks_of(TRACK_VIDEO) &&
		(File::supports(asset.format) & SUPPORTS_VIDEO))
	{
		asset.video_data = 1;
		asset.layers = 1;
		asset.width = edlsession->output_w;
		asset.height = edlsession->output_h;
		asset.frame_rate = edlsession->frame_rate;
		asset.interlace_mode = edlsession->interlace_mode;
		asset.sample_aspect_ratio = edlsession->sample_aspect_ratio;
	}
	else
	{
		asset.video_data = 0;
		asset.layers = 0;
	}

	if(asset.audio_data && 
		edl->playable_tracks_of(TRACK_AUDIO) &&
		(File::supports(asset.format) & SUPPORTS_AUDIO))
	{
		asset.audio_data = 1;
		asset.channels = edlsession->audio_channels;
		asset.sample_rate = edlsession->sample_rate;
		if(asset.format == FILE_MOV) asset.byte_order = 0;
	}
	else
	{
		asset.audio_data = 0;
		asset.channels = 0;
	}

	if(!asset.audio_data &&
		!asset.video_data)
	{
		return 1;
	}
	return 0;
}

int Render::fix_strategy(int strategy, int use_renderfarm)
{
	if(use_renderfarm)
		strategy |= RENDER_FARM;
	else
		strategy &= ~RENDER_FARM;

	return strategy;
}

void Render::start_progress()
{
	progress_max = packages->get_progress_max();
	progress_timer->update();
	last_eta = 0;
	if(mwindow_global)
	{
		progress = mwindow_global->mainprogress->start_progress(
			_("Rendering..."), progress_max);
		render_progress = new RenderProgress(this);
		render_progress->start();
	}
}

void Render::stop_progress()
{
	if(progress)
	{
		char string[BCTEXTLEN];
		delete render_progress;
		progress->get_time(string);
		elapsed_time = progress->get_time();
		progress->stop_progress();
		delete progress;

		mwindow_global->show_message(_("Rendering took %s"), string);
		mwindow_global->gui->stop_hourglass();
	}
	progress = 0;
}

int Render::render(int test_overwrite, 
	Asset *asset,
	EDL *edl,
	int strategy,
	int range_type)
{
	RenderFarmServer *farm_server = 0;
	FileSystem fs;
	int total_digits;      // Total number of digits including padding the user specified.
	int number_start;      // Character in the filename path at which the number begins
	int current_number;    // The number the being injected into the filename.
	int done = 0;
	in_progress = 1;
	this->default_asset = asset;
	progress = 0;
	result = 0;

// Create rendering command
	command = new TransportCommand;
	command->command = NORMAL_FWD;
	command->set_edl(edl);
	command->change_type = CHANGE_ALL;

	switch(range_type)
	{
	case RANGE_PROJECT:
		command->playback_range_project();
		break;

	case RANGE_SINGLEFRAME:
		command->command = CURRENT_FRAME;
		// fall through

	case RANGE_SELECTION:
		command->set_playback_range();
		break;

	case RANGE_INOUT:
		command->playback_range_inout();
		break;
	}
	packages = new PackageDispatcher;

	default_asset->frame_rate = edlsession->frame_rate;
	default_asset->sample_rate = edlsession->sample_rate;

// Conform asset to EDL.  Find out if any tracks are playable.
	result = check_asset(command->get_edl(), *default_asset);

	if(!result)
	{
// Get total range to render
		total_start = command->start_position;
		total_end = command->end_position;

// Nothing to render
		result = EQUIV(total_start, total_end) ||
				total_start > edl->total_length();
	}

// Generate packages
	if(!result)
	{
// Stop background rendering
		if(mwindow_global)
			mwindow_global->stop_brender();

		fs.complete_path(default_asset->path);
		strategy = Render::fix_strategy(strategy, render_preferences->use_renderfarm);

		result = packages->create_packages(mwindow_global,
			command->get_edl(),
			render_preferences,
			strategy, 
			default_asset, 
			total_start, 
			total_end,
			test_overwrite);
	}
	done = 0;
	total_rendered = 0;
	frames_per_second = 0;

	if(!result)
	{
// Start dispatching external jobs
		if(mwindow_global)
		{
			mwindow_global->show_message(_("Starting render farm"));
			mwindow_global->gui->start_hourglass();
		}
		else
		{
			printf("Render::render: starting render farm\n");
		}
		if(strategy & RENDER_FARM)
		{
			farm_server = new RenderFarmServer(packages,
				render_preferences,
				1,
				&result,
				&total_rendered,
				counter_lock,
				default_asset,
				command->get_edl(),
				0);
			result = farm_server->start_clients();

			if(result)
			{
				if(mwindow_global)
					mwindow_global->gui->stop_hourglass();
				errormsg(_("Failed to start render farm"));
			}
		}
	}
	if(!result)
	{
		if(!(strategy & RENDER_FARM))
		{
// Perform local rendering
			start_progress();
			MainPackageRenderer package_renderer(this);
			result = package_renderer.initialize(command->get_edl(),
				render_preferences,
				default_asset);

			while(!result)
			{
// Get unfinished job
				RenderPackage *package;

				if(!(strategy & RENDER_FILE_PER_LABEL))
					package = packages->get_package(frames_per_second, -1, 1);
				else
					package = packages->get_package(0, -1, 1);
// Exit point
				if(!package)
				{
					done = 1;
					break;
				}

				Timer timer;
				timer.update();

				if(package_renderer.render_package(package))
					result = 1;

// Result is also set directly by the RenderFarm.
				frames_per_second = (ptstime)package->count /
					((double)timer.get_difference() / 1000);
			} // file_number
		}
		else
			farm_server->wait_clients();

// Notify of error
		if(result && (!progress || !progress->is_cancelled()) &&
				!batch_cancelled)
			errormsg(_("Error rendering data."));
// Delete the progress box
		stop_progress();
	}

	if(!result && mwindow_global && mode != Render::BATCH)
	{
// Paste all packages into timeline if desired
		if(load_mode != LOADMODE_NOTHING)
		{
			ArrayList<char*> path_list;

			packages->get_package_paths(&path_list);

			if(load_mode == LOADMODE_PASTE)
				mwindow_global->clear(0);
			mwindow_global->load_filenames(&path_list, load_mode);

			path_list.remove_all_objects();

			mwindow_global->save_backup();
			mwindow_global->undo->update_undo(_("render"), LOAD_ALL);
			mwindow_global->update_plugin_guis();
			mwindow_global->update_gui(WUPD_SCROLLBARS | WUPD_CANVREDRAW |
				WUPD_TIMEBAR | WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
			mwindow_global->sync_parameters(0);
		}
		else
			mwindow_global->cwindow->update(WUPD_POSITION);
	}

// Disable hourglass
	if(mwindow_global)
		mwindow_global->gui->stop_hourglass();

// Need to restart because brender always stops before render.
	if(mwindow_global)
		mwindow_global->restart_brender();
	if(farm_server) delete farm_server;
	delete command;
// Must delete packages after server
	delete packages;
	in_progress = 0;
	completion->unlock();
	return result;
}

void Render::create_filename(char *path, 
	char *default_path, 
	int current_number,
	int total_digits,
	int number_start)
{
	int i, j, k;
	int len = strlen(default_path);
	char printf_string[BCTEXTLEN];
	int found_number = 0;

	for(i = 0, j = 0; i < number_start; i++, j++)
	{
		printf_string[j] = default_path[i];
	}

// Found the number
	sprintf(&printf_string[j], "%%0%dd", total_digits);
	j = strlen(printf_string);
	i += total_digits;

// Copy remainder of string
	for( ; i < len; i++, j++)
	{
		printf_string[j] = default_path[i];
	}
	printf_string[j] = 0;
// Print the printf argument to the path
	sprintf(path, printf_string, current_number);
}

void Render::get_starting_number(char *path, 
	int &current_number,
	int &number_start, 
	int &total_digits,
	int min_digits)
{
	int i, j;
	int len = strlen(path);
	char number_text[BCTEXTLEN];
	char *ptr = 0;
	char *ptr2 = 0;

	total_digits = 0;
	number_start = 0;

// Search for last /
	ptr2 = strrchr(path, '/');

// Search for first 0 after last /.
	if(ptr2)
		ptr = strchr(ptr2, '0');

	if(ptr && isdigit(*ptr))
	{
		number_start = ptr - path;

// Store the first number
		char *ptr2 = number_text;
		while(isdigit(*ptr))
			*ptr2++ = *ptr++;
		*ptr2++ = 0;
		current_number = atol(number_text);
		total_digits = strlen(number_text);
	}

// No number found or number not long enough
	if(total_digits < min_digits)
	{
		current_number = 1;
		number_start = len;
		total_digits = min_digits;
	}
}

void Render::load_defaults(Asset *asset)
{
	char string[BCTEXTLEN];
	char *p;

	strategy = mwindow_global->defaults->get("RENDER_STRATEGY", 0);
	load_mode = mwindow_global->defaults->get("RENDER_LOADMODE", LOADMODE_NEW_TRACKS);
	range_type = mwindow_global->defaults->get("RENDER_RANGE_TYPE", RANGE_PROJECT);

	strcpy(string, RENDERCONFIG_DFLT);
	mwindow_global->defaults->get("RENDERPROFILE", string);
	edlsession->configuration_path(RENDERCONFIG_DIR,
		asset->renderprofile_path);
	p = &asset->renderprofile_path[strlen(asset->renderprofile_path)];
	*p++ = '/';
	strcpy(p, string);

	asset->load_defaults(mwindow_global->defaults, "RENDER_", ASSET_ALL);
	load_profile(asset);
}

void Render::load_profile(Asset *asset)
{
	FileXML file;
	Paramlist *dflts = 0;
	Param *par;
	char path[BCTEXTLEN];

	if(asset->render_parameters)
	{
		delete asset->render_parameters;
		asset->render_parameters = 0;
	}

	if(!file.read_from_file(asset->profile_config_path("ProfilData.xml", path), 1) && !file.read_tag())
	{
		dflts = new Paramlist();
		dflts->load_list(&file);

		strategy = dflts->get("strategy", strategy);
		load_mode = dflts->get("loadmode", load_mode);
		range_type = dflts->get("renderrange", range_type);
		asset->load_defaults(dflts, ASSET_ALL);
		asset->render_parameters = dflts;
	}
}

void Render::save_defaults(Asset *asset)
{
	Paramlist params("ProfilData");
	Param *pp;
	FileXML file;
	char path[BCTEXTLEN];

	params.append_param("strategy", strategy);
	params.append_param("loadmode", load_mode);
	params.append_param("renderrange", range_type);
	asset->save_defaults(&params, ASSET_ALL);
	asset->save_render_options();

	if(asset->render_parameters)
		params.remove_equiv(asset->render_parameters);
	else
		asset->render_parameters = new Paramlist("ProfilData");

	if(params.total() > 0)
	{
		for(pp = params.first; pp; pp = pp->next)
			asset->render_parameters->set(pp);
		asset->render_parameters->save_list(&file);
		file.write_to_file(asset->profile_config_path("ProfilData.xml", path));
	}
	// Delete old defaults
	mwindow_global->defaults->delete_key("RENDER_STRATEGY");
	mwindow_global->defaults->delete_key("RENDER_LOADMODE");
	mwindow_global->defaults->delete_key("RENDER_RANGE_TYPE");
}

#define WIDTH 410
#define HEIGHT 500

RenderWindow::RenderWindow(Render *render, Asset *asset)
 : BC_Window(MWindow::create_title(N_("Render")),
	mwindow_global->gui->get_root_w(0, 1) / 2 - WIDTH / 2,
	mwindow_global->gui->get_root_h(1) / 2 - HEIGHT / 2,
	WIDTH, 
	HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	int x = 5, y = 5;

	this->render = render;
	this->asset = asset;

	set_icon(mwindow_global->get_window_icon());
	add_subwindow(new BC_Title(x, 
		y, 
		(char*)((render->strategy & RENDER_FILE_PER_LABEL) ?
			_("Select the first file to render to:") : 
			_("Select a file to render to:"))));
	y += 25;

	format_tools = new FormatTools(mwindow_global,
		this,
		asset,
		x,
		y,
		SUPPORTS_AUDIO | SUPPORTS_VIDEO | SUPPORTS_LIBPARA,
		SUPPORTS_AUDIO | SUPPORTS_VIDEO,
		SUPPORTS_VIDEO,
		&render->strategy);

	add_subwindow(new BC_Title(x, 
		y, _("Render range:")));

	x += 110;
	add_subwindow(rangeproject = new RenderRangeProject(this, 
		render->range_type == RANGE_PROJECT, 
		x, 
		y));
	y += 20;
	add_subwindow(rangeselection = new RenderRangeSelection(this, 
		render->range_type == RANGE_SELECTION, 
		x, 
		y));
	y += 20;
	add_subwindow(rangeinout = new RenderRangeInOut(this, 
		render->range_type == RANGE_INOUT, 
		x, 
		y));
	y += 30;
	x = 5;

	renderprofile = new RenderProfile(mwindow_global, this, x, y);

	y += 70;
	loadmode = new LoadMode(this, x, y, &render->load_mode, 1);

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
}

RenderWindow::~RenderWindow()
{
	delete format_tools;
	delete loadmode;
}

void RenderWindow::load_profile()
{
	render->load_profile(asset);
	update_range_type(render->range_type);
	format_tools->update(asset, &render->strategy);
}

void RenderWindow::update_range_type(int range_type)
{
	render->range_type = range_type;
	rangeproject->update(range_type == RANGE_PROJECT);
	rangeselection->update(range_type == RANGE_SELECTION);
	rangeinout->update(range_type == RANGE_INOUT);
}


RenderRangeProject::RenderRangeProject(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Project"))
{
	this->rwindow = rwindow;
}
int RenderRangeProject::handle_event()
{
	rwindow->update_range_type(RANGE_PROJECT);
	return 1;
}

RenderRangeSelection::RenderRangeSelection(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Selection"))
{
	this->rwindow = rwindow;
}

int RenderRangeSelection::handle_event()
{
	rwindow->update_range_type(RANGE_SELECTION);
	return 1;
}


RenderRangeInOut::RenderRangeInOut(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("In/Out Points"))
{
	this->rwindow = rwindow;
}

int RenderRangeInOut::handle_event()
{
	rwindow->update_range_type(RANGE_INOUT);
	return 1;
}
