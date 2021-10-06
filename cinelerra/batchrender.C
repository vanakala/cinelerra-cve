// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "batchrender.h"
#include "bcsignals.h"
#include "confirmsave.h"
#include "bchash.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bctitle.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "filesystem.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "paramlist.h"
#include "plugindb.h"
#include "preferences.h"
#include "render.h"
#include "renderprofiles.h"
#include "theme.h"
#include "transportcommand.h"
#include "vframe.h"

#include <string.h>
#include <sys/types.h>
#include <dirent.h>


static const char *list_titles[] = 
{
	N_("Enabled"), 
	N_("Output"),
	N_("EDL"),
	N_("Elapsed")
};

static int list_widths[] =
{
	50,
	100,
	200,
	100
};


BatchRenderMenuItem::BatchRenderMenuItem()
 : BC_MenuItem(_("Batch Render..."), "Shift-B", 'B')
{
	set_shift(1); 
}

int BatchRenderMenuItem::handle_event()
{
	mwindow_global->batch_render->start();
	return 1;
}


BatchRenderJob::BatchRenderJob(int jobnum)
{
	this->jobnum = jobnum;
	asset = new Asset;
	edl_path[0] = 0;
	strategy = 0;
	enabled = 1;
	elapsed = 0;
}

BatchRenderJob::~BatchRenderJob()
{
	delete asset;
}

void BatchRenderJob::copy_from(BatchRenderJob *src)
{
	asset->copy_from(src->asset, 0);
	strcpy(edl_path, src->edl_path);
	strategy = src->strategy;
	enabled = src->enabled;
	elapsed = 0;
}

void BatchRenderJob::load(const char *path)
{
	FileXML file;
	Paramlist *dflts = 0;
	char str[BCTEXTLEN];

	strcpy(asset->renderprofile_path, path);
	edl_path[0] = 0;

	if(!file.read_from_file(asset->profile_config_path("ProfilData.xml", str), 1) &&
		!file.read_tag())
	{
		dflts = new Paramlist("ProfilData");
		dflts->load_list(&file);
		strategy = dflts->get("strategy", strategy);
		enabled = dflts->get("enabled", enabled);
		elapsed = dflts->get("elapsed", elapsed);
		dflts->get("EDL_path", edl_path);
		asset->load_defaults(dflts, ASSET_ALL);
		delete asset->render_parameters;
		asset->render_parameters = dflts;
	}
	fix_strategy();
}

void BatchRenderJob::save()
{
	Paramlist params("ProfilData");
	Param *pp;
	FileXML file;
	char path[BCTEXTLEN];

	params.append_param("strategy", strategy);
	params.append_param("enabled", enabled);
	params.append_param("elapsed", elapsed);
	params.append_param("EDL_path", edl_path);
	asset->save_defaults(&params, ASSET_ALL);

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
}

void BatchRenderJob::fix_strategy()
{
	strategy = Render::fix_strategy(strategy, render_preferences->use_renderfarm);
}

void BatchRenderJob::dump(int indent)
{
	printf("%*sBatchRenderJob %p dump:\n", indent, "", this);
	indent++;
	printf("%*sstrategy %d enabled %d jobnum %d elapsed %.2f\n", indent, "",
		strategy, enabled, jobnum, elapsed);
	printf("%*sedl_path: '%s'\n", indent, "", edl_path);
	asset->dump(indent + 2);
}

BatchRenderThread::BatchRenderThread()
 : BC_DialogThread()
{
	current_job = -1;
	rendering_job = -1;
	is_rendering = 0;
	boot_defaults = 0;
	render = 0;
	gui = 0;
	profile_end = 0;
	profile_path[0] = 0;
}

BatchRenderThread::~BatchRenderThread()
{
	delete boot_defaults;
	delete render;
}

void BatchRenderThread::handle_close_event(int result)
{
// Save settings
	save_jobs();
	save_defaults(mwindow_global->defaults);
	jobs.remove_all_objects();
}

