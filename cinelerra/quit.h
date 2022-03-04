// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef QUIT_H
#define QUIT_H

#include "savefile.inc"

class Quit : public BC_MenuItem, public Thread
{
public:
	Quit(Save *save);

	int handle_event();
private:
	void run();

	Save *save;
};

#endif
