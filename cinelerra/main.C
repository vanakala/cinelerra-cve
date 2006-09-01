#include "arraylist.h"
#include "batchrender.h"
#include "bcsignals.h"
#include "builddate.h"
#include "edl.h"
#include "filexml.h"
#include "filesystem.h"
#include "language.h"
#include "loadfile.inc"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarmclient.h"

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>

#define PACKAGE "cinelerra"
#define LOCALEDIR "/usr/share/locale"


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
	EDL::id_lock = new Mutex("EDL::id_lock");

// detect an UTF-8 locale and try to use a non-Unicode locale instead
// <---Beginning of dirty hack
// This hack will be removed as soon as Cinelerra is UTF-8 compliant
    char *s, *language;

// Query user locale
    if ((s = getenv("LC_ALL"))  || 
		(s = getenv("LC_MESSAGES")) || 
		(s = getenv("LC_CTYPE")) || 
		(s = getenv ("LANG")))
    {
// Test if user locale is set to Unicode        
        if (strstr(s, ".UTF-8"))
        {
// extract language  from language-charset@variant
          language = strtok (s, ".@");
// set language as the default locale
          setenv("LANG", language, 1);
        }
    }
// End of dirty hack --->




	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
	setlocale (LC_MESSAGES, "");
	setlocale (LC_CTYPE, "");

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
		CINELERRA_VERSION " " 
		BUILDDATE 
		" (C)2006 Heroine Virtual Ltd.\n\n"

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
				BCASTDIR, 
				CONFIG_FILE);
			printf(_("-r = batch render the contents of the batch file (%s%s) with no GUI.  batch file is optional.\n"), 
				BCASTDIR, 
				BATCH_PATH);
			printf(_("filenames = files to load\n\n\n"));
			exit(0);
			break;

		case DO_DEAMON:
		case DO_DEAMON_FG:
		{
			if(operation == DO_DEAMON)
			{
				int pid = fork();

				if(pid)
				{
// Redhat 9 requires _exit instead of exit here.
					_exit(0);
				}
			}

			RenderFarmClient client(deamon_port, 
				0, 
				nice_value, 
				config_path);
			client.main_loop();
			break;
		}

// Same thing without detachment
		case DO_BRENDER:
		{
			RenderFarmClient client(0, 
				deamon_path, 
				20,
				config_path);
			client.main_loop();
			break;
		}

		case DO_BATCHRENDER:
		{
			BatchRenderThread *thread = new BatchRenderThread;
			thread->start_rendering(config_path, 
				batch_path);
			break;
		}

		case DO_GUI:
		{
			MWindow mwindow;
			mwindow.create_objects(1, 
				!filenames.total,
				config_path);

// load the initial files on seperate tracks
			if(filenames.total)
			{
				mwindow.gui->lock_window("main");
				mwindow.load_filenames(&filenames, LOAD_REPLACE);
				if(filenames.total == 1)
					mwindow.gui->mainmenu->add_load(filenames.values[0]);
				mwindow.gui->unlock_window();
			}

// run the program
			mwindow.start();
			mwindow.save_defaults();
DISABLE_BUFFER
			break;
		}
	}

	filenames.remove_all_objects();
	return 0;
}

