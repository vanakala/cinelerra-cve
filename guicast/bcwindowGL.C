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

void BC_WindowBase::disable_opengl()
{
#ifdef HAVE_GL
	if(enabled_gl)
	{
		resources.get_glthread()->disable_opengl(this);
		enabled_gl = 0;
	}
#endif
}

int BC_WindowBase::get_opengl_version()
{
	if(!glx_version)
		glx_version = resources.get_glthread()->get_glx_version(this);
	return glx_version;
}

void BC_WindowBase::opengl_display(VFrame *frame, double in_x1, double in_y1,
	double in_x2, double in_y2, double out_x1, double out_y1,
	double out_x2, double out_y2)
{
	struct gl_window inwin, outwin;

	inwin.x1 = in_x1;
	inwin.y1 = in_y1;
	inwin.x2 = in_x2;
	inwin.y2 = in_y2;
	outwin.x1 = out_x1;
	outwin.y1 = out_y1;
	outwin.x2 = out_x2;
	outwin.y2 = out_y2;

	resources.get_glthread()->display_vframe(frame, this, &inwin, &outwin);
}

void BC_WindowBase::opengl_release()
{
	resources.get_glthread()->release_resources();
}
