#include "bcfilebox.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bctitle.h"
#include "clip.h"
#include "filesystem.h"
#include "bclistboxitem.h"
#include "language.h"
#include <string.h>
#include <sys/stat.h>


#define DIRECTORY_COLOR BLUE
#define FILE_COLOR BLACK

BC_NewFolder::BC_NewFolder(int x, int y, BC_FileBox *filebox)
 : BC_Window(filebox->get_newfolder_title(), 
 	x, 
	y, 
	320, 
	120, 
	0, 
	0, 
	0, 
	0, 
	1)
{
}

BC_NewFolder::~BC_NewFolder()
{
}


int BC_NewFolder::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Enter the name of the folder:")));
	y += 20;
	add_subwindow(textbox = new BC_TextBox(x, y, 300, 1, _("Untitled")));
	y += 30;
	add_subwindow(new BC_OKButton(this));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(this));
	show_window();
	return 0;
}

char* BC_NewFolder::get_text()
{
	return textbox->get_text();
}


BC_NewFolderThread::BC_NewFolderThread(BC_FileBox *filebox)
 : Thread(0, 0, 0)
{
	this->filebox = filebox;
	window = 0;
}

BC_NewFolderThread::~BC_NewFolderThread() 
{
 	interrupt();
}

void BC_NewFolderThread::run()
{
	change_lock.lock();
	window = new BC_NewFolder(filebox->get_abs_cursor_x(), 
		filebox->get_abs_cursor_y(),
		filebox);
	window->create_objects();
	change_lock.unlock();


	int result = window->run_window();

	if(!result)
	{
		char new_folder[BCTEXTLEN];
		filebox->fs->join_names(new_folder, filebox->fs->get_current_dir(), window->get_text());
		mkdir(new_folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		filebox->lock_window();
		filebox->refresh();
		filebox->unlock_window();
	}

	change_lock.lock();
	delete window;
	window = 0;
	change_lock.unlock();

	completion_lock.unlock();
}

int BC_NewFolderThread::interrupt()
{
	change_lock.lock();
	if(window)
	{
		window->lock_window();
		window->set_done(1);
		window->unlock_window();
	}

	change_lock.unlock();

	completion_lock.lock();
	completion_lock.unlock();
	return 0;
}

int BC_NewFolderThread::start_new_folder()
{
	change_lock.lock();

	if(window)
	{
		window->lock_window();
		window->raise_window();
		window->unlock_window();
		change_lock.unlock();
	}
	else
	{
		change_lock.unlock();
		completion_lock.lock();

		Thread::start();
	}


	return 0;
}







BC_FileBoxListBox::BC_FileBoxListBox(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, 
 			y, 
 			filebox->get_listbox_w(), 
			filebox->get_listbox_h(y), 
 			filebox->get_display_mode(), 
 			filebox->list_column, 
			filebox->column_titles,
			filebox->column_width,
			FILEBOX_COLUMNS,
			0,
			0,
			filebox->select_multiple ? LISTBOX_MULTIPLE : LISTBOX_SINGLE,
			ICON_LEFT,
			0)
{ 
	this->filebox = filebox;
	set_sort_column(filebox->sort_column);
	set_sort_order(filebox->sort_order);
	set_allow_drag_column(1);
}

BC_FileBoxListBox::~BC_FileBoxListBox()
{
}

int BC_FileBoxListBox::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text(), 0);
	return 1;
}

int BC_FileBoxListBox::selection_changed()
{
	BC_ListBoxItem *item = get_selection(
		filebox->column_of_type(FILEBOX_NAME), 0);

	if(item)
		filebox->textbox->update(item->get_text());
	return 1;
}

int BC_FileBoxListBox::column_resize_event()
{
	for(int i = 0; i < FILEBOX_COLUMNS; i++)
		BC_WindowBase::get_resources()->filebox_columnwidth[i] = 
			filebox->column_width[i] = 
			get_column_width(i);
	return 1;
}

int BC_FileBoxListBox::sort_order_event()
{
	get_resources()->filebox_sortcolumn = filebox->sort_column = get_sort_column();
	get_resources()->filebox_sortorder = filebox->sort_order = get_sort_order();
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
	return(column->values[list_item]->get_color() != DIRECTORY_COLOR && 
		strcmp(string, column->values[list_item]->get_text()) <= 0);
}




BC_FileBoxTextBox::BC_FileBoxTextBox(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - 20, 1, filebox->filename)
{
	this->filebox = filebox; 
}

BC_FileBoxTextBox::~BC_FileBoxTextBox()
{
}

int BC_FileBoxTextBox::handle_event()
{
	
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
	return 0;
}




