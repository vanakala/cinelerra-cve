#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>

#include "filesystem.h"

FileItem::FileItem()
{
	path = 0;
	name = 0;
	reset();
}

FileItem::FileItem(char *path, 
	char *name, 
	int is_dir, 
	longest size, 
	int month, 
	int day, 
	int year,
	longest calendar_time)
{
	this->path = new char[strlen(path)];
	this->name = new char[strlen(name)];
	if(this->path) strcpy(this->path, path);
	if(this->name) strcpy(this->name, name);
	this->is_dir = is_dir;
	this->size = size;
	this->month = month;
	this->day = day;
	this->year = year;
	this->calendar_time = calendar_time;
}

FileItem::~FileItem()
{
	reset();
}

int FileItem::reset()
{
	if(this->path) delete [] this->path;
	if(this->name) delete [] this->name;
	path = 0;
	name = 0;
	is_dir = 0;
	size = 0;
	month = 0;
	day = 0;
	year = 0;
	calendar_time = 0;
	return 0;
}

int FileItem::set_path(char *path)
{
	if(this->path) delete [] this->path;
	this->path = new char[strlen(path) + 1];
	strcpy(this->path, path);
	return 0;
}

int FileItem::set_name(char *name)
{
	if(this->name) delete [] this->name;
	this->name = new char[strlen(name) + 1];
	strcpy(this->name, name);
	return 0;
}


FileSystem::FileSystem()
{
// 	if(need_home)
// 	{
// // ========================= set to user's home directory
//   		uid_t uid;
//   		struct passwd *pw;
// 
//   		uid = getuid();             /* get user id */
//   		pw = getpwuid(uid);         /* get info for user */
//   		sprintf(current_dir, "%s", pw->pw_dir);
// 
//  	}
// ========================= set to current directory
	reset_parameters();
	getcwd(current_dir, BCTEXTLEN);
}

FileSystem::~FileSystem()
{
	delete_directory();
}

int FileSystem::reset_parameters()
{
 	show_all_files = 0;
	want_directory = 0;
	strcpy(filter, "");
	strcpy(current_dir, "");
	return 0;
}

int FileSystem::delete_directory()
{
	for(int i = 0; i < dir_list.total; i++)
	{
		delete dir_list.values[i];
	}
	dir_list.remove_all();
	return 0;
}



int FileSystem::sort(ArrayList<FileItem*> *dir_list)
{
	int changed;
	FileItem *temp;
	int i;
	
	changed = 1;
	while(changed)
	{
		changed = 0;
		for(i = 0; i < dir_list->total - 1; i++)
		{
			if(strcmp(dir_list->values[i]->name, dir_list->values[i + 1]->name) > 0)
			{
				temp = dir_list->values[i];
				dir_list->values[i] = dir_list->values[i+1];
				dir_list->values[i+1] = temp;
				changed = 1;
			}
		}
	}
	return 0;
}

int FileSystem::combine(ArrayList<FileItem*> *dir_list, ArrayList<FileItem*> *file_list)
{
	int i;
	FileItem *new_entry, *entry;
	
	sort(dir_list);
	for(i = 0; i < dir_list->total; i++)
	{
		this->dir_list.append(dir_list->values[i]);
	}

	sort(file_list);
	for(i = 0; i < file_list->total; i++)
	{
		this->dir_list.append(file_list->values[i]);
	}
	return 0;
}

int FileSystem::is_root_dir(char *path)
{
	if(!strcmp(current_dir, "/")) return 1;
	return 0;
}

