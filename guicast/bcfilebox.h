#ifndef BCFILEBOX_H
#define BCFILEBOX_H

#include "bcbutton.h"
#include "bcfilebox.inc"
#include "bclistbox.h"
#include "bclistboxitem.inc"
#include "bcresources.inc"
#include "bctextbox.h"
#include "bcwindow.h"
#include "filesystem.inc"
#include "mutex.h"
#include "thread.h"

// Display modes
#define FILEBOX_LIST    0
#define FILEBOX_ICONS   1

#define FILEBOX_COLUMNS 2

class BC_NewFolder : public BC_Window
{
public:
	BC_NewFolder(int x, int y, BC_FileBox *filebox);
	~BC_NewFolder();

	int create_objects();
	char* get_text();

private:
	BC_TextBox *textbox;
};

class BC_NewFolderThread : public Thread
{
public:
	BC_NewFolderThread(BC_FileBox *filebox);
	~BC_NewFolderThread();

	void run();
	int interrupt();
	int start_new_folder();

private:
	Mutex change_lock, completion_lock;
	int active;
	BC_FileBox *filebox;
	BC_NewFolder *window;
};

class BC_FileBoxListBox : public BC_ListBox
{
public:
	BC_FileBoxListBox(int x, int y, BC_FileBox *filebox);
	virtual ~BC_FileBoxListBox();

	int handle_event();
	int selection_changed();
	int evaluate_query(int list_item, char *string);

	BC_FileBox *filebox;
};

class BC_FileBoxTextBox : public BC_TextBox
{
public:
	BC_FileBoxTextBox(int x, int y, BC_FileBox *filebox);
	~BC_FileBoxTextBox();

	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxOK : public BC_OKButton
{
public:
	BC_FileBoxOK(BC_FileBox *filebox);
	~BC_FileBoxOK();

	int handle_event();

	BC_FileBox *filebox;
};

class BC_FileBoxUseThis : public BC_Button
{
public:
	BC_FileBoxUseThis(BC_FileBox *filebox);
	~BC_FileBoxUseThis();
	int handle_event();

	BC_FileBox *filebox;
};

class BC_FileBoxCancel : public BC_CancelButton
{
public:
	BC_FileBoxCancel(BC_FileBox *filebox);
	~BC_FileBoxCancel();

	int handle_event();

	BC_FileBox *filebox;
};

class BC_FileBoxText : public BC_Button
{
public:
	BC_FileBoxText(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxFilterText : public BC_TextBox
{
public:
	BC_FileBoxFilterText(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxFilterMenu : public BC_ListBox
{
public:
	BC_FileBoxFilterMenu(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxIcons : public BC_Button
{
public:
	BC_FileBoxIcons(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxNewfolder : public BC_Button
{
public:
	BC_FileBoxNewfolder(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxUpdir : public BC_Button
{
public:
	BC_FileBoxUpdir(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
	char string[BCTEXTLEN];
};



class BC_FileBox : public BC_Window
{
public:
	BC_FileBox(int x, 
		int y,
		char *init_path,
		char *title,
		char *caption,
// Set to 1 to get hidden files. 
		int show_all_files = 0,
// Want only directories
		int want_directory = 0,
		int multiple_files = 0,
		int h_padding = 0);
	BC_FileBox(int x, 
		int y,
		int w,
		int h,
		char *init_path,
		char *title,
		char *caption,
// Set to 1 to get hidden files. 
		int show_all_files = 0,
// Want only directories
		int want_directory = 0,
		int multiple_files = 0,
		int h_padding = 0);
	virtual ~BC_FileBox();

	friend class BC_FileBoxCancel;
	friend class BC_FileBoxListBox;
	friend class BC_FileBoxTextBox;
	friend class BC_FileBoxText;
	friend class BC_FileBoxIcons;
	friend class BC_FileBoxNewfolder;
	friend class BC_FileBoxOK;
	friend class BC_NewFolderThread;
	friend class BC_FileBoxUpdir;
	friend class BC_FileBoxFilterText;
	friend class BC_FileBoxFilterMenu;
	friend class BC_FileBoxUseThis;

	virtual int create_objects();
	virtual int keypress_event();
	virtual int close_event();

// Give the most recently selected path
	char* get_path();
// Give the path of any selected item or 0
	char* get_path(int selection);
	int update_filter(char *filter);
	virtual int resize_event(int w, int h);
	char* get_newfolder_title();

private:
	int create_icons();
	int create_tables();
	int delete_tables();
	int submit_file(char *path, int return_value, int use_this = 0);
	int get_display_mode();
	int get_listbox_w();
	int get_listbox_h(int y);
	void create_listbox(int x, int y, int mode);
	BC_Pixmap* get_icon(char *path, int is_dir);    // Get the icon number for a listbox

	BC_Pixmap *icons[TOTAL_ICONS];
	FileSystem *fs;
	BC_FileBoxTextBox *textbox;
	BC_FileBoxListBox *listbox;
	BC_FileBoxFilterText *filter_text;
	BC_FileBoxFilterMenu *filter_popup;
	BC_Title *directory_title;
	BC_Button *icon_button, *text_button, *folder_button, *updir_button;
	BC_Button *ok_button, *cancel_button;
	BC_FileBoxUseThis *usethis_button;
	char caption[BCTEXTLEN];
	char path[BCTEXTLEN];
	char directory[BCTEXTLEN];
	char filename[BCTEXTLEN];
	char string[BCTEXTLEN];
	int want_directory;
	int select_multiple;
	static char *column_titles[FILEBOX_COLUMNS];
	int column_width[FILEBOX_COLUMNS];
	ArrayList<BC_ListBoxItem*> list_column[FILEBOX_COLUMNS];
	ArrayList<BC_ListBoxItem*> filter_list;
	char new_folder_title[BCTEXTLEN];
	BC_NewFolderThread *newfolder_thread;
	int h_padding;
};

#endif
