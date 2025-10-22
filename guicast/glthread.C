// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#define GL_GLEXT_PROTOTYPES
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "condition.h"
#include "glthread.h"
#include "tmpframecache.h"
#include "shaders.h"
#include "mutex.h"
#include "vframe.h"

#include <unistd.h>

#include <string.h>

#ifdef HAVE_GL

// YUV -> RGB converter
static const char *yuv_to_rgb_frag =
R"vx(
    void yuv2rgb(inout vec3 yuv)
    {
	yuv -= vec3(0, 0.5, 0.5);
	const mat3 yuv_to_rgb_matrix = mat3(
	    1,       1,        1,
	    0,       -0.34414, 1.77200,
	    1.40200, -0.71414, 0);
	yuv = yuv_to_rgb_matrix * yuv;
    };
)vx";

static int attrib[] =
{
	GLX_RGBA,
	GLX_RED_SIZE, 8,
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_ALPHA_SIZE, 8,
	GLX_DOUBLEBUFFER,
	None
};

// Vertex for texture
static float vertices[GL_VERTICES_SIZE] = {
//  Position      Color             Texcoords
//   x       y      r    g     b     s     t
    -1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Top-left      0..6
     1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Top-right     7..13
     1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Bottom-right 14..20
    -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left  21..27
};

static GLuint elements[] = {
        0, 1, 2,
        2, 3, 0
};

static float brd_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };

static const char *vertex_shader =
R"vx(#version 130
    in vec2 position;
    in vec3 color;
    in vec2 texcoord;
    out vec3 Color;
    out vec2 Texcoord;
    void main()
    {
	Color = color;
	Texcoord = texcoord;
	gl_Position = vec4(position, 0.0, 1.0);
    }
)vx";

static const char *fragment_shader =
R"vx(#version 130
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D tex;
    void main()
    {
	outColor = texture(tex, Texcoord) * vec4(Color, 1.0);
    }
)vx";
#endif

const struct gl_commands GLThreadCommand::gl_names[] =
{
	{ "None", NONE },
	{ "Quit", QUIT },
	{ "Disable", DISABLE },
	{ "Display Frame", DISPLAY_VFRAME },
	{ "Release Resources", RELEASE_RESOURCES },
	{ "Guide Line", GUIDE_LINE },
	{ "Guide Rectangle", GUIDE_RECTANGLE },
	{ "Guide Box", GUIDE_BOX },
	{ "Guide Disc", GUIDE_DISC },
	{ "Guide Circle", GUIDE_CIRCLE },
	{ "Guide Pixel", GUIDE_PIXEL },
	{ "Guide Frame", GUIDE_FRAME },
	{ "Swap Buffers", SWAP_BUFFERS },
	{ 0, 0 }
};

GLThreadCommand::GLThreadCommand()
{
	command = GLThreadCommand::NONE;
	frame = 0;
	dpy = 0;
	win = 0;
	zoom = 1;
	screen = -1;
}

void GLThreadCommand::dump(int indent, int show_frame)
{
	printf("%*sGLThreadCommand '%s' %p dump:\n", indent, "", name(command), this);
	indent += 2;
	printf("%*sdisplay %p win %#lx screen %d\n", indent, "", dpy, win, screen);
	switch(command)
	{
	case DISPLAY_VFRAME:
		printf("%*sframe: %p [%d,%d] zoom:%.3f\n", indent, "",
			frame, width, height, zoom);
		printf("%*swin1:(%.1f,%.1f) (%.1f,%.1f) win2:(%.1f,%.1f) (%.1f,%.1f)\n",
			indent, "", glwin1.x1, glwin1.y1, glwin1.x2, glwin1.y2,
			glwin2.x1, glwin2.y1, glwin2.x2, glwin2.y2);
		break;
	case GUIDE_RECTANGLE:
		printf("%*swin:(%.1f,%.1f)(%.1f,%.1f) canvas: [%.1f, %.1f]\n", indent, "",
			glwin1.x1, glwin1.y1, glwin1.x2, glwin1.y2, glwin2.x2, glwin2.y2);
		printf("%*scolor:%#x opaque %d\n",  indent, "", color, opaque);
		break;
	case GUIDE_LINE:
	case GUIDE_BOX:
	case GUIDE_DISC:
	case GUIDE_CIRCLE:
		printf("%*swin:(%.1f,%.1f)(%.1f,%.1f) color:%#x opaque %d\n",  indent, "",
			glwin1.x1, glwin1.y1, glwin1.x2, glwin1.y2, color, opaque);
		break;
	case GUIDE_PIXEL:
		printf("%*s(%d,%d) color:%#x opaque %d\n",  indent, "",
			width, height, color, opaque);
		break;
	case GUIDE_FRAME:
		printf("%*sframe %p [%d,%d] color:%#x opaque %d\n",  indent, "",
			frame, frame->get_w(), frame->get_h(), color, opaque);
		break;
	}
	if(show_frame && frame)
		frame->dump(indent);
}

const char *GLThreadCommand::name(int command)
{
	for(int i = 0; gl_names[i].name; i++)
		if(gl_names[i].value == command)
			return gl_names[i].name;
	return "Unknown";
}

GLThread::GLThread()
{
	int maj, min;

	next_command = new Condition(0, "GLThread::next_command", 0);
	command_lock = new Mutex("GLThread::command_lock");
	done = 0;
	last_command = 0;
	memset(commands, 0, sizeof(GLThreadCommand*) * GL_MAX_COMMANDS);
#ifdef HAVE_GL
	shaders = 0;
	last_context = 0;
	current_glctx = 0;
	memset(contexts, 0, sizeof(GLXContext) * GL_MAX_CONTEXTS);
	guides.set_glthread(this);
#endif
	BC_WindowBase::get_resources()->set_glthread(this);
}

GLThread::~GLThread()
{
	command_lock->lock("GLThread::~GLThread");
#ifdef HAVE_GL
	delete shaders;
#endif
	for(int i = 0; i < GL_MAX_COMMANDS; i++)
		delete commands[i];
	command_lock->unlock();
	delete next_command;
	delete command_lock;
}

int GLThread::get_glx_version(BC_WindowBase *window)
{
	int maj, min;

#ifdef HAVE_GL
	if(glXQueryVersion(window->get_display(), &maj, &min))
		return 100 * maj + min;
#endif
	return 0;
}