int FileSystem::test_filter(FileItem *file)
{
	char *filter1 = 0, *filter2 = filter, *subfilter1, *subfilter2;
	int total_filters = 0;
	int result = 0;
	int done = 0, token_done;
	int token_number = 0;

// Don't filter directories
	if(file->is_dir) return 0;
// Empty filename string
	if(!file->name) return 1;

	do
	{
// Get next token
		filter1 = strchr(filter2, '[');
		string[0] = 0;

// Get next filter
		if(filter1)
		{
			filter1++;
			filter2 = strchr(filter1, ']');

			if(filter2)
			{
				int i;
				for(i = 0; filter1 + i < filter2; i++)
					string[i] = filter1[i];
				string[i] = 0;
			}
			else
			{
				strcpy(string, filter1);
				done = 1;
			}
		}
		else
		{
			if(!token_number) 
				strcpy(string, filter);
			else
				done = 1;
		}

// Process the token
		if(string[0] != 0)
		{
			char *path = file->name;
			subfilter1 = string;
			token_done = 0;
			result = 0;

			do
			{
				string2[0] = 0;
				subfilter2 = strchr(subfilter1, '*');

				if(subfilter2)
				{
					int i;
					for(i = 0; subfilter1 + i < subfilter2; i++)
						string2[i] = subfilter1[i];
					
					string2[i] = 0;
				}
				else
				{
					strcpy(string2, subfilter1);
					token_done = 1;
				}

				if(string2[0] != 0)
				{
// Subfilter must exist at some later point in the string
					if(subfilter1 > string)
					{
						if(!strstr(path, string2)) 
						{
							result = 1;
							token_done = 1;
						}
						else
						path = strstr(path, string2) + strlen(string2);
					}
					else
// Subfilter must exist at this point in the string
					{
						if(strncasecmp(path, string2, strlen(string2))) 
						{
							result = 1;
							token_done = 1;
						}
						else
						path += strlen(string2);
					}

// String must terminate after subfilter
					if(!subfilter2)
					{
						if(*path != 0)
						{
							result = 1;
							token_done = 1;
						}
					}
				}
				subfilter1 = subfilter2 + 1;
// Let pass if no subfilter
			}while(!token_done && !result);
		}
		token_number++;
	}while(!done && result);

	return result;
}


int FileSystem::update(char *new_dir)
{
	DIR *dirstream;
	struct dirent64 *new_filename;
	struct stat ostat;
	struct tm *mod_time;
	int i, j, k, include_this;
	FileItem *new_file;
	char full_path[BCTEXTLEN], name_only[BCTEXTLEN];
	ArrayList<FileItem*>directories;
	ArrayList<FileItem*>files;

	delete_directory();
	if(new_dir != 0) strcpy(current_dir, new_dir);
	dirstream = opendir(current_dir);
	if(!dirstream) return 1;          // failed to open directory

	while(new_filename = readdir64(dirstream))
	{
		include_this = 1;

// File is directory heirarchy
		if(!strcmp(new_filename->d_name, ".") || 
			!strcmp(new_filename->d_name, "..")) include_this = 0;

// File is hidden and we don't want all files
		if(include_this && !show_all_files && new_filename->d_name[0] == '.') include_this = 0;

// file not hidden
  		if(include_this)
    	{
			new_file = new FileItem;
			sprintf(full_path, "%s", current_dir);
			if(!is_root_dir(current_dir)) strcat(full_path, "/");
			strcat(full_path, new_filename->d_name);
			strcpy(name_only, new_filename->d_name);
			new_file->set_path(full_path);
			new_file->set_name(name_only);

// Get information about the file.
			if(!stat(full_path, &ostat))
			{
				new_file->size = ostat.st_size;
				mod_time = localtime(&(ostat.st_mtime));
				new_file->month = mod_time->tm_mon + 1;
				new_file->day = mod_time->tm_mday;
				new_file->year = mod_time->tm_year + 1900;
				new_file->calendar_time = ostat.st_mtime;

				if(S_ISDIR(ostat.st_mode))
				{
					strcat(name_only, "/"); // is a directory
					new_file->is_dir = 1;
				}

// File is excluded from filter
				if(include_this && test_filter(new_file)) include_this = 0;
//printf("FileSystem::update 3 %d %d\n", include_this, test_filter(new_file));

// File is not a directory and we just want directories
				if(include_this && want_directory && !new_file->is_dir) include_this = 0;
			}
			else
			{
//printf("FileSystem::update 3 %s\n", full_path);
				include_this = 0;
			}

// add to list
			if(include_this)
			{
				if(new_file->is_dir) directories.append(new_file);
 				else files.append(new_file);
			}
			else
				delete new_file;
		}
	}

	closedir(dirstream);
// combine the directories and files in the master list
	combine(&directories, &files);
// remove pointers
	directories.remove_all();
	files.remove_all();

	return 0;           // success
}

