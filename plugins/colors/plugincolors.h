#ifndef PLUGINCOLORS_H
#define PLUGINCOLORS_H

// Duplicate filename in guicast

#include "clip.h"
#include "vframe.inc"

#include <stdint.h>

class YUV
{
public:
	YUV();
	~YUV();

	inline void rgb_to_yuv_8(int r, int g, int b, int &y, int &u, int &v)
	{
		y = (rtoy_tab_8[r] + gtoy_tab_8[g] + btoy_tab_8[b]) >> 8;
		u = (rtou_tab_8[r] + gtou_tab_8[g] + btou_tab_8[b]) >> 8;
		v = (rtov_tab_8[r] + gtov_tab_8[g] + btov_tab_8[b]) >> 8;
	};

	inline void rgb_to_yuv_8(int r, int g, int b, unsigned char &y, unsigned char &u, unsigned char &v)
	{
		y = (rtoy_tab_8[r] + gtoy_tab_8[g] + btoy_tab_8[b]) >> 8;
		u = (rtou_tab_8[r] + gtou_tab_8[g] + btou_tab_8[b]) >> 8;
		v = (rtov_tab_8[r] + gtov_tab_8[g] + btov_tab_8[b]) >> 8;
	};

	inline void yuv_to_rgb_8(int &r, int &g, int &b, int y, int u, int v)
	{
		y = (y << 8) | y;
		r = (y + vtor_tab_8[v]) >> 8;
		g = (y + utog_tab_8[u] + vtog_tab_8[v]) >> 8;
		b = (y + utob_tab_8[u]) >> 8;

		CLAMP(r, 0, 0xff);
		CLAMP(g, 0, 0xff);
		CLAMP(b, 0, 0xff);
	};

	inline void rgb_to_yuv_16(int r, int g, int b, int &y, int &u, int &v)
	{
		y = (rtoy_tab_16[r] + gtoy_tab_16[g] + btoy_tab_16[b]) >> 8;
		u = (rtou_tab_16[r] + gtou_tab_16[g] + btou_tab_16[b]) >> 8;
		v = (rtov_tab_16[r] + gtov_tab_16[g] + btov_tab_16[b]) >> 8;
	};

	inline void rgb_to_yuv_16(int r, int g, int b, uint16_t &y, uint16_t &u, uint16_t &v)
	{
		y = (rtoy_tab_16[r] + gtoy_tab_16[g] + btoy_tab_16[b]) >> 8;
		u = (rtou_tab_16[r] + gtou_tab_16[g] + btou_tab_16[b]) >> 8;
		v = (rtov_tab_16[r] + gtov_tab_16[g] + btov_tab_16[b]) >> 8;
	};

	inline void yuv_to_rgb_16(int &r, int &g, int &b, int y, int u, int v)
	{
		y = (y << 8) | y;
		r = (y + vtor_tab_16[v]) >> 8;
		g = (y + utog_tab_16[u] + vtog_tab_16[v]) >> 8;
		b = (y + utob_tab_16[u]) >> 8;

		CLAMP(r, 0, 0xffff);
		CLAMP(g, 0, 0xffff);
		CLAMP(b, 0, 0xffff);
	};

private:
	int rtoy_tab_8[0x100], gtoy_tab_8[0x100], btoy_tab_8[0x100];
	int rtou_tab_8[0x100], gtou_tab_8[0x100], btou_tab_8[0x100];
	int rtov_tab_8[0x100], gtov_tab_8[0x100], btov_tab_8[0x100];

	int vtor_tab_8[0x100], vtog_tab_8[0x100];
	int utog_tab_8[0x100], utob_tab_8[0x100];
	int *vtor_8, *vtog_8, *utog_8, *utob_8;

	int rtoy_tab_16[0x10000], gtoy_tab_16[0x10000], btoy_tab_16[0x10000];
	int rtou_tab_16[0x10000], gtou_tab_16[0x10000], btou_tab_16[0x10000];
	int rtov_tab_16[0x10000], gtov_tab_16[0x10000], btov_tab_16[0x10000];

	int vtor_tab_16[0x10000], vtog_tab_16[0x10000];
	int utog_tab_16[0x10000], utob_tab_16[0x10000];
	int *vtor_16, *vtog_16, *utog_16, *utob_16;
};














class HSV
{
public:
	HSV();
    ~HSV();

// All units are 0 - 1
	static int rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v);
	static int hsv_to_rgb(float &r, float &g, float &b, float h, float s, float v);

// YUV units are 0 - max.  HSV units are 0 - 1
	static int yuv_to_hsv(int y, int u, int v, float &h, float &s, float &va, int max);
	static int hsv_to_yuv(int &y, int &u, int &v, float h, float s, float va, int max);
	static YUV yuv_static;
};


#endif
