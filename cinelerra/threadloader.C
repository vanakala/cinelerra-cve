#include "mwindow.h"
#include "threadloader.h"

// loads command line filenames at startup

ThreadLoader::ThreadLoader(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}

ThreadLoader::~ThreadLoader()
{
}

int ThreadLoader::set_paths(ArrayList<char *> *paths)
{
	this->paths = paths;
}

void ThreadLoader::run()
{
	int import_ = 0;
	for(int i = 0; i < paths->total; i++)
	{
//printf("%s\n", paths->values[i]);
//		mwindow->load(paths->values[i], import_);
		import_ = 1;
	}
}
