// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "glthread.h"

// OpenGL functions in BC_WindowBase

int BC_WindowBase::enable_opengl()
{
#ifdef HAVE_GL
	enabled_gl = 1;
	return 0;
#else
	return 1;
#endif
}

int BC_WindowBase::get_opengl_version()
{
	if(!glx_version)
		glx_version = resources.get_glthread()->get_glx_version(this);
	return glx_version;
}

void BC_WindowBase::opengl_display(VFrame *frame)
{
	resources.get_glthread()->display_vframe(frame, this);
}

void BC_WindowBase::opengl_release()
{
	resources.get_glthread()->release_resources();
}
