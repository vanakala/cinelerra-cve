// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdelete.h"
#include "bcfilebox.h"
#include "bclistboxitem.h"
#include "bcnewfolder.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bctitle.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "filesystem.h"
#include "language.h"
#include "mutex.h"
#include "vframe.h"

#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>


BC_FileBoxRecent::BC_FileBoxRecent(BC_FileBox *filebox, int x, int y)
 : BC_ListBox(x, y, filebox->get_w() - 20,
	filebox->get_text_height(MEDIUMFONT) * FILEBOX_HISTORY_SIZE +
		BC_ScrollBar::get_span(SCROLL_HORIZ) +
		LISTBOX_MARGIN * 2,
	&filebox->recent_dirs,
	LISTBOX_POPUP)
{
	this->filebox = filebox;
}

int BC_FileBoxRecent::handle_event()
{
	if(get_selection(0, 0))
		filebox->submit_dir(get_selection(0, 0)->get_text());
	return 1;
}


BC_FileBoxListBox::BC_FileBoxListBox(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, y, filebox->get_listbox_w(), filebox->get_listbox_h(y),
	filebox->list_column,
	(filebox->get_display_mode() ? LISTBOX_ICONS : 0) |
		(filebox->select_multiple ? LISTBOX_MULTIPLE : 0),
	filebox->column_titles,
	filebox->column_width,
	filebox->columns)
{
	this->filebox = filebox;
	set_sort_column(filebox->sort_column);
	set_sort_order(filebox->sort_order);
	set_allow_drag_column(1);
}

int BC_FileBoxListBox::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text());
	return 1;
}

void BC_FileBoxListBox::selection_changed()
{
	BC_ListBoxItem *item = get_selection(
		filebox->column_of_type(FILEBOX_NAME), 0);

	if(item)
	{
		char path[BCTEXTLEN];
		strcpy(path, item->get_text());
		filebox->textbox->update(path);
		filebox->fs->extract_dir(filebox->directory, path);
		filebox->fs->extract_name(filebox->filename, path);
		filebox->fs->complete_path(path);
		strcpy(filebox->current_path, path);
		strcpy(filebox->submitted_path, path);
	}
}

int BC_FileBoxListBox::column_resize_event()
{
	for(int i = 0; i < filebox->columns; i++)
		resources.filebox_columnwidth[i] =
			filebox->column_width[i] =
			get_column_width(i);
	return 1;
}

int BC_FileBoxListBox::sort_order_event()
{
	resources.filebox_sortcolumn = filebox->sort_column = get_sort_column();
	resources.filebox_sortorder = filebox->sort_order = get_sort_order();
	filebox->refresh();
	return 1;
}

int BC_FileBoxListBox::move_column_event()
{
	filebox->move_column(get_from_column(), get_to_column());
	return 1;
}

int BC_FileBoxListBox::evaluate_query(int list_item, char *string)
{
	ArrayList<BC_ListBoxItem*> *column =
		&filebox->list_column[filebox->column_of_type(FILEBOX_NAME)];
	return(column->values[list_item]->get_color() != resources.directory_color &&
		strcmp(string, column->values[list_item]->get_text()) <= 0);
}


BC_FileBoxTextBox::BC_FileBoxTextBox(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - 50, 1, filebox->filename)
{
	this->filebox = filebox; 
}


BC_FileBoxDirectoryText::BC_FileBoxDirectoryText(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - 40, 1, filebox->fs->get_current_dir())
{
	this->filebox = filebox;
}

int BC_FileBoxDirectoryText::handle_event()
{
	char *path;

	path = get_text();
	// is a directory, change directories
	if(filebox->fs->is_dir(path))
	{
		filebox->fs->change_dir(path);
		filebox->refresh();
		update(strcat(filebox->fs->get_current_dir(),"/"));
	}
	return 1;
}


BC_FileBoxFilterText::BC_FileBoxFilterText(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - 50, 1, filebox->get_resources()->filebox_filter)
{
	this->filebox = filebox;
}

int BC_FileBoxFilterText::handle_event()
{
	filebox->update_filter(get_text());
	return 1;
}


BC_FileBoxFilterMenu::BC_FileBoxFilterMenu(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, y, filebox->get_w() - 30, 120, &filebox->filter_list,
	LISTBOX_POPUP)
{
	this->filebox = filebox;
	set_tooltip(_("Change the filter"));
}

