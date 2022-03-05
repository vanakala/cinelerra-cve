// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "deleteallindexes.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "question.h"
#include "theme.h"
#include <string.h>


DeleteAllIndexes::DeleteAllIndexes(PreferencesWindow *pwindow, int x, int y)
 : BC_GenericButton(x, y, _("Delete existing indexes")), Thread()
{
	this->pwindow = pwindow;
}

int DeleteAllIndexes::handle_event() 
{ 
	start();
	return 1;
}

static int test_filter(const char *string, const char *filter)
{
	return (strlen(string) > strlen(filter) &&
			!strcmp(string + strlen(string) - strlen(filter), filter));
}

void DeleteAllIndexes::run()
{
	char string1[BCTEXTLEN], string2[BCTEXTLEN];
// prepare directory
	strcpy(string1, pwindow->thread->preferences->index_directory);
	FileSystem dir;
	dir.update(pwindow->thread->preferences->index_directory);
	dir.complete_path(string1);

// prepare filter
	const char *filter1 = ".idx";
	const char *filter2 = ".toc";
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
			}
		}
	}

	pwindow->thread->redraw_indexes = 1;
}
