// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