BC_Window* BatchRenderThread::new_gui()
{
	current_start = 0.0;
	current_end = 0.0;

	if(!render_preferences)
		render_preferences = new Preferences;
	render_preferences->copy_from(preferences_global);

	load_defaults(mwindow_global->defaults);
	load_jobs();
	gui = new BatchRenderGUI(this,
		mainsession->batchrender_x,
		mainsession->batchrender_y,
		mainsession->batchrender_w,
		mainsession->batchrender_h);
	gui->create_list(1);
	gui->change_job();
	return gui;
}

void BatchRenderThread::load_jobs()
{
	DIR *dir;
	FileXML file;
	char *pe, *ne;
	long int jnum;
	struct dirent *entry;
	BatchRenderJob *job;
	struct stat stb;

	int result = 0;

	jobs.remove_all_objects();

	if(dir = opendir(profile_path))
	{
		pe = profile_end;
		*pe++ = '/';

		while(entry = readdir(dir))
		{
			if(entry->d_name[0] == '.' || strlen(entry->d_name) != 4)
				continue;
			jnum = strtol(entry->d_name, &ne, 10);
			strcpy(pe, entry->d_name);

			if(jnum > 0 && *ne == 0 && !stat(profile_path, &stb) &&
					S_ISDIR(stb.st_mode))
			{
				job = merge_jobs(jnum);
				job->load(profile_path);
			}
		}
		if(jobs.total)
			current_job = 0;
		*--pe = 0;
	}
}

BatchRenderJob *BatchRenderThread::merge_jobs(int jnum)
{
	BatchRenderJob *job = 0;
	for(int i = 0; i < jobs.total; i++)
	{
		if(jobs.values[i]->jobnum > jnum)
		{
			jobs.insert(job = new BatchRenderJob(jnum), i);
			break;
		}
	}
	if(!job)
		jobs.append(job = new BatchRenderJob(jnum));
	return job;
}

void BatchRenderThread::save_jobs()
{
	for(int i = 0; i < jobs.total; i++)
		jobs.values[i]->save();
}

void BatchRenderThread::load_defaults(BC_Hash *defaults)
{
	char *p;

	edlsession->configuration_path(RENDERCONFIG_DIR, profile_path);
	RenderProfile::chk_profile_dir(profile_path);
	p = &profile_path[strlen(profile_path)];
	*p++ = '/';
	strcpy(p, RENDERCONFIG_BATCH);
	RenderProfile::chk_profile_dir(profile_path);
	profile_end = &profile_path[strlen(profile_path)];

	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "BATCHRENDER_COLUMN%d", i);
		column_width[i] = defaults->get(string, list_widths[i]);
	}
}

void BatchRenderThread::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		sprintf(string, "BATCHRENDER_COLUMN%d", i);
		defaults->update(string, column_width[i]);
	}
	// Remove old defaults
	defaults->delete_key("DEFAULT_BATCHLOADPATH");
	if(mwindow_global)
		mwindow_global->save_defaults();
	else
		defaults->save();
}

BatchRenderJob *BatchRenderThread::new_job()
{
	char string[32];
	Paramlist *plp;
	BatchRenderJob *curjob;
	BatchRenderJob *job = new BatchRenderJob(jobs.total + 1);

	if(current_job >= 0)
	{
		curjob = get_current_job();
		plp = curjob->asset->render_parameters;
		curjob->asset->render_parameters = 0;
		job->copy_from(get_current_job());
		curjob->asset->render_parameters = plp;
	}
	jobs.append(job);
	current_job = jobs.total - 1;
	sprintf(string, "%04d", job->jobnum);
	job->asset->set_renderprofile(profile_path, string);
	RenderProfile::chk_profile_dir(job->asset->renderprofile_path);

	if(gui)
	{
		gui->create_list(1);
		gui->change_job();
	}
	return job;
}

void BatchRenderThread::delete_job()
{
	if(current_job < jobs.total && current_job >= 0)
	{
		RenderProfile::remove_profiledir(get_current_job()->asset->renderprofile_path);
		jobs.remove_object_number(current_job);
		if(current_job > 0) current_job--;
		gui->create_list(1);
		gui->change_job();
	}
}

