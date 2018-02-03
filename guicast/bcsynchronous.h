
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
#include "bcpixmap.inc"
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

// In addition to synchronous operations, it handles global OpenGL variables.

class ShaderID
{
public:
	ShaderID(int window_id, unsigned int handle, const char *source);
	~ShaderID();

// Should really use an MD5 to compare sources but this is easiest.
	char *source;
	int window_id;
	unsigned int handle;
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
#endif
};


class BC_Synchronous : public Thread
{
public:
	BC_Synchronous();
	virtual ~BC_Synchronous();

	friend class BC_WindowBase;

// Called by another thread
// Quits the loop
	void quit();

	void start();
	void run();

	virtual BC_SynchronousCommand* new_command();
// Handle extra commands not part of the base class.
// Contains a switch statement starting with LAST_COMMAND
	virtual void handle_command(BC_SynchronousCommand *command);

// Get the shader by window_id and source comparison if it exists.
// Not run in OpenGL thread because it has its own lock.
// Sets *got_it to 1 on success.
	unsigned int get_shader(char *source, int *got_it);
// Add a new shader program by title if it doesn't exist.  
// Doesn't check if it already exists.
	void put_shader(unsigned int handle, char *source);
	void dump_shader(unsigned int handle);

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
// Commands which can't be executed until the caller returns.
	ArrayList<BC_SynchronousCommand*> garbage;

};
#endif
