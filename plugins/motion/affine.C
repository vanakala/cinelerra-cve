
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef HAVE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

#include "affine.h"
#include "clip.h"
#include "vframe.h"


#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

AffineMatrix::AffineMatrix()
{
	bzero(values, sizeof(values));
}

void AffineMatrix::identity()
{
	bzero(values, sizeof(values));
	values[0][0] = 1;
	values[1][1] = 1;
	values[2][2] = 1;
}

void AffineMatrix::translate(double x, double y)
{
	double g = values[2][0];
	double h = values[2][1];
	double i = values[2][2];
	values[0][0] += x * g;
	values[0][1] += x * h;
	values[0][2] += x * i;
	values[1][0] += y * g;
	values[1][1] += y * h;
	values[1][2] += y * i;
}

void AffineMatrix::scale(double x, double y)
{
	values[0][0] *= x;
	values[0][1] *= x;
	values[0][2] *= x;

	values[1][0] *= y;
	values[1][1] *= y;
	values[1][2] *= y;
}

void AffineMatrix::multiply(AffineMatrix *dst)
{
	int i, j;
	AffineMatrix tmp;
	double t1, t2, t3;

  	for (i = 0; i < 3; i++)
    {
    	t1 = values[i][0];
    	t2 = values[i][1];
    	t3 = values[i][2];
    	for (j = 0; j < 3; j++)
		{
			tmp.values[i][j]  = t1 * dst->values[0][j];
			tmp.values[i][j] += t2 * dst->values[1][j];
			tmp.values[i][j] += t3 * dst->values[2][j];
		}
    }
	dst->copy_from(&tmp);
}

double AffineMatrix::determinant()
{
	double determinant;

	determinant  = 
        values[0][0] * (values[1][1] * values[2][2] - values[1][2] * values[2][1]);
	determinant -= 
        values[1][0] * (values[0][1] * values[2][2] - values[0][2] * values[2][1]);
	determinant += 
        values[2][0] * (values[0][1] * values[1][2] - values[0][2] * values[1][1]);

	return determinant;
}

void AffineMatrix::invert(AffineMatrix *dst)
{
	double det_1;

	det_1 = determinant();

	if(det_1 == 0.0)
      	return;

	det_1 = 1.0 / det_1;

	dst->values[0][0] =   
      (values[1][1] * values[2][2] - values[1][2] * values[2][1]) * det_1;

	dst->values[1][0] = 
      - (values[1][0] * values[2][2] - values[1][2] * values[2][0]) * det_1;

	dst->values[2][0] =   
      (values[1][0] * values[2][1] - values[1][1] * values[2][0]) * det_1;

	dst->values[0][1] = 
      - (values[0][1] * values[2][2] - values[0][2] * values[2][1] ) * det_1;

	dst->values[1][1] = 
      (values[0][0] * values[2][2] - values[0][2] * values[2][0]) * det_1;

	dst->values[2][1] = 
      - (values[0][0] * values[2][1] - values[0][1] * values[2][0]) * det_1;

	dst->values[0][2] =
      (values[0][1] * values[1][2] - values[0][2] * values[1][1]) * det_1;

	dst->values[1][2] = 
      - (values[0][0] * values[1][2] - values[0][2] * values[1][0]) * det_1;

	dst->values[2][2] = 
      (values[0][0] * values[1][1] - values[0][1] * values[1][0]) * det_1;
}

void AffineMatrix::copy_from(AffineMatrix *src)
{
	memcpy(&values[0][0], &src->values[0][0], sizeof(values));
}

void AffineMatrix::transform_point(float x, 
	float y, 
	float *newx, 
	float *newy)
{
	double w;

	w = values[2][0] * x + values[2][1] * y + values[2][2];

	if (w == 0.0)
    	w = 1.0;
	else
    	w = 1.0 / w;

	*newx = (values[0][0] * x + values[0][1] * y + values[0][2]) * w;
	*newy = (values[1][0] * x + values[1][1] * y + values[1][2]) * w;
}

void AffineMatrix::dump()
{
	printf("AffineMatrix::dump\n");
	printf("%f %f %f\n", values[0][0], values[0][1], values[0][2]);
	printf("%f %f %f\n", values[1][0], values[1][1], values[1][2]);
	printf("%f %f %f\n", values[2][0], values[2][1], values[2][2]);
}





AffinePackage::AffinePackage()
 : LoadPackage()
{
}