int FileSystem::set_filter(char *new_filter)
{
	strcpy(filter, new_filter);
	return 0;
}

int FileSystem::set_show_all()
{
	show_all_files = 1;
	return 0;
}

int FileSystem::set_want_directory()
{
	want_directory = 1;
	return 0;
}

int FileSystem::is_dir(const char *path)      // return 0 if the text is a directory
{
	if(!strlen(path)) return 0;

	char new_dir[BCTEXTLEN];
	struct stat ostat;    // entire name is a directory

//printf("FileSystem::is_dir 1\n");
	strcpy(new_dir, path);
//printf("FileSystem::is_dir 1\n");
	complete_path(new_dir);
//printf("FileSystem::is_dir 1 %s\n", new_dir);
	if(!stat(new_dir, &ostat) && S_ISDIR(ostat.st_mode)) return 0;
//printf("FileSystem::is_dir 2\n");
	return 1;
}

int FileSystem::create_dir(char *new_dir_)
{
	char new_dir[BCTEXTLEN];
	strcpy(new_dir, new_dir_);
	complete_path(new_dir);

	mkdir(new_dir, S_IREAD | S_IWRITE | S_IEXEC);
	return 0;
}

int FileSystem::parse_tildas(char *new_dir)
{
	if(new_dir[0] == 0) return 1;

// Our home directory
	if(new_dir[0] == '~')
	{

		if(new_dir[1] == '/' || new_dir[1] == 0)
		{
// user's home directory
			char *home;
			char string[BCTEXTLEN];
			home = getenv("HOME");

// print starting after tilda
			if(home) sprintf(string, "%s%s", home, &new_dir[1]);
			strcpy(new_dir, string);
			return 0;
		}
		else
// Another user's home directory
		{                
			char string[BCTEXTLEN], new_user[BCTEXTLEN];
			struct passwd *pw;
			int i, j;
      
			for(i = 1, j = 0; new_dir[i] != 0 && new_dir[i] != '/'; i++, j++)
			{                // copy user name
				new_user[j] = new_dir[i];
			}
			new_user[j] = 0;
      
			setpwent();
			while(pw = getpwent())
			{
// get info for user
				if(!strcmp(pw->pw_name, new_user))
				{
// print starting after tilda
      				sprintf(string, "%s%s", pw->pw_dir, &new_dir[i]);
      				strcpy(new_dir, string);
      				break;
      			}
			}
			endpwent();
			return 0;
		}
	}
	return 0;
}

int FileSystem::parse_directories(char *new_dir)
{
//printf("FileSystem::parse_directories 1 %s\n", new_dir);
	if(new_dir[0] != '/')
	{
// extend path completely
		char string[BCTEXTLEN];
//printf("FileSystem::parse_directories 2 %s\n", current_dir);
		if(!strlen(current_dir))
		{
// no current directory
			strcpy(string, new_dir);
		}
		else
		if(!is_root_dir(current_dir))
		{
// current directory is not root
			if(current_dir[strlen(current_dir) - 1] == '/')
// current_dir already has ending /
			sprintf(string, "%s%s", current_dir, new_dir);
			else
// need ending /
			sprintf(string, "%s/%s", current_dir, new_dir);
		}
		else
			sprintf(string, "%s%s", current_dir, new_dir);
		
//printf("FileSystem::parse_directories 3 %s %s\n", new_dir, string);
		strcpy(new_dir, string);
//printf("FileSystem::parse_directories 4\n");
	}
	return 0;
}