int BC_FileBoxFilterMenu::handle_event()
{
	filebox->filter_text->update(
		get_selection(filebox->column_of_type(FILEBOX_NAME), 0)->get_text());
	filebox->update_filter(
		get_selection(filebox->column_of_type(FILEBOX_NAME), 0)->get_text());
	return 1;
}


BC_FileBoxCancel::BC_FileBoxCancel(BC_FileBox *filebox)
 : BC_CancelButton(filebox)
{
	this->filebox = filebox;
	set_tooltip(_("Cancel the operation"));
}

int BC_FileBoxCancel::handle_event()
{
	filebox->newfolder_thread->interrupt();
	filebox->set_done(1);
	return 1;
}


BC_FileBoxUseThis::BC_FileBoxUseThis(BC_FileBox *filebox)
 : BC_Button(filebox->get_w() / 2 -
		BC_WindowBase::get_resources()->usethis_button_images[0]->get_w() / 2,
	filebox->ok_button->get_y(),
	BC_WindowBase::get_resources()->usethis_button_images)
{
	this->filebox = filebox;
	set_tooltip(_("Submit the directory"));
}

int BC_FileBoxUseThis::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text(), 1);
	return 1;
}


BC_FileBoxOK::BC_FileBoxOK(BC_FileBox *filebox)
 : BC_OKButton(filebox, !filebox->want_directory ?
		BC_WindowBase::get_resources()->ok_images :
		BC_WindowBase::get_resources()->filebox_descend_images)
{
	this->filebox = filebox;
	if(filebox->want_directory)
		set_tooltip(_("Descend directory"));
	else
		set_tooltip(_("Submit the file"));
}

int BC_FileBoxOK::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text());
	return 1;
}


BC_FileBoxText::BC_FileBoxText(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_text_images)
{
	this->filebox = filebox;
	set_tooltip(_("Display text"));
}

int BC_FileBoxText::handle_event()
{
	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(),
		FILEBOX_LIST);
	return 1;
}


BC_FileBoxIcons::BC_FileBoxIcons(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_icons_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Display icons"));
}

int BC_FileBoxIcons::handle_event()
{
	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(),
		LISTBOX_ICONS);
	return 1;
}


BC_FileBoxNewfolder::BC_FileBoxNewfolder(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_newfolder_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Create new folder"));
}

int BC_FileBoxNewfolder::handle_event()
{
	filebox->newfolder_thread->start_new_folder();
	return 1;
}


BC_FileBoxUpdir::BC_FileBoxUpdir(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_updir_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Up a directory"));
}

int BC_FileBoxUpdir::handle_event()
{
// Need a temp so submit_file can expand it
	strcpy(string, _(".."));
	filebox->submit_file(string);
	return 1;
}


BC_FileBoxDelete::BC_FileBoxDelete(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_delete_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Delete files"));
}

int BC_FileBoxDelete::handle_event()
{
	filebox->delete_thread->start();
	return 1;
}


BC_FileBoxReload::BC_FileBoxReload(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_reload_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Refresh"));
}

int BC_FileBoxReload::handle_event()
{
	filebox->refresh();
	return 1;
}