BatchRenderJob *BatchRenderThread::get_current_job()
{
	if(current_job >= 0)
		return jobs.values[current_job];
	return new_job();
}

Asset* BatchRenderThread::get_current_asset()
{
	return get_current_job()->asset;
}

char* BatchRenderThread::get_current_edl()
{
	return get_current_job()->edl_path;
}

// Test EDL files for existence
int BatchRenderThread::test_edl_files()
{
	for(int i = 0; i < jobs.total; i++)
	{
		if(jobs.values[i]->enabled)
		{
			FILE *fd = fopen(jobs.values[i]->edl_path, "r");
			if(!fd)
			{
				errorbox(_("EDL '%s' is not found."), basename(jobs.values[i]->edl_path));
				if(mwindow_global)
				{
					gui->new_batch->enable();
					gui->delete_batch->enable();
				}

				is_rendering = 0;
				return 1;
			}
			else
			{
				fclose(fd);
			}
		}
	}
	return 0;
}

void BatchRenderThread::calculate_dest_paths(ArrayList<char*> *paths)
{
	EDL *current_edl;

	for(int i = 0; i < jobs.total; i++)
	{
		BatchRenderJob *job = jobs.values[i];
		if(job->enabled)
		{
			PackageDispatcher *packages = new PackageDispatcher;

			FileXML *file = new FileXML;
			file->read_from_file(job->edl_path);

			current_edl = new EDL(0);
			current_edl->load_xml(file, 0);

// Create test packages
			packages->create_packages(current_edl,
				job->strategy, 
				job->asset, 
				0,
				current_edl->total_length(),
				0);

// Append output paths allocated to total
			packages->get_package_paths(paths);

// Delete package harness
			delete packages;
			delete file;
			delete current_edl;
		}
	}
}

void BatchRenderThread::start_rendering(char *config_path)
{
	char string[BCTEXTLEN];

// Initialize stuff which MWindow does.
	MWindow::init_defaults(boot_defaults, config_path);
	edlsession = new EDLSession();
	edlsession->load_defaults(boot_defaults);
	load_defaults(boot_defaults);
	render_preferences = new Preferences;
	render_preferences->load_defaults(boot_defaults);
	preferences_global = render_preferences;
	plugindb.init_plugins(0);
	strcpy(string, render_preferences->global_plugin_dir);
	strcat(string, "/" FONT_SEARCHPATH);
	BC_Resources::init_fontconfig(string);

	load_jobs();
	save_jobs();
	save_defaults(boot_defaults);

// Test EDL files for existence
	if(test_edl_files()) return;

// Predict all destination paths
	ArrayList<char*> paths;
	calculate_dest_paths(&paths);

	int result = ConfirmSave::test_files(0, &paths);
	paths.remove_all_objects();

// Abort on any existing file because it's so hard to set this up.
	if(result) return;
	render = new Render();
	render->run_batches(&jobs);
}

void BatchRenderThread::start_rendering()
{
	char path[BCTEXTLEN];

	if(is_rendering) return;
	is_rendering = 1;
	path[0] = 0;
	save_jobs();
	save_defaults(mwindow_global->defaults);
	gui->new_batch->disable();
	gui->delete_batch->disable();

// Test EDL files for existence
	if(test_edl_files()) return;

// Predict all destination paths
	ArrayList<char*> paths;
	calculate_dest_paths(&paths);

// Test destination files for overwrite
	int result = ConfirmSave::test_files(mwindow_global, &paths);
	paths.remove_all_objects();

// User cancelled
	if(result)
	{
		is_rendering = 0;
		gui->new_batch->enable();
		gui->delete_batch->enable();
		return;
	}
	mwindow_global->render->start_batches(&jobs);
}

void BatchRenderThread::stop_rendering()
{
	if(!is_rendering) return;
	mwindow_global->render->stop_operation();
	is_rendering = 0;
}

