#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#include "bcsignals.h"
#include "file.h"

class SigHandler : public BC_Signals
{
public:
	SigHandler();
	void signal_handler(int signum);


// Put file pointer on table
	void push_file(File *file);
// Remove file pointer from table
	void pull_file(File *file);

// Files currently open for writing.
// During a crash, the sighandler should close them all.
	ArrayList<File*> files;
};


#endif
