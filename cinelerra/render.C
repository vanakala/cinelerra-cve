#include "arender.h"
#include "assets.h"
#include "auto.h"
#include "bcprogressbox.h"
#include "cache.h"
#include "clip.h"
#include "compresspopup.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "defaults.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "formatcheck.h"
#include "formatpopup.h"
#include "formattools.h"
#include "labels.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "module.h"
#include "mutex.h"
#include "neworappend.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "preferences.h"
#include "statusbar.h"
#include "timebar.h"
#include "tracks.h"
#include "transportque.h"
#include "quicktime.h"
#include "vrender.h"
#include "render.h"
#include "renderfarm.h"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"

#include <ctype.h>
#include <string.h>


RenderItem::RenderItem(MWindow *mwindow)
 : BC_MenuItem("Render...", "Shift+R", 'R')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int RenderItem::handle_event() 
{
	mwindow->render_single();
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

void MainPackageRenderer::set_progress(long value)
{
	render->counter_lock->lock();
	render->total_rendered += audio_read_length;
	render->counter_lock->unlock();
}

int MainPackageRenderer::progress_cancelled()
{
	return render->progress->is_cancelled();
}







Render::Render(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	in_progress = 0;
	package_lock = new Mutex;
	counter_lock = new Mutex;
}

Render::~Render()
{
	delete package_lock;
	delete counter_lock;
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


void Render::start_progress()
{
	char filename[BCTEXTLEN];
	char string[BCTEXTLEN];
	FileSystem fs;
// Generate the progress box
	fs.extract_name(filename, default_asset->path);
	sprintf(string, "Rendering %s...", filename);

// Don't bother with the filename since renderfarm defeats the meaning
	progress = mwindow->mainprogress->start_progress("Rendering...", 
		Units::to_long(default_asset->sample_rate * 
			(total_end - total_start)) +
		Units::to_long(preferences->render_preroll * 
			packages->total_allocated * 
			default_asset->sample_rate));
	render_progress = new RenderProgress(mwindow, this);
	render_progress->start();
}

void Render::stop_progress()
{
	if(progress)
	{
		char string[BCTEXTLEN], string2[BCTEXTLEN];
		delete render_progress;
		progress->get_time(string);
		progress->stop_progress();
		delete progress;

		sprintf(string2, "Rendering took %s", string);
		mwindow->gui->lock_window();
		mwindow->gui->show_message(string2, BLACK);
		mwindow->gui->unlock_window();
	}
	progress = 0;
}


void Render::run()
{
	int format_error;
	char string[BCTEXTLEN];
// Total length in seconds
	double total_length;
	int last_audio_buffer;
	RenderFarmServer *farm_server = 0;
	FileSystem fs;
	int total_digits;       // Total number of digits including padding the user specified.
	int number_start;       // Character in the filename path at which the number begins
	int current_number;    // The number the being injected into the filename.
// Pointer from file
// (VFrame*)(VFrame array [])(Channel [])
	VFrame ***video_output;
// Pointer to output buffers
	VFrame *video_output_ptr[MAX_CHANNELS];
	double *audio_output_ptr[MAX_CHANNELS];
	int done = 0;


	progress = 0;
	result = 0;

// Fix the EDL for rendering
	default_asset = new Asset;
	preferences = new Preferences;
	*preferences = *mwindow->preferences;
	command = new TransportCommand;
	command->command = NORMAL_FWD;
	*command->get_edl() = *mwindow->edl;
	command->change_type = CHANGE_ALL;
	command->set_playback_range(mwindow->edl);



// Adjust playback range with in/out points
	if(mwindow->edl->local_session->in_point >= 0 ||
		mwindow->edl->local_session->out_point >= 0)
	{
		if(mwindow->edl->local_session->in_point >= 0)
			command->start_position = mwindow->edl->local_session->in_point;
		else
			command->start_position = 0;

		if(mwindow->edl->local_session->out_point >= 0)
			command->end_position = mwindow->edl->local_session->out_point;
		else
			command->end_position = mwindow->edl->tracks->total_playable_length();
	}


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
	audio_cache = new CICache(command->get_edl(), preferences, mwindow->plugindb);
	video_cache = new CICache(command->get_edl(), preferences, mwindow->plugindb);

	load_defaults(default_asset);



	default_asset->frame_rate = command->get_edl()->session->frame_rate;
	default_asset->sample_rate = command->get_edl()->session->sample_rate;
	check_asset(command->get_edl(), *default_asset);










// Get format from user
	do
	{
		format_error = 0;
		result = 0;

		{
			RenderWindow window(mwindow, this, default_asset);
			window.create_objects();
			result = window.run_window();
		}

		if(!result)
		{
// Check the asset format for errors.
			FormatCheck format_check(default_asset);
			format_error = format_check.check_format();
		}
	}while(format_error && !result);

	check_asset(command->get_edl(), *default_asset);
//printf("%d %d\n", default_asset->video_data, default_asset->audio_data);
	save_defaults(default_asset);
	mwindow->save_defaults();

//printf("Render 2\n");
//default_asset->dump();
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





//printf("Render 3\n");





// Generate packages
	if(!result)
	{
// Stop background rendering
		mwindow->stop_brender();

		fs.complete_path(default_asset->path);
		if(preferences->use_renderfarm)
		{
			if(strategy == SINGLE_PASS)
				strategy = SINGLE_PASS_FARM;
			else
			if(strategy == FILE_PER_LABEL)
				strategy = FILE_PER_LABEL_FARM;
		}
		else
		{
			if(strategy == SINGLE_PASS_FARM)
				strategy = SINGLE_PASS;
			else
			if(strategy == FILE_PER_LABEL_FARM)
				strategy = FILE_PER_LABEL;
		}

		result = packages->create_packages(mwindow,
			command->get_edl(),
			preferences,
			strategy, 
			default_asset, 
			total_start, 
			total_end);
	}



//printf("Render 4\n");







	done = 0;
	total_rendered = 0;
	frames_per_second = 0;

	if(!result)
	{
// Start dispatching external jobs
		mwindow->gui->lock_window();
		mwindow->gui->show_message("Starting render farm", BLACK);
		mwindow->gui->unlock_window();
		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server = new RenderFarmServer(mwindow, 
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
				mwindow->gui->lock_window();
				mwindow->gui->show_message("Failed to start render farm", BLACK);
				mwindow->gui->unlock_window();
			}
		}
	}


//printf("Render 5\n");


// Perform local rendering


	if(!result)
	{
		start_progress();
	
	


//printf("Render 6\n");
		MainPackageRenderer package_renderer(this);
//printf("Render 7\n");
		package_renderer.initialize(mwindow,
				command->get_edl(),   // Copy of master EDL
				preferences, 
				default_asset,
				mwindow->plugindb);


//printf("Render 8\n");





		while(!result)
		{
// Get unfinished job
			RenderPackage *package;

			if(strategy == SINGLE_PASS_FARM)
			{
//printf("Render 9\n");
				package = packages->get_package(frames_per_second, -1, 1);
//printf("Render 10\n");
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


//printf("Render 11\n");

			Timer timer;
			timer.update();

			if(package_renderer.render_package(package))
				result = 1;
// Result is also set directly by the RenderFarm.
//printf("Render 12\n");

			frames_per_second = (double)(package->video_end - package->video_start) / 
				(double)(timer.get_difference() / 1000);

//printf("Render 13\n");

		} // file_number

//printf("Render::run: Session finished.\n");





		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server->wait_clients();
		}


// Notify of error
		if(result && 
			!progress->is_cancelled())
		{
			ErrorBox error_box(PROGRAM_NAME ": Error",
				mwindow->gui->get_abs_cursor_x(),
				mwindow->gui->get_abs_cursor_y());
			error_box.create_objects("Error rendering data.");
			error_box.run_window();
		}

// Delete the progress box
		stop_progress();





	} // !result

//	mwindow->gui->lock_window();
//	if(!result) mwindow->gui->statusbar->default_message();
//	mwindow->gui->unlock_window();

// Paste all packages into timeline if desired

	if(!result && load_mode != LOAD_NOTHING)
	{             // paste it in
		mwindow->gui->lock_window();


		mwindow->undo->update_undo_before("render", LOAD_EDITS | LOAD_TIMEBAR);


		ArrayList<Asset*> *assets = packages->get_asset_list();
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
			0,
			1,
			0);
		mwindow->sync_parameters(CHANGE_ALL);
		mwindow->gui->unlock_window();
	}

// Need to restart because brender always stops before render.
	mwindow->restart_brender();
	if(farm_server) delete farm_server;
	delete preferences;
	delete command;
	delete playback_config;
	delete audio_cache;
	delete video_cache;
// Must delete packages after server
	delete packages;
	delete default_asset;
	in_progress = 0;
}

