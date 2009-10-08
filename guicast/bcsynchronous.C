
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
#include "condition.h"
#include "mutex.h"


#ifdef HAVE_GL
#include <GL/gl.h>
#endif
#include <unistd.h>

#include <string.h>


TextureID::TextureID(int window_id, int id, int w, int h, int components)
{
	this->window_id = window_id;
	this->id = id;
	this->w = w;
	this->h = h;
	this->components = components;
	in_use = 1;
}

ShaderID::ShaderID(int window_id, unsigned int handle, char *source)
{
	this->window_id = window_id;
	this->handle = handle;
	this->source = strdup(source);
}

ShaderID::~ShaderID()
{
	free(source);
}

#ifdef HAVE_GL
PBufferID::PBufferID(int window_id, 
	GLXPbuffer pbuffer, 
	GLXContext gl_context, 
	int w, 
	int h)
{
	this->pbuffer = pbuffer;
	this->gl_context = gl_context;
	this->window_id = window_id;
	this->w = w;
	this->h = h;
	in_use = 1;
}
#endif



BC_SynchronousCommand::BC_SynchronousCommand()
{
	command = BC_SynchronousCommand::NONE;
	frame = 0;
	frame_return = 0;
	result = 0;
	command_done = new Condition(0, "BC_SynchronousCommand::command_done", 0);
}

BC_SynchronousCommand::~BC_SynchronousCommand()
{
	delete command_done;
}

void BC_SynchronousCommand::copy_from(BC_SynchronousCommand *command)
{
	this->command = 	      command->command;
	this->colormodel =      command->colormodel;
	this->window = 	      command->window;
	this->frame = 	      command->frame;
	this->window_id =       command->window_id;

	this->frame_return =    command->frame_return;

	this->id = 		      command->id;
	this->w = 		      command->w;
	this->h = 		      command->h;
}




BC_Synchronous::BC_Synchronous()
 : Thread(1, 0, 0)
{
	next_command = new Condition(0, "BC_Synchronous::next_command", 0);
	command_lock = new Mutex("BC_Synchronous::command_lock");
	table_lock = new Mutex("BC_Synchronous::table_lock");
	done = 0;
	is_running = 0;
	current_window = 0;
	BC_WindowBase::get_resources()->set_synchronous(this);
}

BC_Synchronous::~BC_Synchronous()
{
	commands.remove_all_objects();
}

BC_SynchronousCommand* BC_Synchronous::new_command()
{
	return new BC_SynchronousCommand;
}

void BC_Synchronous::create_objects()
{
}

void BC_Synchronous::start()
{
	run();
}

void BC_Synchronous::quit()
{
	command_lock->lock("BC_Synchronous::quit");
	BC_SynchronousCommand *command = new_command();
	commands.append(command);
	command->command = BC_SynchronousCommand::QUIT;
	command_lock->unlock();

	next_command->unlock();
}

int BC_Synchronous::send_command(BC_SynchronousCommand *command)
{
	command_lock->lock("BC_Synchronous::send_command");
	BC_SynchronousCommand *command2 = new_command();
	commands.append(command2);
	command2->copy_from(command);
	command_lock->unlock();

	next_command->unlock();
//printf("BC_Synchronous::send_command 1 %d\n", next_command->get_value());

// Wait for completion
	command2->command_done->lock("BC_Synchronous::send_command");
	int result = command2->result;
	delete command2;
	return result;
}

void BC_Synchronous::run()
{
	is_running = 1;
	while(!done)
	{
		next_command->lock("BC_Synchronous::run");


		command_lock->lock("BC_Synchronous::run");
		BC_SynchronousCommand *command = 0;
		if(commands.total)
		{
			command = commands.values[0];
			commands.remove_number(0);
		}
// Prevent executing the same command twice if spurious unlock.
		command_lock->unlock();
//printf("BC_Synchronous::run %d\n", command->command);

		handle_command_base(command);
//		delete command;
	}
	is_running = 0;
}

void BC_Synchronous::handle_command_base(BC_SynchronousCommand *command)
{

	if(command)
	{
//printf("BC_Synchronous::handle_command_base 1 %d\n", command->command);
		switch(command->command)
		{
			case BC_SynchronousCommand::QUIT:
				done = 1;
				break;

			default:
				handle_command(command);
				break;
		}
	}

	handle_garbage();

	if(command)
	{
		command->command_done->unlock();
	}
}

void BC_Synchronous::handle_command(BC_SynchronousCommand *command)
{
}

