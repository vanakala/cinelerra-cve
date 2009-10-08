
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

#include "bcsignals.h"
#include "bcwindowbase.h"
#include "canvas.h"
#include "clip.h"
#include "condition.h"
#include "maskautos.h"
#include "maskauto.h"
#include "mutex.h"
#include "overlayframe.inc"
#include "playback3d.h"
#include "pluginclient.h"
#include "pluginvclient.h"
#include "transportque.inc"
#include "vframe.h"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#endif

#include <string.h>
#include <unistd.h>


// Shaders
// These should be passed to VFrame::make_shader to construct shaders.
// Can't hard code sampler2D

static char *yuv_to_rgb_frag = 
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	vec3 yuv = vec3(texture2D(tex, gl_TexCoord[0].st));\n"
	"   yuv -= vec3(0, 0.5, 0.5);\n"
 	"	const mat3 yuv_to_rgb_matrix = mat3(\n"
 	"		1,       1,        1, \n"
 	"		0,       -0.34414, 1.77200, \n"
 	"		1.40200, -0.71414, 0);\n"
	"	gl_FragColor = vec4(yuv_to_rgb_matrix * yuv, 1);\n"
	"}\n";

static char *yuva_to_rgba_frag = 
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
	"	yuva.rgb -= vec3(0, 0.5, 0.5);\n"
 	"	const mat3 yuv_to_rgb_matrix = mat3(\n"
 	"		1,       1,        1, \n"
 	"		0,       -0.34414, 1.77200, \n"
 	"		1.40200, -0.71414, 0);\n"
	"	gl_FragColor = vec4(yuv_to_rgb_matrix * yuva.rgb, yuva.a);\n"
	"}\n";

static char *blend_add_frag = 
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"void main()\n"
	"{\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec3 opacity = vec3(gl_FragColor.a, gl_FragColor.a, gl_FragColor.a);\n"
	"	vec3 transparency = vec3(1.0, 1.0, 1.0) - opacity;\n"
	"	gl_FragColor.rgb += canvas.rgb;\n"
	"	gl_FragColor.rgb *= opacity;\n"
	"	gl_FragColor.rgb += canvas.rgb * transparency;\n"
	"	gl_FragColor.a = max(gl_FragColor.a, canvas.a);\n"
	"}\n";

static char *blend_max_frag = 
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"void main()\n"
	"{\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec3 opacity = vec3(gl_FragColor.a, gl_FragColor.a, gl_FragColor.a);\n"
	"	vec3 transparency = vec3(1.0, 1.0, 1.0) - opacity;\n"
	"	gl_FragColor.r = max(canvas.r, gl_FragColor.r);\n"
	"	gl_FragColor.g = max(canvas.g, gl_FragColor.g);\n"
	"	gl_FragColor.b = max(canvas.b, gl_FragColor.b);\n"
	"	gl_FragColor.rgb *= opacity;\n"
	"	gl_FragColor.rgb += canvas.rgb * transparency;\n"
	"	gl_FragColor.a = max(gl_FragColor.a, canvas.a);\n"
	"}\n";

static char *blend_subtract_frag = 
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"void main()\n"
	"{\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec3 opacity = vec3(gl_FragColor.a, gl_FragColor.a, gl_FragColor.a);\n"
	"	vec3 transparency = vec3(1.0, 1.0, 1.0) - opacity;\n"
	"	gl_FragColor.rgb = canvas.rgb - gl_FragColor.rgb;\n"
	"	gl_FragColor.rgb *= opacity;\n"
	"	gl_FragColor.rgb += canvas.rgb * transparency;\n"
	"	gl_FragColor.a = max(gl_FragColor.a, canvas.a);\n"
	"}\n";

static char *blend_multiply_frag = 
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"void main()\n"
	"{\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec3 opacity = vec3(gl_FragColor.a, gl_FragColor.a, gl_FragColor.a);\n"
	"	vec3 transparency = vec3(1.0, 1.0, 1.0) - opacity;\n"
	"	gl_FragColor.rgb *= canvas.rgb;\n"
	"	gl_FragColor.rgb *= opacity;\n"
	"	gl_FragColor.rgb += canvas.rgb * transparency;\n"
	"	gl_FragColor.a = max(gl_FragColor.a, canvas.a);\n"
	"}\n";

