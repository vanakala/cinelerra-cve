#include "bcfilebox.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bctitle.h"
#include "clip.h"
#include "filesystem.h"
#include "bclistboxitem.h"
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
	add_tool(new BC_Title(x, y, "Enter the name of the folder:"));
	y += 20;
	add_subwindow(textbox = new BC_TextBox(x, y, 300, 1, "Untitled"));
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
{
	this->filebox = filebox;
	active = 0;
	set_synchronous(0);
}

BC_NewFolderThread::~BC_NewFolderThread() 
{
}

void BC_NewFolderThread::run()
{
	int result = window->run_window();

	if(!result)
	{
		char new_folder[BCTEXTLEN];
		filebox->fs->join_names(new_folder, filebox->fs->get_current_dir(), window->get_text());
		mkdir(new_folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	}

	change_lock.lock();
	active = 0;
	change_lock.unlock();
	delete window;
	completion_lock.unlock();
}

int BC_NewFolderThread::interrupt()
{
	change_lock.lock();
	if(active)
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
	window = new BC_NewFolder(filebox->get_abs_cursor_x(), filebox->get_abs_cursor_y(), filebox);
	window->create_objects();

	change_lock.lock();
	active = 1;
	change_lock.unlock();

	Thread::start();

	completion_lock.lock();
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
}

BC_FileBoxListBox::~BC_FileBoxListBox() {}

int BC_FileBoxListBox::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text(), 0);
	return 1;
}

int BC_FileBoxListBox::selection_changed()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	if(item)
		filebox->textbox->update(item->get_text());
	return 0;
}


int BC_FileBoxListBox::evaluate_query(int list_item, char *string)
{
	return(filebox->list_column[0].values[list_item]->get_color() != DIRECTORY_COLOR && 
		strcmp(string, filebox->list_column[0].values[list_item]->get_text()) <= 0);
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
	set_tooltip("Change the filter");
}

int BC_FileBoxFilterMenu::handle_event()
{
	filebox->filter_text->update(get_selection(0, 0)->get_text());
	filebox->update_filter(get_selection(0, 0)->get_text());
	return 0;
}










BC_FileBoxCancel::BC_FileBoxCancel(BC_FileBox *filebox)
 : BC_CancelButton(filebox)
{
	this->filebox = filebox;
	set_tooltip("Cancel the operation");
}

BC_FileBoxCancel::~BC_FileBoxCancel()
{
}

int BC_FileBoxCancel::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text(), 0);
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
	set_tooltip("Submit the directory");
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
	set_tooltip("Submit the file");
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
	set_tooltip("Display text");
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
	set_tooltip("Display icons");
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
	set_tooltip("Create new folder");
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
	set_tooltip("Up a directory");
}
int BC_FileBoxUpdir::handle_event()
{
// Need a temp so submit_file can expand it
	sprintf(string, "..");
	filebox->submit_file(string, 0);
	return 1;
}








char* BC_FileBox::column_titles[FILEBOX_COLUMNS] = 
{
	"File",
	"Size"
};

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
	strcpy(this->caption, caption);

	strcpy(this->path, init_path);
	if(show_all_files) fs->set_show_all();
	if(want_directory) fs->set_want_directory();
	this->want_directory = want_directory;
	select_multiple = multiple_files;
	fs->complete_path(this->path);
	fs->extract_dir(directory, this->path);
	fs->extract_name(filename, this->path);

// Test directory
	if(fs->update(directory))
	{
		sprintf(this->path, "~");
		fs->complete_path(this->path);
		fs->update(this->path);
		strcpy(directory, fs->get_current_dir());
		sprintf(filename, "");
	}
	this->h_padding = h_padding;
}

BC_FileBox::~BC_FileBox()
{
	delete fs;
	delete_tables();
	for(int i = 0; i < TOTAL_ICONS; i++)
		delete icons[i];
	delete newfolder_thread;
}

int BC_FileBox::create_objects()
{
	int x = 10, y = 10;

	column_width[0] = get_listbox_w() - 120;
	column_width[1] = 120;
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

	newfolder_thread = new BC_NewFolderThread(this);
	listbox->activate();
	
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

//printf("BC_FileBox::resize_event %d %d\n", ok_button->get_w(), ok_button->get_h());
	ok_button->reposition_window(ok_button->get_x(), 
		h - (get_h() - ok_button->get_y()));
	cancel_button->reposition_window(w - (get_w() - cancel_button->get_x()), h - (get_h() - cancel_button->get_y()));
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

	fs->update(0);
	for(int i = 0; i < fs->total_files(); i++)
	{
		int is_dir = fs->get_entry(i)->is_dir;
		BC_Pixmap* icon = get_icon(fs->get_entry(i)->name, is_dir);

// Name entry
		new_item = new BC_ListBoxItem(fs->get_entry(i)->name,
			icon, 
			is_dir ? DIRECTORY_COLOR : FILE_COLOR);
		list_column[0].append(new_item);
		if(is_dir) new_item->set_searchable(0);

// Size entry
		if(!is_dir)
		{
			sprintf(string, "%lld", fs->get_entry(i)->size);
			new_item = new BC_ListBoxItem(string, FILE_COLOR);
		}
		else
		{
			new_item = new BC_ListBoxItem("", FILE_COLOR);
		}
 
 		list_column[1].append(new_item);
	}
	return 0;
}

int BC_FileBox::delete_tables()
{
	for(int j = 0; j < FILEBOX_COLUMNS; j++)
	{
		for(int i = 0; i < list_column[0].total; i++)
			delete list_column[j].values[i];
		list_column[j].remove_all();
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

int BC_FileBox::update_filter(char *filter)
{
	fs->set_filter(filter);
	fs->update();
	create_tables();
	listbox->update(list_column, 
		column_titles, 
		column_width,
		FILEBOX_COLUMNS, 
		0, 
		0,
		-1, 
		1);
	strcpy(get_resources()->filebox_filter, filter);

	return 0;
}

int BC_FileBox::submit_file(char *path, int return_value, int use_this)
{
	if(!path[0]) return 1;   // blank

// is a directory, change directories
	if(!fs->is_dir(path) && !use_this)
	{
		fs->change_dir(path);
		create_tables();
		listbox->update(list_column, 
			column_titles, 
			column_width,
			FILEBOX_COLUMNS, 
			0, 
			0, 
			-1, 
			1);
		directory_title->update(fs->get_current_dir());
		strcpy(this->path, fs->get_current_dir());
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
		strcpy(this->path, path);          // save complete path
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
		return get_path();
	}
	else
	{
		BC_ListBoxItem *item = listbox->get_selection(0, selection - 1);
		if(item) 
		{
			fs->join_names(string, directory, item->get_text());
			return string;
		}
	}
	return 0;
}

char* BC_FileBox::get_path()
{
	return path;
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

	strcat(new_folder_title, ": New folder");

	return new_folder_title;
}



