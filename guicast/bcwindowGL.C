// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#define GL_GLEXT_PROTOTYPES
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "glthread.h"

// OpenGL functions in BC_WindowBase

int BC_WindowBase::enable_opengl()
{
#ifdef HAVE_GL
	top_level->sync_display();
	if(resources.get_glthread()->initialize(top_level->display, win, top_level->screen))
	{
		fputs("BC_WindowBase::enable_opengl: Failed to initialize GLThread\n", stdout);
			return 1;
	}
	have_gl_context = 1;
	return 0;
#else
	return 1;
#endif
}

void BC_WindowBase::flip_opengl()
{
#ifdef HAVE_GL
	glXSwapBuffers(top_level->display, win);
	glFlush();
#endif
}

int BC_WindowBase::get_opengl_version(BC_WindowBase *window)
{
#ifdef HAVE_GL
	int maj, min;

	if(glXQueryVersion(window->top_level->display, &maj, &min))
		return 100 * maj + min;
#endif
	return 0;
}

void BC_WindowBase::draw_opengl(VFrame *frame)
{
	resources.get_glthread()->draw_vframe(frame);
}