static char *blend_divide_frag = 
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"void main()\n"
	"{\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec3 opacity = vec3(gl_FragColor.a, gl_FragColor.a, gl_FragColor.a);\n"
	"	vec3 transparency = vec3(1.0, 1.0, 1.0) - opacity;\n"
	"	vec3 result = canvas.rgb / gl_FragColor.rgb;\n"
	"	if(!gl_FragColor.r) result.r = 1.0;\n"
	"	if(!gl_FragColor.g) result.g = 1.0;\n"
	"	if(!gl_FragColor.b) result.b = 1.0;\n"
	"	result *= opacity;\n"
	"	result += canvas.rgb * transparency;\n"
	"	gl_FragColor = vec4(result, max(gl_FragColor.a, canvas.a));\n"
	"}\n";

static char *multiply_alpha_frag = 
	"void main()\n"
	"{\n"
	"	gl_FragColor.rgb *= vec3(gl_FragColor.a, gl_FragColor.a, gl_FragColor.a);\n"
	"}\n";

static char *read_texture_frag = 
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"}\n";

static char *multiply_mask4_frag = 
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.a *= texture2D(tex1, gl_TexCoord[0].st / vec2(scale, scale)).r;\n"
	"}\n";

static char *multiply_mask3_frag = 
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float scale;\n"
	"uniform bool is_yuv;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"   float a = texture2D(tex1, gl_TexCoord[0].st / vec2(scale, scale)).r;\n"
	"	gl_FragColor.rgb *= vec3(a, a, a);\n"
	"}\n";

static char *multiply_yuvmask3_frag = 
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"   float a = texture2D(tex1, gl_TexCoord[0].st / vec2(scale, scale)).r;\n"
	"	gl_FragColor.gb -= vec2(0.5, 0.5);\n"
	"	gl_FragColor.rgb *= vec3(a, a, a);\n"
	"	gl_FragColor.gb += vec2(0.5, 0.5);\n"
	"}\n";

static char *fade_rgba_frag =
	"uniform sampler2D tex;\n"
	"uniform float alpha;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.a *= alpha;\n"
	"}\n";

static char *fade_yuv_frag =
	"uniform sampler2D tex;\n"
	"uniform float alpha;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.r *= alpha;\n"
	"	gl_FragColor.gb -= vec2(0.5, 0.5);\n"
	"	gl_FragColor.g *= alpha;\n"
	"	gl_FragColor.b *= alpha;\n"
	"	gl_FragColor.gb += vec2(0.5, 0.5);\n"
	"}\n";








Playback3DCommand::Playback3DCommand()
 : BC_SynchronousCommand()
{
	canvas = 0;
}

void Playback3DCommand::copy_from(BC_SynchronousCommand *command)
{
	Playback3DCommand *ptr = (Playback3DCommand*)command;
	this->canvas = ptr->canvas;
	this->is_cleared = ptr->is_cleared;

	this->in_x1 = ptr->in_x1;
	this->in_y1 = ptr->in_y1;
	this->in_x2 = ptr->in_x2;
	this->in_y2 = ptr->in_y2;
	this->out_x1 = ptr->out_x1;
	this->out_y1 = ptr->out_y1;
	this->out_x2 = ptr->out_x2;
	this->out_y2 = ptr->out_y2;
	this->alpha = ptr->alpha;
	this->mode = ptr->mode;
	this->interpolation_type = ptr->interpolation_type;

	this->input = ptr->input;
	this->start_position_project = ptr->start_position_project;
	this->keyframe_set = ptr->keyframe_set;
	this->keyframe = ptr->keyframe;
	this->default_auto = ptr->default_auto;
	this->plugin_client = ptr->plugin_client;
	this->want_texture = ptr->want_texture;

	BC_SynchronousCommand::copy_from(command);
}




Playback3D::Playback3D(MWindow *mwindow)
 : BC_Synchronous()
{
	this->mwindow = mwindow;
	temp_texture = 0;
}

Playback3D::~Playback3D()
{
}




BC_SynchronousCommand* Playback3D::new_command()
{
	return new Playback3DCommand;
}



void Playback3D::handle_command(BC_SynchronousCommand *command)
{
//printf("Playback3D::handle_command 1 %d\n", command->command);
	switch(command->command)
	{
		case Playback3DCommand::WRITE_BUFFER:
			write_buffer_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CLEAR_OUTPUT:
			clear_output_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CLEAR_INPUT:
			clear_input_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_CAMERA:
			do_camera_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::OVERLAY:
			overlay_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_FADE:
			do_fade_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_MASK:
			do_mask_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::PLUGIN:
			run_plugin_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::COPY_FROM:
			copy_from_sync((Playback3DCommand*)command);
			break;

// 		case Playback3DCommand::DRAW_REFRESH:
// 			draw_refresh_sync((Playback3DCommand*)command);
// 			break;
	}
//printf("Playback3D::handle_command 10\n");
}




void Playback3D::copy_from(Canvas *canvas, 
	VFrame *dst,
	VFrame *src,
	int want_texture)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::COPY_FROM;
	command.canvas = canvas;
	command.frame = dst;
	command.input = src;
	command.want_texture = want_texture;
	send_command(&command);
}