AffineUnit::AffineUnit(AffineEngine *server)
 : LoadClient(server)
{
	this->server = server;
}









void AffineUnit::calculate_matrix(
	double in_x1,
	double in_y1,
	double in_x2,
	double in_y2,
	double out_x1,
	double out_y1,
	double out_x2,
	double out_y2,
	double out_x3,
	double out_y3,
	double out_x4,
	double out_y4,
	AffineMatrix *result)
{
	AffineMatrix matrix;
	double scalex;
	double scaley;

	scalex = scaley = 1.0;

	if((in_x2 - in_x1) > 0)
      	scalex = 1.0 / (double)(in_x2 - in_x1);

	if((in_y2 - in_y1) > 0)
      	scaley = 1.0 / (double)(in_y2 - in_y1);

/* Determine the perspective transform that maps from
 * the unit cube to the transformed coordinates
 */
    double dx1, dx2, dx3, dy1, dy2, dy3;
    double det1, det2;

    dx1 = out_x2 - out_x4;
    dx2 = out_x3 - out_x4;
    dx3 = out_x1 - out_x2 + out_x4 - out_x3;

    dy1 = out_y2 - out_y4;
    dy2 = out_y3 - out_y4;
    dy3 = out_y1 - out_y2 + out_y4 - out_y3;
// printf("AffineUnit::calculate_matrix %f %f %f %f %f %f\n",
// dx1,
// dx2,
// dx3,
// dy1,
// dy2,
// dy3
// );

/*  Is the mapping affine?  */
    if((dx3 == 0.0) && (dy3 == 0.0))
    {
        matrix.values[0][0] = out_x2 - out_x1;
        matrix.values[0][1] = out_x4 - out_x2;
        matrix.values[0][2] = out_x1;
        matrix.values[1][0] = out_y2 - out_y1;
        matrix.values[1][1] = out_y4 - out_y2;
        matrix.values[1][2] = out_y1;
        matrix.values[2][0] = 0.0;
        matrix.values[2][1] = 0.0;
    }
    else
    {
        det1 = dx3 * dy2 - dy3 * dx2;
        det2 = dx1 * dy2 - dy1 * dx2;
        matrix.values[2][0] = det1 / det2;
        det1 = dx1 * dy3 - dy1 * dx3;
        det2 = dx1 * dy2 - dy1 * dx2;
        matrix.values[2][1] = det1 / det2;

        matrix.values[0][0] = out_x2 - out_x1 + matrix.values[2][0] * out_x2;
        matrix.values[0][1] = out_x3 - out_x1 + matrix.values[2][1] * out_x3;
        matrix.values[0][2] = out_x1;

        matrix.values[1][0] = out_y2 - out_y1 + matrix.values[2][0] * out_y2;
        matrix.values[1][1] = out_y3 - out_y1 + matrix.values[2][1] * out_y3;
        matrix.values[1][2] = out_y1;
    }

    matrix.values[2][2] = 1.0;

// printf("AffineUnit::calculate_matrix 1 %f %f\n", dx3, dy3);
// matrix.dump();

	result->identity();
	result->translate(-in_x1, -in_y1);
	result->scale(scalex, scaley);
	matrix.multiply(result);
// double test[3][3] = { { 0.0896, 0.0, 0.0 },
// 				  { 0.0, 0.0896, 0.0 },
// 				  { -0.00126, 0.0, 1.0 } };
// memcpy(&result->values[0][0], test, sizeof(test));
// printf("AffineUnit::calculate_matrix 4 %p\n", result);
// result->dump();


}

