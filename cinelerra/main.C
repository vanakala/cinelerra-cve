
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

#include "arraylist.h"
#include "batchrender.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "edl.h"
#include "filexml.h"
#include "filesystem.h"
#include "garbage.h"
#include "language.h"
#include "loadfile.inc"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarmclient.h"
#include "theme.inc"
#include "versioninfo.h"

#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>

MWindow *mwindow;
Theme *theme_global;

enum
{
	DO_GUI,
	DO_DEAMON,
	DO_DEAMON_FG,
	DO_BRENDER,
	DO_USAGE,
	DO_BATCHRENDER
};

#include "thread.h"

int main(int argc, char *argv[])
{
// handle command line arguments first
	srand(time(0));
	ArrayList<char*> filenames;
	FileSystem fs;

	int operation = DO_GUI;
	int deamon_port = DEAMON_PORT;
	char deamon_path[BCTEXTLEN];
	char config_path[BCTEXTLEN];
	char batch_path[BCTEXTLEN];
	int nice_value = 20;
	config_path[0] = 0;
	batch_path[0] = 0;
	deamon_path[0] = 0;
	Garbage::garbage = new Garbage;
	EDL::id_lock = new Mutex("EDL::id_lock");

	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	textdomain(GETTEXT_PACKAGE);
	setlocale(LC_MESSAGES, "");

	if(char *loc = setlocale(LC_CTYPE, ""))
	{
		strcpy(BC_Resources::encoding, nl_langinfo(CODESET));
		BC_Resources::locale_utf8 = !strcmp(BC_Resources::encoding, "UTF-8");

		// Extract from locale language & region
		char locbuf[32];
		char *p;
		locbuf[0] = 0;
		if((p = strchr(loc, '.')) && p - loc < sizeof(locbuf))
		{
			strncpy(locbuf, loc, p - loc);
			locbuf[p - loc] = 0;
		}
		else if(strlen(loc) < sizeof(locbuf))
			strcpy(locbuf, loc);

		// Locale 'C' does not give useful language info - assume en
		if(!locbuf[0] || locbuf[0] == 'C')
			strcpy(locbuf, "en");

		if((p = strchr(locbuf, '_')) && p - locbuf < LEN_LANG)
		{
			*p++ = 0;
			strcpy(BC_Resources::language, locbuf);
			if(strlen(p) < LEN_LANG)
				strcpy(BC_Resources::region, p);
		}
		else if(strlen(locbuf) < LEN_LANG)
			strcpy(BC_Resources::language, locbuf);
	}
	else
		printf(PROGRAM_NAME ": Could not set locale.\n");

	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{
			operation = DO_USAGE;
		}
		else
		if(!strcmp(argv[i], "-r"))
		{
			operation = DO_BATCHRENDER;
			if(argc > i + 1)
			{
				if(argv[i + 1][0] != '-')
				{
					strcpy(batch_path, argv[i + 1]);
					i++;
				}
			}
		}
		else
		if(!strcmp(argv[i], "-c"))
		{
			if(argc > i + 1)
			{
				strcpy(config_path, argv[i + 1]);
				i++;
			}
			else
			{
				fprintf(stderr, "%s: -c needs a filename.\n", argv[0]);
			}
		}
		else
		if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "-f"))
		{
			if(!strcmp(argv[i], "-d"))
				operation = DO_DEAMON;
			else
				operation = DO_DEAMON_FG;

			if(argc > i + 1)
			{
				if(atol(argv[i + 1]) > 0)
				{
					deamon_port = atol(argv[i + 1]);
					i++;
				}
			}
		}
		else
		if(!strcmp(argv[i], "-b"))
		{
			operation = DO_BRENDER;
			if(i > argc - 2)
			{
				fprintf(stderr, "-b may not be used by the user.\n");
				exit(1);
			}
			else
				strcpy(deamon_path, argv[i + 1]);
		}
		else
		if(!strcmp(argv[i], "-n"))
		{
			if(argc > i + 1)
			{
				nice_value = atol(argv[i + 1]);
				i++;
			}
		}
		else
		{
			char *new_filename;
			new_filename = new char[1024];
			strcpy(new_filename, argv[i]);
			fs.complete_path(new_filename);
			filenames.append(new_filename);
		}
	}

	if(operation == DO_GUI || 
		operation == DO_DEAMON || 
		operation == DO_DEAMON_FG || 
		operation == DO_USAGE ||
		operation == DO_BATCHRENDER)
	fprintf(stderr, 
		PROGRAM_NAME " "
		CINELERRA_VERSION
#if defined(REPOMAINTXT)
		" " REPOMAINTXT
#endif
		"\n" COPYRIGHTTEXT1 "\n"
#if defined(COPYRIGHTTEXT2)
		COPYRIGHTTEXT2 "\n"
#endif
		FFMPEG_EXTERNALTEXT "\n"
#if defined(COMPILEDATE)
		COMPILEDATE "\n"
#endif
		"\n"

PROGRAM_NAME " is free software, covered by the GNU General Public License,\n"
"and you are welcome to change it and/or distribute copies of it under\n"
"certain conditions. There is absolutely no warranty for " PROGRAM_NAME ".\n");

	switch(operation)
	{
	case DO_USAGE:
		printf(_("\nUsage:\n"));
		printf(_("%s [-f] [-c configuration] [-d port] [-n nice] [-r batch file] [filenames]\n\n"), argv[0]);
		printf(_("-d = Run in the background as renderfarm client.  The port (400) is optional.\n"));
		printf(_("-f = Run in the foreground as renderfarm client.  Substitute for -d.\n"));
		printf(_("-n = Nice value if running as renderfarm client. (20)\n"));
		printf(_("-c = Configuration file to use instead of %s%s.\n"),
			BCASTDIR, CONFIG_FILE);
		printf(_("-r = batch render the contents of the batch file (%s%s) with no GUI.  batch file is optional.\n"), 
				BCASTDIR, BATCH_PATH);
		printf(_("filenames = files to load\n\n\n"));
		exit(0);

	case DO_DEAMON:
	case DO_DEAMON_FG:
	{
		if(operation == DO_DEAMON)
		{
			pid_t pid;
			if(pid = fork())
			{
				if(pid == -1)
					fputs("Fork failed, can't daemonize\n", stderr);
// Redhat 9 requires _exit instead of exit here.
				_exit(0);
			}
		}

		RenderFarmClient client(deamon_port, 0, nice_value, config_path);
		client.main_loop();
		break;
	}

// Same thing without detachment
	case DO_BRENDER:
	{
		RenderFarmClient client(0, deamon_path, 20, config_path);
		client.main_loop();
		break;
	}

	case DO_BATCHRENDER:
	{
		BatchRenderThread *thread = new BatchRenderThread;
		thread->start_rendering(config_path, batch_path);
		break;
	}

	case DO_GUI:
		setlinebuf(stdout);
		mwindow = new MWindow(config_path);
// load the initial files on seperate tracks
		if(filenames.total)
		{
			mwindow->load_filenames(&filenames, LOADMODE_REPLACE);
			if(filenames.total == 1)
				mwindow->gui->mainmenu->add_load(filenames.values[0]);
		}

// run the program
		mwindow->start();
		mwindow->save_defaults();
DISABLE_BUFFER
		break;
	}

	filenames.remove_all_objects();
	return 0;
}

