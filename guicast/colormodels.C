// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcbitmap.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "language.h"
#include "colormodels.h"
#include "colormodels_private.h"
#include "tmpframecache.h"
#include "vframe.h"
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/avutil.h>
}

struct cm_names ColorModels::color_model_names[] =
{
	{ BC_NOCOLOR, N_("None") },
	{ BC_TRANSPARENCY, N_("Transparency") },
	{ BC_RGB888, N_("RGB-8 Bit") },
	{ BC_RGBA8888, N_("RGBA-8 Bit") },
	{ BC_RGB161616, N_("RGB-16 Bit") },
	{ BC_RGBA16161616, N_("RGBA-16 Bit") },
	{ BC_BGR8888, N_("BGRA-8 Bit") },
	{ BC_YUV888, N_("YUV-8 Bit") },
	{ BC_YUVA8888, N_("YUVA-8 Bit") },
	{ BC_YUV161616, N_("YUV-16 Bit") },
	{ BC_YUVA16161616, N_("YUVA-16 Bit") },
	{ BC_AYUV16161616, N_("AYUV-16 Bit") },
	{ BC_RGB_FLOAT, N_("RGB-FLOAT") },
	{ BC_RGBA_FLOAT, N_("RGBA-FLOAT") },
	{ BC_YUV420P, N_("YUV420P") },
	{ BC_YUV422P, N_("YUV422P") },
	{ BC_YUV444P, N_("YUV444P") },
	{ BC_YUV422, N_("YUV422") },
	{ BC_COMPRESSED, N_("Compressed") },
	{ BC_A8, N_("A-8") },
	{ BC_A_FLOAT, N_("A-FLOAT") },
	{ BC_A16, N_("A-16") },
	{ -1, 0 }
};

struct intp_types ColorModels::interpolation_types[] =
{
	{ NEAREST_NEIGHBOR, SWS_POINT },
	{ CUBIC_CUBIC, SWS_BICUBIC },
	{ LINEAR_LINEAR, SWS_FAST_BILINEAR },
	{ LANCZOS_LANCZOS, SWS_LANCZOS }
};

static void transfer_details(struct SwsContext *sws_ctx, int srange);

ColorModels::ColorModels()
{
}

int ColorModels::is_planar(int colormodel)
{
	switch(colormodel)
	{
	case BC_YUV420P:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
		return 1;
	}
	return 0;
}

int ColorModels::components(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_A16:
	case BC_A_FLOAT:
		return 1;

	case BC_RGB888:
	case BC_YUV888:
	case BC_RGB161616:
	case BC_YUV161616:
	case BC_RGB_FLOAT:
		return 3;

	case BC_RGBA8888:
	case BC_YUVA8888:
	case BC_BGR8888:
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
	case BC_RGBA_FLOAT:
		return 4;
	}
	return 0;
}

int ColorModels::calculate_pixelsize(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_RGB8:
	case BC_TRANSPARENCY:
	case BC_COMPRESSED:
	case BC_YUV420P:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
		return 1;

	case BC_A16:
	case BC_RGB565:
	case BC_BGR565:
	case BC_YUV422:
		return 2;

	case BC_BGR888:
	case BC_RGB888:
	case BC_YUV888:
		return 3;

	case BC_A_FLOAT:
	case BC_BGR8888:
	case BC_ARGB8888:
	case BC_ABGR8888:
	case BC_RGBA8888:
	case BC_YUVA8888:
		return 4;

	case BC_RGB161616:
	case BC_YUV161616:
		return 6;

	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		return 8;

	case BC_RGB_FLOAT:
		return 12;

	case BC_RGBA_FLOAT:
		return 16;
	}
	return 0;
}

int ColorModels::calculate_max(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_RGB888:
	case BC_BGR888:
	case BC_BGR8888:
	case BC_RGBA8888:
	case BC_YUV888:
	case BC_YUVA8888:
		return 0xff;

	case BC_A16:
	case BC_RGB161616:
	case BC_RGBA16161616:
	case BC_YUV161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		return 0xffff;

	case BC_A_FLOAT:
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		return 1;
	}
	return 0;
}