GLThreadCommand* GLThread::new_command()
{
	while(last_command >= GL_MAX_COMMANDS)
	{
		command_lock->lock("GLThread::new_command");
		command_lock->unlock();
	}
	if(!commands[last_command])
		commands[last_command] = new GLThreadCommand;
	return commands[last_command++];
}

#ifdef HAVE_GL
int GLThread::initialize(Display *dpy, Window win, int screen)
{
	XVisualInfo *visinfo;
	GLXContext gl_context;
	struct glctx *ctx = 0;

	if(!(current_glctx = have_context(dpy, screen)))
	{
		for(int i = 0; i < GL_MAX_CONTEXTS; i++)
		{
			if(contexts[i].win == win && contexts[i].dpy == 0)
			{
				ctx = &contexts[i];
				break;
			}
		}
		if(!ctx)
		{
			if(last_context >= GL_MAX_CONTEXTS)
				return 1;
			ctx = &contexts[last_context];
			last_context++;
		}
		if(!(visinfo = glXChooseVisual(dpy, screen, attrib)))
		{
			fputs("GLThread::initialize: Couldn't get visual.\n", stdout);
			return 1;
		}
		if(!(gl_context = glXCreateContext(dpy, visinfo, 0, GL_TRUE)))
		{
			fputs("GLThread::initialize: Couldn't create OpenGL context.\n", stdout);
			XFree(visinfo);
			return 1;
		}
		ctx->dpy = dpy;
		ctx->screen = screen;
		ctx->win = win;
		ctx->gl_context = gl_context;
		ctx->visinfo = visinfo;
		current_glctx = ctx;
		glXMakeCurrent(dpy, win, gl_context);
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
	}

	if(!shaders)
	{
		shaders = new Shaders();
		shaders->add(SHADER_FRAG, "yuv2rgb", yuv_to_rgb_frag, 1);
	}
	// FIXIT: needs texture, fb
	return 0;
}
#endif

#ifdef HAVE_GL
void GLThread::generate_renderframe()
{
	if(current_glctx)
	{
		// Create Vertex Array Object
		glGenVertexArrays(1, &current_glctx->vertexarray);
		glBindVertexArray(current_glctx->vertexarray);
		// Create a Vertex Buffer Object and copy the vertex data to it
		glGenBuffers(1, &current_glctx->vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, current_glctx->vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, GL_VERTICES_SIZE * sizeof(float),
			current_glctx->vertices, GL_STATIC_DRAW);
		// Create an element array
		glGenBuffers(1, &current_glctx->elemarray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, current_glctx->elemarray);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements),
			elements, GL_STATIC_DRAW);
		current_glctx->vertexshader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(current_glctx->vertexshader, 1, &vertex_shader, NULL);
		glCompileShader(current_glctx->vertexshader);
		// Create and compile the fragment shader
		current_glctx->fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(current_glctx->fragmentshader, 1, &fragment_shader, NULL);
		glCompileShader(current_glctx->fragmentshader);
		// Link the vertex and fragment shader into a shader program
		current_glctx->shaderprogram = glCreateProgram();
		glAttachShader(current_glctx->shaderprogram, current_glctx->vertexshader);
		glAttachShader(current_glctx->shaderprogram, current_glctx->fragmentshader);
		glBindFragDataLocation(current_glctx->shaderprogram, 0, "outColor");
		glLinkProgram(current_glctx->shaderprogram);
		glUseProgram(current_glctx->shaderprogram);
		// Specify the layout of the vertex data
		current_glctx->posattrib = glGetAttribLocation(
			current_glctx->shaderprogram, "position");
		glEnableVertexAttribArray(current_glctx->posattrib);
		glVertexAttribPointer(current_glctx->posattrib, 2, GL_FLOAT, GL_FALSE,
			7 * sizeof(GLfloat), 0);
		current_glctx->colattrib = glGetAttribLocation(
			current_glctx->shaderprogram, "color");
		glEnableVertexAttribArray(current_glctx->colattrib);
		glVertexAttribPointer(current_glctx->colattrib, 3, GL_FLOAT, GL_FALSE,
			7 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
		current_glctx->texattrib = glGetAttribLocation(
			current_glctx->shaderprogram, "texcoord");
		glEnableVertexAttribArray(current_glctx->texattrib);
		glVertexAttribPointer(current_glctx->texattrib, 2, GL_FLOAT, GL_FALSE,
			7 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
		// Texture
		// 1st texture
		glGenTextures(1, &current_glctx->firsttexture);
		glBindTexture(GL_TEXTURE_2D, current_glctx->firsttexture);
		// the equivalent of (x,y,z) in texture coordinates is called (s,t,r).
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, brd_color);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}

struct glctx *GLThread::have_context(Display *dpy, int screen)
{
	for(int i = 0; i < last_context; i++)
	{
		if(contexts[i].dpy == dpy && contexts[i].screen == screen)
			return &contexts[i];
	}
	return 0;
}
#endif

void GLThread::quit()
{
	command_lock->lock("GLThread::quit");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::QUIT;
	command_lock->unlock();
	next_command->unlock();
}