BC_FileBox::BC_FileBox(int x, int y, const char *init_path,
	const char *title, const char *caption,
	int show_all_files, int want_directory,
	int multiple_files, int h_padding)
 : BC_Window(title, x, y,
	BC_WindowBase::get_resources()->filebox_w, 
	BC_WindowBase::get_resources()->filebox_h, 
	530, 300, 1, 0, 1)
{
	fs = new FileSystem;
	columns = FILEBOX_COLUMNS;
	list_column = new ArrayList<BC_ListBoxItem*>[columns];
	column_type = new int[columns];
	column_width = new int[columns];

	filter_text = 0;
	filter_popup = 0;
	usethis_button = 0;

	strcpy(this->caption, caption);
	strcpy(this->current_path, init_path);
	strcpy(this->submitted_path, init_path);
	select_multiple = multiple_files;
	this->want_directory = want_directory;
	if(show_all_files) fs->set_show_all();
	fs->complete_path(this->current_path);
	fs->complete_path(this->submitted_path);
	fs->extract_dir(directory, this->current_path);
	fs->extract_name(filename, this->current_path);

	for(int i = 0; i < columns; i++)
	{
		column_type[i] = get_resources()->filebox_columntype[i];
		column_width[i] = get_resources()->filebox_columnwidth[i];
		column_titles[i] = BC_FileBox::columntype_to_text(column_type[i]);
	}
	sort_column = get_resources()->filebox_sortcolumn;
	sort_order = get_resources()->filebox_sortorder;

// Test directory
	if(fs->update(directory))
	{
		strcpy(this->current_path, "~");
		fs->complete_path(this->current_path);
		fs->update(this->current_path);
		strcpy(directory, fs->get_current_dir());
		filename[0] = 0;
	}

	if(h_padding == -1)
		h_padding = BC_WindowBase::get_resources()->ok_images[0]->get_h() - 20;

	this->h_padding = h_padding;
	delete_thread = new BC_DeleteThread(this);

	x = 10;
	y = 10;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int directory_title_margin = MAX(20,
		resources->filebox_text_images[0]->get_h());

// Create recent dir list
	create_history();

// Directories aren't filtered in FileSystem so skip this
	if(!want_directory)
	{
		filter_list.append(new BC_ListBoxItem("*"));
		filter_list.append(new BC_ListBoxItem("[*.ifo][*.vob]"));
		filter_list.append(new BC_ListBoxItem("[*.mp2][*.mp3][*.wav]"));
		filter_list.append(new BC_ListBoxItem("[*.avi][*.mpg][*.m2v][*.m1v][*.mov]"));
		filter_list.append(new BC_ListBoxItem("*.xml"));
		fs->set_filter(get_resources()->filebox_filter);
	}

	fs->update(directory);
	create_icons();
	create_tables();

	add_subwindow(ok_button = new BC_FileBoxOK(this));
	if(want_directory)
		add_subwindow(usethis_button = new BC_FileBoxUseThis(this));
	add_subwindow(cancel_button = new BC_FileBoxCancel(this));

	add_subwindow(new BC_Title(x, y, caption));

	x = get_w() - resources->filebox_icons_images[0]->get_w() - 10;
	add_subwindow(icon_button = new BC_FileBoxIcons(x, y, this));
	x -= resources->filebox_text_images[0]->get_w() + 5;
	add_subwindow(text_button = new BC_FileBoxText(x, y, this));
	x -= resources->filebox_newfolder_images[0]->get_w() + 5;
	add_subwindow(folder_button = new BC_FileBoxNewfolder(x, y, this));
	x -= resources->filebox_delete_images[0]->get_w() + 5;
	add_subwindow(delete_button = new BC_FileBoxDelete(x, y, this));
	x -= resources->filebox_reload_images[0]->get_w() + 5;
	add_subwindow(reload_button = new BC_FileBoxReload(x, y, this));
	x -= resources->filebox_updir_images[0]->get_w() + 5;
	add_subwindow(updir_button = new BC_FileBoxUpdir(x, y, this));

	x = 10;
	y += directory_title_margin + 3;
	add_subwindow(directory_title = new BC_FileBoxDirectoryText(x, y, this));
	add_subwindow(recent_popup = new BC_FileBoxRecent(this,
		x + directory_title->get_w(), y));

	x = 10;
	y += directory_title->get_h() + 5;
	listbox = 0;
	create_listbox(x, y, get_display_mode());
	y += listbox->get_h() + 30;
	add_subwindow(textbox = new BC_FileBoxTextBox(x, y, this));
	y += textbox->get_h() + 10;

	if(!want_directory)
	{
		add_subwindow(filter_text = new BC_FileBoxFilterText(x, y, this));
		add_subwindow(filter_popup = 
			new BC_FileBoxFilterMenu(x + filter_text->get_w(), y, this));;
	}

// listbox has to be active because refresh might be called from newfolder_thread
	listbox->activate();
	newfolder_thread = new BC_NewFolderThread(this);

	show_window();
}

BC_FileBox::~BC_FileBox()
{
// this has to be destroyed before tables, because it can call for an update!
	delete newfolder_thread;
	delete fs;
	delete_tables();
	for(int i = 0; i < TOTAL_ICONS; i++)
		delete icons[i];
	filter_list.remove_all_objects();
	delete [] list_column;
	delete [] column_type;
	delete [] column_width;
	delete delete_thread;
	recent_dirs.remove_all_objects();
}

int BC_FileBox::get_listbox_w()
{
	return get_w() - 20;
}

int BC_FileBox::get_listbox_h(int y)
{
	int result = get_h() - y - h_padding;

	if(want_directory)
		result -= BC_WindowBase::get_resources()->dirbox_margin;
	else
		result -= BC_WindowBase::get_resources()->filebox_margin;

	return result;
}

