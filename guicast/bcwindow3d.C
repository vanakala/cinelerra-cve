
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
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"

// OpenGL functions in BC_WindowBase

void BC_WindowBase::enable_opengl()
{
#ifdef HAVE_GL
	XVisualInfo viproto;
	XVisualInfo *visinfo;
	int nvi;

	top_level->sync_display();

	get_synchronous()->is_pbuffer = 0;
	if(!gl_win_context)
	{
		viproto.screen = top_level->screen;
		visinfo = XGetVisualInfo(top_level->display,
    		VisualScreenMask,
    		&viproto,
    		&nvi);

		gl_win_context = glXCreateContext(top_level->display,
			visinfo,
			0,
			1);
	}


//	if(video_is_on())
//	{
// Make the front buffer's context current.  Pixmaps don't work.
		get_synchronous()->current_window = this;
		glXMakeCurrent(top_level->display,
			win,
			gl_win_context);
// 	}
// 	else
// 	{
// 		get_synchronous()->current_window = this;
// 		pixmap->enable_opengl();
// 	}

#endif
}

void BC_WindowBase::disable_opengl()
{
#ifdef HAVE_GL
// 	unsigned long valuemask = CWEventMask;
// 	XSetWindowAttributes attributes;
// 	attributes.event_mask = DEFAULT_EVENT_MASKS;
// 	XChangeWindowAttributes(top_level->display, win, valuemask, &attributes);
#endif
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