#ifdef HAVE_GL
void GLThread::display_vframe(VFrame *frame, BC_WindowBase *window,
	struct gl_window *inwin, struct gl_window *outwin, double zoom)
{
	command_lock->lock("GLThread::display_vframe");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::DISPLAY_VFRAME;
	command->frame = frame;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->width = window->get_w();
	command->height = window->get_h();
	command->zoom = zoom;
	command->glwin1 = *inwin;
	command->glwin2 = *outwin;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guideline(BC_WindowBase *window, struct gl_window *rect,
	int color, int opaque)
{
	command_lock->lock("GLThread::guideline");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_LINE;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->glwin1 = *rect;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guiderectangle(BC_WindowBase *window, struct gl_window *rect,
	struct gl_window *canvsize, int color, int opaque)
{
	command_lock->lock("GLThread::guiderectangle");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_RECTANGLE;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->glwin1 = *rect;
	command->glwin2 = *canvsize;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guidebox(BC_WindowBase *window, struct gl_window *rect,
	int color, int opaque)
{
	command_lock->lock("GLThread::guidebox");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_BOX;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->glwin1 = *rect;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guidedisc(BC_WindowBase *window, struct gl_window *rect,
	int color, int opaque)
{
	command_lock->lock("GLThread::guidedisc");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_DISC;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->glwin1 = *rect;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guidecircle(BC_WindowBase *window, struct gl_window *rect,
	int color, int opaque)
{
	command_lock->lock("GLThread::guidecircle");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_CIRCLE;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->glwin1 = *rect;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guidepixel(BC_WindowBase *window, int x, int y,
	int color, int opaque)
{
	command_lock->lock("GLThread::guidepixel");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_PIXEL;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->width = x;
	command->height = y;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::guideframe(BC_WindowBase *window, VFrame *frame,
	int color, int opaque)
{
	command_lock->lock("GLThread::guideframe");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::GUIDE_FRAME;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->frame = frame;
	command->color = color;
	command->opaque = opaque;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::release_resources()
{
	command_lock->lock("GLThread::release_resources");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::RELEASE_RESOURCES;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::disable_opengl(BC_WindowBase *window)
{
	command_lock->lock("GLThread::disable_opengl");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::DISABLE;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::swap_buffers(BC_WindowBase *window)
{
	command_lock->lock("GLThread::swap_buffers");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::SWAP_BUFFERS;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command_lock->unlock();
	next_command->unlock();
}
#endif

void GLThread::run()
{
	while(!done)
	{
		next_command->lock("GLThread::run");

		command_lock->lock("GLThread::run");
		for(int i = 0; i < last_command; i++)
		{
			GLThreadCommand *command = commands[i];
			handle_command_base(command);
		}
		last_command = 0;
		command_lock->unlock();
	}
}

void GLThread::handle_command_base(GLThreadCommand *command)
{
	if(command)
	{
		switch(command->command)
		{
		case GLThreadCommand::QUIT:
			delete_contexts();
			done = 1;
			break;
#ifdef HAVE_GL
		case GLThreadCommand::DISPLAY_VFRAME:
			do_display_vframe(command);
			break;

		case GLThreadCommand::RELEASE_RESOURCES:
			do_release_resources(current_glctx);
			break;

		case GLThreadCommand::DISABLE:
			do_disable_opengl(command);
			break;

		case GLThreadCommand::GUIDE_RECTANGLE:
			guides.add_guide(command);
			break;

		case GLThreadCommand::SWAP_BUFFERS:
			do_swap_buffers(command);
			break;
#endif
		default:
			handle_command(command);
			break;
		}
	}
}

void GLThread::handle_command(GLThreadCommand *command)
{
}

void GLThread::delete_contexts()
{
#ifdef HAVE_GL
	for(int i = 0; i < last_context; i++)
	{
		if(contexts[i].dpy)
		{
			XFree(contexts[i].visinfo);
			glXDestroyContext(contexts[i].dpy, contexts[i].gl_context);
			contexts[i].dpy = 0;
			contexts[i].gl_context = 0;
		}
	}
#endif
}

#ifdef HAVE_GL
void GLThread::do_disable_opengl(GLThreadCommand *command)
{
	if(struct glctx *ctx = have_context(command->dpy, command->screen))
	{
		if(ctx->vertexarray)
			do_release_resources(ctx);
		XFree(ctx->visinfo);
		glXMakeCurrent(ctx->dpy, None, NULL);
		glXDestroyContext(ctx->dpy, ctx->gl_context);
		ctx->dpy = 0;
		ctx->gl_context = 0;
		if(ctx == current_glctx)
			current_glctx = 0;
	}
}

void GLThread::do_display_vframe(GLThreadCommand *command)
{
	if(initialize(command->dpy, command->win, command->screen))
		return;
	if(!current_glctx->vertexarray)
	{
		set_viewport(command);
		generate_renderframe();
	}
	glBindTexture(GL_TEXTURE_2D, current_glctx->firsttexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, command->frame->get_w(),
		command->frame->get_h(),
		0, GL_RGBA, GL_UNSIGNED_SHORT, command->frame->get_data());
	// Clear the screen to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(current_glctx->shaderprogram);
	// Draw a rectangle from the 2 triangles using 6 indices
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
show_program_params(current_glctx->shaderprogram);
show_shaders(current_glctx->shaderprogram, 4);
show_uniforms(current_glctx->shaderprogram, 4);
show_attributes(current_glctx->shaderprogram, 4);
}

void GLThread::do_swap_buffers(GLThreadCommand *command)
{
	guides.draw(current_glctx);
	glXSwapBuffers(command->dpy, command->win);
}

GLuint GLThread::create_texture(int num, int width, int height)
{
	struct texture *txp;

	txp = &current_glctx->textures[num];
	glGenTextures(1, &txp->id);
	txp->width = width;
	txp->height = height;
	glBindTexture(GL_TEXTURE_2D, txp->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
		GL_UNSIGNED_SHORT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	return txp->id;
}

void GLThread::do_release_resources(struct glctx *ctx)
{
	glDeleteTextures(1, &ctx->firsttexture);
	glDeleteProgram(ctx->shaderprogram);
	glDeleteShader(ctx->fragmentshader);
	glDeleteShader(ctx->vertexshader);
	glDeleteBuffers(1, &ctx->elemarray);
	glDeleteBuffers(1, &ctx->vertexbuffer);
	glDeleteVertexArrays(1, &ctx->vertexarray);
	ctx->vertexarray = 0;
}

void GLThread::set_viewport(GLThreadCommand *command)
{
	double aspect = command->frame->get_pixel_aspect();
	int w = round(command->frame->get_w() * aspect);
	int h = command->frame->get_h();
	int x = round(command->glwin2.x1);
	int y = round(command->glwin2.y1) + command->height - h;

	if(!aspect)
		aspect = 1.0;
	memcpy(current_glctx->vertices, vertices, GL_VERTICES_SIZE * sizeof(float));
	if(!EQUIV(command->glwin1.x1, 1.0))
	{
		double s = command->glwin1.x1 / w * aspect;
		current_glctx->vertices[5] += s;
		current_glctx->vertices[12] += s;
		current_glctx->vertices[19] += s;
		current_glctx->vertices[26] += s;
	}
	if(!EQUIV(command->glwin1.y1, 1.0))
	{
		double s = command->glwin1.y1 / h;
		current_glctx->vertices[6] += s;
		current_glctx->vertices[13] += s;
		current_glctx->vertices[20] += s;
		current_glctx->vertices[27] += s;
	}
	current_glctx->canvas_zoom = command->zoom;
	if(!EQUIV(command->zoom, 1.0))
	{
		float sz = 2 * command->zoom;

		current_glctx->vertices[14] = current_glctx->vertices[7] =
			current_glctx->vertices[0] + sz;
		current_glctx->vertices[22] = current_glctx->vertices[15] =
			current_glctx->vertices[1] - sz;
	}
	glViewport(x, y, w, h);
}
#endif

#ifdef HAVE_GL
// Debug functions
void GLThread::show_glparams(int indent)
{
	GLint auxbufnum;
	GLint max3dtexsiz;
	GLint maxdrawbufs;
	GLint maxtexiunits;
	GLint maxtexsize;
	GLint maxtexunits;
	GLint maxcomtexunits;
	GLint viewportdims[2];
	GLint viewport[4];

	glGetIntegerv(GL_AUX_BUFFERS, &auxbufnum);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3dtexsiz);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxdrawbufs);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxtexiunits);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxtexunits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxcomtexunits);
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportdims);
	glGetIntegerv(GL_VIEWPORT, viewport);
	printf("%*sSupported GLSL version is %s.\n", indent, "",
		(char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	indent++;
	printf("%*sSome GL parameter values:\n", indent, "");
	indent += 2;
	printf("%*sNumber of auxiliary color buffers available: %d\n", indent, "",
		auxbufnum);
	printf("%*sLargest 3D texture that the GL can handle: %d\n", indent, "",
		max3dtexsiz);
	printf("%*sMaximum number of simultaneous output colors: %d\n", indent, "",
		maxdrawbufs);
	printf("%*sMaximum supported texture image units: %d\n", indent, "",
		maxtexiunits);
	printf("%*sLargest texture that the GL can handle: %d\n", indent, "",
		maxtexsize);
	printf("%*sNumber of conventional texture units supported: %d\n", indent, "",
		maxtexunits);
	printf("%*sMaximum supported combined texture image units: %d\n", indent, "",
		maxcomtexunits);
	printf("%*sMaximum supported width and height of the viewport %dx%d\n", indent, "",
		viewportdims[0], viewportdims[1]);
	printf("%*sCurrent viewport: (%d,%d) [%d,%d]\n", indent, "",
		viewport[0], viewport[1],
		viewport[2], viewport[3]);
}

void GLThread::show_glxcontext(struct glctx *ctx, int indent)
{
	int use_gl;
	int buffer_size;
	int level;
	int rgba;
	int doublebuffer;
	int stereo;
	int aux_bufs;
	int red, green, blue, alpha, depth, stencil;
	int acured, acugreen, acublue, acualpha;
	int maj, min;
	Display *dpy = ctx->dpy;
	XVisualInfo *visinfo = ctx->visinfo;

	glXQueryVersion(dpy, &maj, &min);
	printf("%*sGLX version %d.%d context %p\n", indent, "", maj, min, ctx);
	indent += 2;
	printf("%*sDirect rendering: %s\n", indent, "",
		glXIsDirect(dpy, ctx->gl_context) ? "Yes" : "No");
	glXGetConfig(dpy, visinfo, GLX_USE_GL, &use_gl);
	glXGetConfig(dpy, visinfo, GLX_BUFFER_SIZE, &buffer_size);
	glXGetConfig(dpy, visinfo, GLX_LEVEL, &level);
	glXGetConfig(dpy, visinfo, GLX_RGBA, &rgba);
	glXGetConfig(dpy, visinfo, GLX_DOUBLEBUFFER, &doublebuffer);
	glXGetConfig(dpy, visinfo, GLX_STEREO, &rgba);
	glXGetConfig(dpy, visinfo, GLX_AUX_BUFFERS, &aux_bufs);
	glXGetConfig(dpy, visinfo, GLX_RED_SIZE, &red);
	glXGetConfig(dpy, visinfo, GLX_GREEN_SIZE, &green);
	glXGetConfig(dpy, visinfo, GLX_BLUE_SIZE, &blue);
	glXGetConfig(dpy, visinfo, GLX_ALPHA_SIZE, &alpha);
	glXGetConfig(dpy, visinfo, GLX_DEPTH_SIZE, &depth);
	glXGetConfig(dpy, visinfo, GLX_STENCIL_SIZE, &stencil);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_RED_SIZE, &acured);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_GREEN_SIZE, &acugreen);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_BLUE_SIZE, &acublue);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_ALPHA_SIZE, &acualpha);

	printf("%*sGlx context: use_gl %d buffer_size %d buffer_level %d\n", indent, "",
		use_gl, buffer_size, level);
	printf("%*srgba %d doublebuffer %d stereo %d aux_bufs %d\n", indent, "",
		rgba, doublebuffer, stereo, aux_bufs);
	printf("%*sred %d green %d blue %d alpha %d depth %d stencil %d\n", indent, "",
		red, green, blue, alpha, depth, stencil);
	printf("%*saccum red %d green %d blue %d alpha %d\n", indent, "",
		acured, acugreen, acublue, acualpha);
}

void GLThread::dump_glctx(struct glctx *ctx, int indent)
{
	printf("%*sglctx %p dump:\n", indent, "", ctx);
	indent += 2;
	printf("%*sdpy %p win %#lx screen %d\n", indent, "", ctx->dpy,
		ctx->win, ctx->screen);
	printf("%*sgl_context %p visinfo %p\n", indent, "", ctx->gl_context, ctx->visinfo);
	printf("%*svertexarray %d vertexbuffer %d elemarray %d vertexshader %d\n",
		indent, "", ctx->vertexarray, ctx->vertexbuffer,
		ctx->elemarray, ctx->vertexshader);
	printf("%*sfragmentshader %d shaderprogram %d firsttexture %d fbtexture %d\n",
		indent, "", ctx->fragmentshader, ctx->shaderprogram,
		ctx->firsttexture, ctx->fbtexture);
	printf("%*sposattrib %d colattrib %d texattrib %d\n", indent, "",
		ctx->posattrib, ctx->colattrib, ctx->texattrib);
}

void GLThread::show_errors(const char *loc, int indent)
{
	GLenum err;
	const char *s = 0;

	while((err = glGetError()) != GL_NO_ERROR)
	{
		switch(err)
		{
		case GL_INVALID_ENUM:
			s = "Invalid enum";
			break;
		case GL_INVALID_VALUE:
			s = "Invalid value";
			break;
		case GL_INVALID_OPERATION:
			s = "Invalid operation";
			break;
		case GL_STACK_OVERFLOW:
			s = "Stack overflow";
			break;
		case GL_STACK_UNDERFLOW:
			s = "Stack underflow";
			break;
		case GL_OUT_OF_MEMORY:
			s = "Out of memory";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			s = "Invalid framebuffer operation";
			break;
		case GL_CONTEXT_LOST:
			s = "Context lost";
			break;
		case GL_TABLE_TOO_LARGE:
			s = "Table too large";
			break;
		default:
			s = "Unknown";
			break;
		}
		printf("%*sOpenGL ERROR: %s.\n", indent, "", s);
	}
	if(loc)
	{
		if(s)
			printf("%*sEnd of error list at '%s'\n", indent, "", loc);
		else
			printf("%*sNo errors at '%s'\n", indent, "", loc);
	}
	else
	{
		if(s)
			printf("%*sEnd of error list\n", indent, "");
		else
			printf("%*sNo opengl errors\n", indent, "");
	}
}

void GLThread::show_compile_status(GLuint shader, const char *name)
{
	GLint status;
	char msg_buffer[BCTEXTLEN];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	printf("Compilation of shader '%s' %s\n", name,
		status ? "succeeded" : "failed");
	msg_buffer[0] = 0;
	glGetShaderInfoLog(shader, BCTEXTLEN, NULL, msg_buffer);
	if(msg_buffer[0])
	{
		fputs("Logi:\n", stdout);
		fputs(msg_buffer, stdout);
	}
}

void GLThread::show_link_status(GLuint program, const char *name)
{
	GLint status;
	char msg_buffer[BCTEXTLEN];

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	printf("Linking of program '%s(%d)' %s\n", name, program,
		status ? "succeeded" : "failed");
	if(!status)
	{
		glGetShaderInfoLog(program, BCTEXTLEN, NULL, msg_buffer);
		fputs("Logi:\n", stdout);
		fputs(msg_buffer, stdout);
	}
}

void GLThread::show_validation(GLuint program)
{
	GLint param;
	GLsizei mlen;
	char msg_buffer[BCTEXTLEN];

	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &param);
	printf("Validated program %d status %d\n", program, param);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &param);
	if(param)
	{
		printf("Info log length %d\n", param);
		glGetProgramInfoLog(program, BCTEXTLEN, &mlen, msg_buffer);
		fputs(msg_buffer, stdout);
	}
}

void GLThread::check_framebuffer_status(int indent)
{
	const char *str;
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		str = "Complete";
		break;
	case GL_FRAMEBUFFER_UNDEFINED:
		str = "Undefined";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		str = "Incomplete attachment";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		str = "Missing attachment";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		str = "Draw buffer incomplete";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		str = "Read buffer incomplete";
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		str = "Unsupported by implementation";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		str = "Incomplete multisample";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		str = "Incomplete layer targets";
		break;
	case GL_INVALID_ENUM:
		str = "Invalid enum";
		break;
	case 0:
		str = "Error: zero returned";
		break;
	default:
		str = "Unknown result";
		break;
	}
	printf("%*sFramebuffer status '%s'\n", indent, "", str);
}


void GLThread::show_program_params(GLuint program, int indent)
{
	GLint param;

	printf("%*sProgram %d params:\n", indent, "", program);
	glGetProgramiv(program, GL_DELETE_STATUS, &param);
	indent += 2;
	printf("%*sDelete status %d\n", indent, "", param);
	glGetProgramiv(program, GL_LINK_STATUS, &param);
	printf("%*sLink status %d\n", indent, "", param);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &param);
	printf("%*sValidate status %d\n", indent, "", param);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &param);
	printf("%*sInfo log length %d\n", indent, "", param);
	glGetProgramiv(program, GL_ATTACHED_SHADERS, &param);
	printf("%*sAttached shaders %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &param);
	printf("%*sActive attributes %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &param);
	printf("%*sActive attribute max length %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &param);
	printf("%*sActive uniforms %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &param);
	printf("%*sActive uniform max length %d\n", indent, "", param);
}

void GLThread::show_renderbuffer_params(GLuint id, int indent)
{
	GLint param;

	if(glIsRenderbuffer(id) == GL_FALSE)
	{
		printf("ID %d is not a renderbuffer object", id);
		return;
	}
	glBindRenderbuffer(GL_RENDERBUFFER, id);
	printf("%*sRenderbuffer (%d):\n", indent, "", id);
	indent+= 2;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &param);
	printf("%*sWidth: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &param);
	printf("%*sHeight: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER,
		GL_RENDERBUFFER_INTERNAL_FORMAT, &param);
	printf("%*sInternal format: '%s'\n", indent, "", glname(param));
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_RED_SIZE, &param);
	printf("%*sRed size: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_GREEN_SIZE, &param);
	printf("%*sGreen size: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_BLUE_SIZE, &param);
	printf("%*sBlue size: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_ALPHA_SIZE, &param);
	printf("%*sAlpha size: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &param);
	printf("%*sDepth size: %d\n", indent, "", param);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &param);
	printf("%*sStencil size: %d\n", indent, "", param);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void GLThread::show_fb_attachment(GLuint objid, GLint objtype,
	const char *atname, int indent)
{
	const char *typestr, *paramstr;

	switch(objtype)
	{
	case GL_TEXTURE:
		typestr = "Texture";
		paramstr = textureparamstr(objid);
		break;
	case GL_RENDERBUFFER:
		typestr = "Renderbuffer";
		paramstr = renderbufferparamstr(objid);
		break;
	default:
		typestr = "Unknown";
		paramstr = 0;
		break;
	}
	printf("%*s%s attachment %s, %s\n", indent, "", atname, typestr, paramstr);
}

const char *GLThread::textureparamstr(GLuint id)
{
	int width, height, format;
	static char buf[256];

	if(glIsTexture(id) == GL_FALSE)
		return "Not a texture object";

	glBindTexture(GL_TEXTURE_2D, id);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
		GL_TEXTURE_INTERNAL_FORMAT, &format);
	glBindTexture(GL_TEXTURE_2D, 0);
	sprintf(buf, "[%d,%d] %s", width, height, glname(format));
	return buf;
}

const char *GLThread::renderbufferparamstr(GLuint id)
{
	int width, height, format, samples;
	static char buf[256];

	if(glIsRenderbuffer(id) == GL_FALSE)
		return "Not a renderbuffer object";

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER,
		GL_RENDERBUFFER_HEIGHT, &height);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER,
		GL_RENDERBUFFER_INTERNAL_FORMAT, &format);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER,
		GL_RENDERBUFFER_SAMPLES, &samples);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	sprintf(buf, "[%d,%d] %s MSAA(%d)", width, height,
		glname(format), samples);
	return buf;
}

void GLThread::show_framebuffer_status(GLuint fbo, int indent)
{
	GLint param;
	int cbcount, objtype, objid;
	const char *typestr, *paramstr;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &cbcount);
	glGetIntegerv(GL_MAX_SAMPLES, &param);
	printf("%*sFramebuffer %u status\n", indent, "", fbo);
	indent += 2;
	printf("%*sMax colorbuffers %d max samples of MSAA %d\n", indent, "",
		cbcount, param);

	for(int i = 0; i < cbcount; i++)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + i,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objtype);

		if(objtype != GL_NONE)
		{
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + i,
				GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objid);
			show_fb_attachment(objid, objtype, "Color", indent);
		}
	}
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objtype);
	if(objtype != GL_NONE)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objid);
		show_fb_attachment(objid, objtype, "Depth", indent);
	}
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objtype);

	if(objtype != GL_NONE)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
			GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
			&objid);
		show_fb_attachment(objid, objtype, "Stencil", indent);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