void BC_FileBox::create_icons()
{
	for(int i = 0; i < TOTAL_ICONS; i++)
	{
		icons[i] = new BC_Pixmap(this, 
			BC_WindowBase::get_resources()->type_to_icon[i],
			PIXMAP_ALPHA);
	}
}

void BC_FileBox::resize_event(int w, int h)
{
	draw_background(0, 0, w, h);
	directory_title->reposition_window(directory_title->get_x(),
		directory_title->get_y(), w - 40);
	recent_popup->reposition_window(
		directory_title->get_x() + directory_title->get_w(),
		directory_title->get_y());
	listbox->reposition_window(listbox->get_x(), listbox->get_y(),
		get_listbox_w(), get_listbox_h(listbox->get_y()));
	textbox->reposition_window(textbox->get_x(),
		listbox->get_y() + listbox->get_h() + 30,
		w - 50);
	if(usethis_button)
		usethis_button->reposition_window(w / 2 - usethis_button->get_w() / 2,
			h - (get_h() - usethis_button->get_y()));
	if(filter_popup)
		filter_popup->reposition_window(w - (get_w() - filter_popup->get_x()),
			h - (get_h() - filter_popup->get_y()), w - 30);
	if(filter_text)
		filter_text->reposition_window(filter_text->get_x(),
			h - (get_h() - filter_text->get_y()),
			w - (get_w() - filter_text->get_w()), 1);
	icon_button->reposition_window(w - (get_w() - icon_button->get_x()),
		icon_button->get_y());
	text_button->reposition_window(w - (get_w() - text_button->get_x()),
		text_button->get_y());
	folder_button->reposition_window(w - (get_w() - folder_button->get_x()),
		folder_button->get_y());
	reload_button->reposition_window(w - (get_w() - reload_button->get_x()),
		reload_button->get_y());
	delete_button->reposition_window(w - (get_w() - delete_button->get_x()),
		delete_button->get_y());
	updir_button->reposition_window(w - (get_w() - updir_button->get_x()),
		updir_button->get_y());
	get_resources()->filebox_w = get_w();
	get_resources()->filebox_h = get_h();
	flash();
}

int BC_FileBox::keypress_event()
{
	switch(get_keypress())
	{
	case 'w':
		if(ctrl_down())
		{
			set_done(1);
			return 1;
		}
	}
	return 0;
}

void BC_FileBox::extract_extension(char *out, const char *in)
{
	int i;

	for(i = strlen(in)-1; i > 0 && in[i] != '.'; i--);
	if(in[i] == '.')
	{
		i++;
		strcpy(out, &in[i]);
	}
	else
		out[0] = '\0';
}

void BC_FileBox::create_tables()
{
	delete_tables();
	char string[BCTEXTLEN];
	BC_ListBoxItem *new_item;

	fs->set_sort_order(sort_order);
	fs->set_sort_field(column_type[sort_column]);

// Directory is entered before this from a random source
	fs->update(0);
	for(int i = 0; i < fs->total_files(); i++)
	{
		FileItem *file_item = fs->get_entry(i);
		int is_dir = file_item->is_dir;
		BC_Pixmap* icon = get_icon(file_item->name, is_dir);

// Name entry
		new_item = new BC_ListBoxItem(file_item->name, icon,
			is_dir ? get_resources()->directory_color :
			get_resources()->file_color);

		if(is_dir)
			new_item->set_searchable(0);

		list_column[column_of_type(FILEBOX_NAME)].append(new_item);

		if(!is_dir)
		{
			sprintf(string, "%" PRId64, file_item->size);
			new_item = new BC_ListBoxItem(string, get_resources()->file_color);
		}
		else
			new_item = new BC_ListBoxItem("",
				get_resources()->directory_color);

		list_column[column_of_type(FILEBOX_SIZE)].append(new_item);

// Date entry
		if(!is_dir || 1)
		{
			static const char *month_text[13] =
			{
				N_("Null"),
				N_("Jan"),
				N_("Feb"),
				N_("Mar"),
				N_("Apr"),
				N_("May"),
				N_("Jun"),
				N_("Jul"),
				N_("Aug"),
				N_("Sep"),
				N_("Oct"),
				N_("Nov"),
				N_("Dec")
			};
			sprintf(string, "%s %d, %d", _(month_text[file_item->month]),
				file_item->day, file_item->year);
			new_item = new BC_ListBoxItem(string, get_resources()->file_color);
		}
		else
			new_item = new BC_ListBoxItem("", get_resources()->directory_color);

		list_column[column_of_type(FILEBOX_DATE)].append(new_item);

// Extension entry
		if(!is_dir)
		{
			extract_extension(string, file_item->name);
			new_item = new BC_ListBoxItem(string, get_resources()->file_color);
		}
		else
			new_item = new BC_ListBoxItem("", get_resources()->directory_color);
		list_column[column_of_type(FILEBOX_EXTENSION)].append(new_item);
	}
}

