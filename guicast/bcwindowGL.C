
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#define GL_GLEXT_PROTOTYPES
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"

static int attrib[] =
{
    GLX_RGBA,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_DOUBLEBUFFER,
    None
};


// OpenGL functions in BC_WindowBase

int BC_WindowBase::enable_opengl()
{
#ifdef HAVE_GL
	XVisualInfo *visinfo;

	top_level->sync_display();

	if(!gl_win_context)
	{
		if(!(visinfo = glXChooseVisual(top_level->display, top_level->screen, attrib)))
		{
			fputs("BC_WindowBase::enable_opengl:Couldn't get visual.\n", stdout);
			return 1;
		}
		if(!(gl_win_context = glXCreateContext(top_level->display,
				visinfo, 0, GL_TRUE)))
		{
			fputs("BC_WindowBase::enable_opengl: Couldn't create OpenGL context.\n", stdout);
			XFree(visinfo);
			return 1;
		}
		XFree(visinfo);
	}

// Make the front buffer's context current.
	if(glXMakeCurrent(top_level->display, win, gl_win_context))
	{
		if(!BC_Resources::OpenGLStrings[0])
		{
			const char *string;
			if(string = (const char*)glGetString(GL_VERSION))
				BC_Resources::OpenGLStrings[0] = strdup(string);
			if(string = (const char*)glGetString(GL_VENDOR))
				BC_Resources::OpenGLStrings[1] = strdup(string);
			if(string = (const char*)glGetString(GL_RENDERER))
				BC_Resources::OpenGLStrings[2] = strdup(string);
		}
		return 0;
	}

	glXDestroyContext(top_level->display, gl_win_context);
	gl_win_context = 0;
#endif
	return 1;
}

void BC_WindowBase::flip_opengl()
{
#ifdef HAVE_GL
	glXSwapBuffers(top_level->display, win);
	glFlush();
#endif
}

unsigned int BC_WindowBase::get_shader(char *source, int *got_it)
{
	return get_resources()->get_synchronous()->get_shader(source, got_it);
}

void BC_WindowBase::put_shader(unsigned int handle, char *source)
{
	get_resources()->get_synchronous()->put_shader(handle, source);
}

int BC_WindowBase::get_opengl_version(BC_WindowBase *window)
{
#ifdef HAVE_GL
	int maj, min;
	if(glXQueryVersion(window->get_display(), &maj, &min))
		return 100 * maj + min;
#endif
	return 0;
}