void Playback3D::copy_from_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::draw_refresh_sync");
	BC_WindowBase *window = command->canvas->get_canvas();
	if(window)
	{
		window->lock_window("Playback3D:draw_refresh_sync");
		window->enable_opengl();

		if(command->input->get_opengl_state() == VFrame::SCREEN &&
			command->input->get_w() == command->frame->get_w() &&
			command->input->get_h() == command->frame->get_h())
		{
// printf("Playback3D::copy_from_sync 1 %d %d %d %d %d\n", 
// command->input->get_w(),
// command->input->get_h(),
// command->frame->get_w(),
// command->frame->get_h(),
// command->frame->get_color_model());
			int w = command->input->get_w();
			int h = command->input->get_h();
// With NVidia at least,
			if(command->input->get_w() % 4)
			{
				printf("Playback3D::copy_from_sync: w=%d not supported because it is not divisible by 4.\n", w);
			}
			else
// Copy to texture
			if(command->want_texture)
			{
//printf("Playback3D::copy_from_sync 1 dst=%p src=%p\n", command->frame, command->input);
// Screen_to_texture requires the source pbuffer enabled.
				command->input->enable_opengl();
				command->frame->screen_to_texture();
				command->frame->set_opengl_state(VFrame::TEXTURE);
			}
			else
// Copy to RAM
			{
				command->input->enable_opengl();
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glReadPixels(0,
					0,
					w,
					command->input->get_h(),
					GL_RGB,
					GL_UNSIGNED_BYTE,
					command->frame->get_rows()[0]);
				command->frame->flip_vert();
				command->frame->set_opengl_state(VFrame::RAM);
			}
		}
		else
		{
			printf("Playback3D::copy_from_sync: invalid formats opengl_state=%d %dx%d -> %dx%d\n",
				command->input->get_opengl_state(),
				command->input->get_w(),
				command->input->get_h(),
				command->frame->get_w(),
				command->frame->get_h());
		}

		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif
}




// void Playback3D::draw_refresh(Canvas *canvas, 
// 	VFrame *frame,
// 	float in_x1, 
// 	float in_y1, 
// 	float in_x2, 
// 	float in_y2, 
// 	float out_x1, 
// 	float out_y1, 
// 	float out_x2, 
// 	float out_y2)
// {
// 	Playback3DCommand command;
// 	command.command = Playback3DCommand::DRAW_REFRESH;
// 	command.canvas = canvas;
// 	command.frame = frame;
// 	command.in_x1 = in_x1;
// 	command.in_y1 = in_y1;
// 	command.in_x2 = in_x2;
// 	command.in_y2 = in_y2;
// 	command.out_x1 = out_x1;
// 	command.out_y1 = out_y1;
// 	command.out_x2 = out_x2;
// 	command.out_y2 = out_y2;
// 	send_command(&command);
// }
// 
// void Playback3D::draw_refresh_sync(Playback3DCommand *command)
// {
// 	command->canvas->lock_canvas("Playback3D::draw_refresh_sync");
// 	BC_WindowBase *window = command->canvas->get_canvas();
// 	if(window)
// 	{
// 		window->lock_window("Playback3D:draw_refresh_sync");
// 		window->enable_opengl();
// 
// // Read output pbuffer back to RAM in project colormodel
// // RGB 8bit is fastest for OpenGL to read back.
// 		command->frame->reallocate(0, 
// 			0,
// 			0,
// 			0,
// 			command->frame->get_w(), 
// 			command->frame->get_h(), 
// 			BC_RGB888, 
// 			-1);
// 		command->frame->to_ram();
// 
// 		window->clear_box(0, 
// 						0, 
// 						window->get_w(), 
// 						window->get_h());
// 		window->draw_vframe(command->frame,
// 							(int)command->out_x1, 
// 							(int)command->out_y1, 
// 							(int)(command->out_x2 - command->out_x1), 
// 							(int)(command->out_y2 - command->out_y1),
// 							(int)command->in_x1, 
// 							(int)command->in_y1, 
// 							(int)(command->in_x2 - command->in_x1), 
// 							(int)(command->in_y2 - command->in_y1),
// 							0);
// 
// 		window->unlock_window();
// 	}
// 	command->canvas->unlock_canvas();
// }





void Playback3D::write_buffer(Canvas *canvas, 
	VFrame *frame,
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2, 
	int is_cleared)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::WRITE_BUFFER;
	command.canvas = canvas;
	command.frame = frame;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	command.is_cleared = is_cleared;
	send_command(&command);
}