void BatchRenderThread::update_active(int number)
{
	if(number >= 0)
	{
		current_job = number;
		rendering_job = number;
	}
	else
	{
		rendering_job = -1;
		is_rendering = 0;
	}
	gui->create_list(1);
}

void BatchRenderThread::update_done(int number, 
	int create_list, 
	double elapsed_time)
{
	if(number < 0)
	{
		gui->new_batch->enable();
		gui->delete_batch->enable();
	}
	else
	{
		jobs.values[number]->enabled = 0;
		jobs.values[number]->elapsed = elapsed_time;
		if(create_list) gui->create_list(1);
	}
}

void BatchRenderThread::move_batch(int src, int dst)
{
	BatchRenderJob *src_job = jobs.values[src];
	if(dst < 0) dst = jobs.total - 1;

	if(dst != src)
	{
		for(int i = src; i < jobs.total - 1; i++)
			jobs.values[i] = jobs.values[i + 1];
		for(int i = jobs.total - 1; i > dst; i--)
			jobs.values[i] = jobs.values[i - 1];
		jobs.values[dst] = src_job;
		gui->create_list(1);
	}
}


BatchRenderGUI::BatchRenderGUI(BatchRenderThread *thread,
	int x,
	int y,
	int w,
	int h)
 : BC_Window(MWindow::create_title(N_("Batch Render")),
	x,
	y,
	w, 
	h, 
	50, 
	50, 
	1,
	0, 
	1)
{
	this->thread = thread;

	theme_global->get_batchrender_sizes(this, get_w(), get_h());

	x = theme_global->batchrender_x1;
	y = 5;
	int x1 = theme_global->batchrender_x1;
	int x2 = theme_global->batchrender_x2;
	int x3 = theme_global->batchrender_x3;
	int y1 = y;
	int y2;

	set_icon(mwindow_global->get_window_icon());
// output file
	add_subwindow(output_path_title = new BC_Title(x1, y, _("Output path:")));
	y += 20;
	format_tools = new BatchFormat(this,
					thread->get_current_asset(),
					x,
					y,
					SUPPORTS_AUDIO|SUPPORTS_VIDEO,
					SUPPORTS_AUDIO|SUPPORTS_VIDEO,
					SUPPORTS_VIDEO,
					&thread->get_current_job()->strategy);

	x2 = x;
	y2 = y + 10;
	x += format_tools->get_w();
	y = y1;
	x1 = x;
	x3 = x + 80;

// input EDL
	x = x1;
	add_subwindow(edl_path_title = new BC_Title(x, y, _("EDL Path:")));
	y += 20;
	add_subwindow(edl_path_text = new BatchRenderEDLPath(
		thread, 
		x, 
		y, 
		get_w() - x - 40, 
		thread->get_current_edl()));

	x += edl_path_text->get_w();
	add_subwindow(edl_path_browse = new BrowseButton(
		mwindow_global,
		this,
		edl_path_text, 
		x, 
		y, 
		thread->get_current_edl(),
		_("Input EDL"),
		_("Select an EDL to load:"),
		0));

	x = x1;

	int xx1, yy1;
	y += 45;
	add_subwindow(new_batch = new BatchRenderNew(thread, 
		x, 
		y));
	xx1 = x + new_batch->get_w() + 40;
	yy1 = y + new_batch->get_h() + 20;

	add_subwindow(delete_batch = new BatchRenderDelete(thread, 
		xx1, 
		y));

	x = x2;
	y = y2;
	add_subwindow(list_title = new BC_Title(x, y, _("Batches to render:")));
	y += 20;

	add_subwindow(batch_list = new BatchRenderList(thread, 
		x, 
		y,
		get_w() - x - 10,
		get_h() - y - BC_GenericButton::calculate_h() - 15,
		list_columns));

	y += batch_list->get_h() + 10;

	add_subwindow(start_button = new BatchRenderStart(thread, 
		x,
		y));
	x = get_w() / 2 -
		BC_GenericButton::calculate_w(this, _("Stop")) / 2;
	add_subwindow(stop_button = new BatchRenderStop(thread, 
		x, 
		y));
	x = get_w() - 
		BC_GenericButton::calculate_w(this, _("Close")) - 
		10;
	add_subwindow(cancel_button = new BatchRenderCancel(thread, 
		x, 
		y));
	show_window();
}