void BC_Synchronous::handle_garbage()
{
	while(1)
	{
		table_lock->lock("BC_Synchronous::handle_garbage");
		if(!garbage.total)
		{
			table_lock->unlock();
			return;
		}

		BC_SynchronousCommand *command = garbage.values[0];
		garbage.remove_number(0);
		table_lock->unlock();

		switch(command->command)
		{
			case BC_SynchronousCommand::DELETE_WINDOW:
				delete_window_sync(command);
				break;

			case BC_SynchronousCommand::DELETE_PIXMAP:
				delete_pixmap_sync(command);
				break;
		}

		delete command;
	}
}

void BC_Synchronous::put_texture(int id, int w, int h, int components)
{
	if(id >= 0)
	{
		table_lock->lock("BC_Resources::put_texture");
// Search for duplicate
		for(int i = 0; i < texture_ids.total; i++)
		{
			TextureID *ptr = texture_ids.values[i];
			if(ptr->window_id == current_window->get_id() &&
				ptr->id == id)
			{
				printf("BC_Synchronous::push_texture: texture exists\n"
					"exists: window=%d id=%d w=%d h=%d\n"
					"new:    window=%d id=%d w=%d h=%d\n",
					ptr->window_id,
					ptr->id,
					ptr->w,
					ptr->h,
					current_window->get_id(),
					id,
					w,
					h);
				table_lock->unlock();
				return;
			}
		}

		TextureID *new_id = new TextureID(current_window->get_id(), 
			id, 
			w, 
			h,
			components);
		texture_ids.append(new_id);
		table_lock->unlock();
	}
}

int BC_Synchronous::get_texture(int w, int h, int components)
{
	table_lock->lock("BC_Resources::get_texture");
	for(int i = 0; i < texture_ids.total; i++)
	{
		if(texture_ids.values[i]->w == w &&
			texture_ids.values[i]->h == h &&
			texture_ids.values[i]->components == components &&
			!texture_ids.values[i]->in_use &&
			texture_ids.values[i]->window_id == current_window->get_id())
		{
			int result = texture_ids.values[i]->id;
			texture_ids.values[i]->in_use = 1;
			table_lock->unlock();
			return result;
		}
	}
	table_lock->unlock();
	return -1;
}

void BC_Synchronous::release_texture(int window_id, int id)
{
	table_lock->lock("BC_Resources::release_texture");
	for(int i = 0; i < texture_ids.total; i++)
	{
		if(texture_ids.values[i]->id == id &&
			texture_ids.values[i]->window_id == window_id)
		{
			texture_ids.values[i]->in_use = 0;
			table_lock->unlock();
			return;
		}
	}
	table_lock->unlock();
}





unsigned int BC_Synchronous::get_shader(char *source, int *got_it)
{
	table_lock->lock("BC_Resources::get_shader");
	for(int i = 0; i < shader_ids.total; i++)
	{
		if(shader_ids.values[i]->window_id == current_window->get_id() &&
			!strcmp(shader_ids.values[i]->source, source))
		{
			unsigned int result = shader_ids.values[i]->handle;
			table_lock->unlock();
			*got_it = 1;
			return result;
		}
	}
	table_lock->unlock();
	*got_it = 0;
	return 0;
}

void BC_Synchronous::put_shader(unsigned int handle, 
	char *source)
{
	table_lock->lock("BC_Resources::put_shader");
	shader_ids.append(new ShaderID(current_window->get_id(), handle, source));
	table_lock->unlock();
}

void BC_Synchronous::dump_shader(unsigned int handle)
{
	int got_it = 0;
	table_lock->lock("BC_Resources::dump_shader");
	for(int i = 0; i < shader_ids.total; i++)
	{
		if(shader_ids.values[i]->handle == handle)
		{
			printf("BC_Synchronous::dump_shader\n"
				"%s", shader_ids.values[i]->source);
			got_it = 1;
			break;
		}
	}
	table_lock->unlock();
	if(!got_it) printf("BC_Synchronous::dump_shader couldn't find %d\n", handle);
}


void BC_Synchronous::delete_window(BC_WindowBase *window)
{
#ifdef HAVE_GL
	BC_SynchronousCommand *command = new_command();
	command->command = BC_SynchronousCommand::DELETE_WINDOW;
	command->window_id = window->get_id();
	command->display = window->get_display();
	command->win = window->win;
	command->gl_context = window->gl_win_context;

	send_garbage(command);
#endif
}