void Playback3D::write_buffer_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::write_buffer_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::write_buffer_sync");
// Update hidden cursor
		window->update_video_cursor();
// Make sure OpenGL is enabled first.
		window->enable_opengl();


//printf("Playback3D::write_buffer_sync 1 %d\n", window->get_id());
		switch(command->frame->get_opengl_state())
		{
// Upload texture and composite to screen
			case VFrame::RAM:
				command->frame->to_texture();
				draw_output(command);
				break;
// Composite texture to screen and swap buffer
			case VFrame::TEXTURE:
				draw_output(command);
				break;
			case VFrame::SCREEN:
// swap buffers only
				window->flip_opengl();
				break;
			default:
				printf("Playback3D::write_buffer_sync unknown state\n");
				break;
		}
		window->unlock_window();
	}

	command->canvas->unlock_canvas();
}



void Playback3D::draw_output(Playback3DCommand *command)
{
#ifdef HAVE_GL
	int texture_id = command->frame->get_texture_id();
	BC_WindowBase *window = command->canvas->get_canvas();

// printf("Playback3D::draw_output 1 texture_id=%d window=%p\n", 
// texture_id,
// command->canvas->get_canvas());




// If virtual console is being used, everything in this function has
// already been done except the page flip.
	if(texture_id >= 0)
	{
		canvas_w = window->get_w();
		canvas_h = window->get_h();
		VFrame::init_screen(canvas_w, canvas_h);

		if(!command->is_cleared)
		{
// If we get here, the virtual console was not used.
			init_frame(command);
		}

// Texture
// Undo any previous shader settings
		command->frame->bind_texture(0);




// Convert colormodel
		unsigned int frag_shader = 0;
		switch(command->frame->get_color_model())
		{
			case BC_YUV888:
				frag_shader = VFrame::make_shader(0,
					yuv_to_rgb_frag,
					0);
				break;

			case BC_YUVA8888:
				frag_shader = VFrame::make_shader(0,
					yuva_to_rgba_frag,
					0);
				break;
		}


		if(frag_shader > 0) 
		{
			glUseProgram(frag_shader);
			int variable = glGetUniformLocation(frag_shader, "tex");
// Set texture unit of the texture
			glUniform1i(variable, 0);
		}

		if(cmodel_components(command->frame->get_color_model()) == 4)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		command->frame->draw_texture(command->in_x1, 
			command->in_y1,
			command->in_x2,
			command->in_y2,
			command->out_x1,
			command->out_y1,
			command->out_x2,
			command->out_y2,
			1);


// printf("Playback3D::draw_output 2 %f,%f %f,%f -> %f,%f %f,%f\n",
// command->in_x1,
// command->in_y1,
// command->in_x2,
// command->in_y2,
// command->out_x1,
// command->out_y1,
// command->out_x2,
// command->out_y2);

		glUseProgram(0);

		command->canvas->get_canvas()->flip_opengl();
		
	}
#endif
}


void Playback3D::init_frame(Playback3DCommand *command)
{
#ifdef HAVE_GL
	canvas_w = command->canvas->get_canvas()->get_w();
	canvas_h = command->canvas->get_canvas()->get_h();

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}


void Playback3D::clear_output(Canvas *canvas, VFrame *output)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::CLEAR_OUTPUT;
	command.canvas = canvas;
	command.frame = output;
	send_command(&command);
}

void Playback3D::clear_output_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::clear_output_sync");
	if(command->canvas->get_canvas())
	{
		command->canvas->get_canvas()->lock_window("Playback3D::clear_output_sync");
// If we get here, the virtual console is being used.
		command->canvas->get_canvas()->enable_opengl();

// Using pbuffer for refresh frame.
		if(command->frame)
		{
			command->frame->enable_opengl();
		}	


		init_frame(command);
		command->canvas->get_canvas()->unlock_window();
	}
	command->canvas->unlock_canvas();
}


void Playback3D::clear_input(Canvas *canvas, VFrame *frame)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::CLEAR_INPUT;
	command.canvas = canvas;
	command.frame = frame;
	send_command(&command);
}

void Playback3D::clear_input_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::clear_output_sync");
	if(command->canvas->get_canvas())
	{
		command->canvas->get_canvas()->lock_window("Playback3D::clear_output_sync");
		command->canvas->get_canvas()->enable_opengl();
		command->frame->enable_opengl();
		command->frame->clear_pbuffer();
		command->frame->set_opengl_state(VFrame::SCREEN);
		command->canvas->get_canvas()->unlock_window();
	}
	command->canvas->unlock_canvas();
}

void Playback3D::do_camera(Canvas *canvas,
	VFrame *output,
	VFrame *input,
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_CAMERA;
	command.canvas = canvas;
	command.input = input;
	command.frame = output;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	send_command(&command);
}

