
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

#ifndef BCSYNCHRONOUS_H
#define BCSYNCHRONOUS_H

#include "arraylist.h"
#include "bcpbuffer.inc"
#include "bcpixmap.inc"
#include "bctexture.inc"
#include "bcwindowbase.inc"
#include "condition.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include <X11/Xlib.h>

// This takes requests and runs all OpenGL calls in the main thread.
// Past experience showed OpenGL couldn't be run from multiple threads
// reliably even if MakeCurrent was used and only 1 thread at a time did 
// anything.

// Also manages texture memory.  Textures are not deleted until the gl_context
// is deleted, so they have to be reused as much as possible.
// Must be run as the main loop of the application.  Other threads must
// call into it to dispatch commands.

// In addition to synchronous operations, it handles global OpenGL variables.

// Users should create a subclass of the command and thread components to
// add specific commands.



class TextureID
{
public:
	TextureID(int window_id, int id, int w, int h, int components);
	int window_id;
	int id;
	int w;
	int h;
	int components;
	BC_WindowBase *window;
	int in_use;
};

class ShaderID
{
public:
	ShaderID(int window_id, unsigned int handle, char *source);
	~ShaderID();

// Should really use an MD5 to compare sources but this is easiest.	
	char *source;
	int window_id;
	unsigned int handle;
};

class PBufferID
{
public:
	PBufferID() {};
#ifdef HAVE_GL
	PBufferID(int window_id, 
		GLXPbuffer pbuffer, 
		GLXContext gl_context, 
		int w, 
		int h);
	GLXPbuffer pbuffer;
	GLXContext gl_context;
	int window_id;
	int w;
	int h;
	int in_use;
#endif
};

class BC_SynchronousCommand
{
public:
	BC_SynchronousCommand();
	~BC_SynchronousCommand();

	virtual void copy_from(BC_SynchronousCommand *command);

	Condition *command_done;
	int result;
	int command;

// Commands
	enum
	{
		NONE,
		QUIT,
// Used by garbage collector
		DELETE_WINDOW,
		DELETE_PIXMAP,
// subclasses create new commands starting with this enumeration
		LAST_COMMAND
	};

	int colormodel;
	BC_WindowBase *window;
	VFrame *frame;

	VFrame *frame_return;

	int id;
	int w;
	int h;

// For garbage collection
	int window_id;
	Display *display;
	Window win;
#ifdef HAVE_GL
	GLXContext gl_context;
	GLXPixmap gl_pixmap;
#endif
};


class BC_Synchronous : public Thread
{
public:
	BC_Synchronous();
	virtual ~BC_Synchronous();

	friend class BC_WindowBase;
	friend class VFrame;
	friend class BC_PBuffer;
	friend class BC_Pixmap;
	friend class BC_Texture;

// Called by another thread
// Quits the loop
	void quit();
// Must be called after constructor to create inherited objects.
	void create_objects();
	void start();
	void run();

	virtual BC_SynchronousCommand* new_command();
// Handle extra commands not part of the base class.
// Contains a switch statement starting with LAST_COMMAND
	virtual void handle_command(BC_SynchronousCommand *command);


// OpenGL texture removal doesn't work.  Need to store the ID's of all the deleted
// textures in this stack and reuse them.  Also since OpenGL needs synchronous
// commands, be sure this is always called synchronously.
// Called when a texture is created to associate it with the current window.
// Must be called inside synchronous loop.
	void put_texture(int id, int w, int h, int components);
// Search for existing texture matching the parameters and not in use 
// and return it.  If -1 is returned, a new texture must be created.
// Must be called inside synchronous loop.
// If someone proves OpenGL can delete texture memory, this function can be
// forced to always return -1.
	int get_texture(int w, int h, int components);
// Release a texture for use by the get_texture call.
// Can be called outside synchronous loop.
	void release_texture(int window_id, int id);

// Get the shader by window_id and source comparison if it exists.
// Not run in OpenGL thread because it has its own lock.
// Sets *got_it to 1 on success.
	unsigned int get_shader(char *source, int *got_it);
// Add a new shader program by title if it doesn't exist.  
// Doesn't check if it already exists.
	void put_shader(unsigned int handle, char *source);
	void dump_shader(unsigned int handle);


#ifdef HAVE_GL
// Push a pbuffer when it's created.
// Must be called inside synchronous loop.
	void put_pbuffer(int w, 
		int h, 
		GLXPbuffer pbuffer, 
		GLXContext gl_context);
// Get the PBuffer by window_id and dimensions if it exists.
// Must be called inside synchronous loop.
	GLXPbuffer get_pbuffer(int w, 
		int h, 
		int *window_id, 
		GLXContext *gl_context);
// Release a pbuffer for use by get_pbuffer.
	void release_pbuffer(int window_id, GLXPbuffer pbuffer);

// Schedule GL pixmap for deletion by the garbage collector.
// Pixmaps don't wait until until the window is deleted but they must be
// deleted before the window is deleted to have the display connection.
	void delete_pixmap(BC_WindowBase *window, 
		GLXPixmap pixmap, 
		GLXContext context);
#endif



// Called by ~BC_WindowBase to delete OpenGL objects related to the window.
// This function returns immediately instead of waiting for the synchronous
// part to finish.
	void delete_window(BC_WindowBase *window);


	int send_command(BC_SynchronousCommand *command);
	void send_garbage(BC_SynchronousCommand *command);


// Get the window currently bound to the context.
	BC_WindowBase* get_window();

private:
	void handle_command_base(BC_SynchronousCommand *command);

// Execute commands which can't be executed until the caller returns.
	void handle_garbage();

// Release OpenGL objects related to the window id.
// Called from the garbage collector only
	void delete_window_sync(BC_SynchronousCommand *command);
	void delete_pixmap_sync(BC_SynchronousCommand *command);

	Condition *next_command;
	Mutex *command_lock;

// Must be locked in order of current_window->lock_window, table_lock
// or just table_lock.
	Mutex *table_lock;

// This quits the program when it's 1.
	int done;
// Command stack
	ArrayList<BC_SynchronousCommand*> commands;
	int is_running;
// The window the opengl context is currently bound to.
// Set by BC_WindowBase::enable_opengl.
	BC_WindowBase *current_window;

	ArrayList<ShaderID*> shader_ids;
 	ArrayList<TextureID*> texture_ids;
 	ArrayList<PBufferID*> pbuffer_ids;
// Commands which can't be executed until the caller returns.
	ArrayList<BC_SynchronousCommand*> garbage;

// When the context is bound to a pbuffer, this
// signals glCopyTexSubImage2D to use the front buffer.
	int is_pbuffer;
};




#endif
