#include "cmodel_permutation.h"
#include "colormodels.h"
#include "workarounds.h"

// ********************************** RGB FLOAT -> *******************************



static inline void transfer_RGB_FLOAT_to_RGB8(unsigned char *(*output), float *input)
{
	unsigned char r, g, b;
	r = (unsigned char)(CLIP(input[0], 0, 1) * 0x3);
	g = (unsigned char)(CLIP(input[1], 0, 1) * 0x7);
	b = (unsigned char)(CLIP(input[2], 0, 1) * 0x3);

	*(*output) = (r << 6) +
			     (g << 2) +
		 	     b;
	(*output)++;
}

static inline void transfer_RGB_FLOAT_to_BGR565(unsigned char *(*output), float *input)
{
	unsigned char r, g, b;
	r = (unsigned char)(CLIP(input[0], 0, 1) * 0x1f);
	g = (unsigned char)(CLIP(input[1], 0, 1) * 0x3f);
	b = (unsigned char)(CLIP(input[2], 0, 1) * 0x1f);

	*(uint16_t*)(*output) = (b << 11) |
		(g << 5) |
		r;
	(*output) += 2;
}

static inline void transfer_RGB_FLOAT_to_RGB565(unsigned char *(*output), float *input)
{
	unsigned char r, g, b;
	r = (unsigned char)(CLIP(input[0], 0, 1) * 0x1f);
	g = (unsigned char)(CLIP(input[1], 0, 1) * 0x3f);
	b = (unsigned char)(CLIP(input[2], 0, 1) * 0x1f);

	*(uint16_t*)(*output) = (r << 11) |
		(g << 5) |
		b;
	(*output) += 2;
}

static inline void transfer_RGB_FLOAT_to_BGR888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
}

static inline void transfer_RGB_FLOAT_to_RGB888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_RGB_FLOAT_to_RGBA8888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 0xff;
}

static inline void transfer_RGB_FLOAT_to_ARGB8888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = 0xff;
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_RGB_FLOAT_to_RGBA_FLOAT(float *(*output), 
	float *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = 1.0;
}

static inline void transfer_RGB_FLOAT_to_BGR8888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
	(*output)++;
}

static inline void transfer_RGB_FLOAT_to_YUV888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
}

static inline void transfer_RGB_FLOAT_to_YUVA8888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = 255;
}

static inline void transfer_RGB_FLOAT_to_YUV161616(uint16_t *(*output), 
	float *input)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = quicktime_copy(y);
	*(*output)++ = quicktime_copy(u);
	*(*output)++ = quicktime_copy(v);
}

static inline void transfer_RGB_FLOAT_to_YUVA16161616(uint16_t *(*output), 
	float *input)
{
	int y, u, v, r, g, b;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = quicktime_copy(y);
	*(*output)++ = quicktime_copy(u);
	*(*output)++ = quicktime_copy(v);
	*(*output)++ = 0xffff;
}


static inline void transfer_RGB_FLOAT_to_YUV101010(unsigned char *(*output), 
	float *input)
{
	int r, g, b;
	int y, u, v;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}

static inline void transfer_RGB_FLOAT_to_VYU888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = v >> 8;
	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
}

static inline void transfer_RGB_FLOAT_to_UYVA8888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = u >> 8;
	*(*output)++ = y >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = 0xff;
}


static inline void transfer_RGB_FLOAT_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column / 2] = u >> 8;
	output_v[output_column / 2] = v >> 8;
}

static inline void transfer_RGB_FLOAT_to_YUV422(unsigned char *(*output),
	float *input,
	int j)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);
	if(!(j & 1))
	{
// Store U and V for even pixels only
		(*output)[1] = u >> 8;
		(*output)[3] = v >> 8;
		(*output)[0] = y >> 8;
	}
	else
	{
// Store Y and advance output for odd pixels only
		(*output)[2] = y >> 8;
		(*output) += 4;
	}
}

static inline void transfer_RGB_FLOAT_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column] = u >> 8;
	output_v[output_column] = v >> 8;
}










// ****************************** RGBA FLOAT -> *********************************

static inline void transfer_RGBA_FLOAT_to_RGB8(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0x10000);
	r = (uint32_t)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * 0xff * a);

	*(*output) = (unsigned char)(((r & 0xc00000) >> 16) + 
				((g & 0xe00000) >> 18) + 
				((b & 0xe00000) >> 21));
	(*output)++;
}

static inline void transfer_RGBA_FLOAT_to_BGR565(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0x10000);
	r = (uint32_t)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * 0xff * a);

	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf80000) >> 8) + 
				((g & 0xfc0000) >> 13) + 
				((r & 0xf80000) >> 19));
	(*output) += 2;
}

static inline void transfer_RGBA_FLOAT_to_RGB565(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0x10000);
	r = (uint32_t)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * 0xff * a);

	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf80000) >> 8) + 
				((g & 0xfc0000) >> 13) + 
				((b & 0xf80000) >> 19));
	(*output) += 2;
}