void BC_Synchronous::delete_window_sync(BC_SynchronousCommand *command)
{
#ifdef HAVE_GL
	int window_id = command->window_id;
	Display *display = command->display;
	Window win = command->win;
	GLXContext gl_context = command->gl_context;
int debug = 0;

// texture ID's are unique to different contexts
	glXMakeCurrent(display,
		win,
		gl_context);

	table_lock->lock("BC_Resources::release_textures");
	for(int i = 0; i < texture_ids.total; i++)
	{
		if(texture_ids.values[i]->window_id == window_id)
		{
			GLuint id = texture_ids.values[i]->id;
			glDeleteTextures(1, &id);
if(debug)
printf("BC_Synchronous::delete_window_sync texture_id=%d window_id=%d\n", 
id,
window_id);
			texture_ids.remove_object_number(i);
			i--;
		}
	}

	for(int i = 0; i < shader_ids.total; i++)
	{
		if(shader_ids.values[i]->window_id == window_id)
		{
			glDeleteShader(shader_ids.values[i]->handle);
if(debug)
printf("BC_Synchronous::delete_window_sync shader_id=%d window_id=%d\n", 
shader_ids.values[i]->handle,
window_id);
			shader_ids.remove_object_number(i);
			i--;
		}
	}

	for(int i = 0; i < pbuffer_ids.total; i++)
	{
		if(pbuffer_ids.values[i]->window_id == window_id)
		{
			glXDestroyPbuffer(display, pbuffer_ids.values[i]->pbuffer);
			glXDestroyContext(display, pbuffer_ids.values[i]->gl_context);
if(debug)
printf("BC_Synchronous::delete_window_sync pbuffer_id=%p window_id=%d\n", 
pbuffer_ids.values[i]->pbuffer,
window_id);
			pbuffer_ids.remove_object_number(i);
			i--;
		}
	}


	table_lock->unlock();

	XDestroyWindow(display, win);
	if(gl_context) glXDestroyContext(display, gl_context);
#endif
}



#ifdef HAVE_GL
void BC_Synchronous::put_pbuffer(int w, 
	int h, 
	GLXPbuffer pbuffer, 
	GLXContext gl_context)
{
	int exists = 0;
	table_lock->lock("BC_Resources::release_textures");
	for(int i = 0; i < pbuffer_ids.total; i++)
	{
		PBufferID *ptr = pbuffer_ids.values[i];
		if(ptr->w == w &&
			ptr->h == h &&
			ptr->pbuffer == pbuffer)
		{
// Exists
			exists = 1;
			break;
		}
	}


	if(!exists)
	{
		PBufferID *ptr = new PBufferID(current_window->get_id(),
			pbuffer,
			gl_context,
			w,
			h);
		pbuffer_ids.append(ptr);
	}
	table_lock->unlock();
}

GLXPbuffer BC_Synchronous::get_pbuffer(int w, 
	int h, 
	int *window_id, 
	GLXContext *gl_context)
{
	table_lock->lock("BC_Resources::release_textures");
	for(int i = 0; i < pbuffer_ids.total; i++)
	{
		PBufferID *ptr = pbuffer_ids.values[i];
		if(ptr->w == w &&
			ptr->h == h &&
			ptr->window_id == current_window->get_id() &&
			!ptr->in_use)
		{
			GLXPbuffer result = ptr->pbuffer;
			*gl_context = ptr->gl_context;
			*window_id = ptr->window_id;
			ptr->in_use = 1;
			table_lock->unlock();
			return result;
		}
	}
	table_lock->unlock();
	return 0;
}

void BC_Synchronous::release_pbuffer(int window_id, GLXPbuffer pbuffer)
{
	table_lock->lock("BC_Resources::release_textures");
	for(int i = 0; i < pbuffer_ids.total; i++)
	{
		PBufferID *ptr = pbuffer_ids.values[i];
		if(ptr->window_id == window_id)
		{
			ptr->in_use = 0;
		}
	}
	table_lock->unlock();
}

void BC_Synchronous::delete_pixmap(BC_WindowBase *window, 
	GLXPixmap pixmap, 
	GLXContext context)
{
	BC_SynchronousCommand *command = new_command();
	command->command = BC_SynchronousCommand::DELETE_PIXMAP;
	command->window_id = window->get_id();
	command->display = window->get_display();
	command->win = window->win;
	command->gl_pixmap = pixmap;
	command->gl_context = context;

	send_garbage(command);
}
#endif

void BC_Synchronous::delete_pixmap_sync(BC_SynchronousCommand *command)
{
#ifdef HAVE_GL
	Display *display = command->display;
	Window win = command->win;
	glXMakeCurrent(display,
		win,
		command->gl_context);
	glXDestroyContext(display, command->gl_context);
	glXDestroyGLXPixmap(display, command->gl_pixmap);
#endif
}



void BC_Synchronous::send_garbage(BC_SynchronousCommand *command)
{
	table_lock->lock("BC_Synchronous::delete_window");
	garbage.append(command);
	table_lock->unlock();

	next_command->unlock();
}

BC_WindowBase* BC_Synchronous::get_window()
{
	return current_window;
}







