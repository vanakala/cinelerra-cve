#include "deleteallindexes.h"
#include "filesystem.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "question.h"
#include "theme.h"
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

DeleteAllIndexes::DeleteAllIndexes(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y)
 : BC_GenericButton(x, y, _("Delete existing indexes")), Thread()
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

DeleteAllIndexes::~DeleteAllIndexes() 
{
}

int DeleteAllIndexes::handle_event() 
{ 
	start(); 
}

static int test_filter(char *string, char *filter)
{
	return (strlen(string) > strlen(filter) &&
			!strcmp(string + strlen(string) - strlen(filter), filter));
}

void DeleteAllIndexes::run()
{
	char string[BCTEXTLEN], string1[BCTEXTLEN], string2[BCTEXTLEN];
// prepare directory
	strcpy(string1, pwindow->thread->preferences->index_directory);
	FileSystem dir;
	dir.update(pwindow->thread->preferences->index_directory);
	dir.complete_path(string1);
// prepare filter
	char *filter1 = ".idx";
	char *filter2 = ".toc";

//	pwindow->disable_window();
	sprintf(string, _("Delete all indexes in %s?"), string1);
//	QuestionWindow confirm(mwindow);
//	confirm.create_objects(string, 0);

//	int result = confirm.run_window();

	int result = 0;
	if(!result)
	{
		static int i, j, k;

		for(i = 0; i < dir.dir_list.total; i++)
		{
  			result = 1;
			sprintf(string2, "%s%s", string1, dir.dir_list.values[i]->name);
// test filter
			if(test_filter(string2, filter1) ||
				test_filter(string2, filter2))
			{
				remove(string2);
printf("DeleteAllIndexes::run %s\n", string2);
			}
		}
	}

	pwindow->thread->redraw_indexes = 1;
//	pwindow->enable_window();
}


ConfirmDeleteAllIndexes::ConfirmDeleteAllIndexes(MWindow *mwindow, char *string)
 : BC_Window(PROGRAM_NAME ": Delete All Indexes", 
 		mwindow->gui->get_abs_cursor_x(1), 
		mwindow->gui->get_abs_cursor_y(1), 
		340, 
		140)
{ 
	this->string = string; 
}

ConfirmDeleteAllIndexes::~ConfirmDeleteAllIndexes()
{
}
	
int ConfirmDeleteAllIndexes::create_objects()
{ 
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, string));
	
	y += 20;
	add_subwindow(new BC_OKButton(x, y));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(x, y));
	return 0;
}
