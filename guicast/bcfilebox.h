// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCFILEBOX_H
#define BCFILEBOX_H

#include "bcbutton.h"
#include "bcdelete.inc"
#include "bcfilebox.inc"
#include "bclistbox.inc"
#include "bclistboxitem.inc"
#include "bcnewfolder.inc"
#include "bcresources.inc"
#include "bctextbox.h"
#include "bcwindow.h"
#include "condition.inc"
#include "filesystem.inc"
#include "mutex.inc"
#include "thread.h"

class BC_FileBoxListBox : public BC_ListBox
{
public:
	BC_FileBoxListBox(int x, int y, BC_FileBox *filebox);
	virtual ~BC_FileBoxListBox() {};

	int handle_event();
	void selection_changed();
	int column_resize_event();
	int sort_order_event();
	int move_column_event();
	int evaluate_query(int list_item, char *string);

	BC_FileBox *filebox;
};

class BC_FileBoxTextBox : public BC_TextBox
{
public:
	BC_FileBoxTextBox(int x, int y, BC_FileBox *filebox);

	BC_FileBox *filebox;
};

class BC_FileBoxOK : public BC_OKButton
{
public:
	BC_FileBoxOK(BC_FileBox *filebox);

	int handle_event();

	BC_FileBox *filebox;
};

class BC_FileBoxUseThis : public BC_Button
{
public:
	BC_FileBoxUseThis(BC_FileBox *filebox);

	int handle_event();

	BC_FileBox *filebox;
};

class BC_FileBoxCancel : public BC_CancelButton
{
public:
	BC_FileBoxCancel(BC_FileBox *filebox);

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

class BC_FileBoxDirectoryText : public BC_TextBox
{
public:
	BC_FileBoxDirectoryText(int x, int y, BC_FileBox *filebox);
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

class BC_FileBoxDelete : public BC_Button
{
public:
	BC_FileBoxDelete(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};

class BC_FileBoxReload : public BC_Button
{
public:
	BC_FileBoxReload(int x, int y, BC_FileBox *filebox);
	int handle_event();
	BC_FileBox *filebox;
};


class BC_FileBoxRecent : public BC_ListBox
{
public:
	BC_FileBoxRecent(BC_FileBox *filebox, int x, int y);
	int handle_event();
	BC_FileBox *filebox;
};


class BC_FileBox : public BC_Window
{
public:
	BC_FileBox(int x, int y, const char *init_path,
		const char *title, const char *caption,
// Set to 1 to get hidden files.
		int show_all_files = 0,
// Want only directories
		int want_directory = 0,
		int multiple_files = 0,
		int h_padding = -1);
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
	friend class BC_DeleteThread;
	friend class BC_FileBoxDelete;
	friend class BC_FileBoxReload;
	friend class BC_FileBoxRecent;

	virtual int keypress_event();

// When file is submitted this is called for the user to retrieve it before the
// window is deleted.
	virtual int handle_event() { return 0; };

	void create_history();
	void update_history();
	void refresh();

// The OK and Use This button submits a path.
// The cancel button has a current path highlighted but possibly different from the
// path actually submitted.
// Give the most recently submitted path
	char* get_submitted_path();
// Give the path currently highlighted
	char* get_current_path();

// Give the path of any selected item or 0.  Used when many items are
// selected in the list.  Should only be called when OK is pressed.
	char* get_path(int selection);
	void update_filter(const char *filter);
	virtual void resize_event(int w, int h);
	char* get_newfolder_title();
	char* get_delete_title();
	void delete_files();
	BC_Button* get_ok_button();
	BC_Button* get_cancel_button();
	FileSystem *fs;

private:
	void create_icons();
	void extract_extension(char *out, const char *in);
	void create_tables();
	void delete_tables();
// Called by directory history menu to change directories but leave
// filename untouched.
	void submit_dir(char *dir);
	int submit_file(char *path, int use_this = 0);
// Called by move_column_event
	void move_column(int src, int dst);
	int get_display_mode();
	int get_listbox_w();
	int get_listbox_h(int y);
	void create_listbox(int x, int y, int mode);
// Get the icon number for a listbox
	BC_Pixmap* get_icon(char *path, int is_dir);
	const char* columntype_to_text(int type);
// Get the column whose type matches type.
	int column_of_type(int type);

	BC_Pixmap *icons[TOTAL_ICONS];
	BC_FileBoxRecent *recent_popup;
	BC_FileBoxTextBox *textbox;
	BC_FileBoxListBox *listbox;
	BC_FileBoxFilterText *filter_text;
	BC_FileBoxFilterMenu *filter_popup;
	BC_TextBox *directory_title;
	BC_Button *icon_button, *text_button, *folder_button, *updir_button, *delete_button, *reload_button;
	BC_Button *ok_button, *cancel_button;
	BC_FileBoxUseThis *usethis_button;
	char caption[BCTEXTLEN];
	char current_path[BCTEXTLEN];
	char submitted_path[BCTEXTLEN];
	char directory[BCTEXTLEN];
	char filename[BCTEXTLEN];
	char string[BCTEXTLEN];
	int want_directory;
	int select_multiple;

	int sort_column;
	int sort_order;

	const char *column_titles[FILEBOX_COLUMNS];
	ArrayList<BC_ListBoxItem*> filter_list;
	ArrayList<BC_ListBoxItem*> *list_column;
	int *column_type;
	int *column_width;
// Calculated based on directory or regular file searching
	int columns;

	char new_folder_title[BCTEXTLEN];
	BC_NewFolderThread *newfolder_thread;
	BC_DeleteThread *delete_thread;
	int h_padding;
	ArrayList<BC_ListBoxItem*> recent_dirs;
};

#endif