int Render::test_existence(MWindow *mwindow, Asset *asset)
{
	int result = 0;
	NewOrAppend window(mwindow);
// Append is not compatible with resample plugin.
	result = window.test_file(asset);
	switch(result)
	{
		case 0:
//			mwindow->purge_asset(asset->path);
//			remove(asset->path);              // overwrite
//			append_to_file = 0;
			break;

		case 1:
			result = 1;                      // cancel
			break;

		case 2:
//					append_to_file = 1;              // append
			result = 0;       // reset result;
			break;
	}
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
	char *ptr = path;

	total_digits = 0;
	number_start = 0;

// Search for first 0.
	ptr = strchr(ptr, '0');

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
	mwindow->defaults->get("RENDER_PATH", asset->path);
//	to_tracks = mwindow->defaults->get("RENDER_TO_TRACKS", 1);
	strategy = mwindow->defaults->get("RENDER_STRATEGY", SINGLE_PASS);
	load_mode = mwindow->defaults->get("RENDER_LOADMODE", LOAD_NEW_TRACKS);
	asset->audio_data = mwindow->defaults->get("RENDER_AUDIO", 1);
	asset->video_data = mwindow->defaults->get("RENDER_VIDEO", 1);
//printf("Render::load_defaults 1 %d %d\n", asset->audio_data, asset->video_data);

	char string[BCTEXTLEN];
	sprintf(string, "WAV");
	mwindow->defaults->get("RENDER_FORMAT", string);
	sprintf(asset->vcodec, QUICKTIME_YUV2);
	mwindow->defaults->get("RENDER_COMPRESSION", asset->vcodec);

	asset->format = File::strtoformat(mwindow->plugindb, string);

	asset->bits = mwindow->defaults->get("RENDER_BITS", 16);
	asset->dither = mwindow->defaults->get("RENDER_DITHER", 0);
	asset->signed_ = mwindow->defaults->get("RENDER_SIGNED", 1);
	asset->byte_order = mwindow->defaults->get("RENDER_BYTE_ORDER", 1);




	asset->load_defaults(mwindow->defaults);
	mwindow->defaults->get("RENDER_AUDIO_CODEC", asset->acodec);
	mwindow->defaults->get("RENDER_VIDEO_CODEC", asset->vcodec);
//printf("Render 1 %d\n", default_asset->divx_fix_bitrate);

	return 0;
}