BC_FileBoxFilterMenu::BC_FileBoxFilterMenu(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, 
 	y, 
	filebox->get_w() - 30, 
	120, 
	LISTBOX_TEXT, 
	&filebox->filter_list, 
	0, 
	0, 
	1, 
	0, 
	1)
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
	return 0;
}










BC_FileBoxCancel::BC_FileBoxCancel(BC_FileBox *filebox)
 : BC_CancelButton(filebox)
{
	this->filebox = filebox;
	set_tooltip(_("Cancel the operation"));
}

BC_FileBoxCancel::~BC_FileBoxCancel()
{
}

int BC_FileBoxCancel::handle_event()
{
//	filebox->submit_file(filebox->textbox->get_text(), 0);
	filebox->newfolder_thread->interrupt();
	filebox->set_done(1);
	return 1;
}







BC_FileBoxUseThis::BC_FileBoxUseThis(BC_FileBox *filebox)
 : BC_Button(filebox->get_w() / 2 - 50, 
 	filebox->get_h() - 35, 
	BC_WindowBase::get_resources()->usethis_button_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Submit the directory"));
}

BC_FileBoxUseThis::~BC_FileBoxUseThis()
{
}

int BC_FileBoxUseThis::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text(), 0, 1);
	return 1;
}





BC_FileBoxOK::BC_FileBoxOK(BC_FileBox *filebox)
 : BC_OKButton(filebox)
{
	this->filebox = filebox; 
	set_tooltip(_("Submit the file"));
}

BC_FileBoxOK::~BC_FileBoxOK()
{
}

int BC_FileBoxOK::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text(), 0);
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
	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(), LISTBOX_TEXT);
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
	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(), LISTBOX_ICONS);
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
	sprintf(string, _(".."));
	filebox->submit_file(string, 0);
	return 1;
}








BC_FileBox::BC_FileBox(int x, 
		int y, 
		char *init_path,
		char *title,
		char *caption,
		int show_all_files,
		int want_directory,
		int multiple_files,
		int h_padding)
 : BC_Window(title, 
 	x,
	y,
 	BC_WindowBase::get_resources()->filebox_w, 
	BC_WindowBase::get_resources()->filebox_h, 
	10, 
	10,
	1,
	0,
	1)
{
	fs = new FileSystem;
	list_column = new ArrayList<BC_ListBoxItem*>[FILEBOX_COLUMNS];
	column_type = new int[FILEBOX_COLUMNS];
	column_width = new int[FILEBOX_COLUMNS];




	strcpy(this->caption, caption);
	strcpy(this->current_path, init_path);
	strcpy(this->submitted_path, init_path);
	select_multiple = multiple_files;
	this->want_directory = want_directory;
	if(show_all_files) fs->set_show_all();
	if(want_directory) fs->set_want_directory();
	fs->complete_path(this->current_path);
	fs->complete_path(this->submitted_path);
	fs->extract_dir(directory, this->current_path);
	fs->extract_name(filename, this->current_path);


	for(int i = 0; i < FILEBOX_COLUMNS; i++)
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
		sprintf(this->current_path, "~");
		fs->complete_path(this->current_path);
		fs->update(this->current_path);
		strcpy(directory, fs->get_current_dir());
		sprintf(filename, "");
	}


	this->h_padding = h_padding;
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
}

int BC_FileBox::create_objects()
{
	int x = 10, y = 10;

	filter_list.append(new BC_ListBoxItem("*"));
	filter_list.append(new BC_ListBoxItem("[*.ifo][*.vob]"));
	filter_list.append(new BC_ListBoxItem("[*.mp2][*.mp3][*.wav]"));
	filter_list.append(new BC_ListBoxItem("[*.avi][*.mpg][*.m2v][*.m1v][*.mov]"));
	filter_list.append(new BC_ListBoxItem("heroine*"));
	filter_list.append(new BC_ListBoxItem("*.xml"));
	fs->set_filter(get_resources()->filebox_filter);
	fs->update(directory);
	create_icons();
	create_tables();

	add_subwindow(ok_button = new BC_FileBoxOK(this));
	if(want_directory)
		add_subwindow(usethis_button = new BC_FileBoxUseThis(this));
	add_subwindow(cancel_button = new BC_FileBoxCancel(this));

	add_subwindow(new BC_Title(x, y, caption));

	add_subwindow(directory_title = 
		new BC_Title(x, y + 20, fs->get_current_dir()));
	x = get_w() - 50;
	add_subwindow(icon_button = new BC_FileBoxIcons(x, y, this));
	x -= icon_button->get_w();
	add_subwindow(text_button = new BC_FileBoxText(x, y, this));
	x -= text_button->get_w();
	add_subwindow(folder_button = new BC_FileBoxNewfolder(x, y, this));
	x -= folder_button->get_w();
	add_subwindow(updir_button = new BC_FileBoxUpdir(x, y, this));

	x = 10;
	y = MAX(updir_button->get_y() + updir_button->get_h() + 5, 
		directory_title->get_h() + directory_title->get_y());
	listbox = 0;

	create_listbox(x, y, get_display_mode());
	y += listbox->get_h() + 10;
	add_subwindow(textbox = new BC_FileBoxTextBox(x, y, this));
	y += textbox->get_h() + 10;
	add_subwindow(filter_text = new BC_FileBoxFilterText(x, y, this));
	add_subwindow(filter_popup = 
		new BC_FileBoxFilterMenu(x + filter_text->get_w(), y, this));

// listbox has to be active because refresh might be called from newfolder_thread
 	listbox->activate();    
	newfolder_thread = new BC_NewFolderThread(this);
	
	show_window();
	return 0;
}