VFrame *GLThread::get_framebuffer_data(GLuint fbo)
{
	GLint height, width, cbcount;
	GLint objtype, objid;
	int i;
	VFrame *frame = 0;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	width = height = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &cbcount);
	for(i = 0; i < cbcount; i++)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + i,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objtype);
		if(objtype != GL_NONE)
		{
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + i,
				GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objid);
			switch(objtype)
			{
			case GL_TEXTURE:
				glBindTexture(GL_TEXTURE_2D, objid);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
					GL_TEXTURE_WIDTH, &width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
					GL_TEXTURE_HEIGHT, &height);
				glBindTexture(GL_TEXTURE_2D, 0);
				break;
			case GL_RENDERBUFFER:
				glBindRenderbuffer(GL_RENDERBUFFER, objid);
				glGetRenderbufferParameteriv(GL_RENDERBUFFER,
					GL_RENDERBUFFER_WIDTH, &width);
				glGetRenderbufferParameteriv(GL_RENDERBUFFER,
					GL_RENDERBUFFER_HEIGHT, &height);
				glBindRenderbuffer(GL_RENDERBUFFER, 0);
				break;
			default:
				continue;
			}
		}
		break;
	}

	if(!width || !height)
	{
		show_errors("get_framebuffer_dat");
		fputs("get_framebuffer_data: No width or height\n", stdout);
		return 0;
	}
	printf("Reading data [%d,%d]..\n", width, height);
	if(frame = BC_Resources::tmpframes.get_tmpframe(width, height,
			BC_RGB8, "get_framebuffer_data"))
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE,
			frame->get_data());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return frame;
}