BatchRenderGUI::~BatchRenderGUI()
{
	delete format_tools;
}

void BatchRenderGUI::resize_event(int w, int h)
{
	mainsession->batchrender_w = w;
	mainsession->batchrender_h = h;
	theme_global->get_batchrender_sizes(this, w, h);

	int x = theme_global->batchrender_x1;
	int y = 5;
	int x1 = theme_global->batchrender_x1;
	int x2 = theme_global->batchrender_x2;
	int x3 = theme_global->batchrender_x3;
	int y1 = y;
	int y2;

	output_path_title->reposition_window(x1, y);
	y += 20;
	format_tools->reposition_window(x, y);
	x2 = x;
	y2 = y + 10;
	y = y1;
	x += format_tools->get_w();
	x1 = x;
	x3 = x + 80;

	x = x1;
	edl_path_title->reposition_window(x, y);
	y += 20;
	edl_path_text->reposition_window(x, y, w - x - 40);
	x += edl_path_text->get_w();
	edl_path_browse->reposition_window(x, y);

	x = x1;
	y += 45;

	int xx, yy;
	new_batch->reposition_window(x, y);
	xx = x + new_batch->get_w() + 40;
	yy = y + new_batch->get_h() + 20;
	delete_batch->reposition_window(xx, y);

	x = x2;
	y = y2;
	int y_margin = get_h() - batch_list->get_h();
	list_title->reposition_window(x, y);
	y += 20;
	batch_list->reposition_window(x, y, w - x - 10, h - y_margin);

	y += batch_list->get_h() + 10;
	start_button->reposition_window(x, y);
	x = w / 2 - 
		stop_button->get_w() / 2;
	stop_button->reposition_window(x, y);
	x = w -
		cancel_button->get_w() - 
		10;
	cancel_button->reposition_window(x, y);
}

void BatchRenderGUI::translation_event()
{
	mainsession->batchrender_x = get_x();
	mainsession->batchrender_y = get_y();
}

void BatchRenderGUI::close_event()
{
// Stop batch rendering
	thread->stop_rendering();
	set_done(1);
}

void BatchRenderGUI::create_list(int update_widget)
{
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		list_columns[i].remove_all_objects();
	}

	for(int i = 0; i < thread->jobs.total; i++)
	{
		BatchRenderJob *job = thread->jobs.values[i];
		char string[BCTEXTLEN];
		BC_ListBoxItem *enabled = new BC_ListBoxItem(job->enabled ? 
			(char*)"X" : 
			(char*)" ");
		BC_ListBoxItem *item1 = new BC_ListBoxItem(job->asset->path);
		BC_ListBoxItem *item2 = new BC_ListBoxItem(job->edl_path);
		BC_ListBoxItem *item3;
		if(job->elapsed)
			item3 = new BC_ListBoxItem(
				Units::totext(string,
					job->elapsed,
					TIME_HMS2));
		else
			item3 = new BC_ListBoxItem(_("Unknown"));
		list_columns[0].append(enabled);
		list_columns[1].append(item1);
		list_columns[2].append(item2);
		list_columns[3].append(item3);
		if(i == thread->current_job)
		{
			enabled->set_selected(1);
			item1->set_selected(1);
			item2->set_selected(1);
			item3->set_selected(1);
		}
		if(i == thread->rendering_job)
		{
			enabled->set_color(RED);
			item1->set_color(RED);
			item2->set_color(RED);
			item3->set_color(RED);
		}
	}

	if(update_widget)
	{
		batch_list->update(list_columns,
				list_titles,
				thread->column_width,
				BATCHRENDER_COLUMNS,
				batch_list->get_xposition(),
				batch_list->get_yposition(), 
				batch_list->get_highlighted_item(),  // Flat index of item cursor is over
				1,     // set all autoplace flags to 1
				1);
	}
}

