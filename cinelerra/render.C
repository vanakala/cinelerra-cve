#include "arender.h"
#include "asset.h"
#include "auto.h"
#include "batchrender.h"
#include "bcprogressbox.h"
#include "cache.h"
#include "clip.h"
#include "compresspopup.h"
#include "condition.h"
#include "confirmsave.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "defaults.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "formatcheck.h"
#include "formatpopup.h"
#include "formattools.h"
#include "labels.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "module.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "preferences.h"
#include "quicktime.h"
#include "renderfarm.h"
#include "render.h"
#include "statusbar.h"
#include "timebar.h"
#include "tracks.h"
#include "transportque.h"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"
#include "vrender.h"

#include <ctype.h>
#include <string.h>



RenderItem::RenderItem(MWindow *mwindow)
 : BC_MenuItem(_("Render..."), "Shift+R", 'R')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int RenderItem::handle_event() 
{
	mwindow->render->start_interactive();
	return 1;
}










RenderProgress::RenderProgress(MWindow *mwindow, Render *render)
 : Thread()
{
	this->mwindow = mwindow;
	this->render = render;
	last_value = 0;
	Thread::set_synchronous(1);
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



MainPackageRenderer::~MainPackageRenderer()
{
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

void MainPackageRenderer::set_progress(int64_t value)
{
	render->counter_lock->lock();
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

			printf("%d%% ETA: %s      \r", (int)(100 * 
				(float)render->total_rendered / 
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












Render::Render(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	if(mwindow) plugindb = mwindow->plugindb;
	in_progress = 0;
	progress = 0;
	preferences = 0;
	elapsed_time = 0.0;
	package_lock = new Mutex("Render::package_lock");
	counter_lock = new Mutex("Render::counter_lock");
	completion = new Condition(0, "Render::completion");
	progress_timer = new Timer;
}

Render::~Render()
{
	delete package_lock;
	delete counter_lock;
	delete completion;
	if(preferences) delete preferences;
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
		ErrorBox error_box(PROGRAM_NAME ": Error",
			mwindow->gui->get_abs_cursor_x(),
			mwindow->gui->get_abs_cursor_y());
		error_box.create_objects("Already rendering");
		error_box.run_window();
	}
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
	{
		ErrorBox error_box(PROGRAM_NAME ": Error",
			mwindow->gui->get_abs_cursor_x(),
			mwindow->gui->get_abs_cursor_y());
		error_box.create_objects("Already rendering");
		error_box.run_window();
	}
}

void Render::start_batches(ArrayList<BatchRenderJob*> *jobs,
	Defaults *boot_defaults,
	Preferences *preferences,
	ArrayList<PluginServer*> *plugindb)
{
	mode = Render::BATCH;
	batch_cancelled = 0;
	this->jobs = jobs;
	this->preferences = preferences;
	this->plugindb = plugindb;

	completion->reset();
	run();
	this->preferences = 0;
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
	int format_error;


	result = 0;

	if(mode == Render::INTERACTIVE)
	{
// Fix the asset for rendering
		Asset *asset = new Asset;
		load_defaults(asset);
		check_asset(mwindow->edl, *asset);

// Get format from user
		do
		{
			format_error = 0;
			result = 0;

			{
				RenderWindow window(mwindow, this, asset);
				window.create_objects();
				result = window.run_window();
			}

			if(!result)
			{
// Check the asset format for errors.
				FormatCheck format_check(asset);
				format_error = format_check.check_format();
			}
		}while(format_error && !result);

		save_defaults(asset);
		mwindow->save_defaults();

		if(!result) render(1, asset, mwindow->edl, strategy);

		delete asset;
	}
	else
	if(mode == Render::BATCH)
	{
		for(int i = 0; i < jobs->total && !result; i++)
		{
			BatchRenderJob *job = jobs->values[i];
			if(job->enabled)
			{
				if(mwindow)
				{
					mwindow->batch_render->update_active(i);
				}
				else
				{
					printf("Render::run: %s\n", job->edl_path);
				}


				FileXML *file = new FileXML;
				EDL *edl = new EDL;
				edl->create_objects();
				file->read_from_file(job->edl_path);
				if(!plugindb && mwindow)
					plugindb = mwindow->plugindb;
				edl->load_xml(plugindb, file, LOAD_ALL);

				render(0, job->asset, edl, job->strategy);

				delete edl;
				delete file;
				if(!result)
				{
					if(mwindow)
						mwindow->batch_render->update_done(i, 1, elapsed_time);
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
					if(mwindow)
						mwindow->batch_render->update_active(-1);
					else
						printf("Render::run: failed\n");
				}
			}
		}

		if(mwindow)
		{
			mwindow->batch_render->update_active(-1);
			mwindow->batch_render->update_done(-1, 0, 0);
		}
	}
}



int Render::check_asset(EDL *edl, Asset &asset)
{
	if(asset.video_data && 
		edl->tracks->playable_video_tracks() &&
		File::supports_video(asset.format))
	{
		asset.video_data = 1;
		asset.layers = 1;
		asset.width = edl->session->output_w;
		asset.height = edl->session->output_h;
	}
	else
	{
		asset.video_data = 0;
		asset.layers = 0;
	}

	if(asset.audio_data && 
		edl->tracks->playable_audio_tracks() &&
		File::supports_audio(asset.format))
	{
		asset.audio_data = 1;
		asset.channels = edl->session->audio_channels;
		if(asset.format == FILE_MOV) asset.byte_order = 0;
	}
	else
	{
		asset.audio_data = 0;
		asset.channels = 0;
	}
//printf("Render::check_asset 3\n");
}

int Render::fix_strategy(int strategy, int use_renderfarm)
{
	if(use_renderfarm)
	{
		if(strategy == FILE_PER_LABEL)
			strategy = FILE_PER_LABEL_FARM;
		else
		if(strategy == SINGLE_PASS)
			strategy = SINGLE_PASS_FARM;
	}
	else
	{
		if(strategy == FILE_PER_LABEL_FARM)
			strategy = FILE_PER_LABEL;
		else
		if(strategy == SINGLE_PASS_FARM)
			strategy = SINGLE_PASS;
	}
	return strategy;
}

void Render::start_progress()
{
	char filename[BCTEXTLEN];
	char string[BCTEXTLEN];
	FileSystem fs;

	progress_max = Units::to_int64(default_asset->sample_rate * 
			(total_end - total_start)) +
		Units::to_int64(preferences->render_preroll * 
			packages->total_allocated * 
			default_asset->sample_rate);
	progress_timer->update();
	last_eta = 0;
	if(mwindow)
	{
// Generate the progress box
		fs.extract_name(filename, default_asset->path);
		sprintf(string, _("Rendering %s..."), filename);

// Don't bother with the filename since renderfarm defeats the meaning
		progress = mwindow->mainprogress->start_progress(_("Rendering..."), 
			progress_max);
		render_progress = new RenderProgress(mwindow, this);
		render_progress->start();
	}
}

void Render::stop_progress()
{
	if(progress)
	{
		char string[BCTEXTLEN], string2[BCTEXTLEN];
		delete render_progress;
		progress->get_time(string);
		elapsed_time = progress->get_time();
		progress->stop_progress();
		delete progress;

		sprintf(string2, _("Rendering took %s"), string);
		mwindow->gui->lock_window();
		mwindow->gui->show_message(string2, BLACK);
		mwindow->gui->unlock_window();
	}
	progress = 0;
}



int Render::render(int test_overwrite, 
	Asset *asset,
	EDL *edl,
	int strategy)
{
	char string[BCTEXTLEN];
// Total length in seconds
	double total_length;
	int last_audio_buffer;
	RenderFarmServer *farm_server = 0;
	FileSystem fs;
	int total_digits;      // Total number of digits including padding the user specified.
	int number_start;      // Character in the filename path at which the number begins
	int current_number;    // The number the being injected into the filename.
// Pointer from file
// (VFrame*)(VFrame array [])(Channel [])
	VFrame ***video_output;
// Pointer to output buffers
	VFrame *video_output_ptr[MAX_CHANNELS];
	double *audio_output_ptr[MAX_CHANNELS];
	int done = 0;
	in_progress = 1;


	this->default_asset = asset;
	progress = 0;
	result = 0;

	if(mwindow)
	{
		if(!preferences)
			preferences = new Preferences;

		preferences->copy_from(mwindow->preferences);
	}


// Create rendering command
	command = new TransportCommand;
	command->command = NORMAL_FWD;
	command->get_edl()->copy_all(edl);
	command->change_type = CHANGE_ALL;
// Get highlighted playback range
	command->set_playback_range();
// Adjust playback range with in/out points
	command->adjust_playback_range();
	packages = new PackageDispatcher;


// Configure preview monitor
	VideoOutConfig vconfig(PLAYBACK_LOCALHOST, 0);
	PlaybackConfig *playback_config = new PlaybackConfig(PLAYBACK_LOCALHOST, 0);
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		vconfig.do_channel[i] = (i < command->get_edl()->session->video_channels);
		playback_config->vconfig->do_channel[i] = (i < command->get_edl()->session->video_channels);
		playback_config->aconfig->do_channel[i] = (i < command->get_edl()->session->audio_channels);
	}

// Create caches
	audio_cache = new CICache(command->get_edl(), preferences, plugindb);
	video_cache = new CICache(command->get_edl(), preferences, plugindb);

	default_asset->frame_rate = command->get_edl()->session->frame_rate;
	default_asset->sample_rate = command->get_edl()->session->sample_rate;

// Conform asset to EDL.
	check_asset(command->get_edl(), *default_asset);

	if(!result)
	{
// Get total range to render
		total_start = command->start_position;
		total_end = command->end_position;
		total_length = total_end - total_start;

// Nothing to render
		if(EQUIV(total_length, 0))
		{
			result = 1;
		}
	}







// Generate packages
	if(!result)
	{
// Stop background rendering
		if(mwindow) mwindow->stop_brender();

		fs.complete_path(default_asset->path);
		strategy = Render::fix_strategy(strategy, preferences->use_renderfarm);

		result = packages->create_packages(mwindow,
			command->get_edl(),
			preferences,
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
		if(mwindow)
		{
			mwindow->gui->lock_window();
			mwindow->gui->show_message(_("Starting render farm"), BLACK);
			mwindow->gui->unlock_window();
		}
		else
		{
			printf("Render::render: starting render farm\n");
		}

		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server = new RenderFarmServer(plugindb, 
				packages,
				preferences, 
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
				if(mwindow)
				{
					mwindow->gui->lock_window();
					mwindow->gui->show_message(_("Failed to start render farm"), BLACK);
					mwindow->gui->unlock_window();
				}
				else
				{
					printf("Render::render: Failed to start render farm\n");
				}
			}
		}
	}




// Perform local rendering


	if(!result)
	{
		start_progress();
	
	


		MainPackageRenderer package_renderer(this);
		package_renderer.initialize(mwindow,
				command->get_edl(),   // Copy of master EDL
				preferences, 
				default_asset,
				plugindb);







		while(!result)
		{
// Get unfinished job
			RenderPackage *package;

			if(strategy == SINGLE_PASS_FARM)
			{
				package = packages->get_package(frames_per_second, -1, 1);
			}
			else
			{
				package = packages->get_package(0, -1, 1);
			}

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

			frames_per_second = (double)(package->video_end - package->video_start) / 
				(double)(timer.get_difference() / 1000);


		} // file_number



//printf("Render::run: Session finished.\n");


//printf("Render::render 80\n");



		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server->wait_clients();
		}

//printf("Render::render 90\n");

// Notify of error
		if(result && 
			(!progress || !progress->is_cancelled()) &&
			!batch_cancelled)
		{
			if(mwindow)
			{
				ErrorBox error_box(PROGRAM_NAME ": Error",
					mwindow->gui->get_abs_cursor_x(),
					mwindow->gui->get_abs_cursor_y());
				error_box.create_objects(_("Error rendering data."));
				error_box.run_window();
			}
			else
			{
				printf("Render::render: Error rendering data\n");
			}
		}

// Delete the progress box
		stop_progress();

//printf("Render::render 100\n");




	} // !result


// Paste all packages into timeline if desired

	if(!result && load_mode != LOAD_NOTHING && mwindow)
	{
		mwindow->gui->lock_window();


		mwindow->undo->update_undo_before(_("render"), LOAD_ALL);


		ArrayList<Asset*> *assets = packages->get_asset_list();
		if(load_mode == LOAD_PASTE)
			mwindow->clear(0);
		mwindow->load_assets(assets, 
			-1, 
			load_mode,
			0,
			0,
			mwindow->edl->session->labels_follow_edits,
			mwindow->edl->session->plugins_follow_edits);
		assets->remove_all_objects();
		delete assets;


		mwindow->save_backup();
		mwindow->undo->update_undo_after();
		mwindow->update_plugin_guis();
		mwindow->gui->update(1, 
			2,
			1,
			1,
			1,
			1,
			0);
		mwindow->sync_parameters(CHANGE_ALL);
		mwindow->gui->unlock_window();
	}

//printf("Render::render 110\n");
// Need to restart because brender always stops before render.
	if(mwindow)
		mwindow->restart_brender();
	if(farm_server) delete farm_server;
	delete command;
	delete playback_config;
	delete audio_cache;
	delete video_cache;
// Must delete packages after server
	delete packages;
	in_progress = 0;
	completion->unlock();
//printf("Render::render 120\n");

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







int Render::load_defaults(Asset *asset)
{
	strategy = mwindow->defaults->get("RENDER_STRATEGY", SINGLE_PASS);
	load_mode = mwindow->defaults->get("RENDER_LOADMODE", LOAD_NEW_TRACKS);


	asset->load_defaults(mwindow->defaults, 
		"RENDER_", 
		1,
		1,
		1,
		1,
		1);


	return 0;
}

int Render::save_defaults(Asset *asset)
{
	mwindow->defaults->update("RENDER_STRATEGY", strategy);
	mwindow->defaults->update("RENDER_LOADMODE", load_mode);




	asset->save_defaults(mwindow->defaults, 
		"RENDER_",
		1,
		1,
		1,
		1,
		1);

	return 0;
}




#define WIDTH 410
#define HEIGHT 360


RenderWindow::RenderWindow(MWindow *mwindow, Render *render, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Render", 
 	mwindow->gui->get_root_w() / 2 - WIDTH / 2,
	mwindow->gui->get_root_h() / 2 - HEIGHT / 2,
 	WIDTH, 
	HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->render = render;
	this->asset = asset;
}

RenderWindow::~RenderWindow()
{
	delete format_tools;
	delete loadmode;
}



int RenderWindow::create_objects()
{
	int x = 5, y = 5;
	add_subwindow(new BC_Title(x, 
		y, 
		(char*)((render->strategy == FILE_PER_LABEL || 
				render->strategy == FILE_PER_LABEL_FARM) ? 
			_("Select the first file to render to:") : 
			_("Select a file to render to:"))));
	y += 25;

	format_tools = new FormatTools(mwindow,
					this, 
					asset);
	format_tools->create_objects(x, 
		y, 
		1, 
		1, 
		1, 
		1, 
		0,
		1,
		0,
		0,
		&render->strategy,
		0);

	loadmode = new LoadMode(mwindow, this, x, y, &render->load_mode, 1);
	loadmode->create_objects();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	return 0;
}
