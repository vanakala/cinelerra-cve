#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "arraylist.h"
#include "bcwindowbase.inc"
#include "sizes.h"

class FileItem
{
public:
	FileItem();
	FileItem(char *path, 
		char *name, 
		int is_dir, 
		int64_t size, 
		int month, 
		int day, 
		int year,
		int64_t calendar_time);
	~FileItem();

	int set_path(char *path);
	int set_name(char *name);
	int reset();
	char *path;
	char *name;
	int is_dir;
	int64_t size;
	int month;
	int day;
	int year;
	int64_t calendar_time;
};

class FileSystem
{
public:
// sets the working directory to the user
	FileSystem();
	virtual ~FileSystem();

// Load the new directory and change current_dir to it.
// This does not complete the dir path.
	int update(char *new_dir = 0);

// Complete the path in the string and change to the directory in the string.
// Does not change new_dir
	int change_dir(char *new_dir);
// Set the current_dir to something without completing the path.
	int set_current_dir(char *new_dir);

	int move_up();
	char *get_current_dir();
// Syntax of filter is
// single filter without [].
// multiple filters enclosed in [].
	int set_filter(char *new_filter);
	int set_show_all();     // show hidden files
	int set_want_directory();
	int set_sort_order(int value);
	int set_sort_field(int field);
	int create_dir(char *new_dir_);    // create a new directory
	int complete_path(char *filename);   // use the filename and the current_dir to create a complete filename
	int is_dir(const char *new_dir_);	  // return 0 if the text is a directory
	int extract_dir(char *out, const char *in);    // extract the directory from the path
	int extract_name(char *out, const char *in, int test_dir = 1);	// extract the name from the path
	int join_names(char *out, char *dir_in, char *name_in);    // combine a directory and filename
	long get_date(char *filename);        // get the date of the filename modification
	int64_t get_size(char *filename);        // Get the number of bytes in the file.
	int add_end_slash(char *new_dir);
	int total_files();
	FileItem* get_entry(int entry);

	int parse_tildas(char *new_dir);     // expand tildas
	int parse_directories(char *new_dir);  // add directories
	int parse_dots(char *new_dir);         // move up directory tree after expanding tildas

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
		SORT_DATE
	};

private:
	int compare_items(ArrayList<FileItem*> *dir_list, int item1, int item2);
	int sort_table(ArrayList<FileItem*> *dir_list);


// Combine the directories and files into the master list, directories first.
	int combine(ArrayList<FileItem*> *dir_list, ArrayList<FileItem*> *file_list);
// Return whether or not the string is the root directory.
	int is_root_dir(char *path);
// Whether or not the file passes the current filter.
	int test_filter(FileItem *file);
	int reset_parameters();
	int delete_directory();
	char filter[BCTEXTLEN];     // what filenames have to end in to get displayed
	int want_directory;
	int show_all_files;       // shows . files
	char current_dir[BCTEXTLEN];
	char string[BCTEXTLEN], string2[BCTEXTLEN];
	int sort_order;
	int sort_field;
};

#endif