void GLThread::show_shaders(GLuint program, int indent)
{
	GLuint shaders[20];
	GLsizei count;
	GLint params[20];

	if(!glIsProgram(program))
	{
		printf("%d is not a program\n", program);
		return;
	}
	glGetAttachedShaders(program, 20, &count, shaders);
	printf("%*sProgram %d has %d shaders:\n", indent, "", program, count);
	indent += 2;
	for(int i = 0; i < count; i++)
	{
		glGetShaderiv(shaders[i], GL_SHADER_TYPE, params);
		printf("%*sattached shader: %u %s\n", indent, "",
			shaders[i], glname(params[0]));
		show_shader_src(shaders[i], indent);
	}
}

void GLThread::show_uniforms(GLuint program, int indent)
{
	GLint n, max, i;

	if(!glIsProgram(program))
	{
		printf("%d is not a program\n", program);
		return;
	}
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);
	printf("%*sProgram %d has %d uniforms with max name %d:\n", indent, "",
		program, n, max);

	indent += 2;
	for(int i = 0; i < n; i++)
	{
		GLint size, len;
		GLenum type;
		char name[100];
		glGetActiveUniform(program, i, 100, &len, &size, &type, name);

		printf("%*s%d: '%s' nameLen: %d size: %d type: '%s'\n", indent, "",
			i, name, len, size, glname(type));
	}
}