void BC_FileBox::delete_tables()
{
	for(int j = 0; j < columns; j++)
		list_column[j].remove_all_objects();
}

BC_Pixmap* BC_FileBox::get_icon(char *path, int is_dir)
{
	char *suffix = strrchr(path, '.');
	int icon_type = ICON_UNKNOWN;

	if(is_dir)
		return icons[ICON_FOLDER];

	if(suffix)
	{
		suffix++;
		if(*suffix != 0)
		{
			for(int i = 0; get_resources()->suffix_to_type[i].suffix; i++)
			{
				if(!strcasecmp(suffix, BC_WindowBase::get_resources()->suffix_to_type[i].suffix)) 
				{
					icon_type = BC_WindowBase::get_resources()->suffix_to_type[i].icon_type;
					break;
				}
			}
		}
	}
	return icons[icon_type];
}

const char* BC_FileBox::columntype_to_text(int type)
{
	switch(type)
	{
	case FILEBOX_NAME:
		return _("File");

	case FILEBOX_SIZE:
		return _("Size");

	case FILEBOX_DATE:
		return _("Date");

	case FILEBOX_EXTENSION:
		return _("Ext.");
	}
	return "";
}

int BC_FileBox::column_of_type(int type)
{
	for(int i = 0; i < columns; i++)
		if(column_type[i] == type)
			return i;
	return 0;
}

void BC_FileBox::refresh()
{
	create_tables();
	listbox->set_master_column(column_of_type(FILEBOX_NAME), 0);
	listbox->update(list_column, column_titles, column_width,
		columns, 0, 0,-1, 1);
}

void BC_FileBox::update_filter(const char *filter)
{
	fs->set_filter(filter);
	fs->update(0);
	refresh();
	strcpy(get_resources()->filebox_filter, filter);
}

void BC_FileBox::move_column(int src, int dst)
{
	ArrayList<BC_ListBoxItem*> *new_columns = 
		new ArrayList<BC_ListBoxItem*>[columns];
	int *new_types = new int[columns];
	int *new_widths = new int[columns];

// Fill in remaining columns with consecutive data
	for(int out_column = 0, in_column = 0; out_column < columns;
		out_column++, in_column++)
	{
// Copy destination column from src column
		if(out_column == dst)
		{
			for(int i = 0; i < list_column[src].total; i++)
				new_columns[out_column].append(list_column[src].values[i]);

			new_types[out_column] = column_type[src];
			new_widths[out_column] = column_width[src];
			in_column--;
		}
		else
		{
// Skip source column
			if(in_column == src)
				in_column++;

			for(int i = 0; i < list_column[src].total; i++)
				new_columns[out_column].append(list_column[in_column].values[i]);

			new_types[out_column] = column_type[in_column];
			new_widths[out_column] = column_width[in_column];
		}
	}

// Swap tables
	delete [] list_column;
	delete [] column_type;
	delete [] column_width;
	list_column = new_columns;
	column_type = new_types;
	column_width = new_widths;

	for(int i = 0; i < columns; i++)
	{
		get_resources()->filebox_columntype[i] = column_type[i];
		get_resources()->filebox_columnwidth[i] = column_width[i];
		column_titles[i] = BC_FileBox::columntype_to_text(column_type[i]);
	}

	refresh();
}

void BC_FileBox::submit_dir(char *dir)
{
	strcpy(directory, dir);
	fs->join_names(current_path, directory, filename);
	strcpy(submitted_path, current_path);
	fs->change_dir(dir);
	refresh();
	directory_title->update(fs->get_current_dir());
	if(want_directory)
		textbox->update(fs->get_current_dir());
	else
		textbox->update(filename);
	listbox->reset_query();
}