int BC_FileBox::get_listbox_w()
{
	return get_w() - 20;
}

int BC_FileBox::get_listbox_h(int y)
{
	return get_h() - y - h_padding - 110;
}

int BC_FileBox::create_icons()
{
	for(int i = 0; i < TOTAL_ICONS; i++)
	{
		icons[i] = new BC_Pixmap(this, 
			BC_WindowBase::get_resources()->type_to_icon[i],
			PIXMAP_ALPHA);
	}
	return 0;
}

int BC_FileBox::resize_event(int w, int h)
{
	draw_background(0, 0, w, h);
	flash();

// OK button handles resize event itself
// 	ok_button->reposition_window(ok_button->get_x(), 
// 		h - (get_h() - ok_button->get_y()));
// 	cancel_button->reposition_window(w - (get_w() - cancel_button->get_x()), 
// 		h - (get_h() - cancel_button->get_y()));
	if(want_directory)
		usethis_button->reposition_window(w / 2 - 50, h - (get_h() - usethis_button->get_y()));
	filter_popup->reposition_window(w - (get_w() - filter_popup->get_x()), 
		h - (get_h() - filter_popup->get_y()),
		w - 30);
	filter_text->reposition_window(filter_text->get_x(), 
		h - (get_h() - filter_text->get_y()),
		w - (get_w() - filter_text->get_w()),
		1);
	textbox->reposition_window(textbox->get_x(), 
		h - (get_h() - textbox->get_y()),
		w - (get_w() - textbox->get_w()),
		1);
	listbox->reposition_window(listbox->get_x(),
		listbox->get_y(),
		w - (get_w() - listbox->get_w()),
		h - (get_h() - listbox->get_h()));
	icon_button->reposition_window(w - (get_w() - icon_button->get_x()), 
		icon_button->get_y());
	text_button->reposition_window(w - (get_w() - text_button->get_x()), 
		text_button->get_y());
	folder_button->reposition_window(w - (get_w() - folder_button->get_x()), 
		folder_button->get_y());
	updir_button->reposition_window(w - (get_w() - updir_button->get_x()), 
		updir_button->get_y());
	set_w(w);
	set_h(h);
	get_resources()->filebox_w = get_w();
	get_resources()->filebox_h = get_h();
	return 1;
}

int BC_FileBox::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
			if(ctrl_down()) set_done(1);
			return 1;
			break;
	}
	return 0;
}

int BC_FileBox::close_event()
{
	set_done(1);
	return 1;
}


int BC_FileBox::create_tables()
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
		new_item = new BC_ListBoxItem(file_item->name,
			icon, 
			is_dir ? DIRECTORY_COLOR : FILE_COLOR);
		if(is_dir) new_item->set_searchable(0);
		list_column[column_of_type(FILEBOX_NAME)].append(new_item);
	
// Size entry
		if(!is_dir)
		{
			sprintf(string, "%lld", file_item->size);
			new_item = new BC_ListBoxItem(string, FILE_COLOR);
		}
		else
		{
			new_item = new BC_ListBoxItem("", DIRECTORY_COLOR);
		}
 
 		list_column[column_of_type(FILEBOX_SIZE)].append(new_item);

// Date entry
		if(!is_dir || 1)
		{
			static char *month_text[13] = 
			{
				"Null",
				"Jan",
				"Feb",
				"Mar",
				"Apr",
				"May",
				"Jun",
				"Jul",
				"Aug",
				"Sep",
				"Oct",
				"Nov",
				"Dec"
			};
			sprintf(string, 
				"%s %d, %d", 
				month_text[file_item->month],
				file_item->day,
				file_item->year);
			new_item = new BC_ListBoxItem(string, FILE_COLOR);
		}
		else
		{
			new_item = new BC_ListBoxItem("", DIRECTORY_COLOR);
		}

		list_column[column_of_type(FILEBOX_DATE)].append(new_item);
	}
	
	return 0;
}

