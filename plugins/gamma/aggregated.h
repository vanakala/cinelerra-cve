
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

#ifndef GAMMA_AGGREGATED
#define GAMMA_AGGREGATED

// Gamma sections performed by other plugins

// Functions to get pixel from either previous effect or texture
static char *gamma_get_pixel1 =
	"vec4 gamma_get_pixel()\n"
	"{\n"
	"	return gl_FragColor;\n"
	"}\n";

static char *gamma_get_pixel2 =
	"uniform sampler2D tex;\n"
	"vec4 gamma_get_pixel()\n"
	"{\n"
	"	return texture2D(tex, gl_TexCoord[0].st);\n"
	"}\n";

static char *gamma_pow_frag = 
	"float my_pow(float x, float y, float max)\n"
	"{\n"
	"	return (x > 0.0) ? pow(x * 2.0 / max, y) : 0.0;\n"
	"}\n";

static char *gamma_rgb_frag = 
	"uniform float gamma_scale;\n"
	"uniform float gamma_gamma;\n"
	"uniform float gamma_max;\n"
	"void main()\n"
	"{\n"
	"	vec4 pixel = gamma_get_pixel();\n"
	"	pixel.r = pixel.r * gamma_scale * my_pow(pixel.r, gamma_gamma, gamma_max);\n"
	"	pixel.g = pixel.g * gamma_scale * my_pow(pixel.g, gamma_gamma, gamma_max);\n"
	"	pixel.b = pixel.b * gamma_scale * my_pow(pixel.b, gamma_gamma, gamma_max);\n"
	"	gl_FragColor = pixel;\n"
	"}\n";

static char *gamma_yuv_frag = 
	"uniform float gamma_scale;\n"
	"uniform float gamma_gamma;\n"
	"uniform float gamma_max;\n"
	"void main()\n"
	"{\n"
	"	vec4 pixel = gamma_get_pixel();\n"
		YUV_TO_RGB_FRAG("pixel")
	"	pixel.r = pixel.r * gamma_scale * my_pow(pixel.r, gamma_gamma, gamma_max);\n"
	"	pixel.g = pixel.g * gamma_scale * my_pow(pixel.g, gamma_gamma, gamma_max);\n"
	"	pixel.b = pixel.b * gamma_scale * my_pow(pixel.b, gamma_gamma, gamma_max);\n"
		RGB_TO_YUV_FRAG("pixel")
	"	gl_FragColor = pixel;\n"
	"}\n";

#define GAMMA_COMPILE(shader_stack, current_shader, aggregate_interpolation) \
{ \
	if(aggregate_interpolation) \
		shader_stack[current_shader++] = gamma_get_pixel1; \
	else \
		shader_stack[current_shader++] = gamma_get_pixel2; \
 \
	switch(get_output()->get_color_model()) \
	{ \
		case BC_YUV888: \
		case BC_YUVA8888: \
			shader_stack[current_shader++] = gamma_pow_frag; \
			shader_stack[current_shader++] = gamma_yuv_frag; \
			break; \
		default: \
			shader_stack[current_shader++] = gamma_pow_frag; \
			shader_stack[current_shader++] = gamma_rgb_frag; \
			break; \
	} \
}

#define GAMMA_UNIFORMS(frag) \
{ \
	float max = get_output()->get_params()->get("GAMMA_MAX", (float)1); \
	float gamma = get_output()->get_params()->get("GAMMA_GAMMA", (float)1) - 1.0; \
	float scale = 1.0 / max; \
	glUniform1f(glGetUniformLocation(frag, "gamma_scale"), scale); \
	glUniform1f(glGetUniformLocation(frag, "gamma_gamma"), gamma); \
	glUniform1f(glGetUniformLocation(frag, "gamma_max"), max); \
printf("GAMMA_UNIFORMS %f %f\n", max, gamma); \
}





#endif
