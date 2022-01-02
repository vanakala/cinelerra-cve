// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "arraylist.h"
#include "assetlist.h"
#include "atmpframecache.h"
#include "cliplist.h"
#include "batchrender.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "edl.h"
#include "filexml.h"
#include "filesystem.h"
#include "fileavlibs.h"
#include "hashcache.h"
#include "language.h"
#include "loadfile.inc"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "plugindb.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarmclient.h"
#include "theme.inc"
#include "versioninfo.h"

#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

MWindow *mwindow_global;
Theme *theme_global;
Preferences *preferences_global;
Preferences *render_preferences;
AssetList assetlist_global;
ClipList cliplist_global;
EDL *master_edl;
EDL *vwindow_edl;
EDL *render_edl;
MainSession *mainsession;
EDLSession *edlsession;
PluginDB plugindb;
ATmpFrameCache audio_frames;

const char *version_name = PROGRAM_NAME " " CINELERRA_VERSION;

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
	int nice_value = 20;
	config_path[0] = 0;
	deamon_path[0] = 0;
	EDL::id_lock = new Mutex("EDL::id_lock");

	if(!getuid() || !geteuid())
	{
		fprintf(stderr, "\nYou can't run " PROGRAM_NAME " as root. It is unsecure.\n");
		exit(0);
	}

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
		);
	fputs(FFMPEG_EXTERNALTEXT ", library versions:\n", stderr);
	FileAVlibs::versionifo(4);
#if defined(COMPILEDATE)
	fputs(COMPILEDATE "\n", stderr);
#endif
	fputc('\n', stderr);

	fputs(PROGRAM_NAME " is free software, covered by the GNU General Public License,\n"
		"and you are welcome to change it and/or distribute copies of it under\n"
		"certain conditions. There is absolutely no warranty for " PROGRAM_NAME ".\n",
		stderr);

	switch(operation)
	{
	case DO_USAGE:
		printf(_("\nUsage:\n"));
		printf(_("%s [-f] [-c configuration] [-d port] [-n nice] [-r batch file] [filenames]\n\n"), argv[0]);
		printf(_("-d = Run in the background as renderfarm client.  The port (%d) is optional.\n"), DEAMON_PORT);
		printf(_("-f = Run in the foreground as renderfarm client.  Substitute for -d.\n"));
		printf(_("-n = Nice value if running as renderfarm client. (20)\n"));
		printf(_("-c = Configuration file to use instead of %s%s.\n"),
			BCASTDIR, CONFIG_FILE);
		printf(_("-r = batch render with no GUI.\n"));
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
		thread->start_rendering(config_path);
		break;
	}

	case DO_GUI:
		setlinebuf(stdout);
		mwindow_global = new MWindow(config_path);
// load the initial files on seperate tracks
		if(filenames.total)
		{
			mwindow_global->load_filenames(&filenames, LOADMODE_REPLACE);
			if(filenames.total == 1)
				mwindow_global->add_load_path(filenames.values[0]);
		}

// run the program
		mwindow_global->start();
		mwindow_global->save_defaults();
		break;
	}

	filenames.remove_all_objects();
	BC_Resources::hash_cache.save_changed();
	return 0;
}