float AffineUnit::transform_cubic(float dx,
                               float jm1,
                               float j,
                               float jp1,
                               float jp2)
{
/* Catmull-Rom - not bad */
  	float result = ((( ( - jm1 + 3.0 * j - 3.0 * jp1 + jp2 ) * dx +
            	       ( 2.0 * jm1 - 5.0 * j + 4.0 * jp1 - jp2 ) ) * dx +
            	       ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;
// printf("%f %f %f %f %f\n", 
// result,
// jm1,
// j,
// jp1,
// jp2);


  	return result;
}


void AffineUnit::process_package(LoadPackage *package)
{
	AffinePackage *pkg = (AffinePackage*)package;
	int minx = server->x;
	int miny = server->y;
	int maxx = server->x + server->w - 1;
	int maxy = server->y + server->h - 1;

// Calculate real coords
	float out_x1, out_y1, out_x2, out_y2, out_x3, out_y3, out_x4, out_y4;
	if(server->mode == AffineEngine::STRETCH ||
		server->mode == AffineEngine::PERSPECTIVE ||
		server->mode == AffineEngine::ROTATE)
	{
		out_x1 = (float)server->x + (float)server->x1 * server->w / 100;
		out_y1 = (float)server->y + (float)server->y1 * server->h / 100;
		out_x2 = (float)server->x + (float)server->x2 * server->w / 100;
		out_y2 = (float)server->y + (float)server->y2 * server->h / 100;
		out_x3 = (float)server->x + (float)server->x3 * server->w / 100;
		out_y3 = (float)server->y + (float)server->y3 * server->h / 100;
		out_x4 = (float)server->x + (float)server->x4 * server->w / 100;
		out_y4 = (float)server->y + (float)server->y4 * server->h / 100;
	}
	else
	{
		out_x1 = (float)server->x + (float)server->x1 * server->w / 100;
		out_y1 = server->y;
		out_x2 = out_x1 + server->w;
		out_y2 = server->y;
		out_x4 = (float)server->x + (float)server->x4 * server->w / 100;
		out_y4 = server->y + server->h;
		out_x3 = out_x4 + server->w;
		out_y3 = server->y + server->h;
	}



// Rotation with OpenGL uses a simple quad.
	if(server->mode == AffineEngine::ROTATE &&
		server->use_opengl)
	{
#ifdef HAVE_GL
		server->output->to_texture();
		server->output->enable_opengl();
		server->output->init_screen();
		server->output->bind_texture(0);
		server->output->clear_pbuffer();

		int texture_w = server->output->get_texture_w();
		int texture_h = server->output->get_texture_h();
		float output_h = server->output->get_h();
		float in_x1 = (float)server->x / texture_w;
		float in_x2 = (float)(server->x + server->w) / texture_w;
		float in_y1 = (float)server->y / texture_h;
		float in_y2 = (float)(server->y + server->h) / texture_h;

// printf("%f %f %f %f\n%f,%f %f,%f %f,%f %f,%f\n", in_x1, in_y1, in_x2, in_y2,
// out_x1, out_y1, out_x2, out_y2, out_x3, out_y3, out_x4, out_y4);

		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1.0);

		glTexCoord2f(in_x1, in_y1);
		glVertex3f(out_x1, -output_h+out_y1, 0);

		glTexCoord2f(in_x2, in_y1);
		glVertex3f(out_x2, -output_h+out_y2, 0);

		glTexCoord2f(in_x2, in_y2);
		glVertex3f(out_x3, -output_h+out_y3, 0);

		glTexCoord2f(in_x1, in_y2);
		glVertex3f(out_x4, -output_h+out_y4, 0);


		glEnd();

		server->output->set_opengl_state(VFrame::SCREEN);
#endif
	}
	else
	if(server->mode == AffineEngine::PERSPECTIVE ||
		server->mode == AffineEngine::SHEER ||
		server->mode == AffineEngine::ROTATE)
	{
		AffineMatrix matrix;
		float temp;
		temp = out_x4;
		out_x4 = out_x3;
		out_x3 = temp;
		temp = out_y4;
		out_y4 = out_y3;
		out_y3 = temp;






		calculate_matrix(
			server->x,
			server->y,
			server->x + server->w,
			server->y + server->h,
			out_x1,
			out_y1,
			out_x2,
			out_y2,
			out_x3,
			out_y3,
			out_x4,
			out_y4,
			&matrix);

// printf("AffineUnit::process_package 10 %f %f %f %f %f %f %f %f\n", 
// out_x1,
// out_y1,
// out_x2,
// out_y2,
// out_x3,
// out_y3,
// out_x4,
// out_y4);
		int interpolate = 1;
		int reverse = !server->forward;
		float tx, ty, tw;
		float xinc, yinc, winc;
		AffineMatrix m, im;
		float ttx, tty;
		int itx, ity;
		int tx1, ty1, tx2, ty2;

		if(reverse)
		{
			m.copy_from(&matrix);
			m.invert(&im);
			matrix.copy_from(&im);
		}
		else
		{
			matrix.invert(&m);
		}

		float dx1, dy1;
		float dx2, dy2;
		float dx3, dy3;
		float dx4, dy4;
		matrix.transform_point(server->x, server->y, &dx1, &dy1);
		matrix.transform_point(server->x + server->w, server->y, &dx2, &dy2);
		matrix.transform_point(server->x, server->y + server->h, &dx3, &dy3);
		matrix.transform_point(server->x + server->w, server->y + server->h, &dx4, &dy4);

//printf("AffineUnit::process_package 1 y1=%d y2=%d\n", pkg->y1, pkg->y2);
//printf("AffineUnit::process_package 1 %f %f %f %f\n", dy1, dy2, dy3, dy4);
//printf("AffineUnit::process_package 2 %d ty1=%d %d ty2=%d %f %f\n", tx1, ty1, tx2, ty2, out_x4, out_y4);


		if(server->use_opengl)
		{
#ifdef HAVE_GL
			static char *affine_frag =
				"uniform sampler2D tex;\n"
				"uniform mat3 affine_matrix;\n"
				"uniform vec2 texture_extents;\n"
				"uniform vec2 image_extents;\n"
				"uniform vec4 border_color;\n"
				"void main()\n"
				"{\n"
				"	vec2 outcoord = gl_TexCoord[0].st;\n"
				"	outcoord *= texture_extents;\n"
				"	mat3 coord_matrix = mat3(\n"
				"		outcoord.x, outcoord.y, 1.0, \n"
				"		outcoord.x, outcoord.y, 1.0, \n"
				"		outcoord.x, outcoord.y, 1.0);\n"
				"	mat3 incoord_matrix = affine_matrix * coord_matrix;\n"
				"	vec2 incoord = vec2(incoord_matrix[0][0], incoord_matrix[0][1]);\n"
				"	incoord /= incoord_matrix[0][2];\n"
			 	"	incoord /= texture_extents;\n"
				"	if(incoord.x > image_extents.x || incoord.y > image_extents.y)\n"
				"		gl_FragColor = border_color;\n"
				"	else\n"
			 	"		gl_FragColor = texture2D(tex, incoord);\n"
				"}\n";

			float affine_matrix[9] = {
				m.values[0][0], m.values[1][0], m.values[2][0],
				m.values[0][1], m.values[1][1], m.values[2][1],
				m.values[0][2], m.values[1][2], m.values[2][2] 
			};


			server->output->to_texture();
			server->output->enable_opengl();
			unsigned int frag_shader = VFrame::make_shader(0,
					affine_frag,
					0);
			if(frag_shader > 0)
			{
				glUseProgram(frag_shader);
				glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
				glUniformMatrix3fv(glGetUniformLocation(frag_shader, "affine_matrix"), 
					1,
					0,
					affine_matrix);
				glUniform2f(glGetUniformLocation(frag_shader, "texture_extents"), 
					(GLfloat)server->output->get_texture_w(),
					(GLfloat)server->output->get_texture_h());
				glUniform2f(glGetUniformLocation(frag_shader, "image_extents"), 
					(GLfloat)server->output->get_w() / server->output->get_texture_w(),
					(GLfloat)server->output->get_h() / server->output->get_texture_h());
				float border_color[] = { 0, 0, 0, 0 };
				if(cmodel_is_yuv(server->output->get_color_model()))
				{
					border_color[1] = 0.5;
					border_color[2] = 0.5;
				}
				glUniform4fv(glGetUniformLocation(frag_shader, "border_color"), 
					1,
					(GLfloat*)border_color);
				server->output->init_screen();
				server->output->bind_texture(0);
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				server->output->draw_texture();
				glUseProgram(0);
				server->output->set_opengl_state(VFrame::SCREEN);
			}
			return;
#endif
		}






#define ROUND(x) ((int)((x > 0) ? (x) + 0.5 : (x) - 0.5))
#define MIN4(a,b,c,d) MIN(MIN(MIN(a,b),c),d)
#define MAX4(a,b,c,d) MAX(MAX(MAX(a,b),c),d)

    	tx1 = ROUND(MIN4(dx1, dx2, dx3, dx4));
    	ty1 = ROUND(MIN4(dy1, dy2, dy3, dy4));

    	tx2 = ROUND(MAX4(dx1, dx2, dx3, dx4));
    	ty2 = ROUND(MAX4(dy1, dy2, dy3, dy4));

		CLAMP(ty1, pkg->y1, pkg->y2);
		CLAMP(ty2, pkg->y1, pkg->y2);

		xinc = m.values[0][0];
		yinc = m.values[1][0];
		winc = m.values[2][0];


#define CUBIC_ROW(in_row, chroma_offset) \
	transform_cubic(dx, \
		in_row[col1_offset] - chroma_offset, \
		in_row[col2_offset] - chroma_offset, \
		in_row[col3_offset] - chroma_offset, \
		in_row[col4_offset] - chroma_offset)

#define TRANSFORM(components, type, temp_type, chroma_offset, max) \
{ \
	type **in_rows = (type**)server->input->get_rows(); \
	float round_factor = 0.0; \
	if(sizeof(type) < 4) round_factor = 0.5; \
	for(int y = ty1; y < ty2; y++) \
	{ \
		type *out_row = (type*)server->output->get_rows()[y]; \
 \
		if(!interpolate) \
		{ \
        	tx = xinc * (tx1 + 0.5) + m.values[0][1] * (y + 0.5) + m.values[0][2]; \
        	ty = yinc * (tx1 + 0.5) + m.values[1][1] * (y + 0.5) + m.values[1][2]; \
        	tw = winc * (tx1 + 0.5) + m.values[2][1] * (y + 0.5) + m.values[2][2]; \
		} \
      	else \
        { \
        	tx = xinc * tx1 + m.values[0][1] * y + m.values[0][2]; \
        	ty = yinc * tx1 + m.values[1][1] * y + m.values[1][2]; \
        	tw = winc * tx1 + m.values[2][1] * y + m.values[2][2]; \
        } \
 \
 \
		out_row += tx1 * components; \
		for(int x = tx1; x < tx2; x++) \
		{ \
/* Normalize homogeneous coords */ \
			if(tw == 0.0) \
			{ \
				ttx = 0.0; \
				tty = 0.0; \
			} \
			else \
			if(tw != 1.0) \
			{ \
				ttx = tx / tw; \
				tty = ty / tw; \
			} \
			else \
			{ \
				ttx = tx; \
				tty = ty; \
			} \
			itx = (int)ttx; \
			ity = (int)tty; \
 \
			int row1 = ity - 1; \
			int row2 = ity; \
			int row3 = ity + 1; \
			int row4 = ity + 2; \
			CLAMP(row1, miny, maxy); \
			CLAMP(row2, miny, maxy); \
			CLAMP(row3, miny, maxy); \
			CLAMP(row4, miny, maxy); \
 \
/* Set destination pixels */ \
			if(!interpolate && x >= server->x && x < server->x + server->w) \
			{ \
				if(itx >= server->x && itx < server->x + server->w && \
					ity >= server->y && ity < server->y + server->h) \
				{ \
					type *src = in_rows[ity] + itx * components; \
					*out_row++ = *src++; \
					*out_row++ = *src++; \
					*out_row++ = *src++; \
					if(components == 4) *out_row++ = *src; \
				} \
				else \
/* Fill with chroma */ \
				{ \
					*out_row++ = 0; \
					*out_row++ = chroma_offset; \
					*out_row++ = chroma_offset; \
					if(components == 4) *out_row++ = 0; \
				} \
			} \
			else \
/* Bicubic algorithm */ \
			if(interpolate && x >= server->x && x < server->x + server->w) \
			{ \
				if ((itx + 2) >= server->x && (itx - 1) < server->x + server->w && \
                  	(ity + 2) >= server->y && (ity - 1) < server->y + server->h) \
                { \
                	float dx, dy; \
 \
/* the fractional error */ \
                	dx = ttx - itx; \
                	dy = tty - ity; \
 \
/* Row and column offsets in cubic block */ \
					int col1 = itx - 1; \
					int col2 = itx; \
					int col3 = itx + 1; \
					int col4 = itx + 2; \
					CLAMP(col1, minx, maxx); \
					CLAMP(col2, minx, maxx); \
					CLAMP(col3, minx, maxx); \
					CLAMP(col4, minx, maxx); \
					int col1_offset = col1 * components; \
					int col2_offset = col2 * components; \
					int col3_offset = col3 * components; \
					int col4_offset = col4 * components; \
					type *row1_ptr = in_rows[row1]; \
					type *row2_ptr = in_rows[row2]; \
					type *row3_ptr = in_rows[row3]; \
					type *row4_ptr = in_rows[row4]; \
					temp_type r, g, b, a; \
 \
					r = (temp_type)(transform_cubic(dy, \
                    		 CUBIC_ROW(row1_ptr, 0x0), \
                    		 CUBIC_ROW(row2_ptr, 0x0), \
                    		 CUBIC_ROW(row3_ptr, 0x0), \
                    		 CUBIC_ROW(row4_ptr, 0x0)) + \
							 round_factor); \
 \
					row1_ptr++; \
					row2_ptr++; \
					row3_ptr++; \
					row4_ptr++; \
					g = (temp_type)(transform_cubic(dy, \
                    		 CUBIC_ROW(row1_ptr, chroma_offset), \
                    		 CUBIC_ROW(row2_ptr, chroma_offset), \
                    		 CUBIC_ROW(row3_ptr, chroma_offset), \
                    		 CUBIC_ROW(row4_ptr, chroma_offset)) + \
							 round_factor); \
					g += chroma_offset; \
 \
					row1_ptr++; \
					row2_ptr++; \
					row3_ptr++; \
					row4_ptr++; \
					b = (temp_type)(transform_cubic(dy, \
                    		 CUBIC_ROW(row1_ptr, chroma_offset), \
                    		 CUBIC_ROW(row2_ptr, chroma_offset), \
                    		 CUBIC_ROW(row3_ptr, chroma_offset), \
                    		 CUBIC_ROW(row4_ptr, chroma_offset)) + \
							 round_factor); \
					b += chroma_offset; \
 \
					if(components == 4) \
					{ \
						row1_ptr++; \
						row2_ptr++; \
						row3_ptr++; \
						row4_ptr++; \
						a = (temp_type)(transform_cubic(dy, \
                    			 CUBIC_ROW(row1_ptr, 0x0), \
                    			 CUBIC_ROW(row2_ptr, 0x0), \
                    			 CUBIC_ROW(row3_ptr, 0x0), \
                    			 CUBIC_ROW(row4_ptr, 0x0)) +  \
								 round_factor); \
					} \
 \
 					if(sizeof(type) < 4) \
					{ \
						*out_row++ = CLIP(r, 0, max); \
						*out_row++ = CLIP(g, 0, max); \
						*out_row++ = CLIP(b, 0, max); \
						if(components == 4) *out_row++ = CLIP(a, 0, max); \
					} \
					else \
					{ \
						*out_row++ = r; \
						*out_row++ = g; \
						*out_row++ = b; \
						if(components == 4) *out_row++ = a; \
					} \
                } \
				else \
/* Fill with chroma */ \
				{ \
					*out_row++ = 0; \
					*out_row++ = chroma_offset; \
					*out_row++ = chroma_offset; \
					if(components == 4) *out_row++ = 0; \
				} \
			} \
			else \
			{ \
				out_row += components; \
			} \
 \
/*  increment the transformed coordinates  */ \
			tx += xinc; \
			ty += yinc; \
			tw += winc; \
		} \
	} \
}




		switch(server->input->get_color_model())
		{
			case BC_RGB_FLOAT:
				TRANSFORM(3, float, float, 0x0, 1.0)
				break;
			case BC_RGB888:
				TRANSFORM(3, unsigned char, int, 0x0, 0xff)
				break;
			case BC_RGBA_FLOAT:
				TRANSFORM(4, float, float, 0x0, 1.0)
				break;
			case BC_RGBA8888:
				TRANSFORM(4, unsigned char, int, 0x0, 0xff)
				break;
			case BC_YUV888:
				TRANSFORM(3, unsigned char, int, 0x80, 0xff)
				break;
			case BC_YUVA8888:
				TRANSFORM(4, unsigned char, int, 0x80, 0xff)
				break;
			case BC_RGB161616:
				TRANSFORM(3, uint16_t, int, 0x0, 0xffff)
				break;
			case BC_RGBA16161616:
				TRANSFORM(4, uint16_t, int, 0x0, 0xffff)
				break;
			case BC_YUV161616:
				TRANSFORM(3, uint16_t, int, 0x8000, 0xffff)
				break;
			case BC_YUVA16161616:
				TRANSFORM(4, uint16_t, int, 0x8000, 0xffff)
				break;
		}

	}
	else
	{
		int min_x = server->x * AFFINE_OVERSAMPLE;
		int min_y = server->y * AFFINE_OVERSAMPLE;
		int max_x = server->x * AFFINE_OVERSAMPLE + server->w * AFFINE_OVERSAMPLE - 1;
		int max_y = server->y * AFFINE_OVERSAMPLE + server->h * AFFINE_OVERSAMPLE - 1;
		float top_w = out_x2 - out_x1;
		float bottom_w = out_x3 - out_x4;
		float left_h = out_y4 - out_y1;
		float right_h = out_y3 - out_y2;
		float out_w_diff = bottom_w - top_w;
		float out_left_diff = out_x4 - out_x1;
		float out_h_diff = right_h - left_h;
		float out_top_diff = out_y2 - out_y1;
		float distance1 = DISTANCE(out_x1, out_y1, out_x2, out_y2);
		float distance2 = DISTANCE(out_x2, out_y2, out_x3, out_y3);
		float distance3 = DISTANCE(out_x3, out_y3, out_x4, out_y4);
		float distance4 = DISTANCE(out_x4, out_y4, out_x1, out_y1);
		float max_v = MAX(distance1, distance3);
		float max_h = MAX(distance2, distance4);
		float max_dimension = MAX(max_v, max_h);
		float min_dimension = MIN(server->h, server->w);
		float step = min_dimension / max_dimension / AFFINE_OVERSAMPLE;
		float x_f = server->x;
		float y_f = server->y;
		float h_f = server->h;
		float w_f = server->w;



		if(server->use_opengl)
		{
			return;
		}



// Projection
#define DO_STRETCH(type, components) \
{ \
	type **in_rows = (type**)server->input->get_rows(); \
	type **out_rows = (type**)server->temp->get_rows(); \
 \
	for(float in_y = pkg->y1; in_y < pkg->y2; in_y += step) \
	{ \
		int i = (int)in_y; \
		type *in_row = in_rows[i]; \
		for(float in_x = x_f; in_x < w_f; in_x += step) \
		{ \
			int j = (int)in_x; \
			float in_x_fraction = (in_x - x_f) / w_f; \
			float in_y_fraction = (in_y - y_f) / h_f; \
			int out_x = (int)((out_x1 + \
				out_left_diff * in_y_fraction + \
				(top_w + out_w_diff * in_y_fraction) * in_x_fraction) *  \
				AFFINE_OVERSAMPLE); \
			int out_y = (int)((out_y1 +  \
				out_top_diff * in_x_fraction + \
				(left_h + out_h_diff * in_x_fraction) * in_y_fraction) * \
				AFFINE_OVERSAMPLE); \
			CLAMP(out_x, min_x, max_x); \
			CLAMP(out_y, min_y, max_y); \
			type *dst = out_rows[out_y] + out_x * components; \
			type *src = in_row + j * components; \
			dst[0] = src[0]; \
			dst[1] = src[1]; \
			dst[2] = src[2]; \
			if(components == 4) dst[3] = src[3]; \
		} \
	} \
}

		switch(server->input->get_color_model())
		{
			case BC_RGB_FLOAT:
				DO_STRETCH(float, 3)
				break;
			case BC_RGB888:
				DO_STRETCH(unsigned char, 3)
				break;
			case BC_RGBA_FLOAT:
				DO_STRETCH(float, 4)
				break;
			case BC_RGBA8888:
				DO_STRETCH(unsigned char, 4)
				break;
			case BC_YUV888:
				DO_STRETCH(unsigned char, 3)
				break;
			case BC_YUVA8888:
				DO_STRETCH(unsigned char, 4)
				break;
			case BC_RGB161616:
				DO_STRETCH(uint16_t, 3)
				break;
			case BC_RGBA16161616:
				DO_STRETCH(uint16_t, 4)
				break;
			case BC_YUV161616:
				DO_STRETCH(uint16_t, 3)
				break;
			case BC_YUVA16161616:
				DO_STRETCH(uint16_t, 4)
				break;
		}
	}




}






AffineEngine::AffineEngine(int total_clients,
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
{
	user_viewport = 0;
	user_pivot = 0;
	use_opengl = 0;
}

void AffineEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		AffinePackage *package = (AffinePackage*)get_package(i);
		package->y1 = y + (h * i / get_total_packages());
		package->y2 = y + (h * (i + 1) / get_total_packages());
	}
}

