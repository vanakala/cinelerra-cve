#ifndef INDEXFILE_H
#define INDEXFILE_H

#include "asset.inc"
#include "edit.inc"
#include "file.inc"
#include "guicast.h"
#include "indexthread.inc"
#include "mainprogress.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "resourcepixmap.inc"
#include "bctimer.inc"
#include "tracks.inc"

class IndexFile
{
public:
	IndexFile(MWindow *mwindow);
	IndexFile(MWindow *mwindow, Asset *asset);
	~IndexFile();

	int open_index(Asset *asset);
	int open_index(MWindow *mwindow, Asset *asset);
	int create_index(Asset *asset, MainProgressBar *progress);
	int create_index(MWindow *mwindow, Asset *asset, MainProgressBar *progress);
	int interrupt_index();
	static void delete_index(Preferences *preferences, Asset *asset);
	static int get_index_filename(char *source_filename, 
		char *index_directory, 
		char *index_filename, 
		char *input_filename);
	void update_edl_asset();
	int redraw_edits(int force);
	int draw_index(ResourcePixmap *pixmap, Edit *edit, int x, int w);
	int close_index();
	int remove_index();
	int read_info(Asset *test_asset = 0);
	int write_info();

	MWindow *mwindow;
	char index_filename[BCTEXTLEN], source_filename[BCTEXTLEN];
	Asset *asset;
	Timer *redraw_timer;

private:
	void update_mainasset();

	int open_file();
	int open_source(File *source);
	int64_t get_required_scale(File *source);
	FILE *file;
	int64_t file_length;   // Length of index file in bytes
	int interrupt_flag;    // Flag set when index building is interrupted
};

#endif