int BC_FileBox::delete_tables()
{
	for(int j = 0; j < FILEBOX_COLUMNS; j++)
	{
		list_column[j].remove_all_objects();
	}
	return 0;
}

BC_Pixmap* BC_FileBox::get_icon(char *path, int is_dir)
{
	char *suffix = strrchr(path, '.');
	int icon_type = ICON_UNKNOWN;

	if(is_dir) return icons[ICON_FOLDER];

	if(suffix)
	{
		suffix++;
		if(*suffix != 0)
		{
			for(int i = 0; i < TOTAL_SUFFIXES; i++)
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

char* BC_FileBox::columntype_to_text(int type)
{
	switch(type)
	{
		case FILEBOX_NAME:
			return FILEBOX_NAME_TEXT;
			break;
		case FILEBOX_SIZE:
			return FILEBOX_SIZE_TEXT;
			break;
		case FILEBOX_DATE:
			return FILEBOX_DATE_TEXT;
			break;
	}
	return "";
}

int BC_FileBox::column_of_type(int type)
{
	for(int i = 0; i < FILEBOX_COLUMNS; i++)
		if(column_type[i] == type) return i;
	return 0;
}



int BC_FileBox::refresh()
{
	create_tables();
	listbox->set_master_column(column_of_type(FILEBOX_NAME), 0);
	listbox->update(list_column, 
		column_titles, 
		column_width,
		FILEBOX_COLUMNS, 
		0, 
		0,
		-1, 
		1);

	return 0;
}

int BC_FileBox::update_filter(char *filter)
{
	fs->set_filter(filter);
	fs->update(0);
	refresh();
	strcpy(get_resources()->filebox_filter, filter);

	return 0;
}


void BC_FileBox::move_column(int src, int dst)
{
	ArrayList<BC_ListBoxItem*> *new_columns = 
		new ArrayList<BC_ListBoxItem*>[FILEBOX_COLUMNS];
	int *new_types = new int[FILEBOX_COLUMNS];
	int *new_widths = new int[FILEBOX_COLUMNS];

// Fill in remaining columns with consecutive data
	for(int out_column = 0, in_column = 0; 
		out_column < FILEBOX_COLUMNS; 
		out_column++,
		in_column++)
	{
// Copy destination column from src column
		if(out_column == dst)
		{
			for(int i = 0; i < list_column[src].total; i++)
			{
				new_columns[out_column].append(list_column[src].values[i]);
			}
			new_types[out_column] = column_type[src];
			new_widths[out_column] = column_width[src];
			in_column--;
		}
		else
		{
// Skip source column
			if(in_column == src) in_column++;
			for(int i = 0; i < list_column[src].total; i++)
			{
				new_columns[out_column].append(list_column[in_column].values[i]);
			}
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

	for(int i = 0; i < FILEBOX_COLUMNS; i++)
	{
		get_resources()->filebox_columntype[i] = column_type[i];
		get_resources()->filebox_columnwidth[i] = column_width[i];
		column_titles[i] = BC_FileBox::columntype_to_text(column_type[i]);
	}
	

	refresh();
}


int BC_FileBox::submit_file(char *path, int return_value, int use_this)
{
// blank.  Take the current directory as the desired file.
	if(!path[0])
	{
// save complete path
		strcpy(this->current_path, directory);          
// save complete path
		strcpy(this->submitted_path, directory);          
// Zero out filename
		filename[0] = 0;
		set_done(return_value);
		return 0;
	}

//printf("BC_FileBox::submit_file 1 %s\n", path);
// is a directory, change directories
	if(!fs->is_dir(path) && !use_this)
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
//printf("BC_FileBox::submit_file 1\n");
		fs->extract_dir(directory, path);     // save directory for defaults
		fs->extract_name(filename, path);     // save filename
		fs->complete_path(path);
		strcpy(this->current_path, path);          // save complete path
		strcpy(this->submitted_path, path);          // save complete path
		newfolder_thread->interrupt();
		set_done(return_value);
		return 0;
	}
	return 0;
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
	{
		return get_submitted_path();
	}
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
//printf("BC_FileBox::get_submitted_path 1 %s\n", submitted_path);
	return submitted_path;
}

char* BC_FileBox::get_current_path()
{
//printf("BC_FileBox::get_current_path 1 %s\n", current_path);
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