LoadClient* AffineEngine::new_client()
{
	return new AffineUnit(this);
}

LoadPackage* AffineEngine::new_package()
{
	return new AffinePackage;
}

void AffineEngine::process(VFrame *output,
	VFrame *input, 
	VFrame *temp,
	int mode,
	float x1, 
	float y1, 
	float x2, 
	float y2, 
	float x3, 
	float y3, 
	float x4, 
	float y4,
	int forward)
{
	this->output = output;
	this->input = input;
	this->temp = temp;
	this->mode = mode;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	this->x3 = x3;
	this->y3 = y3;
	this->x4 = x4;
	this->y4 = y4;
	this->forward = forward;


	if(!user_viewport)
	{
		x = 0;
		y = 0;
		w = input->get_w();
		h = input->get_h();
	}

	if(use_opengl)
		process_single();
	else
		process_packages();
}




void AffineEngine::rotate(VFrame *output,
	VFrame *input, 
	float angle)
{
	this->output = output;
	this->input = input;
	this->temp = 0;
	this->mode = ROTATE;
	this->forward = 1;

	if(!user_viewport)
	{
		x = 0;
		y = 0;
		w = input->get_w();
		h = input->get_h();
	}

	if(!user_pivot)
	{
		pivot_x = x + w / 2;
		pivot_y = y + h / 2;
	}

// All subscripts are clockwise around the quadrangle
	angle = angle * 2 * M_PI / 360;
	double angle1 = atan((double)(pivot_y - y) / (double)(pivot_x - x)) + angle;
	double angle2 = atan((double)(x + w - pivot_x) / (double)(pivot_y - y)) + angle;
	double angle3 = atan((double)(y + h - pivot_y) / (double)(x + w - pivot_x)) + angle;
	double angle4 = atan((double)(pivot_x - x) / (double)(y + h - pivot_y)) + angle;
	double radius1 = DISTANCE(x, y, pivot_x, pivot_y);
	double radius2 = DISTANCE(x + w, y, pivot_x, pivot_y);
	double radius3 = DISTANCE(x + w, y + h, pivot_x, pivot_y);
	double radius4 = DISTANCE(x, y + h, pivot_x, pivot_y);

	x1 = ((pivot_x - x) - cos(angle1) * radius1) * 100 / w;
	y1 = ((pivot_y - y) - sin(angle1) * radius1) * 100 / h;
	x2 = ((pivot_x - x) + sin(angle2) * radius2) * 100 / w;
	y2 = ((pivot_y - y) - cos(angle2) * radius2) * 100 / h;
	x3 = ((pivot_x - x) + cos(angle3) * radius3) * 100 / w;
	y3 = ((pivot_y - y) + sin(angle3) * radius3) * 100 / h;
	x4 = ((pivot_x - x) - sin(angle4) * radius4) * 100 / w;
	y4 = ((pivot_y - y) + cos(angle4) * radius4) * 100 / h;

// printf("AffineEngine::rotate x=%d y=%d w=%d h=%d pivot_x=%d pivot_y=%d angle=%f\n",
// x, y, w, h, 
// pivot_x, 
// pivot_y,
// angle * 360 / 2 / M_PI);
// 
// printf("	angle1=%f angle2=%f angle3=%f angle4=%f\n",
// angle1 * 360 / 2 / M_PI, 
// angle2 * 360 / 2 / M_PI, 
// angle3 * 360 / 2 / M_PI, 
// angle4 * 360 / 2 / M_PI);
// 
// printf("	radius1=%f radius2=%f radius3=%f radius4=%f\n",
// radius1,
// radius2,
// radius3,
// radius4);
// 
// printf("	x1=%f y1=%f x2=%f y2=%f x3=%f y3=%f x4=%f y4=%f\n",
// x1 * w / 100, 
// y1 * h / 100, 
// x2 * w / 100, 
// y2 * h / 100, 
// x3 * w / 100, 
// y3 * h / 100, 
// x4 * w / 100, 
// y4 * h / 100);

	if(use_opengl)
		process_single();
	else
		process_packages();
//	process_packages();
}

void AffineEngine::set_viewport(int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	user_viewport = 1;
}

void AffineEngine::set_opengl(int value)
{
	this->use_opengl = value;
}

void AffineEngine::set_pivot(int x, int y)
{
	this->pivot_x = x;
	this->pivot_y = y;
	this->user_pivot = 1;
}

void AffineEngine::unset_pivot()
{
	user_pivot = 0;
}

void AffineEngine::unset_viewport()
{
	user_viewport = 0;
}