int FileSystem::parse_dots(char *new_dir)
{
// recursively remove ..s
	int changed = 1;
	while(changed)
	{
		int i, j, len;
		len = strlen(new_dir);
		changed = 0;
		for(i = 0, j = 1; !changed && j < len; i++, j++)
		{
			if(new_dir[i] == '.' && new_dir[j] == '.')
			{
				changed = 1;
				while(new_dir[i] != '/' && i > 0)
				{
// look for first / before ..
					i--;
				}

// find / before this /
				if(i > 0) i--;  
				while(new_dir[i] != '/' && i > 0)
				{
// look for first / before first / before ..
					i--;
				}

// i now equals /first filename before ..
// look for first / after ..
				while(new_dir[j] != '/' && j < len)
				{
					j++;
				}

// j now equals /first filename after ..
				while(j < len)
				{
					new_dir[i++] = new_dir[j++];
				}

				new_dir[i] = 0;
// default to root directory
				if((new_dir[0]) == 0) sprintf(new_dir, "/");
				break;
			}
		}
	}
	return 0;
}

int FileSystem::complete_path(char *filename)
{
//printf("FileSystem::complete_path 1\n");
	if(!strlen(filename)) return 1;
//printf("FileSystem::complete_path 1\n");
	parse_tildas(filename);
//printf("FileSystem::complete_path 1\n");
	parse_directories(filename);
//printf("FileSystem::complete_path 1\n");
	parse_dots(filename);
// don't add end slash since this requires checking if dir
//printf("FileSystem::complete_path 2\n");
	return 0;
}

int FileSystem::extract_dir(char *out, const char *in)
{
	strcpy(out, in);
	if(is_dir(in))
	{
// complete string is not directory
		int i;

		complete_path(out);

		for(i = strlen(out); i > 0 && out[i] != '/'; i--)
		{
			;
		}
		if(i >= 0) out[i] = 0;
	}
	return 0;
}

int FileSystem::extract_name(char *out, const char *in, int test_dir)
{
	int i;

	if(test_dir && !is_dir(in))
		sprintf(out, "");    // complete string is directory
	else
	{
		for(i = strlen(in)-1; i > 0 && in[i] != '/'; i--)
		{
			;
		}
		if(in[i] == '/') i++;
		strcpy(out, &in[i]);
	}
	return 0;
}

int FileSystem::join_names(char *out, char *dir_in, char *name_in)
{
	strcpy(out, dir_in);
	int len = strlen(out);
	int result = 0;
	
	while(!result)
		if(len == 0 || out[len] != 0) result = 1; else len--;
	
	if(len != 0)
	{
		if(out[len] != '/') strcat(out, "/");
	}
	
	strcat(out, name_in);
	return 0;
}

long FileSystem::get_date(char *filename)
{
	struct stat file_status;
	stat (filename, &file_status);
	return file_status.st_mtime;
}

longest FileSystem::get_size(char *filename)
{
	struct stat file_status;
	stat(filename, &file_status);
	return file_status.st_size;
}

int FileSystem::change_dir(char *new_dir)
{
	char new_dir_full[BCTEXTLEN];
	
	strcpy(new_dir_full, new_dir);

	complete_path(new_dir_full);
// cut ending slash
	if(strcmp(new_dir_full, "/") && 
		new_dir_full[strlen(new_dir_full) - 1] == '/') 
		new_dir_full[strlen(new_dir_full) - 1] = 0;
	update(new_dir_full);
	return 0;
}

int FileSystem::set_current_dir(char *new_dir)
{
	strcpy(current_dir, new_dir);
	return 0;
}

int FileSystem::add_end_slash(char *new_dir)
{
	if(new_dir[strlen(new_dir) - 1] != '/') strcat(new_dir, "/");
	return 0;
}

char* FileSystem::get_current_dir()
{
	return current_dir;
}

int FileSystem::total_files()
{
	return dir_list.total;
}


FileItem* FileSystem::get_entry(int entry)
{
	return dir_list.values[entry];
}