static inline void transfer_RGBA_FLOAT_to_BGR888(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0xff);
	r = (uint32_t)(CLIP(input[0], 0, 1) * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * a);

	*(*output)++ = (unsigned char)b;
	*(*output)++ = (unsigned char)g;
	*(*output)++ = (unsigned char)r;
}

static inline void transfer_RGBA_FLOAT_to_RGB888(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0xff);
	r = (uint32_t)(CLIP(input[0], 0, 1) * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * a);

	*(*output)++ = (unsigned char)r;
	*(*output)++ = (unsigned char)g;
	*(*output)++ = (unsigned char)b;
}

static inline void transfer_RGBA_FLOAT_to_RGB_FLOAT(float *(*output), 
	float *input)
{
	float a;
	a = input[3];

	*(*output)++ = input[0] * a;
	*(*output)++ = input[1] * a;
	*(*output)++ = input[2] * a;
}


static inline void transfer_RGBA_FLOAT_to_RGBA8888(unsigned char *(*output), 
	float *input)
{
	*(*output)++ = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[3], 0, 1) * 0xff);
}

static inline void transfer_RGBA_FLOAT_to_ARGB8888(unsigned char *(*output), 
	float *input)
{
	*(*output)++ = (unsigned char)(CLIP(input[3], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
}


static inline void transfer_RGBA_FLOAT_to_BGR8888(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0xff);
	r = (uint32_t)(CLIP(input[0], 0, 1) * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * a);

	*(*output)++ = (unsigned char)(b);
	*(*output)++ = (unsigned char)(g);
	*(*output)++ = (unsigned char)(r);
	*(*output)++;
}

static inline void transfer_RGBA_FLOAT_to_YUV888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;
	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
}

static inline void transfer_RGBA_FLOAT_to_YUVA8888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;

	a = (int)(CLIP(input[3], 0, 1) * 0xff);
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = a;
}

static inline void transfer_RGBA_FLOAT_to_YUV161616(uint16_t *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;

	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

// GCC 3.3 optimization error
	*(*output)++ = quicktime_copy(y);
	*(*output)++ = quicktime_copy(u);
	*(*output)++ = quicktime_copy(v);
}

static inline void transfer_RGBA_FLOAT_to_YUVA16161616(uint16_t *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);
	a = (int)(CLIP(input[3], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

// GCC 3.3 optimization error
	*(*output)++ = quicktime_copy(y);
	*(*output)++ = quicktime_copy(u);
	*(*output)++ = quicktime_copy(v);
	*(*output)++ = quicktime_copy(a);
}

static inline void transfer_RGBA_FLOAT_to_YUV101010(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;

	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}


static inline void transfer_RGBA_FLOAT_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b, a;
	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column / 2] = u >> 8;
	output_v[output_column / 2] = v >> 8;
}

static inline void transfer_RGBA_FLOAT_to_YUV422(unsigned char *(*output),
	float *input,
	int j)
{
	int y, u, v, r, g, b;
	float a = CLIP(input[3], 0, 1);
	r = (int)(CLIP(input[0], 0, 1) * a * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * a * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * a * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);
	if(!(j & 1))
	{
// Store U and V for even pixels only
		(*output)[1] = u >> 8;
		(*output)[3] = v >> 8;
		(*output)[0] = y >> 8;
	}
	else
	{
// Store Y and advance output for odd pixels only
		(*output)[2] = y >> 8;
		(*output) += 4;
	}
}


static inline void transfer_RGBA_FLOAT_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b, a;
	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column] = u >> 8;
	output_v[output_column] = v >> 8;
}















#define TRANSFER_DEFAULT(output, \
	input, \
	y_in_offset, \
	u_in_offset, \
	v_in_offset, \
	input_column) \
{ \
	register int i, j; \
 \
	switch(in_colormodel) \
	{ \
		case BC_RGB_FLOAT: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_RGB8((output), (float*)(input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_BGR565((output), (float*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_RGB565((output), (float*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_BGR888((output), (float*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_BGR8888((output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_RGB888((output), (float*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_RGBA8888((output), (float*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_ARGB8888((output), (float*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_RGBA_FLOAT((float**)(output), (float*)(input));    \
					*output += 4 * sizeof(float); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_YUV888((output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_YUVA8888((output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_YUV161616((uint16_t**)(output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_YUVA16161616((uint16_t**)(output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_YUV101010((output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGB_FLOAT_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGB_FLOAT_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB_FLOAT_to_YUV422((output), \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGB_FLOAT_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGBA_FLOAT: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_RGB8((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_BGR565((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_RGB565((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_BGR888((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_BGR8888((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_RGB888((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_RGB_FLOAT((float**)(output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_RGBA8888((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_ARGB8888((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_YUV888((output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_YUVA8888((output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_YUV161616((uint16_t**)(output), (float*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_YUVA16161616((uint16_t**)(output), (float*)(input));  \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_YUV101010((output), (float*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGBA_FLOAT_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGBA_FLOAT_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA_FLOAT_to_YUV422((output), \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGBA_FLOAT_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(float*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
	} \
}



void cmodel_float(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			0,
			0,
			0,
			0);
	}
	else
	{
		TRANSFER_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			0,
			0,
			0,
			0);
	}
}