void GLThread::show_attributes(GLuint program, int indent)
{
	GLint n, max, i;

	if(!glIsProgram(program))
	{
		printf("%d is not a program\n", program);
		return;
	}
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max);
	printf("%*sProgram %d has %d attributes with max name %d:\n", indent, "",
		program, n, max);

	indent += 2;
	for(int i = 0; i < n; i++)
	{
		GLint size, len;
		GLenum type;
		char name[100];
		glGetActiveAttrib(program, i, 100, &len, &size, &type, name);

		printf("%*s%d: '%s' nameLen: %d size: %d type: '%s'\n", indent, "",
			i, name, len, size, glname(type));
	}
}

void GLThread::show_texture2d_params(GLuint tex, int indent)
{
	int width, height, format, depth;
	int red, green, blue, alpha, depth_sz;
	int luminance, intensity;
	float coords[4];

	if(glIsTexture(tex) == GL_FALSE)
	{
		printf("%*sID %d is not a texture\n", indent, "", tex);
		return;
	}
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
		GL_TEXTURE_INTERNAL_FORMAT, &format);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH, &depth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &red);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &green);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &blue);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &alpha);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_LUMINANCE_SIZE, &luminance);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTENSITY_SIZE, &intensity);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH_SIZE, &depth_sz);
	glGetFloatv(GL_CURRENT_TEXTURE_COORDS, coords);
	glBindTexture(GL_TEXTURE_2D, 0);

	printf("%*sTexture %d [%d, %d] depth %d fmt:%s\n", indent, "",
		tex, width, height, depth, glname(format));
	indent += 2;
	printf("%*ssizes r:%d g:%d b:%d alph:%d, lum:%d, ints:%d\n", indent, "",
		red, green, blue, alpha, luminance, intensity);
	printf("%*scoords %.3f, %.3f, %.3f, %.3f\n", indent, "",
		coords[0], coords[1], coords[2], coords[3]);
}

void GLThread::show_matrixes(int indent)
{
	GLint matmode;
	GLfloat modlmatx[16];
	GLfloat projmatx[16];
	GLfloat textmatx[16];

	glGetIntegerv(GL_MATRIX_MODE, &matmode);
	glGetFloatv(GL_MODELVIEW_MATRIX, modlmatx);
	glGetFloatv(GL_PROJECTION_MATRIX, projmatx);
	glGetFloatv(GL_TEXTURE_MATRIX, textmatx);
	printf("%*sMatrix mode %#x (%s)\n", indent, "", matmode, glname(matmode));
	print_mat4(modlmatx, "Model matrix:", indent);
	print_mat4(projmatx, "Projection matrix:", indent);
	print_mat4(textmatx, "Texture matrix:", indent);
}

