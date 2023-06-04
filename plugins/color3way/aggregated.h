
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

#ifndef COLORBALANCE_AGGREGATED
#define COLORBALANCE_AGGREGATED

static const char *colorbalance_get_pixel1 =
	"vec4 colorbalance_get_pixel()\n"
	"{\n"
	"	return gl_FragColor;\n"
	"}\n";

static const char *colorbalance_get_pixel2 =
	"uniform sampler2D tex;\n"
	"vec4 colorbalance_get_pixel()\n"
	"{\n"
	"	return texture2D(tex, gl_TexCoord[0].st);\n"
	"}\n";


static const char *colorbalance_rgb_shader = 
	"uniform vec3 colorbalance_scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorbalance_get_pixel();\n"
	"	gl_FragColor.rgb *= colorbalance_scale;\n"
	"}\n";
/* FIXIT
static const char *colorbalance_yuv_shader = 
	"uniform vec3 colorbalance_scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorbalance_get_pixel();\n"
	YUV_TO_RGB_FRAG("gl_FragColor")
	"	gl_FragColor.rgb *= colorbalance_scale;\n"
	RGB_TO_YUV_FRAG("gl_FragColor")
	"}\n";

static const char *colorbalance_yuv_preserve_shader = 
	"uniform vec3 colorbalance_scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorbalance_get_pixel();\n"
	"	float y = gl_FragColor.r;\n"
	YUV_TO_RGB_FRAG("gl_FragColor")
	"	gl_FragColor.rgb *= colorbalance_scale.rgb;\n"
	RGB_TO_YUV_FRAG("gl_FragColor")
	"	gl_FragColor.r = y;\n"
	"}\n";
	*/
#define COLORBALANCE_COMPILE(shader_stack, current_shader, aggregate_prev) \
{ \
	if(aggregate_prev) \
		shader_stack[current_shader++] = colorbalance_get_pixel1; \
	else \
		shader_stack[current_shader++] = colorbalance_get_pixel2; \
	if(BC_CModels::is_yuv(get_output()->get_color_model())) \
	{\
		if(get_output()->get_params()->get("COLORBALANCE_PRESERVE", (int)0)) \
			shader_stack[current_shader++] = colorbalance_yuv_preserve_shader; \
		else \
			shader_stack[current_shader++] = colorbalance_yuv_shader; \
	} \
	else \
		shader_stack[current_shader++] = colorbalance_rgb_shader; \
}

#define COLORBALANCE_UNIFORMS(shader) \
	glUniform3f(glGetUniformLocation(shader, "colorbalance_scale"),  \
		get_output()->get_params()->get("COLORBALANCE_CYAN", (float)1), \
		get_output()->get_params()->get("COLORBALANCE_MAGENTA", (float)1), \
		get_output()->get_params()->get("COLORBALANCE_YELLOW", (float)1));

#endif

