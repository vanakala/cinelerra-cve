#include "assets.h"
#include "file.h"
#include "sighandler.h"


SigHandler::SigHandler()
 : BC_Signals()
{
}

void SigHandler::signal_handler(int signum)
{
	printf("SigHandler::signal_handler total files=%d\n", 
		files.total);
	for(int i = 0; i < files.total; i++)
	{
		printf("Closing %s\n", files.values[i]->asset->path);
		files.values[i]->close_file(1);
	}
	BC_Signals::signal_handler(signum);
}

void SigHandler::push_file(File *file)
{
// Check for duplicate
	for(int i = 0; i < files.total; i++)
	{
		if(files.values[i] == file)
		{
			printf("SigHandler::push_file: file %s already on table.\n",
				file->asset->path);
			return;
		}
	}

// Append file
	files.append(file);
}

void SigHandler::pull_file(File *file)
{
	for(int i = 0; i < files.total; i++)
	{
		if(files.values[i] == file)
		{
			files.remove_number(i);
			return;
		}
	}
	printf("SigHandler::pull_file: file %s not on table.\n",
		file->asset->path);
}