void Playback3D::do_camera_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::do_camera_sync");
	if(command->canvas->get_canvas())
	{
		command->canvas->get_canvas()->lock_window("Playback3D::clear_output_sync");
		command->canvas->get_canvas()->enable_opengl();

		command->input->to_texture();
		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->clear_pbuffer();

		command->input->bind_texture(0);
// Must call draw_texture in input frame to get the texture coordinates right.

// printf("Playback3D::do_camera_sync 1 %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", 
// command->in_x1, 
// command->in_y2, 
// command->in_x2, 
// command->in_y1, 
// command->out_x1,
// (float)command->input->get_h() - command->out_y1,
// command->out_x2,
// (float)command->input->get_h() - command->out_y2);
		command->input->draw_texture(
			command->in_x1, 
			command->in_y2, 
			command->in_x2, 
			command->in_y1, 
			command->out_x1,
			(float)command->frame->get_h() - command->out_y1,
			command->out_x2,
			(float)command->frame->get_h() - command->out_y2);


		command->frame->set_opengl_state(VFrame::SCREEN);
		command->canvas->get_canvas()->unlock_window();
	}
	command->canvas->unlock_canvas();
}

void Playback3D::overlay(Canvas *canvas,
	VFrame *input, 
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2, 
	float alpha,        // 0 - 1
	int mode,
	int interpolation_type,
	VFrame *output)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::OVERLAY;
	command.canvas = canvas;
	command.frame = output;
	command.input = input;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	command.alpha = alpha;
	command.mode = mode;
	command.interpolation_type = interpolation_type;
	send_command(&command);
}

void Playback3D::overlay_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::overlay_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
	    window->lock_window("Playback3D::overlay_sync");
// Make sure OpenGL is enabled first.
		window->enable_opengl();

		window->update_video_cursor();


// Render to PBuffer
		if(command->frame)
		{
			command->frame->enable_opengl();
			command->frame->set_opengl_state(VFrame::SCREEN);
			canvas_w = command->frame->get_w();
			canvas_h = command->frame->get_h();
		}
		else
		{
			canvas_w = window->get_w();
			canvas_h = window->get_h();
		}

		glColor4f(1, 1, 1, 1);

//printf("Playback3D::overlay_sync 1 %d\n", command->input->get_opengl_state());
		switch(command->input->get_opengl_state())
		{
// Upload texture and composite to screen
			case VFrame::RAM:
				command->input->to_texture();
				break;
// Just composite texture to screen
			case VFrame::TEXTURE:
				break;
// read from PBuffer to texture, then composite texture to screen
			case VFrame::SCREEN:
				command->input->enable_opengl();
				command->input->screen_to_texture();
				if(command->frame)
					command->frame->enable_opengl();
				else
					window->enable_opengl();
				break;
			default:
				printf("Playback3D::overlay_sync unknown state\n");
				break;
		}


		char *shader_stack[3] = { 0, 0, 0 };
		int total_shaders = 0;

		VFrame::init_screen(canvas_w, canvas_h);

// Enable texture
		command->input->bind_texture(0);


// Convert colormodel.
		switch(command->input->get_color_model())
		{
			case BC_YUV888:
				shader_stack[total_shaders++] = yuv_to_rgb_frag;
				break;
			case BC_YUVA8888:
				shader_stack[total_shaders++] = yuva_to_rgba_frag;
				break;
		}

// Change blend operation
		switch(command->mode)
		{
			case TRANSFER_NORMAL:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case TRANSFER_REPLACE:
// This requires overlaying an alpha multiplied image on a black screen.
				glDisable(GL_BLEND);
				if(command->input->get_texture_components() == 4)
				{
					if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
					shader_stack[total_shaders++] = multiply_alpha_frag;
				}
				break;

// To do these operations, we need to copy the input buffer to a texture
// and blend 2 textures in another shader
			case TRANSFER_ADDITION:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
				shader_stack[total_shaders++] = blend_add_frag;
				break;
			case TRANSFER_SUBTRACT:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
				shader_stack[total_shaders++] = blend_subtract_frag;
				break;
			case TRANSFER_MULTIPLY:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
				shader_stack[total_shaders++] = blend_multiply_frag;
				break;
			case TRANSFER_MAX:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
				shader_stack[total_shaders++] = blend_max_frag;
				break;
			case TRANSFER_DIVIDE:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
				shader_stack[total_shaders++] = blend_divide_frag;
				break;
		}

		unsigned int frag_shader = 0;
		if(shader_stack[0]) 
		{
			frag_shader = VFrame::make_shader(0,
				shader_stack[0],
				shader_stack[1],
				0);

			glUseProgram(frag_shader);


// Set texture unit of the texture
			glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
// Set texture unit of the temp texture
			glUniform1i(glGetUniformLocation(frag_shader, "tex2"), 1);
// Set dimensions of the temp texture
			if(temp_texture)
				glUniform2f(glGetUniformLocation(frag_shader, "tex2_dimensions"), 
					(float)temp_texture->get_texture_w(), 
					(float)temp_texture->get_texture_h());
		}
		else
			glUseProgram(0);