int ColorModels::calculate_datasize(int w, int h, int bytes_per_line, int color_model)
{
	if(bytes_per_line < 0)
		bytes_per_line = w * calculate_pixelsize(color_model);

	switch(color_model)
	{
	case BC_YUV420P:
	case BC_YUV411P:
		return w * h + w * h / 2;

	case BC_YUV422P:
		return w * h * 2;

	case BC_YUV444P:
		return w * h * 3;

	default:
		return h * bytes_per_line;
	}
	return 0;
}

void ColorModels::transfer_sws(unsigned char *output,
	unsigned char *input,
	unsigned char *out_y_plane,
	unsigned char *out_u_plane,
	unsigned char *out_v_plane,
	unsigned char *in_y_plane,
	unsigned char *in_u_plane,
	unsigned char *in_v_plane,
	int in_w,
	int in_h,
	int out_w,
	int out_h,
	int in_colormodel,
	int out_colormodel,
	int in_rowspan,
	int out_rowspan)
{
	unsigned char *in_data[4], *out_data[4];
	int in_linesizes[4], out_linesizes[4];
	AVPixelFormat pix_in, pix_out;
	struct SwsContext *sws_ctx = 0;

	pix_in = color_model_to_pix_fmt(in_colormodel);
	pix_out = color_model_to_pix_fmt(out_colormodel);

	if(pix_in != AV_PIX_FMT_NONE && pix_out != AV_PIX_FMT_NONE)
	{
		sws_ctx = sws_getCachedContext(sws_ctx, in_w, in_h, pix_in,
			out_w, out_h, pix_out,
			libinterpolate(), NULL, NULL, NULL);
		fill_data(in_colormodel, in_data, input,
			in_y_plane, in_u_plane, in_v_plane);
		fill_linesizes(in_colormodel, in_rowspan, in_w, in_linesizes);
		fill_data(out_colormodel, out_data, output,
			out_y_plane, out_u_plane, out_v_plane);
		fill_linesizes(out_colormodel, out_rowspan, out_w, out_linesizes);

		if(!sws_ctx)
		{
			printf("ColorModels::transfer_sws: swscale context initialization failed\n");
			return;
		}
		sws_scale(sws_ctx, in_data, in_linesizes,
			0, in_h, out_data, out_linesizes);
		sws_freeContext(sws_ctx);
	}
	else
	{
		VFrame *tmpframe = 0;

		if(pix_out == AV_PIX_FMT_NONE && pix_in != AV_PIX_FMT_NONE)
		{
			int tmp_cmodel = inter_color_model(out_colormodel);

			tmpframe = BC_Resources::tmpframes.get_tmpframe(out_w, out_h, tmp_cmodel);
			transfer_sws(tmpframe->get_data(), input,
				tmpframe->get_y(), tmpframe->get_u(), tmpframe->get_v(),
				in_y_plane, in_u_plane, in_v_plane,
				in_w, in_h, out_w, out_h,
				in_colormodel, tmp_cmodel,
				in_rowspan, tmpframe->get_bytes_per_line());

			copy_colors(out_w, out_h,
				output, out_colormodel, out_rowspan,
				tmpframe->get_data(), tmp_cmodel, tmpframe->get_bytes_per_line());
			BC_Resources::tmpframes.release_frame(tmpframe);
		}
		else if(pix_out != AV_PIX_FMT_NONE && pix_in == AV_PIX_FMT_NONE)
		{
			int tmp_cmodel = inter_color_model(in_colormodel);

			tmpframe = BC_Resources::tmpframes.get_tmpframe(in_w, in_h, tmp_cmodel);
			copy_colors(in_w, in_h,
				tmpframe->get_data(), tmp_cmodel, tmpframe->get_bytes_per_line(),
				input, in_colormodel, in_rowspan);

			transfer_sws(output, tmpframe->get_data(),
				out_y_plane, out_u_plane, out_v_plane,
				tmpframe->get_y(), tmpframe->get_u(), tmpframe->get_v(),
				in_w, in_h, out_w, out_h,
				tmp_cmodel, out_colormodel,
				tmpframe->get_bytes_per_line(), out_rowspan);
			BC_Resources::tmpframes.release_frame(tmpframe);
		}
		else
		{
			slow_transfer(out_w, out_h, output, out_colormodel, out_rowspan,
				in_w, in_h, input, in_colormodel, in_rowspan);
		}
	}
}

