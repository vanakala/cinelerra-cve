
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "deleteallindexes.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "question.h"
#include "theme.h"
#include <string.h>


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
