#include "arraylist.h"
#include "builddate.h"
#include "filexml.h"
#include "filesystem.h"
#include "loadfile.inc"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarmclient.h"
#include <stdlib.h>
#include <string.h>



// Instantiation of the main loop

enum
{
	DO_GUI,
	DO_DEAMON,
	DO_BRENDER,
	DO_USAGE
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

	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{                     // help
			operation = DO_USAGE;
		}
		else
		if(!strcmp(argv[i], "-d"))
		{
			operation = DO_DEAMON;

			if(argc > i + 1)
			{
				if(atol(argv[i + 1]) > 0)
					deamon_port = atol(argv[i + 1]);
			}
		}
		else
		if(!strcmp(argv[i], "-b"))
		{
			operation = DO_BRENDER;
			strcpy(deamon_path, argv[i + 1]);
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




	if(operation == DO_GUI || operation == DO_DEAMON || operation == DO_USAGE)
	fprintf(stderr, 
		PROGRAM_NAME " " 
		VERSION " " 
		BUILDDATE 
		" (C)2003 Heroine Virtual Ltd.\n\n"

PROGRAM_NAME " is free software, covered by the GNU General Public License,\n"
"and you are welcome to change it and/or distribute copies of it under\n"
"certain conditions. There is absolutely no warranty for " PROGRAM_NAME ".\n");





	switch(operation)
	{
		case DO_USAGE:
			printf("\nUsage:\n");
			printf("%s [-d] [port]\n", argv[0]);
			printf("\n-d = Run in the background as renderfarm client.\n");
			printf("port = Port for client to listen on. (400)\n\n\n");
			exit(0);
			break;

		case DO_DEAMON:
		{
			int pid = fork();
			
			if(pid) exit(0);

			RenderFarmClient client(deamon_port, 0);
			client.main_loop();
			break;
		}

// Same thing without detachment
		case DO_BRENDER:
		{
			RenderFarmClient client(0, deamon_path);
			client.main_loop();
			break;
		}

		case DO_GUI:
		{
			MWindow mwindow;
			mwindow.create_objects(1, !filenames.total);

// load the initial files on seperate tracks
			if(filenames.total)
			{
				mwindow.gui->lock_window();
				mwindow.load_filenames(&filenames, LOAD_REPLACE);
				if(filenames.total == 1) 
					mwindow.gui->mainmenu->add_load(filenames.values[0]);
				mwindow.gui->unlock_window();
			}

// run the program
			mwindow.start();
//			mwindow.gui->run_window();
			mwindow.save_defaults();
			break;
		}
	}

	filenames.remove_all_objects();
	return 0;
}