void ColorModels::transfer_frame(unsigned char *output,
	VFrame *frame,
	unsigned char *out_y_plane,
	unsigned char *out_u_plane,
	unsigned char *out_v_plane,
	int out_w,           // output width
	int out_h,           // output height
	int out_colormodel,
	int out_rowspan)
{
	unsigned char *in_data[4], *out_data[4];
	int in_linesizes[4], out_linesizes[4];
	AVPixelFormat pix_in, pix_out;
	struct SwsContext *sws_ctx = 0;

	pix_in = color_model_to_pix_fmt(frame->get_color_model());
	pix_out = color_model_to_pix_fmt(out_colormodel);

	if(pix_in != AV_PIX_FMT_NONE && pix_out != AV_PIX_FMT_NONE)
	{
		sws_ctx = sws_getCachedContext(sws_ctx, frame->get_w(), frame->get_h(),
			pix_in, out_w, out_h, pix_out,
			libinterpolate(), NULL, NULL, NULL);
		fill_data(frame->get_color_model(), in_data, frame->get_data(),
			frame->get_y(), frame->get_u(), frame->get_v());
		fill_linesizes(frame->get_color_model(), frame->get_bytes_per_line(),
			frame->get_w(), in_linesizes);
		fill_data(out_colormodel, out_data, output,
			out_y_plane, out_u_plane, out_v_plane);
		fill_linesizes(out_colormodel, out_rowspan, out_w, out_linesizes);

		if(!sws_ctx)
		{
			printf("ColorModels::transfer_frame: swscale context initialization failed\n");
			return;
		}
		sws_scale(sws_ctx, in_data, in_linesizes,
			0, frame->get_h(), out_data, out_linesizes);
		sws_freeContext(sws_ctx);
	}
	else
	{
		VFrame *tmpframe = 0;

		if(pix_out == AV_PIX_FMT_NONE && pix_in != AV_PIX_FMT_NONE)
		{
			int tmp_cmodel = inter_color_model(out_colormodel);

			tmpframe = BC_Resources::tmpframes.get_tmpframe(out_w, out_h, tmp_cmodel);
			transfer_frame(tmpframe->get_data(), frame,
				tmpframe->get_y(), tmpframe->get_u(), tmpframe->get_v(),
				out_w, out_h,
				tmp_cmodel, tmpframe->get_bytes_per_line());
			copy_colors(out_w, out_h,
				output, out_colormodel, out_rowspan,
				tmpframe->get_data(), tmp_cmodel, tmpframe->get_bytes_per_line());
			BC_Resources::tmpframes.release_frame(tmpframe);
		}
		else if(pix_out != AV_PIX_FMT_NONE && pix_in == AV_PIX_FMT_NONE)
		{
			int tmp_cmodel = inter_color_model(frame->get_color_model());

			tmpframe = BC_Resources::tmpframes.get_tmpframe(frame->get_w(),
				frame->get_h(), tmp_cmodel);
			copy_colors(frame->get_w(), frame->get_h(),
				tmpframe->get_data(), tmp_cmodel, tmpframe->get_bytes_per_line(),
				frame->get_data(), frame->get_color_model(),
				frame->get_bytes_per_line());

			transfer_frame(output, tmpframe,
				out_y_plane, out_u_plane, out_v_plane,
				out_w, out_h,
				out_colormodel, out_rowspan);
			BC_Resources::tmpframes.release_frame(tmpframe);
		}
		else
		{
			slow_transfer(out_w, out_h, output, out_colormodel, out_rowspan,
				frame->get_w(), frame->get_h(), frame->get_data(),
				frame->get_color_model(), frame->get_bytes_per_line());
		}
	}
}