int BC_FileBox::submit_file(char *path, int use_this)
{
// blank.  
// If file wanted, take the current directory as the desired file.
// If directory wanted, ignore it.
	if(!path[0] && !want_directory)
	{
// save complete path
		strcpy(this->current_path, directory);
// save complete path
		strcpy(this->submitted_path, directory);
		update_history();
// Zero out filename
		filename[0] = 0;
		set_done(0);
		return 0;
	}

// is a directory, change directories
	if(fs->is_dir(path) && !use_this)
	{
		fs->change_dir(path);
		refresh();
		directory_title->update(fs->get_current_dir());
		strcpy(this->current_path, fs->get_current_dir());
		strcpy(this->submitted_path, fs->get_current_dir());
		strcpy(this->directory, fs->get_current_dir());
		filename[0] = 0;
		if(want_directory)
			textbox->update(fs->get_current_dir());
		else
			textbox->update("");
		listbox->reset_query();
		return 1;
	}
	else
// Is a file or desired directory.  Quit the operation.
	{
// save directory for defaults
		fs->extract_dir(directory, path);

// Just take the directory
		if(want_directory)
		{
			filename[0] = 0;
			strcpy(path, directory);
		}
		else
// Take the complete path
		{
			fs->extract_name(filename, path);     // save filename
		}

		fs->complete_path(path);
		strcpy(this->current_path, path);          // save complete path
		strcpy(this->submitted_path, path);          // save complete path
		update_history();
		newfolder_thread->interrupt();
		set_done(0);
		return 0;
	}
	return 0;
}

void BC_FileBox::update_history()
{
// Look for path already in history
	BC_Resources *resources = get_resources();
	int new_slot = FILEBOX_HISTORY_SIZE - 1;

	for(int i = FILEBOX_HISTORY_SIZE - 1; i >= 0; i--)
	{
		if(!strcmp(resources->filebox_history[i], directory))
		{
// Shift down from this point
			while(i > 0)
			{
				strcpy(resources->filebox_history[i],
					resources->filebox_history[i - 1]);
				if(resources->filebox_history[i][0])
					new_slot--;
				i--;
			}
			break;
		}
		else
			if(resources->filebox_history[i][0])
				new_slot--;
		else
			break;
	}

	if(new_slot < 0)
	{
		for(int i = FILEBOX_HISTORY_SIZE - 1; i > 0; i--)
		{
			strcpy(resources->filebox_history[i],
					resources->filebox_history[i - 1]);
		}
		new_slot = 0;
	}
	strcpy(resources->filebox_history[new_slot], directory);

	create_history();
	recent_popup->update(&recent_dirs, 0, 0, 1);
}

void BC_FileBox::create_history()
{
	BC_Resources *resources = get_resources();

	recent_dirs.remove_all_objects();
	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
	{
		if(resources->filebox_history[i][0])
			recent_dirs.append(new BC_ListBoxItem(resources->filebox_history[i]));
	}
}

int BC_FileBox::get_display_mode()
{
	return top_level->get_resources()->filebox_mode;
}

void BC_FileBox::create_listbox(int x, int y, int mode)
{
	if(listbox && listbox->get_display_mode() != mode)
	{
		delete listbox;
		listbox = 0;
		top_level->get_resources()->filebox_mode = mode;
	}

	if(!listbox)
		add_subwindow(listbox = new BC_FileBoxListBox(x, y, this));
}

char* BC_FileBox::get_path(int selection)
{
	if(selection == 0)
		return get_submitted_path();
	else
	{
		BC_ListBoxItem *item = listbox->get_selection(
			column_of_type(FILEBOX_NAME), selection - 1);
		if(item)
		{
			fs->join_names(string, directory, item->get_text());
			return string;
		}
	}
	return 0;
}

char* BC_FileBox::get_submitted_path()
{
	return submitted_path;
}

char* BC_FileBox::get_current_path()
{
	return current_path;
}

char* BC_FileBox::get_newfolder_title()
{
	char *letter2 = strchr(title, ':');

	new_folder_title[0] = 0;

	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": New folder"));

	return new_folder_title;
}

char* BC_FileBox::get_delete_title()
{
	char *letter2 = strchr(title, '-');

	strcpy(new_folder_title, _("Delete "));

	if(letter2)
		strcat(new_folder_title, letter2);

	return new_folder_title;
}

void BC_FileBox::delete_files()
{
// Starting at 1 causes it to ignore what's in the textbox.
	int i = 1;
	char *path;
	FileSystem fs;

	while((path = get_path(i)))
	{
// Not directory.  Remove it.
		if(!fs.is_dir(path))
			remove(path);
		i++;
	}
	refresh();
}

BC_Button* BC_FileBox::get_ok_button()
{
	return ok_button;
}

BC_Button* BC_FileBox::get_cancel_button()
{
	return cancel_button;
}