void GLThread::print_mat4(GLfloat *matx, const char *name, int indent)
{
	printf("%*s%s\n", indent, "", name);
	indent += 2;
	for(int i = 0; i < 4; i++)
	{
		printf("%*s", indent, "");
		for(int j = 0; j < 4; j++)
			printf(" %6.2f", matx[4 * i + j]);
		putchar('\n');
	}
}

void GLThread::show_shader_src(GLuint shader, int indent)
{
	char buf[BCTEXTLEN];
	GLsizei len;

	glGetShaderSource(shader, BCTEXTLEN, &len, buf);
	printf("%*sShader %d src length %d\n", indent, "", shader, len);
	puts(buf);
}

void GLThread::show_vertex_array(int indent)
{
	GLint bufbind, countbuf, readbuf, writebuf;
	GLint drawbuf, dispbuf, elemarry, pixpack;
	GLint pixunpack, stobuf, fdbuf, ufrmbuf;

	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufbind);
	glGetIntegerv(GL_ATOMIC_COUNTER_BUFFER_BINDING, &countbuf);
	glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &readbuf);
	glGetIntegerv(GL_COPY_WRITE_BUFFER_BINDING, &writebuf);
	glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &drawbuf);
	glGetIntegerv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &dispbuf);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elemarry);
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pixpack);
	glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pixunpack);
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &stobuf);
	glGetIntegerv(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, &fdbuf);
	glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &ufrmbuf);

	printf("%*sVertex array object bindings:\n", indent, "");
	indent += 2;
	printf("%*sArray buffer %d\n", indent, "", bufbind);
	printf("%*sAtomic counter buffer %d\n", indent, "", countbuf);
	printf("%*sCopy read buffer %d\n", indent, "", readbuf);
	printf("%*sCopy write buffer %d\n", indent, "", writebuf);
	printf("%*sDraw indirect buffer %d\n", indent, "", drawbuf);
	printf("%*sDispatch buffer %d\n", indent, "", dispbuf);
	printf("%*sElement array %d\n", indent, "", elemarry);
	printf("%*sPixel pack buffer %d\n", indent, "", pixpack);
	printf("%*sPixel unpack buffer %d\n", indent, "", pixunpack);
	printf("%*sShader storage buffer %d\n", indent, "", stobuf);
	printf("%*sTransform feedback buffer %d\n", indent, "", fdbuf);
	printf("%*sUniform buffer %d\n", indent, "", ufrmbuf);
}

