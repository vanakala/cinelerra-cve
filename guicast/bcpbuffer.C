
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

#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"




BC_PBuffer::BC_PBuffer(int w, int h)
{
	reset();
	this->w = w;
	this->h = h;


	new_pbuffer(w, h);
}

BC_PBuffer::~BC_PBuffer()
{
#ifdef HAVE_GL
	BC_WindowBase::get_synchronous()->release_pbuffer(window_id, pbuffer);
#endif
}


void BC_PBuffer::reset()
{
#ifdef HAVE_GL
	pbuffer = 0;
	window_id = -1;
#endif
}

#ifdef HAVE_GL

GLXPbuffer BC_PBuffer::get_pbuffer()
{
	return pbuffer;
}
#endif


void BC_PBuffer::new_pbuffer(int w, int h)
{
#ifdef HAVE_GL

	if(!pbuffer)
	{
		BC_WindowBase *current_window = BC_WindowBase::get_synchronous()->current_window;
// Try previously created PBuffers
		pbuffer = BC_WindowBase::get_synchronous()->get_pbuffer(w, 
			h, 
			&window_id,
			&gl_context);

		if(pbuffer)
		{
			return;
		}





// You're supposed to try different configurations of decreasing overhead 
// until one works.
// In reality, only a very specific configuration works at all.
#define TOTAL_CONFIGS 1
		static int framebuffer_attributes[] = 
		{
        	GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
          	GLX_DOUBLEBUFFER, False,
     		GLX_DEPTH_SIZE, 1,
			GLX_ACCUM_RED_SIZE, 1,
			GLX_ACCUM_GREEN_SIZE, 1,
			GLX_ACCUM_BLUE_SIZE, 1,
			GLX_ACCUM_ALPHA_SIZE, 1,
        	GLX_RED_SIZE, 8,
        	GLX_GREEN_SIZE, 8,
        	GLX_BLUE_SIZE, 8,
			GLX_ALPHA_SIZE, 8,
			None
		};

		static int pbuffer_attributes[] = 
		{
			GLX_PBUFFER_WIDTH, 0,
			GLX_PBUFFER_HEIGHT, 0,
    		GLX_LARGEST_PBUFFER, False,
    		GLX_PRESERVED_CONTENTS, True,
    		None
		};

		pbuffer_attributes[1] = w;
		pbuffer_attributes[3] = h;
		if(w % 4) pbuffer_attributes[1] += 4 - (w % 4);
		if(h % 4) pbuffer_attributes[3] += 4 - (h % 4);

		GLXFBConfig *config_result;
		XVisualInfo *visinfo;
		int config_result_count = 0;
		config_result = glXChooseFBConfig(current_window->get_display(), 
			current_window->get_screen(), 
			framebuffer_attributes, 
			&config_result_count);
// printf("BC_PBuffer::new_pbuffer 1 config_result=%p config_result_count=%d\n", 
// config_result, 
// config_result_count);

		if(!config_result || !config_result_count)
		{
			printf("BC_PBuffer::new_pbuffer: glXChooseFBConfig failed\n");
			return;
		}


static int current_config = 0;

		BC_Resources::error = 0;
		pbuffer = glXCreatePbuffer(current_window->get_display(), 
			config_result[current_config], 
			pbuffer_attributes);
    	visinfo = glXGetVisualFromFBConfig(current_window->get_display(), 
			config_result[current_config]);
// printf("BC_PBuffer::new_pbuffer 2 visual=%d current_config=%d error=%d pbuffer=%p\n",
// visinfo->visual,
// current_config,
// BC_Resources::error,
// pbuffer);
///current_config++;

// Got it
		if(!BC_Resources::error && pbuffer && visinfo)
		{
			window_id = current_window->get_id();
			gl_context = glXCreateContext(current_window->get_display(),
				visinfo,
				current_window->gl_win_context,
				1);
			BC_WindowBase::get_synchronous()->put_pbuffer(w, 
				h, 
				pbuffer, 
				gl_context);
// printf("BC_PBuffer::new_pbuffer gl_context=%p window_id=%d\n",
// gl_context,
// current_window->get_id());
		}

		if(config_result) XFree(config_result);
    	if(visinfo) XFree(visinfo);
	}


	if(!pbuffer) printf("BC_PBuffer::new_pbuffer: failed\n");
#endif
}


void BC_PBuffer::enable_opengl()
{
#ifdef HAVE_GL
	BC_WindowBase *current_window = BC_WindowBase::get_synchronous()->current_window;
	int result = glXMakeCurrent(current_window->get_display(),
		pbuffer,
		gl_context);
	BC_WindowBase::get_synchronous()->is_pbuffer = 1;
#endif
}