void BatchRenderGUI::change_job()
{
	BatchRenderJob *job = thread->get_current_job();
	format_tools->update(job->asset, &job->strategy);
	edl_path_text->update(job->edl_path);
}


BatchFormat::BatchFormat(BatchRenderGUI *gui,
			Asset *asset,
			int &init_x,
			int &init_y,
			int support,
			int checkbox,
			int details,
			int *strategy)
 : FormatTools(gui, asset, init_x, init_y, support, checkbox, details,
	strategy)
{
	this->gui = gui;
}

int BatchFormat::handle_event()
{
	gui->create_list(1);
	return 1;
}


BatchRenderEDLPath::BatchRenderEDLPath(BatchRenderThread *thread, 
	int x, 
	int y, 
	int w, 
	char *text)
 : BC_TextBox(x, 
		y, 
		w, 
		1,
		text)
{
	this->thread = thread;
}

int BatchRenderEDLPath::handle_event()
{
	strcpy(thread->get_current_edl(), get_text());
	thread->gui->create_list(1);
	return 1;
}


BatchRenderNew::BatchRenderNew(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("New"))
{
	this->thread = thread;
}

int BatchRenderNew::handle_event()
{
	thread->new_job();
	return 1;
}


BatchRenderDelete::BatchRenderDelete(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->thread = thread;
}

int BatchRenderDelete::handle_event()
{
	thread->delete_job();
	return 1;
}


BatchRenderList::BatchRenderList(BatchRenderThread *thread,
	int x, 
	int y,
	int w,
	int h,
	ArrayList<BC_ListBoxItem*> *list_columns)
 : BC_ListBox(x, 
	y,
	w, 
	h, 
	list_columns,
	LISTBOX_DRAG,
	list_titles,
	thread->column_width,
	BATCHRENDER_COLUMNS)
{
	this->thread = thread;
	dragging_item = 0;
	set_process_drag(0);
}

int BatchRenderList::handle_event()
{
	return 1;
}

void BatchRenderList::selection_changed()
{
	thread->current_job = get_selection_number(0, 0);
	thread->gui->change_job();
	if(get_cursor_x() < thread->column_width[0])
	{
		BatchRenderJob *job = thread->get_current_job();
		job->enabled = !job->enabled;
		thread->gui->create_list(1);
	}
}

int BatchRenderList::column_resize_event()
{
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		thread->column_width[i] = get_column_width(i);
	}
	return 1;
}

int BatchRenderList::drag_start_event()
{
	if(BC_ListBox::drag_start_event())
	{
		dragging_item = 1;
		return 1;
	}
	return 0;
}

void BatchRenderList::drag_stop_event()
{
	if(dragging_item)
	{
		int src = get_selection_number(0, 0);
		int dst = get_highlighted_item();
		if(src != dst)
		{
			thread->move_batch(src, dst);
		}
		BC_ListBox::drag_stop_event();
	}
}


BatchRenderStart::BatchRenderStart(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, 
	y,
	_("Start"))
{
	this->thread = thread;
}

int BatchRenderStart::handle_event()
{
	thread->start_rendering();
	return 1;
}


BatchRenderStop::BatchRenderStop(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, 
	y,
	_("Stop"))
{
	this->thread = thread;
}

int BatchRenderStop::handle_event()
{
	thread->stop_rendering();
	return 1;
}


BatchRenderCancel::BatchRenderCancel(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, 
	y,
	_("Close"))
{
	this->thread = thread;
}

int BatchRenderCancel::handle_event()
{
	thread->stop_rendering();
	thread->gui->set_done(1);
	return 1;
}

int BatchRenderCancel::keypress_event()
{
	if(get_keypress() == ESC) 
	{
		thread->stop_rendering();
		thread->gui->set_done(1);
		return 1;
	}
	return 0;
}