const char *GLThread::glname(GLenum type)
{
	static char bufr[64];

	switch(type)
	{
	case GL_BLEND_EQUATION_RGB:                   // 0x8009
		return "Blend equation RGB";
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:          // 0x8622
		return "Vertex attrib array enabled";
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:             // 0x8623
		return "Vertex attrib array size";
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:           // 0x8624
		return "Vertex attrib array stride";
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:             // 0x8625
		return "Vertex attrib array";
	case GL_CURRENT_VERTEX_ATTRIB:                // 0x8626
		return "Current vertex attrib";
	case GL_VERTEX_PROGRAM_POINT_SIZE:            // 0x8642
		return "Vertex program point size";
	case GL_VERTEX_ATTRIB_ARRAY_POINTER:          // 0x8645
		return "Vertex attrib array pointer";
	case GL_STENCIL_BACK_FUNC:                    // 0x8800
		return "Stencil back func";
	case GL_STENCIL_BACK_FAIL:                    // 0x8801
		return "Stencil back fail";
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL:         // 0x8802
		return "Stencil back pass depth fail";
	case GL_STENCIL_BACK_PASS_DEPTH_PASS:         // 0x8803
		return "Stencil back pass depth pass";
	case GL_MAX_DRAW_BUFFERS:                    // 0x8824
		return "Max draw buffers";
	case GL_DRAW_BUFFER0:                        // 0x8825
		return "Draw buffer 0";
	case GL_DRAW_BUFFER1:                        // 0x8826
		return "Draw buffer 1";
	case GL_DRAW_BUFFER2:                        // 0x8827
		return "Draw buffer 2";
	case GL_DRAW_BUFFER3:                        // 0x8828
		return "Draw buffer 3";
	case GL_DRAW_BUFFER4:                        // 0x8829
		return "Draw buffer 4";
	case GL_DRAW_BUFFER5:                        // 0x882A
		return "Draw buffer 5";
	case GL_DRAW_BUFFER6:                        // 0x882B
		return "Draw buffer 6";
	case GL_DRAW_BUFFER7:                        // 0x882C
		return "Draw buffer 7";
	case GL_DRAW_BUFFER8:                        // 0x882D
		return "Draw buffer 8";
	case GL_DRAW_BUFFER9:                        // 0x882E
		return "Draw buffer 9";
	case GL_DRAW_BUFFER10:                       // 0x882F
		return "Draw buffer 10";
	case GL_DRAW_BUFFER11:                       // 0x8830
		return "Draw buffer 11";
	case GL_DRAW_BUFFER12:                       // 0x8831
		return "Draw buffer 12";
	case GL_DRAW_BUFFER13:                       // 0x8832
		return "Draw buffer 13";
	case GL_DRAW_BUFFER14:                       // 0x8833
		return "Draw buffer 14";
	case GL_DRAW_BUFFER15:                       // 0x8834
		return "Draw buffer 15";
	case GL_BLEND_EQUATION_ALPHA:                // 0x883D
		return "Blend equation alpha";
	case GL_MAX_VERTEX_ATTRIBS:                  // 0x8869
		return "Max vertex attribs";
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:      // 0x886A
		return "Vertex attrib array normalized";
	case GL_MAX_TEXTURE_IMAGE_UNITS:             // 0x8872
		return "Max texture image units";
	case GL_FRAGMENT_SHADER:                     // 0x8B30
		return "Fragment shader";
	case GL_VERTEX_SHADER:                       // 0x8B31
		return "Vertex shader";
	case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:     // 0x8B49
		return "Max fragment uniform components";
	case GL_MAX_VERTEX_UNIFORM_COMPONENTS:       // 0x8B4A
		return "Max vertex uniform components";
	case GL_MAX_VARYING_FLOATS:                  // 0x8B4B
		return "Max varying floats";
	case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:      // 0x8B4C
		return "Max vertex texture image units";
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:    // 0x8B4D
		return "Max combined texture image units";
	case GL_SHADER_TYPE:                         // 0x8B4F
		return "Shader type";
	case GL_FLOAT_VEC2:                          // 0x8B50
		return "Float vec2";
	case GL_FLOAT_VEC3:                          // 0x8B51
		return "Float vec3";
	case GL_FLOAT_VEC4:                          // 0x8B52
		return "Float vec4";
	case GL_INT_VEC2:                            // 0x8B53
		return "Int vec2";
	case GL_INT_VEC3:                            // 0x8B54
		return "Int vec3";
	case GL_INT_VEC4:                            // 0x8B55
		return "Int vec4";
	case GL_BOOL:                                // 0x8B56
		return "Bool";
	case GL_BOOL_VEC2:                           // 0x8B57
		return "Bool vec2";
	case GL_BOOL_VEC3:                           // 0x8B58
		return "Bool vec3";
	case GL_BOOL_VEC4:                           // 0x8B59
		return "Bool vec4";
	case GL_FLOAT_MAT2:                          // 0x8B5A
		return "Float mat2";
	case GL_FLOAT_MAT3:                          // 0x8B5B
		return "Float mat3";
	case GL_FLOAT_MAT4:                          // 0x8B5C
		return "Float mat4";
	case GL_SAMPLER_1D:                          // 0x8B5D
		return "Sampler 1D";
	case GL_SAMPLER_2D:                          // 0x8B5E
		return "Sampler 2D";
	case GL_SAMPLER_3D:                          // 0x8B5F
		return "Sampler 3D";
	case GL_SAMPLER_CUBE:                        // 0x8B60
		return "Sampler cube";
	case GL_SAMPLER_1D_SHADOW:                   // 0x8B61
		return "Sampler 1D shadow";
	case GL_SAMPLER_2D_SHADOW:                   // 0x8B62
		return "Sampler 2D shadow";
	case GL_STENCIL_INDEX:                       // 0x1901
		return "Stencil index";
	case GL_DEPTH_COMPONENT:                     // 0x1902
		return "Depth component";
	case GL_ALPHA:                               // 0x1906
		return "Alpha";
	case GL_RGB:                                 // 0x1907
		return "RGB";
	case GL_RGBA:                                // 0x1908
		return "RGBA";
	case GL_LUMINANCE:                           // 0x1909
		return "Luminance";
	case GL_LUMINANCE_ALPHA:                     // 0x190A
		return "Luminance Alpha";
	case GL_R3_G3_B2:                            // 0x2A10
		return "R3G3B2";
	case GL_ALPHA4:                              // 0x803B
		return "Alpha4";
	case GL_ALPHA8:                              // 0x803C
		return "Alpha8";
	case GL_ALPHA12:                             // 0x803D
		return "Alpha12";
	case GL_ALPHA16:                             // 0x803E
		return "Alpha16";
	case GL_LUMINANCE4:                          // 0x803F
		return "Luminance4";
	case GL_LUMINANCE8:                          // 0x8040
		return "Luminance8";
	case GL_LUMINANCE12:                         // 0x8041
		return "Luminance12";
	case GL_LUMINANCE16:                         // 0x8042
		return "Luminance16";
	case GL_LUMINANCE4_ALPHA4:                   // 0x8043
		return "Luminance4 Alpha4";
	case GL_LUMINANCE6_ALPHA2:                   // 0x8044
		return "Luminance6 Alpha2";
	case GL_LUMINANCE8_ALPHA8:                   // 0x8045
		return "Luminance8 Alpha8";
	case GL_LUMINANCE12_ALPHA4:                  // 0x8046
		return "Luminance12 Alpha4";
	case GL_LUMINANCE12_ALPHA12:                 // 0x8047
		return "Luminance12 Alpha12";
	case GL_LUMINANCE16_ALPHA16:                 // 0x8048
		return "GL_Luminance16 Alpha16";
	case GL_INTENSITY:                           // 0x8049
		return "Intensity";
	case GL_INTENSITY4:                          // 0x804A
		return "Intensity4";
	case GL_INTENSITY8:                          // 0x804B
		return "Intensity8";
	case GL_INTENSITY12:                         // 0x804C
		return "Intensity12";
	case GL_INTENSITY16:                         // 0x804D
		return "Intensity16";
	case GL_RGB4:                                // 0x804F
		return "RGB4";
	case GL_RGB5:                                // 0x8050
		return "RGB5";
	case GL_RGB8:                                // 0x8051
		return "RGB8";
	case GL_RGB10:                               // 0x8052
		return "RGB10";
	case GL_RGB12:                               // 0x8053
		return "RGB12";
	case GL_RGB16:                               // 0x8054
		return "RGB16";
	case GL_RGBA2:                               // 0x8055
		return "RGBA2";
	case GL_RGBA4:                               // 0x8056
		return "RGBA4";
	case GL_RGB5_A1:                             // 0x8057
		return "RGB5A1";
	case GL_RGBA8:                               // 0x8058
		return "GL_RGBA8";
	case GL_RGB10_A2:                            // 0x8059
		return "RGB10A2";
	case GL_RGBA12:                              // 0x805A
		return "RGBA12";
	case GL_RGBA16:                              // 0x805B
		return "RGBA16";
	case GL_DEPTH_COMPONENT16:                   // 0x81A5
		return "Depth Component16";
	case GL_DEPTH_COMPONENT24:                   // 0x81A6
		return "Depth Component24";
	case GL_DEPTH_COMPONENT32:                   // 0x81A7
		return "Depth Component32";
	case GL_DEPTH_STENCIL:                       // 0x84F9
		return "Depth Stencil";
	case GL_RGBA32F:                             // 0x8814
		return "RGBA32F";
	case GL_RGB32F:                              // 0x8815
		return "RGB32F";
	case GL_RGBA16F:                             // 0x881A
		return "RGBA16F";
	case GL_RGB16F:                              // 0x881B
		return "RGB16F";
	case GL_DEPTH24_STENCIL8:                    // 0x88F0
		return "Depth24 Stencil8";
	case GL_MATRIX_MODE:                         // 0x0BA0
		return "Matrix mode";
	case GL_MODELVIEW:                           // 0x1700
		return "Modelview";
	case GL_PROJECTION:                          // 0x1701
		return "Projection";
	case GL_TEXTURE:                             // 0x1702
		return "Texture";
	default:
		sprintf(bufr, "Unknown %#x", type);
		return bufr;
	}
}
#endif