int Render::save_defaults(Asset *asset)
{
//printf("Render::save_defaults 1\n");
	mwindow->defaults->update("RENDER_PATH", asset->path);
//	mwindow->defaults->update("RENDER_TO_TRACKS", to_tracks);
	mwindow->defaults->update("RENDER_STRATEGY", strategy);
	mwindow->defaults->update("RENDER_LOADMODE", load_mode);
	mwindow->defaults->update("RENDER_AUDIO", asset->audio_data);
	mwindow->defaults->update("RENDER_VIDEO", asset->video_data);
//printf("Render::save_defaults 2\n");

	mwindow->defaults->update("RENDER_FORMAT", File::formattostr(mwindow->plugindb, asset->format));
//printf("Render::save_defaults 3\n");

	mwindow->defaults->update("RENDER_VCODEC", asset->vcodec);
	mwindow->defaults->update("RENDER_BITS", asset->bits);
	mwindow->defaults->update("RENDER_DITHER", asset->dither);
	mwindow->defaults->update("RENDER_SIGNED", asset->signed_);
	mwindow->defaults->update("RENDER_BYTE_ORDER", asset->byte_order);

	asset->save_defaults(mwindow->defaults);
	mwindow->defaults->update("RENDER_AUDIO_CODEC", asset->acodec);
	mwindow->defaults->update("RENDER_VIDEO_CODEC", asset->vcodec);
//printf("Render::save_defaults 4\n");
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
		(char*)((render->strategy == FILE_PER_LABEL || render->strategy == FILE_PER_LABEL_FARM) ? 
			"Select the first file to render to:" : 
			"Select a file to render to:")));
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