void ColorModels::fill_linesizes(int colormodel, int rowspan, int w, int *linesizes)
{
	if(rowspan > 0)
		linesizes[0] = rowspan;
	else
		linesizes[0] = calculate_pixelsize(colormodel) * w;

	linesizes[3] = 0;

	switch(colormodel)
	{
	case BC_YUV411P:
		linesizes[2] = linesizes[1] = linesizes[0] / 4;
		break;
	case BC_YUV420P:
	case BC_YUV422P:
		linesizes[2] = linesizes[1] = linesizes[0] / 2;
		break;
	case BC_YUV444P:
		linesizes[2] = linesizes[1] = linesizes[0];
		break;
	default:
		linesizes[2] = linesizes[1] = 0;
		break;
	}
}

void ColorModels::fill_data(int colormodel, unsigned char **data,
	unsigned char *xbuf,
	unsigned char *y, unsigned char *u, unsigned char *v)
{
	data[3] = 0;

	if(is_planar(colormodel))
	{
		data[0] = y;
		data[1] = u;
		data[2] = v;
	}
	else
	{
		data[0] = xbuf;
		data[2] = data[1] = 0;
	}
}

AVPixelFormat color_model_to_pix_fmt(int color_model)
{
	switch(color_model)
	{
	case BC_YUV422:
		return AV_PIX_FMT_YUYV422;
	case BC_RGB888:
		return AV_PIX_FMT_RGB24;
	case BC_BGR8888:
		return AV_PIX_FMT_BGRA;
	case BC_BGR888:
		return AV_PIX_FMT_BGR24;
	case BC_YUV420P:
		return AV_PIX_FMT_YUV420P;
	case BC_YUV422P:
		return AV_PIX_FMT_YUV422P;
	case BC_YUV444P:
		return AV_PIX_FMT_YUV444P;
	case BC_YUV411P:
		return AV_PIX_FMT_YUV411P;
	case BC_RGB565:
		return AV_PIX_FMT_RGB565;
	case BC_RGBA8888:
		return AV_PIX_FMT_RGBA;
	case BC_RGB8:
		return AV_PIX_FMT_RGB8;
	case BC_BGR565:
		return AV_PIX_FMT_BGR565;
	case BC_ARGB8888:
		return AV_PIX_FMT_ARGB;
	case BC_ABGR8888:
		return AV_PIX_FMT_ABGR;
	case BC_RGB161616:
		return AV_PIX_FMT_RGB48;
	case BC_RGBA16161616:
		return AV_PIX_FMT_RGBA64;
	case BC_AYUV16161616:
		return AV_PIX_FMT_AYUV64;
	case BC_A8:
		return AV_PIX_FMT_GRAY8;
	case BC_A16:
		return AV_PIX_FMT_GRAY16;
	};
	return AV_PIX_FMT_NONE;
}

int ColorModels::inter_color_model(int color_model)
{
	switch(color_model)
	{
	case BC_YUV888:
	case BC_YUVA8888:
		return BC_AYUV16161616;

	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		return BC_RGBA16161616;

	case BC_A_FLOAT:
		return BC_A16;
	}
	return color_model;
}

void ColorModels::rgba2transparency(int w, int h,
	unsigned char *output, unsigned char *input,
	int out_rowspan, int in_rowspan)
{
	int val, bit;

	for(int j = 0; j < h; j++)
	{
		unsigned char *idata = input + j * in_rowspan;
		unsigned char *dp = (unsigned char*)(output + j * out_rowspan);

		val = bit = 0;

		for(int i = 0; i < w; i++)
		{
			if(idata[4 * i + 3] < 127)
				val |= 1 << bit;
			if(++bit > 7)
			{
				*dp++ = val;
				val = 0;
				bit = 0;
			}
		}
		if(bit)
			*dp = val;
	}
}

