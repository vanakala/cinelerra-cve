// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "arraylist.h"
#include "bcwindowbase.inc"

class FileItem
{
public:
	FileItem();
	FileItem(const char *path, 
		const char *name, 
		int is_dir, 
		off_t size, 
		int month, 
		int day, 
		int year,
		time_t calendar_time);
	~FileItem();

	void set_path(const char *path);
	void set_name(const char *name);
	void reset();
	char *path;
	char *name;
	int is_dir;
	off_t size;
	int month;
	int day;
	int year;
	time_t calendar_time;
};

class FileSystem
{
public:
// sets the working directory to the user
	FileSystem();
	virtual ~FileSystem();

// Load the new directory and change current_dir to it.
// This does not complete the dir path.
// If any of the files failed to stat, it returns nonzero.
	int update(const char *new_dir = 0);

// Complete the path in the string and change to the directory in the string.
	void change_dir(const char *new_dir);

// Set the current_dir to something without completing the path.
	void set_current_dir(const char *new_dir);

	int move_up();
	char *get_current_dir();
// Syntax of filter is
// single filter without [].
// multiple filters enclosed in [].
	void set_filter(const char *new_filter);
	void set_show_all();     // show hidden files
	void set_want_directory();
	void set_sort_order(int value);
	void set_sort_field(int field);
	void create_dir(const char *new_dir_);    // create a new directory
	int complete_path(char *filename);   // use the filename and the current_dir to create a complete filename
// return 1 if the text is a directory
	int is_dir(const char *new_dir_);
	void extract_dir(char *out, const char *in);    // extract the directory from the path
	void extract_name(char *out, const char *in, int test_dir = 1);	// extract the name from the path
	void join_names(char *out, const char *dir_in, 
		const char *name_in);    // combine a directory and filename
	static time_t get_date(const char *filename);  // get the date of the filename modification
	static off_t get_size(const char *filename);  // Get the number of bytes in the file.
	void add_end_slash(char *new_dir);
	int total_files();
	FileItem* get_entry(int entry);

	int parse_tildas(char *new_dir);     // expand tildas
	void parse_directories(char *new_dir);  // add directories
	void parse_dots(char *new_dir);         // move up directory tree after expanding tildas

// Alphabetize all the directories and files.  By default
// directories come first.
	void alphabetize();

// Array of files and directories in the directory pointed to by current_dir.
// Directories are first.
	ArrayList<FileItem*> dir_list;

// Sorting order and sorting field.  These are identical in BC_ListBox.
	enum
	{
		SORT_ASCENDING,
		SORT_DESCENDING
	};

// Match column definitions in BC_FileBox.
	enum
	{
		SORT_PATH,
		SORT_SIZE,
		SORT_DATE,
		SORT_EXTENSION
	};

private:
	void dot_reverse_filename(char *out, const char *in);
	int compare_items(ArrayList<FileItem*> *dir_list, int item1, int item2);
	void sort_table(ArrayList<FileItem*> *dir_list);


// Combine the directories and files into the master list, directories first.
	void combine(ArrayList<FileItem*> *dir_list, ArrayList<FileItem*> *file_list);
// Return whether or not the string is the root directory.
	static int is_root_dir(const char *path);
// Whether or not the file passes the current filter.
	int test_filter(FileItem *file);
	void reset_parameters();
	void delete_directory();
	char filter[BCTEXTLEN];     // what filenames have to end in to get displayed
	int want_directory;
	int show_all_files;       // shows . files
	char current_dir[BCTEXTLEN];
	char string[BCTEXTLEN], string2[BCTEXTLEN];
	int sort_order;
	int sort_field;
};

#endif