// printf("Playback3D::overlay_sync %f %f %f %f %f %f %f %f\n",
// command->in_x1,
// command->in_y1,
// command->in_x2,
// command->in_y2,
// command->out_x1,
// command->out_y1,
// command->out_x2,
// command->out_y2);




		command->input->draw_texture(command->in_x1, 
			command->in_y1,
			command->in_x2,
			command->in_y2,
			command->out_x1,
			command->out_y1,
			command->out_x2,
			command->out_y2,
			1);


		glUseProgram(0);


// Delete temp texture
		if(temp_texture)
		{
			delete temp_texture;
			temp_texture = 0;
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_2D);
		}
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);



		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif
}


void Playback3D::enable_overlay_texture(Playback3DCommand *command)
{
#ifdef HAVE_GL
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	BC_Texture::new_texture(&temp_texture,
		canvas_w, 
		canvas_h, 
		command->input->get_color_model());
	temp_texture->bind(1);

// Read canvas into texture
	glReadBuffer(GL_BACK);
	glCopyTexSubImage2D(GL_TEXTURE_2D,
		0,
		0,
		0,
		0,
		0,
		canvas_w,
		canvas_h);
#endif
}


void Playback3D::do_mask(Canvas *canvas,
	VFrame *output, 
	int64_t start_position_project,
	MaskAutos *keyframe_set, 
	MaskAuto *keyframe,
	MaskAuto *default_auto)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_MASK;
	command.canvas = canvas;
	command.frame = output;
	command.start_position_project = start_position_project;
	command.keyframe_set = keyframe_set;
	command.keyframe = keyframe;
	command.default_auto = default_auto;

	send_command(&command);
}



#ifdef HAVE_GL
struct Vertex : ListItem<Vertex>
{
	GLdouble c[3];
};
// this list is only used from the main thread, no locking needed
// this must be a list so that pointers to allocated entries remain valid
// when new entries are added
static List<Vertex> *vertex_cache = 0;

static void combine_callback(GLdouble coords[3], 
	GLdouble *vertex_data[4],
	GLfloat weight[4], 
	GLdouble **dataOut)
{
// can't use malloc here; GLU doesn't delete the memory for us!
	Vertex* vertex = vertex_cache->append();
	vertex->c[0] = coords[0];
	vertex->c[1] = coords[1];
	vertex->c[2] = coords[2];
// we don't need to interpolate anything

	*dataOut = &vertex->c[0];
}
#endif


void Playback3D::do_mask_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::do_mask_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::do_mask_sync");
		window->enable_opengl();
		
		switch(command->frame->get_opengl_state())
		{
			case VFrame::RAM:
// Time to upload to the texture
				command->frame->to_texture();
				break;

			case VFrame::SCREEN:
// Read back from PBuffer
// Bind context to pbuffer
				command->frame->enable_opengl();
				command->frame->screen_to_texture();
				break;
		}



// Create PBuffer and draw the mask on it
		command->frame->enable_opengl();

// Initialize coordinate system
		int w = command->frame->get_w();
		int h = command->frame->get_h();
		command->frame->init_screen();