void ColorModels::copy_colors(int w, int h,
	unsigned char *output, int out_cmodel, int out_rowspan,
	unsigned char *input, int in_cmodel, int in_rowspan)
{
	int wb;
	double pxmax;

	switch(in_cmodel)
	{
	case BC_AYUV16161616:
		// BC_YUV888 or BC_YUVA8888
		wb = components(out_cmodel) * w;

		for(int j = 0; j < h; j++)
		{
			uint16_t *idata = (uint16_t*)(input + j * in_rowspan);
			unsigned char *dp = output + j * out_rowspan;

			for(int i = 0; i < wb;)
			{
				int alpha = *idata++;

				dp[i++] = (*idata++) >> 8;
				dp[i++] = (*idata++) >> 8;
				dp[i++] = (*idata++) >> 8;
				if(out_cmodel == BC_YUVA8888)
					dp[i++] = alpha >> 8;
			}
		}
		break;

	case BC_YUV888:
	case BC_YUVA8888:
		// BC_AYUV16161616
		wb = components(out_cmodel) * w;

		for(int j = 0; j < h; j++)
		{
			unsigned char *idata = input + j * in_rowspan;
			uint16_t *dp = (uint16_t*)(output + j * out_rowspan);

			for(int i = 0; i < wb;)
			{
				if(in_cmodel == BC_YUVA8888)
					dp[i++] = idata[3] << 8;
				else
					dp[i++] = 0xffff;
				dp[i++] = (*idata++) << 8;
				dp[i++] = (*idata++) << 8;
				dp[i++] = (*idata++) << 8;
				if(in_cmodel == BC_YUVA8888)
					idata++;
			}
		}
		break;
	case BC_RGBA16161616:
		// to BC_RGB_FLOAT or BC_RGBA_FLOAT
		wb = components(out_cmodel) * w;
		pxmax = calculate_max(in_cmodel);

		for(int j = 0; j < h; j++)
		{
			uint16_t *idata = (uint16_t*)(input + j * in_rowspan);
			float *dp = (float*)(output + j * out_rowspan);

			for(int i = 0; i < wb;)
			{
				dp[i++] = *idata++ / pxmax;
				dp[i++] = *idata++ / pxmax;
				dp[i++] = *idata++ / pxmax;
				if(out_cmodel == BC_RGBA_FLOAT)
					dp[i++] = *idata++ / pxmax;
				else
					idata++;
			}
		}
		break;
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		// BC_RGBA16161616
		wb = components(out_cmodel) * w;
		pxmax = calculate_max(out_cmodel);

		for(int j = 0; j < h; j++)
		{
			float *idata = (float*)(input + j * in_rowspan);
			uint16_t *dp = (uint16_t*)(output + j * out_rowspan);

			for(int i = 0; i < wb;)
			{
				dp[i++] = *idata++ * pxmax;
				dp[i++] = *idata++ * pxmax;
				dp[i++] = *idata++ * pxmax;
				if(in_cmodel == BC_RGBA_FLOAT)
					dp[i++] = *idata++ * pxmax;
				else
					dp[i++] = pxmax;
			}
		}
		break;
	default:
		printf("Missing copy conversion from '%s' to '%s'\n", 
			name(in_cmodel), name(out_cmodel));
		break;
	}
}

void transfer_details(struct SwsContext *sws_ctx, int srange)
{
	int *inv_table, *table;
	int srcRange, dstRange;
	int brightness, contrast, saturation;

	if(sws_getColorspaceDetails(sws_ctx, &inv_table,
		&srcRange, &table, &dstRange,
		&brightness, &contrast, &saturation) == 0)
	{
		sws_setColorspaceDetails(sws_ctx, inv_table,
			srange, table, dstRange,
			brightness, contrast, saturation);
	}
}

