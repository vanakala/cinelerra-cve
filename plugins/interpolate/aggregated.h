
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

#ifndef INTERPOLATE_AGGREGATED
#define INTERPOLATE_AGGREGATED

// Interpolation sections performed by other plugins

static const char *interpolate_shader = 
"uniform sampler2D tex;\n"
"uniform vec2 pattern_offset;\n"
"uniform vec2 pattern_size;\n"
"uniform vec2 pixel_size;\n"
"uniform mat3 color_matrix;\n"
"\n"
"void main()\n"
"{\n"
"	vec2 pixel_coord = gl_TexCoord[0].st;\n"
"	vec2 pattern_coord = pixel_coord;\n"
"	vec3 result;\n"
"	pattern_coord -= pattern_offset;\n"
"	pattern_coord = fract(pattern_coord / pattern_size);\n"
"	if(pattern_coord.x >= 0.5)\n"
"	{\n"
"		if(pattern_coord.y >= 0.5)\n"
"		{\n"
/* Bottom right of pattern */
/* Bottom right pixels are: */
/*    2    */
/*   1*3   */
/*    4    */
"			vec2 pixel1 = pixel_coord - vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel2 = pixel_coord - vec2(0.0, pixel_size.y);\n"
"			vec2 pixel3 = pixel_coord + vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel4 = pixel_coord + vec2(0.0, pixel_size.y);\n"
"			result = vec3((texture2D(tex, pixel1).r + \n"
"								texture2D(tex, pixel3).r) / 2.0, \n"
"							texture2D(tex, pixel_coord).g, \n"
"							(texture2D(tex, pixel2).b + \n"
"								texture2D(tex, pixel4).b) / 2.0);\n"
"		}\n"
"		else\n"
"		{\n"
/* Top right of pattern */
/* Top right pixels are: */
/*   123   */
/*   4*5   */
/*   678   */
"			vec2 pixel1 = pixel_coord - pixel_size;\n"
"			vec2 pixel2 = pixel_coord - vec2(0.0, pixel_size.y);\n"
"			vec2 pixel3 = pixel_coord + vec2(pixel_size.x, -pixel_size.y);\n"
"			vec2 pixel4 = pixel_coord - vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel5 = pixel_coord + vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel6 = pixel_coord + vec2(-pixel_size.x, pixel_size.y);\n"
"			vec2 pixel7 = pixel_coord + vec2(0.0, pixel_size.y);\n"
"			vec2 pixel8 = pixel_coord + pixel_size;\n"
"			result = vec3((texture2D(tex, pixel1).r + \n"
"								texture2D(tex, pixel3).r + \n"
"								texture2D(tex, pixel6).r + \n"
"								texture2D(tex, pixel8).r) / 4.0, \n"
"							(texture2D(tex, pixel4).g + \n"
"								texture2D(tex, pixel2).g + \n"
"								texture2D(tex, pixel5).g + \n"
"								texture2D(tex, pixel7).g) / 4.0, \n"
"							texture2D(tex, pixel_coord).b);\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		if(pattern_coord.y >= 0.5)\n"
"		{\n"
/* Bottom left of pattern */
/* Bottom left pixels are: */
/*   123   */
/*   4*5   */
/*   678   */
"			vec2 pixel1 = pixel_coord - pixel_size;\n"
"			vec2 pixel2 = pixel_coord - vec2(0.0, pixel_size.y);\n"
"			vec2 pixel3 = pixel_coord + vec2(pixel_size.x, -pixel_size.y);\n"
"			vec2 pixel4 = pixel_coord - vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel5 = pixel_coord + vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel6 = pixel_coord + vec2(-pixel_size.x, pixel_size.y);\n"
"			vec2 pixel7 = pixel_coord + vec2(0.0, pixel_size.y);\n"
"			vec2 pixel8 = pixel_coord + pixel_size;\n"
"			result = vec3(\n"
"							texture2D(tex, pixel_coord).r, \n"
"							(texture2D(tex, pixel4).g + \n"
"								texture2D(tex, pixel2).g + \n"
"								texture2D(tex, pixel5).g + \n"
"								texture2D(tex, pixel7).g) / 4.0, \n"
"							(texture2D(tex, pixel1).b + \n"
"								texture2D(tex, pixel3).b + \n"
"								texture2D(tex, pixel6).b + \n"
"								texture2D(tex, pixel8).b) / 4.0);\n"
"		}\n"
"		else\n"
"		{\n"
/* Top left of pattern */
/* Top left pixels are: */
/*    2    */
/*   1 3   */
/*    4    */
"			vec2 pixel1 = pixel_coord - vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel2 = pixel_coord - vec2(0.0, pixel_size.y);\n"
"			vec2 pixel3 = pixel_coord + vec2(pixel_size.x, 0.0);\n"
"			vec2 pixel4 = pixel_coord + vec2(0.0, pixel_size.y);\n"
"			result = vec3(\n"
"							(texture2D(tex, pixel2).r + \n"
"								texture2D(tex, pixel4).r) / 2.0, \n"
"							texture2D(tex, pixel_coord).g, \n"
"							(texture2D(tex, pixel1).b + \n"
"								texture2D(tex, pixel3).b) / 2.0);\n"
"		}\n"
"	}\n"
"\n"
"\n"
"	gl_FragColor = vec4(result * color_matrix, 1.0);\n"
"}\n";


#define INTERPOLATE_COMPILE(shader_stack, current_shader) \
{ \
	shader_stack[current_shader++] = interpolate_shader; \
}

#define INTERPOLATE_UNIFORMS(frag) \
{ \
	int x_offset = get_output()->get_params()->get("INTERPOLATEPIXELS_X", (int)0); \
	int y_offset = get_output()->get_params()->get("INTERPOLATEPIXELS_Y", (int)0); \
	float color_matrix[9]; \
	char string[BCTEXTLEN]; \
	string[0] = 0; \
	get_output()->get_params()->get("DCRAW_MATRIX", string); \
	sscanf(string,  \
		"%f %f %f %f %f %f %f %f %f",  \
		&color_matrix[0], \
		&color_matrix[1], \
		&color_matrix[2], \
		&color_matrix[3], \
		&color_matrix[4], \
		&color_matrix[5], \
		&color_matrix[6], \
		&color_matrix[7], \
		&color_matrix[8]); \
	glUniformMatrix3fv(glGetUniformLocation(frag, "color_matrix"),  \
		1, \
		0, \
		color_matrix); \
	glUniform2f(glGetUniformLocation(frag, "pattern_offset"),  \
		(float)x_offset / get_output()->get_texture_w(), \
		(float)y_offset / get_output()->get_texture_h()); \
	glUniform2f(glGetUniformLocation(frag, "pattern_size"),  \
		2.0 / get_output()->get_texture_w(), \
		2.0 / get_output()->get_texture_h()); \
	glUniform2f(glGetUniformLocation(frag, "pixel_size"),  \
		1.0 / get_output()->get_texture_w(), \
		1.0 / get_output()->get_texture_h()); \
}


#endif