// Clear screen
		glDisable(GL_TEXTURE_2D);
		if(command->default_auto->mode == MASK_MULTIPLY_ALPHA)
		{
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glColor4f((float)command->keyframe->value / 100, 
				(float)command->keyframe->value / 100, 
				(float)command->keyframe->value / 100, 
				1.0);
		}
		else
		{
			glClearColor(1.0, 1.0, 1.0, 1.0);
			glColor4f((float)1.0 - (float)command->keyframe->value / 100, 
				(float)1.0 - (float)command->keyframe->value / 100, 
				(float)1.0 - (float)command->keyframe->value / 100, 
				1.0);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
// Draw mask with scaling to simulate feathering
		GLUtesselator *tesselator = gluNewTess();
		gluTessProperty(tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
		gluTessCallback(tesselator, GLU_TESS_VERTEX, (GLvoid (*) ( )) &glVertex3dv);
		gluTessCallback(tesselator, GLU_TESS_BEGIN, (GLvoid (*) ( )) &glBegin);
		gluTessCallback(tesselator, GLU_TESS_END, (GLvoid (*) ( )) &glEnd);
		gluTessCallback(tesselator, GLU_TESS_COMBINE, (GLvoid (*) ( ))&combine_callback);

		vertex_cache = new List<Vertex>;


// Draw every submask as a new polygon
		int total_submasks = command->keyframe_set->total_submasks(
			command->start_position_project, 
			PLAY_FORWARD);
		float scale = command->keyframe->feather + 1;
 		int display_list = glGenLists(1);
 		glNewList(display_list, GL_COMPILE);
		for(int k = 0; k < total_submasks; k++)
		{
			gluTessBeginPolygon(tesselator, NULL);
			gluTessBeginContour(tesselator);
			ArrayList<MaskPoint*> *points = new ArrayList<MaskPoint*>;
			command->keyframe_set->get_points(points, 
				k, 
				command->start_position_project, 
				PLAY_FORWARD);

			int first_point = 0;
// Need to tabulate every vertex in persistent memory because
// gluTessVertex doesn't copy them.
			ArrayList<GLdouble*> coords;
			for(int i = 0; i < points->total; i++)
			{
				MaskPoint *point1 = points->values[i];
				MaskPoint *point2 = (i >= points->total - 1) ? 
					points->values[0] : 
					points->values[i + 1];


				float x, y;
				int segments = 0;
				if(point1->control_x2 == 0 &&
					point1->control_y2 == 0 &&
					point2->control_x1 == 0 &&
					point2->control_y1 == 0)
					segments = 1;

				float x0 = point1->x;
				float y0 = point1->y;
				float x1 = point1->x + point1->control_x2;
				float y1 = point1->y + point1->control_y2;
				float x2 = point2->x + point2->control_x1;
				float y2 = point2->y + point2->control_y1;
				float x3 = point2->x;
				float y3 = point2->y;

				// forward differencing bezier curves implementation taken from GPL code at
				// http://cvs.sourceforge.net/viewcvs.py/guliverkli/guliverkli/src/subtitles/Rasterizer.cpp?rev=1.3

				float cx3, cx2, cx1, cx0, cy3, cy2, cy1, cy0;

				// [-1 +3 -3 +1]
				// [+3 -6 +3  0]
				// [-3 +3  0  0]
				// [+1  0  0  0]

				cx3 = -  x0 + 3*x1 - 3*x2 + x3;
				cx2 =  3*x0 - 6*x1 + 3*x2;
				cx1 = -3*x0 + 3*x1;
				cx0 =    x0;

				cy3 = -  y0 + 3*y1 - 3*y2 + y3;
				cy2 =  3*y0 - 6*y1 + 3*y2;
				cy1 = -3*y0 + 3*y1;
				cy0 =    y0;

				// This equation is from Graphics Gems I.
				//
				// The idea is that since we're approximating a cubic curve with lines,
				// any error we incur is due to the curvature of the line, which we can
				// estimate by calculating the maximum acceleration of the curve.  For
				// a cubic, the acceleration (second derivative) is a line, meaning that
				// the absolute maximum acceleration must occur at either the beginning
				// (|c2|) or the end (|c2+c3|).  Our bounds here are a little more
				// conservative than that, but that's okay.
				if (segments == 0)
				{
					float maxaccel1 = fabs(2*cy2) + fabs(6*cy3);
					float maxaccel2 = fabs(2*cx2) + fabs(6*cx3);

					float maxaccel = maxaccel1 > maxaccel2 ? maxaccel1 : maxaccel2;
					float h = 1.0;

					if(maxaccel > 8.0) h = sqrt((8.0) / maxaccel);
					segments = int(1/h);
				}

				for(int j = 0; j <= segments; j++)
				{
					float t = (float)j / segments;
					x = cx0 + t*(cx1 + t*(cx2 + t*cx3));
					y = cy0 + t*(cy1 + t*(cy2 + t*cy3));

					if(j > 0 || first_point)
					{
						GLdouble *coord = new GLdouble[3];
						coord[0] = x / scale;
						coord[1] = -h + y / scale;
						coord[2] = 0;
						coords.append(coord);
						first_point = 0;
					}
				}
			}

// Now that we know the total vertices, send them to GLU
			for(int i = 0; i < coords.total; i++)
				gluTessVertex(tesselator, coords.values[i], coords.values[i]);

			gluTessEndContour(tesselator);
			gluTessEndPolygon(tesselator);
			points->remove_all_objects();
			delete points;
			coords.remove_all_objects();
		}
		glEndList();
 		glCallList(display_list);
 		glDeleteLists(display_list, 1);
		gluDeleteTess(tesselator);

		delete vertex_cache;
		vertex_cache = 0;

		glColor4f(1, 1, 1, 1);


// Read mask into temporary texture.
// For feathering, just read the part of the screen after the downscaling.


		float w_scaled = w / scale;
		float h_scaled = h / scale;
// Don't vary the texture size according to scaling because that 
// would waste memory.
// This enables and binds the temporary texture.
		glActiveTexture(GL_TEXTURE1);
		BC_Texture::new_texture(&temp_texture,
			w, 
			h, 
			command->frame->get_color_model());
		temp_texture->bind(1);
		glReadBuffer(GL_BACK);

// Need to add extra size to fill in the bottom right
		glCopyTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			0,
			(int)MIN(w_scaled + 2, w),
			(int)MIN(h_scaled + 2, h));

		command->frame->bind_texture(0);


// For feathered masks, use a shader to multiply.
// For unfeathered masks, we could use a stencil buffer 
// for further optimization but we also need a YUV algorithm.
		unsigned int frag_shader = 0;
		switch(temp_texture->get_texture_components())
		{
			case 3: 
				if(command->frame->get_color_model() == BC_YUV888)
					frag_shader = VFrame::make_shader(0,
						multiply_yuvmask3_frag,
						0);
				else
					frag_shader = VFrame::make_shader(0,
						multiply_mask3_frag,
						0);
				break;
			case 4: 
				frag_shader = VFrame::make_shader(0,
					multiply_mask4_frag,
					0);
				break;
		}

		if(frag_shader)
		{
			int variable;
			glUseProgram(frag_shader);
			if((variable = glGetUniformLocation(frag_shader, "tex")) >= 0)
				glUniform1i(variable, 0);
			if((variable = glGetUniformLocation(frag_shader, "tex1")) >= 0)
				glUniform1i(variable, 1);
			if((variable = glGetUniformLocation(frag_shader, "scale")) >= 0)
				glUniform1f(variable, scale);
		}



// Write texture to PBuffer with multiply and scaling for feather.

		
		command->frame->draw_texture(0, 0, w, h, 0, 0, w, h);
		command->frame->set_opengl_state(VFrame::SCREEN);


// Disable temp texture
		glUseProgram(0);

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		delete temp_texture;
		temp_texture = 0;

		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);