void ColorModels::slow_transfer(int out_w, int out_h,
	unsigned char *output, int out_cmodel, int out_rowspan,
	int in_w, int in_h,
	unsigned char *input, int in_cmodel, int in_rowspan)
{
	int in_medmodel = inter_color_model(in_cmodel);
	int out_medmodel = inter_color_model(out_cmodel);
	VFrame *in_tmp = BC_Resources::tmpframes.get_tmpframe(in_w, in_h, in_medmodel);
	VFrame *out_tmp = BC_Resources::tmpframes.get_tmpframe(out_w, out_h, out_medmodel);

	// copy input -> in_tmp
	copy_colors(in_w, in_h, in_tmp->get_data(), in_medmodel,
		in_tmp->get_bytes_per_line(), input, in_cmodel, in_rowspan);
	// in_tmp -> out_tmp
	transfer_frame(out_tmp->get_data(), in_tmp, 0, 0, 0, out_w, out_h,
		out_medmodel, out_tmp->get_bytes_per_line());
	// copy out_tmp -> output
	copy_colors(out_w, out_h, output, out_cmodel, out_rowspan,
		out_tmp->get_data(), out_medmodel, out_tmp->get_bytes_per_line());
	BC_Resources::tmpframes.release_frame(in_tmp);
	BC_Resources::tmpframes.release_frame(out_tmp);
}

void ColorModels::to_text(char *string, int cmodel)
{
	strcpy(string, name(cmodel));
}

const char *ColorModels::name(int cmodel)
{
	for(int i = 0;; i++)
	{
		if(color_model_names[i].value < 0)
			break;
		if(color_model_names[i].value == cmodel)
			return color_model_names[i].name;
	}
	return "Unknown";
}

int ColorModels::from_text(char *text)
{
	for(int i = 0;; i++)
	{
		if(color_model_names[i].value < 0)
			break;
		if(!strcasecmp(text, color_model_names[i].name))
			return color_model_names[i].value;
	}
	return BC_RGB888;
}

int ColorModels::is_yuv(int colormodel)
{
	switch(colormodel)
	{
	case BC_YUV888:
	case BC_YUVA8888:
	case BC_YUV161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
	case BC_YUV422:
	case BC_YUV420P:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
		return 1;
	}
	return 0;
}

int ColorModels::has_alpha(int colormodel)
{
	switch(colormodel)
	{
	case BC_A8:
	case BC_A16:
	case BC_A_FLOAT:
	case BC_RGBA8888:
	case BC_RGBA16161616:
	case BC_YUVA8888:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
	case BC_RGBA_FLOAT:
		return 1;
	}
	return 0;
}

int ColorModels::libinterpolate(int value)
{
	for(int i = 0; i < sizeof(interpolation_types) / sizeof(struct intp_types); i++)
	{
		if(interpolation_types[i].value == value)
			return interpolation_types[i].libvalue;
	}
	return SWS_POINT;
}

int ColorModels::libinterpolate()
{
	return libinterpolate(BC_Resources::interpolation_method);
}

void ColorModels::dump_frame(BC_Bitmap *frame, int opts)
{
	dump_frame(frame->get_data(), frame->get_w(),
		frame->get_h(), frame->get_bytes_per_line(),
		frame->get_color_model(), opts);
}

void ColorModels::dump_frame(VFrame *frame, int opts)
{
	dump_frame(frame->get_data(), frame->get_w(),
		frame->get_h(), frame->get_bytes_per_line(),
		frame->get_color_model(), opts);
}

void ColorModels::dump_frame(unsigned char *data, int width, int height,
    int bytes_per_line, int colormodel, int opts)
{
	int wi_chunk = (width + 15) / 16;
	int he_chunk = (height + 15) / 16;
	int min0[16], max0[16], avg0[16];
	int min1[16], max1[16], avg1[16];
	int min2[16], max2[16], avg2[16];
	int min3[16], max3[16], avg3[16];
	int vals[4], **valsp;
	int *mins[4], *maxs[4], *avgs[4];
	int row, col, pixcount, chrow, nextch;
	int comps, compsize;
	int comprval = opts & DUMP_FRAME_COMB;
	const char *s, *t;

	opts &= DUMP_FRAME_MASK;
	if(!data)
	{
		printf("dump_frame: No data\n");
		return;
	}
	comps = components(colormodel);
	if(!comps)
	{
		printf("dump_frame: No components\n");
		return;
	}
	if(comprval)
		t = " combined";
	else
		t = "";
	switch(opts)
	{
	case DUMP_FRAME_AVG:
		s = "averages";
		break;
	case DUMP_FRAME_MIN:
		s = "minimums";
		break;
	case DUMP_FRAME_MAX:
		s = "maximums";
		break;
	}
	compsize = calculate_pixelsize(colormodel) / comps;
	printf("Frame %p [%d,%d] dump: subframe [%d,%d] %s %s%s:\n",
		data, width, height, wi_chunk, he_chunk,
		name(colormodel), s, t);
	mins[0] = min0;
	mins[1] = min1;
	mins[2] = min2;
	mins[3] = min3;
	maxs[0] = max0;
	maxs[1] = max1;
	maxs[2] = max2;
	maxs[3] = max3;
	avgs[0] = avg0;
	avgs[1] = avg1;
	avgs[2] = avg2;
	avgs[3] = avg3;
	nextch = 1;

	for(row = 0; row < height;)
	{
		if(nextch)
		{
			for(int i = 0; i < 16; i++)
			{
				min0[i] = min1[i] = min2[i] = min3[i] = 0xffff;
				max0[i] = max1[i] = max2[i] = max3[i] = 0;
				avg0[i] = avg1[i] = avg2[i] = avg3[i] = 0;
			}
			pixcount = 0;
			chrow = 0;
			nextch = 0;
		}

		for(col = 0; col < width; col++)
		{
			switch(compsize)
			{
			case 1:
				{
					unsigned char *bytep = data + row * bytes_per_line +
						col * comps;
					for(int i = 0; i < comps; i++)
						vals[i] = bytep[i] << 8;
				}
				break;
			case 2:
				{
					uint16_t *shortp = (uint16_t*)(data + row * bytes_per_line) +
						col * comps;
					for(int i = 0; i < comps; i++)
						vals[i] = shortp[i];
				}
				break;
			case 4:
				{
					float *floatp = (float*)(data + row * bytes_per_line) +
						col * comps;
					for(int i = 0; i < comps; i++)
						vals[i] = floatp[i] * 0xffff;
				}
				break;
			default:
				printf("ColorModels::dump_frame illegal component size %d\n", compsize);
				return;
			}

			int ap = col / wi_chunk;

			for(int i = 0; i < comps; i++)
			{
				if(vals[i] < mins[i][ap])
					mins[i][ap] = vals[i];
				if(vals[i] > maxs[i][ap])
					maxs[i][ap] = vals[i];
				avgs[i][ap] += vals[i];
			}
			pixcount++;
		}
		row++;
		chrow++;
		if(chrow > he_chunk)
		{
			nextch++;
			for(int k = 0; k < comps; k++)
			{
				for(int i = 0; i < 16; i++)
					avgs[k][i] /= pixcount / 16;
			}
			switch(opts)
			{
			case DUMP_FRAME_MIN:
				valsp = mins;
				break;
			case DUMP_FRAME_MAX:
				valsp = maxs;
				break;
			case DUMP_FRAME_AVG:
				valsp = avgs;
				break;
			}
			for(int i = 0; i < 16; i++)
			{
				int resval = 0;

				for(int k = 0; k < comps; k++)
				{
					resval <<= 4;
					if(comprval)
					{
						int c = valsp[k][i];
						resval |= c >> 12 |
							(c & 0xf00) >> 9 |
							(c & 0xf0) >> 6 |
							(c & 0xf) >> 3;
					}
					else
						resval |= (valsp[k][i] >> 12) & 0xf;
				}
				printf(" %0*x", comps, resval);
			}
			putchar('\n');
		}
	}
}