// Default drawable
		window->enable_opengl();
		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif
}












void Playback3D::do_fade(Canvas *canvas, VFrame *frame, float fade)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_FADE;
	command.canvas = canvas;
	command.frame = frame;
	command.alpha = fade;
	send_command(&command);
}

void Playback3D::do_fade_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::do_mask_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::do_fade_sync");
		window->enable_opengl();

		switch(command->frame->get_opengl_state())
		{
			case VFrame::RAM:
				command->frame->to_texture();
				break;

			case VFrame::SCREEN:
// Read back from PBuffer
// Bind context to pbuffer
				command->frame->enable_opengl();
				command->frame->screen_to_texture();
				break;
		}


		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->bind_texture(0);

//		glClearColor(0.0, 0.0, 0.0, 0.0);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		unsigned int frag_shader = 0;
		switch(command->frame->get_color_model())
		{
// For the alpha colormodels, the native function seems to multiply the 
// components by the alpha instead of just the alpha.
			case BC_RGBA8888:
			case BC_RGBA_FLOAT:
			case BC_YUVA8888:
				frag_shader = VFrame::make_shader(0,
					fade_rgba_frag,
					0);
				break;

			case BC_RGB888:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
				glColor4f(command->alpha, command->alpha, command->alpha, 1);
				break;


			case BC_YUV888:
				frag_shader = VFrame::make_shader(0,
					fade_yuv_frag,
					0);
				break;
		}


		if(frag_shader)
		{
			glUseProgram(frag_shader);
			int variable;
			if((variable = glGetUniformLocation(frag_shader, "tex")) >= 0)
				glUniform1i(variable, 0);
			if((variable = glGetUniformLocation(frag_shader, "alpha")) >= 0)
				glUniform1f(variable, command->alpha);
		}

		command->frame->draw_texture();
		command->frame->set_opengl_state(VFrame::SCREEN);

		if(frag_shader)
		{
			glUseProgram(0);
		}

		glColor4f(1, 1, 1, 1);
		glDisable(GL_BLEND);

		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif
}











int Playback3D::run_plugin(Canvas *canvas, PluginClient *client)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::PLUGIN;
	command.canvas = canvas;
	command.plugin_client = client;
	return send_command(&command);
}

void Playback3D::run_plugin_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::run_plugin_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::run_plugin_sync");
		window->enable_opengl();

		command->result = ((PluginVClient*)command->plugin_client)->handle_opengl();

		window->unlock_window();
	}
	command->canvas->unlock_canvas();
}


